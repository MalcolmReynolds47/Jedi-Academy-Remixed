"""Offline source/mesh/animation/event qualification of the local legacy bridge."""
import argparse
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import re
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from jar_catalog_assets import LEGACY_PACK

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--game-data', type=Path, required=True)
p.add_argument('--outcast-data', type=Path)
a = p.parse_args()


def inventory(base):
    files = {}
    for path in sorted((base / 'base').glob('assets*.pk3')):
        z = zipfile.ZipFile(path)
        for name in z.namelist():
            files[name.lower()] = (z, name)
    assert files
    return files


def raw(files, name):
    z, key = files[name]
    return z.read(key)


def rows(data):
    text = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/', '', data.decode('latin1').replace('\r', ''))
    return [line.split() for line in text.splitlines() if line.split()]


ja = inventory(a.game_data)
jo = inventory(a.outcast_data or a.game_data.parent.parent / 'Jedi Outcast/GameData')
pack = zipfile.ZipFile(LEGACY_PACK)
names = pack.namelist()
assert len(names) == len(set(names)) and not set(names) & ja.keys()
assert not any(name.startswith(('ext_data/', 'models/players/_humanoid/')) for name in names)
manifest = json.loads(pack.read('jar/legacy-npcs-source.json'))
assert set(manifest['families']) == {'glider', 'mark1', 'mark2', 'minemonster', 'galak_mech'}
assert set(names) == set(manifest['files']) | {'jar/legacy-npcs-source.json'}
files = dict(ja, **{name: (pack, name) for name in names})
for name, digest in manifest['files'].items():
    assert hashlib.sha256(pack.read(name)).hexdigest() == digest
    source = manifest['sources'].get(name)
    if source and 'entry' in source:
        original = raw(jo, source['entry'].lower())
        assert hashlib.sha256(original).hexdigest() == source.get('sourceSHA256', source['sha256'])
        if 'sourceSHA256' not in source:
            assert pack.read(name) == original


def sound_exists(path):
    stem = str(PurePosixPath(path).with_suffix(''))
    return any(name in files for name in (path, stem + '.wav', stem + '.mp3'))


