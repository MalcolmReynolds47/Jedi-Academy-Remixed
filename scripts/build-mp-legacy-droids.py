"""Convert the two original static MD3 droid bodies to native MP Ghoul2 assets.

Geometry, surface materials, UVs and triangle winding come from the owner's
retail Academy files. A single identity bone cancels the engine's Ghoul2 axis
convention; it does not invent animation or replace any stock file.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zipfile
from jar_catalog_assets import ROOT

MODELS = {'mouse': 'jar_mouse', 'remote_sp': 'jar_remote'}


def string(value):
    b = value.encode('ascii')
    assert len(b) < 64
    return b.ljust(64, b'\0')


def fnv(b):
    value = 2166136261
    for byte in b:
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def skeleton(name):
    identity = (1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0.)
    bone = string('model_root') + struct.pack('<Ii24fi', 0, -1, *identity, *identity, 0)
    # Header, relative bone-offset table, one bone, frame indices plus alignment,
    # and an exactly representable identity quaternion/zero translation.
    return (struct.pack('<4sI64sf6i', b'2LGA', 6, string(name), 1., 1, 276, 1, 280, 104, 294)
            + struct.pack('<i', 4) + bone + bytes(4)
            + struct.pack('<7H', 49149, 32766, 32766, 32766, 32768, 32768, 32768))


def convert(data, target):
    assert data[:4] == b'IDP3' and struct.unpack_from('<i', data, 4)[0] == 15
    frames, tags, count = struct.unpack_from('<3i', data, 76)
    offset, end = struct.unpack_from('<2i', data, 100)
    assert frames == 1 and tags == 0 and 0 < count <= 32 and end == len(data)
    surfaces = []
    for i in range(count):
        assert data[offset:offset+4] == b'IDP3'
        name = data[offset+4:offset+68].split(b'\0')[0].decode('ascii').lower()
        sf, nsh, nv, nt, ot, osh, ost, ov, size = struct.unpack_from('<9i', data, offset+72)
        assert sf == 1 and nsh == 1 and 0 < nv < 4096 and 0 < nt < 8192
        assert offset+size <= end and all(108 <= start and start+length <= size
            for start, length in ((ot, nt*12), (osh, 68), (ost, nv*8), (ov, nv*8)))
        shader = data[offset+osh:offset+osh+64].split(b'\0')[0].decode('ascii')
        triangles = data[offset+ot:offset+ot+nt*12]
        assert all(v < nv for v in struct.unpack('<' + str(nt*3) + 'I', triangles))
        vertices = bytearray()
        for v in range(nv):
            x, y, z, normal = struct.unpack_from('<3hH', data, offset+ov+v*8)
            lat, lng = (normal >> 8)*math.tau/256, (normal & 255)*math.tau/256
            nx, ny, nz = math.cos(lat)*math.sin(lng), math.sin(lat)*math.sin(lng), math.cos(lng)
            # Engine root matrix maps (x,y,z) to (-y,x,z). Undo that in the
            # bind mesh, including normals. One bone weight is implicitly 1.
            vertices += struct.pack('<6fI4B', ny, -nx, nz, y/64, -x/64, z/64, 0, 0, 0, 0, 0)
        uv = data[offset+ost:offset+ost+nv*8]
        surfaces.append((name, shader, nv, nt, triangles, vertices, uv))
        offset += size
    assert offset == end and len({s[0] for s in surfaces}) == count
    hierarchy, offsets = bytearray(), []
    for i, (name, shader, *_) in enumerate(surfaces):
        offsets.append(count*4+len(hierarchy))
        children = list(range(1, count)) if i == 0 else []
        hierarchy += (string(name) + struct.pack('<I', 0) + string(shader)
                      + struct.pack('<3i', 0, -1 if i == 0 else 0, len(children))
                      + struct.pack('<' + str(len(children)) + 'i', *children))
    lod_offset = 164+count*4+len(hierarchy)
    geometry, lod_offsets = bytearray(), []
    for i, (_, _, nv, nt, triangles, vertices, uv) in enumerate(surfaces):
        relative = count*4+len(geometry)
        lod_offsets.append(relative)
        verts_offset = 40+len(triangles)
        bone_offset = verts_offset+len(vertices)+len(uv)
        geometry += (struct.pack('<10i', 0, i, -(lod_offset+4+relative), nv, verts_offset,
                                 nt, 40, 1, bone_offset, bone_offset+4)
                     + triangles + vertices + uv + struct.pack('<i', 0))
    lod = struct.pack('<i', 4+count*4+len(geometry)) + struct.pack('<'+str(count)+'i', *lod_offsets) + geometry
    prefix = f'models/players/{target}/'
    glm = (struct.pack('<4sI64s64s7i', b'2LGM', 6, string(prefix+'model.glm'), string(prefix+'model'),
                       0, 1, 1, lod_offset, count, 164+count*4, lod_offset+len(lod))
           + struct.pack('<'+str(count)+'i', *offsets) + hierarchy + lod)
    skin = ''.join(name+','+shader+'\n' for name, shader, *_ in surfaces).encode('ascii')
    return glm, skeleton(prefix+'model'), skin, sum(s[2] for s in surfaces), sum(s[3] for s in surfaces)


def main():
    p = argparse.ArgumentParser();p.add_argument('--game-data', type=Path, required=True);a = p.parse_args()
    files, packs = {}, []
    for path in sorted((a.game_data/'base').glob('assets*.pk3')):
        z = zipfile.ZipFile(path);packs.append(z)
        for n in z.namelist():files[n.lower()] = (z, n)
    content, rows, audit = {}, [], []
    for original, target in MODELS.items():
        source = f'models/players/{original}/lower.md3';z, n = files[source];data = z.read(n)
        glm, gla, skin, vertices, triangles = convert(data, target)
        prefix = f'models/players/{target}/'
        zc, nc = files[f'models/players/{original}/animation.cfg'];animation = zc.read(nc)
        assert [line.split() for line in animation.decode().splitlines() if line.strip() and not line.strip().startswith('//')] == [['BOTH_STAND1','0','1','0','20']]
        for suffix, value in [('model.glm', glm), ('model.gla', gla), ('model_default.skin', skin), ('animation.cfg', animation)]:
            assert prefix+suffix not in files
            content[prefix+suffix] = value
        rows.append((source, target, len(data), fnv(data), len(glm), fnv(glm)))
        audit.append(dict(source=source, archive=Path(z.filename).name, sourceSHA256=hashlib.sha256(data).hexdigest(),
                          target=prefix+'model.glm', targetSHA256=hashlib.sha256(glm).hexdigest(),
                          vertices=vertices, triangles=triangles, frames=1, bones=1))
    content['jar-mp-legacy-droids.json'] = (json.dumps(dict(schema=1,models=audit), indent=2)+'\n').encode()
    output=ROOT/'build/jar-mp-legacy-droids.pk3'
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, data in sorted(content.items()):
            info=zipfile.ZipInfo(name, date_time=(2026,10,11,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,data)
    header='// Generated from original static MD3s; GPL-2.0-only.\n#pragma once\n'
    header+='typedef struct { const char *source,*target; int bytes; unsigned hash; int modelBytes; unsigned modelHash; } jarMPDroidModel_t;\n'
    header+='static const jarMPDroidModel_t jarMPDroidModels[]={\n'
    for source,target,size,h,ms,mh in rows:header+=f' {{"{source}","{target}",{size},{h}u,{ms},{mh}u}},\n'
    header+='};\n'
    dest=ROOT/'codemp/game/jar_mp_droid_models.h'
    if not dest.exists() or dest.read_text()!=header:dest.write_text(header,newline='\n')
    print('MP legacy droids:',len(content),'files;',sum(m['vertices'] for m in audit),'original vertices;',output)


if __name__ == '__main__':main()
