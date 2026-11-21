"""Fit selected Academy NPC meshes to the stock multiplayer humanoid rig."""
from argparse import ArgumentParser
from contextlib import ExitStack
from hashlib import sha256
import json
from pathlib import Path
import re
from struct import Struct, pack, unpack_from
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

HUMANOID = 'models/players/_humanoid/_humanoid'
MODEL_HEADER = Struct('<4si64s64s7i')
SURFACE = Struct('<10i')
VERTEX = Struct('<6fI4B')
REQUIRED_TAGS = ('*r_hand', '*l_hand', '*chestg', '*head_top',
                 '*r_hand_cap_r_arm', '*l_hand_cap_l_arm')
NPC_MODEL = re.compile(rb'^\s*playerModel\s+"?([a-z0-9_-]+)"?\s*$', re.I | re.M)
ALIASES = {
    'grandmaster_null': 'pelvis', 'hip_bone': 'pelvis',
    'base_tail': 'pelvis', 'lower_spine': 'lower_lumbar',
    'mid_spine': 'upper_lumbar', 'upper_spine': 'thoracic',
    'neck_base': 'cervical', 'neck_mid': 'cervical', 'head_bone': 'cranium',
    'jaw_bone': 'jaw', 'rhand_tag_bone': 'rhang_tag_bone',
    'lhand_tag_bone': 'lhang_tag_bone',
}
for side, name in (('l', 'left'), ('r', 'right')):
    ALIASES.update({f'{side}_{source}': side + target for source, target in (
        ('thigh', 'femurYZ'), ('calve', 'tibia'), ('foot_bone', 'talus'),
        ('heel_bone', 'talus'), ('shoulder_bone', 'clavical'),
        ('upper_arm', 'humerus'), ('lower_arm', 'radius'), ('hand_bone', 'hand'))})
    ALIASES.update({f'{name}_{source}': side + target for source, target in (
        ('thigh_bone', 'femurYZ'), ('foot_bone', 'talus'), ('heel_bone', 'talus'),
        ('shoulder_bone', 'clavical'))})
    ALIASES[side + 'femur'] = side + 'femurYZ'
    ALIASES[side + 'tarsal'] = side + 'talus'


def string(data):
    return data.split(b'\0', 1)[0].decode('ascii')


def field(text):
    encoded = text.encode('ascii')
    assert len(encoded) < 64, text
    return encoded.ljust(64, b'\0')


def skeleton(data):
    assert data[:4] == b'2LGA' and unpack_from('<i', data, 4)[0] == 6
    bones = []
    for index in range(unpack_from('<i', data, 84)[0]):
        start = 100 + unpack_from('<i', data, 100 + index * 4)[0]
        bones.append((string(data[start:start+64]).lower(), unpack_from('<i', data, start+68)[0],
                      tuple(unpack_from('<f', data, start+84+axis*16)[0] for axis in range(3))))
    return bones


def read_model(data):
    ident, version, _, gla, _, bones, lods, lod, count, _, end = MODEL_HEADER.unpack_from(data)
    assert ident == b'2LGM' and version == 6 and end == len(data)
    hierarchy = []
    for index in range(count):
        start = 164 + unpack_from('<i', data, 164 + index*4)[0]
        children = unpack_from('<i', data, start+140)[0]
        hierarchy.append(data[start:start+144+4*children])
    geometry = []
    for _ in range(lods):
        surfaces = []
        for index in range(count):
            start = lod + 4 + unpack_from('<i', data, lod+4+index*4)[0]
            _, actual, _, nv, vo, nt, to, nr, ro, size = SURFACE.unpack_from(data, start)
            assert actual == index and start+size <= end
            refs = unpack_from(f'<{nr}i', data, start+ro)
            vertices = tuple(VERTEX.unpack_from(data, start+vo+i*32) for i in range(nv))
            uv = data[start+vo+32*nv:start+vo+40*nv]
            surfaces.append((vertices, uv, data[start+to:start+to+12*nt], refs))
        geometry.append(surfaces)
        lod += unpack_from('<i', data, lod)[0]
    assert lod == end
    return string(gla), bones, hierarchy, geometry