summary = []
for family in manifest['families']:
    prefix = f'models/players/{family}/'
    mesh = pack.read(prefix + 'model.glm')
    assert mesh == raw(jo, prefix + 'model.glm') and mesh[:4] == b'2LGM'
    bones, lods, lod_offset, surfaces, hierarchy, end = struct.unpack_from('<6i', mesh, 140)
    assert 0 < bones <= 512 and 0 < lods <= 4 and 0 < surfaces <= 512 and end == len(mesh)
    animation = mesh[72:136].split(b'\0', 1)[0].decode('ascii') + '.gla'
    rig = raw(files, animation)
    assert rig == raw(jo, animation) and rig[:4] == b'2LGA'
    frames, frame_offset, rig_bones, pool_offset, skeleton_offset, rig_end = struct.unpack_from('<6i', rig, 76)
    assert 0 < frames <= 65536 and bones == rig_bones and rig_end == len(rig)
    assert frame_offset + frames * bones * 3 <= pool_offset <= rig_end
    for offset in range(frame_offset, frame_offset + frames * bones * 3, 3):
        index = int.from_bytes(rig[offset:offset+3], 'little')
        assert pool_offset + (index + 1) * 14 <= rig_end
    # All LOD triangles and per-vertex bone references must stay in their arrays.
    vertices = 0
    for lod in range(lods):
        for i in range(surfaces):
            offset = lod_offset + 4 + struct.unpack_from('<i', mesh, lod_offset + 4 + i * 4)[0]
            _, index, header, count, vertex_offset, triangles, triangle_offset, refs, ref_offset, surface_end = struct.unpack_from('<10i', mesh, offset)
            assert index == i and 0 < count <= 250000 and 0 <= triangles <= 250000 and 0 < refs <= 32
            assert 40 <= vertex_offset and offset + surface_end <= len(mesh)
            assert vertex_offset + count * 32 <= surface_end and triangle_offset + triangles * 12 <= surface_end
            assert ref_offset + refs * 4 <= surface_end
            assert all(0 <= n < count for n in struct.unpack_from('<' + str(triangles * 3) + 'i', mesh, offset + triangle_offset))
            assert all(0 <= n < bones for n in struct.unpack_from('<' + str(refs) + 'i', mesh, offset + ref_offset))
            for vertex in range(count):
                v = offset + vertex_offset + vertex * 32
                assert all(math.isfinite(n) for n in struct.unpack_from('<6f', mesh, v))
                packed = struct.unpack_from('<I', mesh, v + 24)[0]
                assert all(((packed >> (5*k)) & 31) < refs for k in range((packed >> 30) + 1))
            vertices += count
        lod_offset += struct.unpack_from('<i', mesh, lod_offset)[0]
    configs = {row[0]: row[1:] for row in rows(pack.read(prefix + 'animation.cfg'))}
    supported = set(re.findall(r'\b(?:BOTH|TORSO|LEGS|FACE)_[A-Z0-9_]+', (ROOT / 'code/game/anims.h').read_text()))
    supported &= set(re.findall(r'\b(?:BOTH|TORSO|LEGS|FACE)_[A-Z0-9_]+', (ROOT / 'codemp/game/anims.h').read_text()))
    assert configs and configs.keys() <= supported and {'BOTH_STAND1', 'BOTH_WALK1', 'BOTH_RUN1'} <= configs.keys()
    for name, values in configs.items():
        assert len(values) == 4
        first, count, loop, fps = map(float, values)
        assert 0 <= first < frames and 0 < count <= frames-first and -1 <= loop < count and fps != 0
    for suffix in ('code/game/AI_', 'codemp/game/NPC_AI_'):
        if family == 'glider':
            continue
        native = ROOT / (suffix + {'mark1': 'Mark1', 'mark2': 'Mark2', 'minemonster': 'MineMonster', 'galak_mech': 'GalakMech_legacy' if suffix.startswith('code/') else 'GalakMech'}[family] + ('.cpp' if suffix.startswith('code/') else '.c'))
        source = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/', '', native.read_text())
        if family == 'galak_mech':
            for macro, target, _ in re.findall(r'#define (JAR_GALAK_\w+) (\w+) // (\w+)', (ROOT / 'shared/qcommon/jar_galak_anims.h').read_text()):
                source = re.sub(r'\b' + macro + r'\b', target, source)
            # These clips are assigned to humanoid victims, never to the mech.
            source = re.sub(r'NPC_SetAnim\(\s*(?:NPC|NPCS.NPC)->enemy,[^;]+;', '', source)
        requested = set(re.findall(r'\b(?:BOTH|TORSO)_[A-Z0-9_]+', source))
        if family == 'galak_mech':
            requested -= {'BOTH_KNOCKDOWN1', 'BOTH_KNOCKDOWN4'}  # knockAnim local is applied only to enemy
        assert requested <= configs.keys(), (family, requested - configs.keys())
        for sound in re.findall(r'"(sound/[^"%]+)"', source):
            assert sound_exists(sound), (family, sound)
        for effect in re.findall(r'G_EffectIndex\(\s*"([^"]+)"', source):
            assert 'effects/' + effect.removesuffix('.efx') + '.efx' in files, (family, effect)
    old_events = [row for row in rows(raw(jo, prefix + 'animsounds.cfg')) if len(row) == 6]
    events = [row for row in rows(pack.read(prefix + 'animevents.cfg')) if len(row) == 7]
    assert len(events) == len(old_events) == manifest['animationEvents'][family]
    for old, new in zip(old_events, events):
        expected = list(old)
        expected[0] = next((target for target, original in manifest['animationAliases'][family].items() if original == old[0]), old[0])
        assert new[1] == 'AEV_SOUND' and [new[0], *new[2:]] == expected
        assert expected[0] in configs and 0 <= int(old[1]) < int(configs[expected[0]][1])
        low, high = map(int, old[3:5])
        paths = [old[2] % i for i in range(low, high + 1)] if low and high else [old[2]]
        assert all(sound_exists(path) for path in paths), (family, paths)
    summary.append(dict(family=family, frames=frames, bones=bones, lods=lods, vertices=vertices, animations=len(configs), soundEvents=len(events)))

# The replicated surface table and native bolt lookups must exist on the mesh.
mesh = pack.read('models/players/galak_mech/model.glm')
surfaces = set()
for i in range(struct.unpack_from('<i', mesh, 152)[0]):
    offset = 164 + struct.unpack_from('<i', mesh, 164 + 4*i)[0]
    surfaces.add(mesh[offset:offset+64].split(b'\0', 1)[0].decode())
table = set(re.findall(r'"([^"]+)"', (ROOT / 'shared/qcommon/jar_galak_surfaces.h').read_text()))
assert table <= surfaces, table - surfaces
assert {'*flasha', '*flashb', '*flashc', '*antenna_base', '*antenna_effect'} <= surfaces
skin = pack.read('models/players/galak_mech/model_default.skin')
assert b'torso_shield_off,gfx/effects/irid_shield' in skin
for name in ('beam_warmup', 'trace_beam'):
    source = 'effects/galak/' + name + '.efx'
    derived = source.replace('.efx', '_follow.efx')
    original, adapted = pack.read(source), pack.read(derived)
    assert original == raw(jo, source)
    assert hashlib.sha256(original).hexdigest() == manifest['sources'][derived]['sourceSHA256']
    assert set(re.findall(rb'\blife\s+(\d+)', adapted)) == {b'125'}
    assert (b'Line' in adapted) == (name == 'beam_warmup')
    assert re.findall(rb'(?i)(?:gfx|sound)/[\w/]+', adapted) == re.findall(rb'(?i)(?:gfx|sound)/[\w/]+', original)[(1 if name == 'trace_beam' else 0):]

result = dict(families=summary, files=len(names), archiveSHA256=hashlib.sha256(LEGACY_PACK.read_bytes()).hexdigest(),
              NoAcademyOverrides=True, GameLaunched=False, RenderedAcceptance=False)
destination = ROOT / 'build/legacy-npcs-qualification.json'
destination.write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