def convert(source, species, source_bones, target_bones, kyle):
    _, bone_count, hierarchy, geometry = source
    assert bone_count == len(source_bones) and len(target_bones) == 53
    humanoid_indices = {name: i for i, (name, _, _) in enumerate(target_bones)}
    def closest_mapped_bone(index):
        for _ in source_bones:
            name, parent, _ = source_bones[index]
            mapped = humanoid_indices.get(ALIASES.get(name, name).lower())
            if mapped is not None:
                return mapped
            if parent < 0:
                return 0
            index = parent
        raise ValueError(f'Cyclic bone hierarchy in {species}')

    remap = [closest_mapped_bone(i) for i in range(bone_count)]
    names = [string(item[:64]) for item in hierarchy]
    template = {string(item[:64]): i for i, item in enumerate(kyle[2])}
    tags = [tag for tag in REQUIRED_TAGS if tag not in names]
    for tag in tags:
        original = kyle[2][template[tag]]
        hierarchy.append(original[:136] + pack('<2i', -1, 0))
    count = len(hierarchy)
    body = [vertex[3:6] for index, surface in enumerate(geometry[0]) if not names[index].startswith('*')
            for vertex in surface[0]]
    low = [min(pos[axis] for pos in body) for axis in range(3)]
    high = [max(pos[axis] for pos in body) for axis in range(3)]
    span = [high[i]-low[i] for i in range(3)]
    scale = min(1.0, 62/max(1, span[2]), 52/max(1, span[0]), 52/max(1, span[1]))
    shift = (-(low[0]+high[0])/2*scale, -(low[1]+high[1])/2*scale, -low[2]*scale)

    def write_surface(surface, index, file_offset, borrowed=False):
        vertices, uv, triangles, refs = surface
        ref_table = refs if borrowed else tuple(remap[ref] for ref in refs)
        assert len(ref_table) <= 32 and all(0 <= ref < 53 for ref in ref_table)
        packed_verts = bytearray()
        for vertex in vertices:
            x, y, z = vertex[3:6]
            if not borrowed:
                x, y, z = (x*scale+shift[0], y*scale+shift[1], z*scale+shift[2])
            packed_verts += VERTEX.pack(*vertex[:3], x, y, z, *vertex[6:])
        vo = SURFACE.size+len(triangles)
        ro = vo+len(packed_verts)+len(uv)
        return (SURFACE.pack(0, index, -file_offset, len(vertices), vo, len(triangles)//12, SURFACE.size,
                            len(ref_table), ro, ro+4*len(ref_table)) + triangles + packed_verts + uv +
                pack(f'<{len(ref_table)}i', *ref_table))

    hier_bytes = b''.join(hierarchy)
    hier_start = 164+4*count
    lod_start = hier_start+len(hier_bytes)
    model_body = bytearray()
    for lod_num, surfaces in enumerate(geometry):
        offset = 4*count
        offsets = []
        chunks = []
        template_lod = kyle[3][min(lod_num, len(kyle[3])-1)]
        all_surfaces = surfaces+[template_lod[template[tag]] for tag in tags]
        for index, surface in enumerate(all_surfaces):
            offsets.append(offset)
            chunk = write_surface(surface, index, lod_start+len(model_body)+4+offset, index >= len(surfaces))
            chunks.append(chunk)
            offset += len(chunk)
        model_body += pack(f'<{count+1}i', offset+4, *offsets)+b''.join(chunks)
    offsets = []
    offset = 4*count
    for item in hierarchy:
        offsets.append(offset)
        offset += len(item)
    alias = 'jar_player_'+species
    header = MODEL_HEADER.pack(b'2LGM', 6, field(f'models/players/{alias}/model.glm'), field(HUMANOID),
                               0, len(target_bones), len(geometry), lod_start, count, hier_start,
                               lod_start+len(model_body))
    data = header+pack(f'<{count}i', *offsets)+hier_bytes+model_body
    check = read_model(data)
    assert check[0] == HUMANOID and check[1] == 53
    return alias, data, [round(value*scale, 2) for value in span]


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--game-data', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--companion', action='append', default=[], type=Path)
    args = parser.parse_args()
    if not args.companion:
        build = Path(__file__).resolve().parents[1] / 'build'
        args.companion = [build / name for name in ('jar-legacy-npcs.pk3','jar-mp-legacy-droids.pk3')
                          if (build / name).is_file()]
    with ExitStack() as stack:
        packs = [stack.enter_context(ZipFile(path)) for path in
                 [*sorted((args.game_data/'base').glob('assets*.pk3')), *args.companion]]
        files = {name.lower(): (archive, name) for archive in packs for name in archive.namelist()}

        def read(name):
            archive, actual = files[name.lower()]
            return archive.read(actual)

        target = skeleton(read(HUMANOID+'.gla'))
        kyle = read_model(read('models/players/kyle/model.glm'))
        output = {}
        manifest = {}
        candidates = {'jar_mouse', 'jar_remote'}
        for name in files:
            if name.startswith('ext_data/npcs/') and name.endswith('.npc'):
                text = re.sub(rb'//[^\r\n]*|/\*[\s\S]*?\*/', b'', read(name))
                candidates.update(model.decode('ascii').lower() for model in NPC_MODEL.findall(text))
        unavailable = []
        for species in sorted(candidates):
            prefix = f'models/players/{species}/'
            if prefix+'model.glm' not in files:
                unavailable.append(species)
                continue
            source = read(prefix+'model.glm')
            geometry = read_model(source)
            if geometry[0].lower() == HUMANOID:
                continue
            if geometry[0].lower()+'.gla' not in files:
                unavailable.append(species)
                continue
            alias, model, dimensions = convert(geometry, species, skeleton(read(geometry[0]+'.gla')), target, kyle)
            dst = f'models/players/{alias}/'
            output[dst+'model.glm'] = model
            for name in files:
                if name.startswith(prefix) and '/' not in name[len(prefix):] and name.endswith(('.skin', '.surf')):
                    output[dst+name[len(prefix):]] = read(name)
            if dst+'model_default.skin' not in output:
                # Default GLM shaders remain valid if the original has no default skin.
                output[dst+'model_default.skin'] = b'// Original model shaders\n'
            manifest[alias] = dict(source=prefix+'model.glm', source_sha256=sha256(source).hexdigest(),
                                   model_sha256=sha256(model).hexdigest(), bounds=dimensions)
    output['jar/player-models.json'] = (json.dumps(dict(adapted=manifest, unavailable=unavailable), indent=2)+'\n').encode()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(args.output, 'w', ZIP_DEFLATED) as archive:
        for name, data in sorted(output.items()):
            archive.writestr(ZipInfo(name, (2026, 11, 14, 0, 0, 0)), data, compress_type=ZIP_DEFLATED)
    print(f'Multiplayer humanoid models: {len(manifest)} adapted meshes with player-sized bounds.')
    print('NPC model keys without local Ghoul2 input:', ', '.join(unavailable))


if __name__ == '__main__':
    main()
