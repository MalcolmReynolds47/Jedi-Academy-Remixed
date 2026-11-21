"""Record the production portable-content decision for every stock BSP.

This deliberately does not treat an explained refusal as a spawnable menu entry.
"""
import argparse
from datetime import datetime
import hashlib
import json
import math
import re
import struct
from pathlib import Path
import subprocess
import zipfile


def lump_offset(bsp, index):
    return struct.unpack_from('<ii', bsp, 8 + 8 * index)[0]


parser = argparse.ArgumentParser()
parser.add_argument('--game-data', type=Path, required=True)
parser.add_argument('--build', type=Path, required=True)
args = parser.parse_args()
output = args.build / ('retail-bsp-admission-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
output.mkdir()
maps = {}
for package in sorted((args.game_data / 'base').glob('assets*.pk3')):
    with zipfile.ZipFile(package) as archive:
        for name in archive.namelist():
            if name.lower().startswith('maps/') and name.lower().endswith('.bsp'):
                maps[name.lower()] = archive.read(name)
root = Path(__file__).resolve().parents[2]
profiles = {f'maps/{resource}.bsp': f'#{resource}' for resource in re.findall(
    r'\{"[^"\n]+","#([^"\n]+)","[^"\n]+",Mode::\w+\}', (root / 'code/game/g_bsp_section.h').read_text())}
assert profiles and profiles.keys() <= maps.keys() and 'maps/t1_inter.bsp' in maps
evidence = []
for i, (name, data) in enumerate(sorted(maps.items())):
    path = output / f'map-{i}.bsp'
    path.write_bytes(data)
    section = name in profiles
    inspect = ['--section', str(path), profiles[name]] if section else ['--inspect', str(path)]
    result = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), *inspect],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(result.stdout)
    # All shipped BSPs need further class/field/script adapters, or duplicate
    # key resolution. Other structural refusals are regressions to investigate.
    assert report['Models'] > 0 and report['Entities'] > 0, (name, report)
    assert report['ContentAdmitted'] == section, (name, report)
    if not section:
        assert any(reason in report['Reason'] for reason in ['adapter is pending', 'Script isolation is pending', 'Duplicate entity key:']), (name, report)
    evidence.append(dict(Name=name, SHA256=hashlib.sha256(data).hexdigest(), File=str(path), **report))
for section in (entry for entry in evidence if entry['ContentAdmitted']):
    native = subprocess.run([str(args.build / 'jar_bsp_section_tests.exe'), section['File'], profiles[section['Name']]],
                            capture_output=True, text=True, timeout=30, check=True)
    (output / (Path(section['Name']).stem + '-section.log')).write_text(native.stdout + native.stderr, encoding='utf-8')
# The same model AABB is not evidence of a box brush. Mutate the actual stock
# geometry with the original bounds intact and exercise the production gate.
hazard_rejections = []
for name in ('maps/mp/duel2.bsp', 'maps/mp/duel3.bsp', 'maps/mp/duel5.bsp', 'maps/mp/duel9.bsp'):
    data = maps[name]
    model_index = 5 if name == 'maps/mp/duel3.bsp' else 1
    first_brush = struct.unpack_from('<i', data, lump_offset(data, 7) + 40 * model_index + 32)[0]
    first_side = struct.unpack_from('<i', data, lump_offset(data, 8) + 12 * first_brush)[0]
    plane = struct.unpack_from('<i', data, lump_offset(data, 9) + 12 * first_side)[0]
    plane_offset = lump_offset(data, 2) + 16 * plane
    for field, offset, value, why in (
        ('oblique-face', plane_offset + 4, 0.25, 'six axial faces'),
        ('different-face-limit', plane_offset + 12,
         struct.unpack_from('<f', data, plane_offset + 12)[0] + 16, 'disagree'),
    ):
        changed = bytearray(data)
        struct.pack_into('<f', changed, offset, value)
        mutated = output / (Path(name).stem + '-' + field + '.bsp')
        mutated.write_bytes(changed)
        native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                                capture_output=True, text=True, timeout=20, check=True)
        report = json.loads(native.stdout)
        assert not report['ContentAdmitted'] and why in report['Reason'], (name, field, report)
        hazard_rejections.append(dict(Name=name, Mutation=field, Reason=report['Reason']))

# The Shaft pads must resolve to their own native lifts, with box contact
# geometry proven for the floor switches as well as the fall volume.
name = 'maps/mp/duel3.bsp'
data = maps[name]
entity_offset = lump_offset(data, 0)
target = data.index(b'"target" "t29"', entity_offset) + len(b'"target" "')
pad_brush = struct.unpack_from('<i', data, lump_offset(data, 7) + 40 + 32)[0]
pad_side = struct.unpack_from('<i', data, lump_offset(data, 8) + 12 * pad_brush)[0]
pad_plane = struct.unpack_from('<i', data, lump_offset(data, 9) + 12 * pad_side)[0]
shaft_rejections = []
for field, offset, fmt, value, why in (
    ('foreign-lift-target', target, '<3s', b'bad', 'matching local target'),
    ('oblique-floor-pad', lump_offset(data, 2) + 16 * pad_plane + 4, '<f', .25, 'six axial faces'),
):
    changed = bytearray(data)
    struct.pack_into(fmt, changed, offset, value)
    mutated = output / f'duel3-{field}.bsp'
    mutated.write_bytes(changed)
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and why in report['Reason'], (name, field, report)
    shaft_rejections.append(dict(Mutation=field, Reason=report['Reason']))

# The second half of the Rift's compound fall trigger is part of the admitted
# geometry, and the map must retain its authored pickup class counts.
name = 'maps/mp/ffa4.bsp'
data = maps[name]
entities_offset = lump_offset(data, 0)
blaster = data.index(b'"ammo_blaster"', entities_offset) + 1
first_brush = struct.unpack_from('<i', data, lump_offset(data, 7) + 40 + 32)[0]
second_brush_side = struct.unpack_from('<i', data, lump_offset(data, 8) + 12 * (first_brush + 1))[0]
second_plane = struct.unpack_from('<i', data, lump_offset(data, 9) + 12 * second_brush_side)[0]
rift_rejections = []
for field, offset, fmt, value, why in (
    ('changed-pickup-count', blaster, '<12s', b'ammo_rockets', 'pickup counts differ'),
    ('non-axial-second-fall-brush', lump_offset(data, 2) + 16 * second_plane + 4, '<f', .25, 'six axial faces'),
):
    changed = bytearray(data)
    struct.pack_into(fmt, changed, offset, value)
    mutated = output / f'ffa4-{field}.bsp'
    mutated.write_bytes(changed)
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and why in report['Reason'], (name, field, report)
    rift_rejections.append(dict(Mutation=field, Reason=report['Reason']))


def point_leaf(bsp, point):
    node = 0
    while node >= 0:
        plane, front, back = struct.unpack_from('<3i', bsp, lump_offset(bsp, 3) + node * 36)
        x, y, z, distance = struct.unpack_from('<4f', bsp, lump_offset(bsp, 2) + plane * 16)
        node = front if x * point[0] + y * point[1] + z * point[2] >= distance else back
    return struct.unpack_from('<12i', bsp, lump_offset(bsp, 4) + (-1 - node) * 48)


rift_policy = (root / 'code/game/g_bsp_section_rift.h').read_text()
rift_area = int(re.search(r'arenaArea = (\d+)', rift_policy)[1])
entity_start, entity_length = struct.unpack_from('<ii', data, 8)
rift_entities = data[entity_start:entity_start + entity_length]
assert re.search(rb'"breath"\s+"2"', rift_entities.split(b'}', 1)[0])
rift_entries = re.findall(rb'\{[^{}]*"classname"\s+"info_player_(?:duel[12]?|deathmatch|start)"[^{}]*\}', rift_entities)
assert len(rift_entries) == 16
for entry in rift_entries:
    x, y, z = map(float, re.search(rb'"origin"\s+"([^"]+)"', entry)[1].split())
    cluster, area, *_ = point_leaf(data, (x, y, z + 24))
    assert cluster >= 0 and area == rift_area
assert point_leaf(data, (0, 0, 0))[1] != rift_area
assert point_leaf(data, (7000, 2500, -2000))[0] < 0

name = 'maps/mp/ffa2.bsp'
data = maps[name]
entity_start, entity_length = struct.unpack_from('<ii', data, 8)
entity_text = data[entity_start:entity_start + entity_length]
korriban_entries = re.findall(rb'\{[^{}]*"classname"\s+"info_player_(?:start|deathmatch)"[^{}]*\}', entity_text)
korriban_policy = (root / 'code/game/g_bsp_section_korriban.h').read_text()
korriban_area = int(re.search(r'arenaArea = (\d+)', korriban_policy)[1])
assert len(korriban_entries) == 17
for entry in korriban_entries:
    x, y, z = map(float, re.search(rb'"origin"\s+"([^"]+)"', entry)[1].split())
    cluster, area, *_ = point_leaf(data, (x, y, z + 24))
    assert cluster >= 0 and area == korriban_area
fog_start, fog_length = struct.unpack_from('<ii', data, 8 + 12 * 8)
assert (b'textures/mp/s_kfog', -1, -1) in [
    (shader.split(b'\0', 1)[0], brush, side)
    for shader, brush, side in struct.iter_unpack('<64sii', data[fog_start:fog_start + fog_length])]
last_brush = struct.unpack_from('<i', data, lump_offset(data, 7) + 8 * 40 + 32)[0]
last_side = struct.unpack_from('<i', data, lump_offset(data, 8) + last_brush * 12)[0]
last_plane = struct.unpack_from('<i', data, lump_offset(data, 9) + last_side * 12)[0]
korriban_rejections = []
for label, offset, replacement, reason in (
    ('oblique-eighth-fall-brush', lump_offset(data, 2) + 16 * last_plane + 4,
     struct.pack('<f', .25), 'six axial faces'),
    ('changed-pickup-count', entity_start + entity_text.index(b'"ammo_thermal"') + 1,
     b'ammo_blaster', 'pickup counts differ'),
    ('changed-torch', entity_start + entity_text.index(b'"env/small_fire"') + 1,
     b'env/other_fire', 'stock torch effect'),
):
    mutated = output / f'ffa2-{label}.bsp'
    mutated.write_bytes(data[:offset] + replacement + data[offset + len(replacement):])
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    korriban_rejections.append(dict(Mutation=label, Reason=report['Reason']))

# A section must never publish the host map's global sky-camera settings.
name = 'maps/mp/duel9.bsp'
data = maps[name]
entity_start, entity_length = struct.unpack_from('<ii', data, 8)
entity_text = data[entity_start:entity_start + entity_length]
sky_camera = re.search(rb'\{[^{}]*"classname"\s+"misc_skyportal"[^{}]*\}', entity_text)
assert sky_camera and b'"origin"' in sky_camera[0]
mutated = output / 'duel9-invalid-sky-camera.bsp'
start, end = (entity_start + index for index in sky_camera.span())
mutated.write_bytes(data[:start] + sky_camera[0].replace(b'"origin"', b'"target"') + data[end:])
native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                        capture_output=True, text=True, timeout=20, check=True)
sky_rejection = json.loads(native.stdout)
assert not sky_rejection['ContentAdmitted'] and 'requires only a local origin' in sky_rejection['Reason'], sky_rejection

# Verify the detached sky partitions against the stock cameras' BSP PVS.
def stock_sky(map_name, policy_file, entry_count):
    data = maps[map_name]
    entity_start, entity_length = struct.unpack_from('<ii', data, 8)
    entity_text = data[entity_start:entity_start + entity_length]
    sky_camera = re.search(rb'\{[^{}]*"classname"\s+"misc_skyportal"[^{}]*\}', entity_text)
    sky_policy = (root / policy_file).read_text()
    backdrop = {surface for first, last in re.findall(r'index >= (\d+) && index <= (\d+)', sky_policy)
                for surface in range(int(first), int(last) + 1)}
    arena_area = int(re.search(r'arenaArea = (\d+)', sky_policy)[1])
    surface_count = int(re.search(r'surfaceCount = (\d+)', sky_policy)[1])
    assert struct.unpack_from('<2i', data, lump_offset(data, 7) + 24) == (0, surface_count)
    camera = tuple(map(float, re.search(rb'"origin"\s+"([^"]+)"', sky_camera[0])[1].split()))
    camera_cluster, camera_area, *_ = point_leaf(data, camera)
    assert camera_cluster >= 0 and camera_area != arena_area
    visibility = lump_offset(data, 16)
    clusters, stride = struct.unpack_from('<2i', data, visibility)
    assert camera_cluster < clusters
    mask = data[visibility + 8 + camera_cluster * stride:visibility + 8 + (camera_cluster + 1) * stride]
    leaves = struct.unpack_from('<2i', data, 8 + 8 * 4)[1] // 48
    visible, arena = set(), set()
    for index in range(leaves):
        leaf = struct.unpack_from('<12i', data, lump_offset(data, 4) + index * 48)
        cluster, area = leaf[:2]
        if cluster < 0:
            continue
        first, count = leaf[8:10]
        surfaces = struct.unpack_from(f'<{count}i', data, lump_offset(data, 5) + first * 4)
        if mask[cluster // 8] & (1 << (cluster % 8)):
            visible.update(surfaces)
        if area == arena_area:
            arena.update(i for i in surfaces if i < surface_count)
    assert visible == backdrop and arena.isdisjoint(backdrop)
    assert len(arena | backdrop) == surface_count
    entries = re.findall(rb'\{[^{}]*"classname"\s+"info_player_(?:duel[12]?|deathmatch|start)"[^{}]*\}', entity_text)
    assert len(entries) == entry_count and all(
        point_leaf(data, tuple(map(float, re.search(rb'"origin"\s+"([^"]+)"', entry)[1].split())))[1] == arena_area
        for entry in entries)
    assert point_leaf(data, (0, 0, 7000))[0] < 0
    return dict(BackdropSurfaces=len(backdrop), ArenaSurfaces=len(arena),
                ArenaArea=arena_area, CheckedEntryPoints=len(entries))

hoth_sky = stock_sky('maps/mp/duel9.bsp', 'shared/qcommon/jar_hoth_sky.h', 14)
yavin_sky = stock_sky('maps/mp/duel6.bsp', 'shared/qcommon/jar_yavin_sky.h', 16)

name = 'maps/mp/duel6.bsp'
data = maps[name]
entity_start, entity_length = struct.unpack_from('<ii', data, 8)
entity_text = data[entity_start:entity_start + entity_length]
yavin_rejections = []
for label, old, new, reason in (
    ('foreign-emitter-aim', b'"target" "t608"', b'"target" "z608"', 'one local marker'),
    ('foreign-timer-target', b'"target" "t533"', b'"target" "x533"', 'local random output'),
    ('non-saber-breakable', b'"spawnflags" "2064"', b'"spawnflags" "2048"', 'saber-only breakable'),
):
    offset = entity_start + entity_text.index(old)
    mutated = output / f'duel6-{label}.bsp'
    mutated.write_bytes(data[:offset] + new + data[offset + len(old):])
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    yavin_rejections.append(dict(Mutation=label, Reason=report['Reason']))

# Abandoned City uses native convex block brushes. Keep the original model
# bounds while introducing unsafe brushes, aliases and a host-geometry overlap.
name = 'maps/mp/duel8.bsp'
data = maps[name]
entity_start, entity_length = struct.unpack_from('<ii', data, 8)
entity_text = data[entity_start:entity_start + entity_length]
model = lump_offset(data, 7) + 40
first_surface, _, first_brush, _ = struct.unpack_from('<4i', data, model + 24)
first_side = struct.unpack_from('<i', data, lump_offset(data, 8) + 12 * first_brush)[0]
side = lump_offset(data, 9) + 12 * first_side
plane = struct.unpack_from('<i', data, side)[0]
plane_distance = lump_offset(data, 2) + 16 * plane + 12
first_vertex = struct.unpack_from('<i', data, lump_offset(data, 13) + 148 * first_surface + 12)[0]
bad_flags = entity_start + entity_text.index(b'"spawnflags" "2048"') + len(b'"spawnflags" "')
duplicate_model = entity_start + entity_text.index(b'"model" "*2"') + len(b'"model" "*')
breakable_rejections = []
for field, offset, fmt, value, why in (
    ('unqualified-flags', bad_flags, '<4s', b'0000', 'stock health, flags'),
    ('duplicate-inline-model', duplicate_model, '<c', b'1', 'assigned more than once'),
    ('overlapping-surface-range', model + 24, '<i', 0, 'ranges overlap'),
    ('invalid-plane-index', side, '<i', -1, 'invalid brush side reference'),
    ('axial-plane-exceeds-model', plane_distance, '<f', struct.unpack_from('<f', data, plane_distance)[0] + 64,
     'planes exceed its inline model'),
    ('draw-vertex-exceeds-model', lump_offset(data, 10) + 80 * first_vertex, '<f', 60000, 'draw vertices exceed its inline model'),
):
    changed = bytearray(data)
    struct.pack_into(fmt, changed, offset, value)
    mutated = output / (Path(name).stem + '-' + field + '.bsp')
    mutated.write_bytes(changed)
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and why in report['Reason'], (name, field, report)
    breakable_rejections.append(dict(Name=name, Mutation=field, Reason=report['Reason']))
name = 'maps/mp/ffa3.bsp'
data = maps[name]
entity_start, entity_length = struct.unpack_from('<ii', data, 8)
tatooine_rejections = []
for label, source, replacement, reason in (
    ('changed-platform-travel', b'"height" "296"', b'"height" "290"', 'stock automatic platform'),
    ('foreign-lift-group', b'"targetname" "lift2"', b'"targetname" "bad_2"', 'timing and local group'),
):
    assert len(source) == len(replacement)
    offset = data.index(source, entity_start, entity_start + entity_length)
    mutated = output / f'ffa3-{label}.bsp'
    mutated.write_bytes(data[:offset] + replacement + data[offset + len(replacement):])
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    tatooine_rejections.append(dict(Mutation=label, Reason=report['Reason']))

name = 'maps/mp/ffa5.bsp'
data = maps[name]
entity_start, entity_length = struct.unpack_from('<ii', data, 8)
taspir_rejections = []
for label, source, replacement, reason in (
    ('conflicting-lift-sound', b'"soundSet" "pedestal"', b'"soundSet" "pedeXXal"', 'Duplicate entity key: soundset'),
    ('broken-lift-route', b'"target" "floor2_stop"', b'"target" "flo0r2_stop"', 'stock cyclic path and timing'),
):
    assert len(source) == len(replacement)
    offset = data.index(source, entity_start, entity_start + entity_length)
    mutated = output / f'ffa5-{label}.bsp'
    mutated.write_bytes(data[:offset] + replacement + data[offset + len(replacement):])
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    taspir_rejections.append(dict(Mutation=label, Reason=report['Reason']))

name = 'maps/mp/ctf5.bsp'
data = maps[name]
entity_start = lump_offset(data, 0)
first_glass_face = struct.unpack_from('<i', data, lump_offset(data, 7) + 40 + 24)[0]
first_glass_vertex = struct.unpack_from('<i', data, lump_offset(data, 13) + first_glass_face * 148 + 12)[0]
factory_rejections = []
for label, offset, replacement, reason in (
    ('changed-shard-count', data.index(b'"maxshards" "12"', entity_start) + len(b'"maxshards" "'), b'13', 'stock model and shard count'),
    ('moved-flag', data.index(b'1548 2592 -320', entity_start), b'1549', 'invalid home'),
    ('changed-glass-vertex', lump_offset(data, 10) + first_glass_vertex * 80, struct.pack('<f', 5241.0), 'glass geometry differs'),
):
    mutated = output / f'ctf5-{label}.bsp'
    mutated.write_bytes(data[:offset] + replacement + data[offset + len(replacement):])
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    factory_rejections.append(dict(Mutation=label, Reason=report['Reason']))

name = 'maps/mp/ctf4.bsp'
data = maps[name]
entity_start = lump_offset(data, 0)
fall_brush = struct.unpack_from('<i', data, lump_offset(data, 7) + 9 * 40 + 32)[0]
fall_side = struct.unpack_from('<i', data, lump_offset(data, 8) + fall_brush * 12)[0]
coruscant_rejections = []
for label, offset, replacement, reason in (
    ('foreign-lift-target', data.index(b'"target" "t217"', entity_start) + len(b'"target" "'), b'b217', 'direction, delay or link'),
    ('wrong-fog', lump_offset(data, 12), b'x', 'stock global fog record'),
    ('invalid-fall-plane', lump_offset(data, 9) + (fall_side + 6) * 12, struct.pack('<i', -1), 'invalid plane reference'),
):
    mutated = output / f'ctf4-{label}.bsp'
    mutated.write_bytes(data[:offset] + replacement + data[offset + len(replacement):])
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    coruscant_rejections.append(dict(Mutation=label, Reason=report['Reason']))

name = 'maps/mp/ctf3.bsp'
data = maps[name]
entity_start = lump_offset(data, 0)
model = lump_offset(data, 7) + 40
third_brush = struct.unpack_from('<i', data, model + 32)[0] + 2
third_side = struct.unpack_from('<i', data, lump_offset(data, 8) + third_brush * 12)[0]
hilltops_rejections = []
for label, offset, replacement, reason in (
    ('missing-lift-brush', model + 36, struct.pack('<i', 2), 'expected trigger brushes'),
    ('invalid-third-lift-plane', lump_offset(data, 9) + third_side * 12, struct.pack('<i', -1), 'invalid plane reference'),
    ('changed-exterior-soundset', data.index(b'yavin_exterior', entity_start), b'badin_exterior', 'stock ambience'),
):
    mutated = output / f'ctf3-{label}.bsp'
    mutated.write_bytes(data[:offset] + replacement + data[offset + len(replacement):])
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    hilltops_rejections.append(dict(Mutation=label, Reason=report['Reason']))

name = 'maps/mp/ctf1.bsp'
data = maps[name]
entity_start = lump_offset(data, 0)
fall_model = lump_offset(data, 7) + 22 * 40
dropzone_rejections = []
for label, offset, replacement, reason in (
    ('foreign-platform-target', data.index(b'"target" "t56"', entity_start) + len(b'"target" "'),
     b'bad', 'stock timing or link'),
    ('changed-platform-delay', data.index(b'"delay" "50"', entity_start) + len(b'"delay" "'),
     b'80', 'stock fields'),
    ('missing-fall-brush', fall_model + 36, struct.pack('<i', 0), 'expected trigger brushes'),
):
    mutated = output / f'ctf1-{label}.bsp'
    mutated.write_bytes(data[:offset] + replacement + data[offset + len(replacement):])
    native = subprocess.run([str(args.build / 'jar_subbsp_admission_tests.exe'), '--section', str(mutated), profiles[name]],
                            capture_output=True, text=True, timeout=20, check=True)
    report = json.loads(native.stdout)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    dropzone_rejections.append(dict(Mutation=label, Reason=report['Reason']))

name = 'maps/mp/ctf2.bsp'
data = maps[name]
entity_start = lump_offset(data, 0)
weather_model = lump_offset(data, 7) + 3 * 40
weather_brush = struct.unpack_from('<i', data, weather_model + 32)[0]
weather_side = struct.unpack_from('<i', data, lump_offset(data, 8) + weather_brush * 12)[0]
weather_plane = struct.unpack_from('<i', data, lump_offset(data, 9) + weather_side * 12)[0]
plane_distance = lump_offset(data, 2) + weather_plane * 16 + 12
inspector = str(args.build / 'jar_subbsp_admission_tests.exe')


def inspect_wasteland(bsp_path):
    args_for_profile = ['--section', str(bsp_path), profiles[name]]
    native = subprocess.run([inspector, *args_for_profile], capture_output=True, text=True, timeout=20, check=True)
    return json.loads(native.stdout)


wasteland_path = next(row['File'] for row in evidence if row['Name'] == name)
wasteland = inspect_wasteland(wasteland_path)
assert wasteland['ContentAdmitted'] and wasteland['Models'] == 18 and wasteland['Entities'] == 269
wasteland_rejections = []
for label, offset, replacement, reason in (
    ('foreign-door-team', data.index(b'blue_door1', entity_start), b'blue_door2', 'stock team'),
    ('shortened-gun-respawn', data.index(b'"count" "15000"', entity_start) + len(b'"count" "'), b'10000', 'respawn rule'),
    ('shifted-weather-plane', plane_distance, struct.pack('<f', 1849), 'planes disagree'),
    ('missing-second-fall-brush', lump_offset(data, 7) + 7 * 40 + 36, struct.pack('<i', 1), 'expected trigger brushes'),
    ('wrong-local-fog', lump_offset(data, 12), b'x', 'stock global fog record'),
):
    mutated = output / f'ctf2-{label}.bsp'
    mutated.write_bytes(data[:offset] + replacement + data[offset + len(replacement):])
    report = inspect_wasteland(mutated)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    wasteland_rejections.append(dict(Mutation=label, Reason=report['Reason']))

name = 'maps/mp/siege_korriban.bsp'
data = maps[name]
entity_start, entity_length = struct.unpack_from('<ii', data, 8)
counter = re.search(rb'\{[^{}]*"targetname" "cyrstalsinplacecounter"[^{}]*\}',
                    data[entity_start:entity_start + entity_length])
assert counter
counter_count = entity_start + counter.start() + counter.group().index(b'"count" "3"') + len(b'"count" "')
lava_model = lump_offset(data, 7) + 40
lava_brush = struct.unpack_from('<i', data, lava_model + 32)[0]
lava_side = struct.unpack_from('<i', data, lump_offset(data, 8) + lava_brush * 12)[0]


def inspect_siege_korriban(bsp_path):
    args_for_profile = (['--section', str(bsp_path), profiles[name]] if name in profiles else
                        ['--siege-korriban', str(bsp_path)])
    native = subprocess.run([inspector, *args_for_profile], capture_output=True, text=True, timeout=20, check=True)
    return json.loads(native.stdout)


siege_path = next(row['File'] for row in evidence if row['Name'] == name)
siege = inspect_siege_korriban(siege_path)
assert siege['ContentAdmitted'] and siege['Models'] == 70 and siege['Entities'] == 691, siege


def verify_rancor_route(bsp):
    def lump(index):
        start, size = struct.unpack_from('<ii', bsp, 8 + 8 * index)
        return bsp[start:start + size]

    planes = list(struct.iter_unpack('<4f', lump(2)))
    sides = list(struct.iter_unpack('<3i', lump(9)))
    shaders = list(struct.iter_unpack('<64sII', lump(1)))
    first, count = struct.unpack_from('<ii', lump(7), 32)
    solids = [tuple(planes[sides[j][0]] for j in range(start, start + size))
              for i, (start, size, shader) in enumerate(struct.iter_unpack('<3i', lump(8)))
              if first <= i < first + count and shaders[shader][2] & 0x61]

    def inside_segment(start, end, radius, height, brush):
        enter, leave = 0.0, 1.0
        for x, y, z, distance in brush:
            boundary = distance + radius * (abs(x) + abs(y)) + max(0, -z * height)
            a = x * start[0] + y * start[1] + z * start[2] - boundary
            b = x * end[0] + y * end[1] + z * end[2] - boundary
            if a >= 0 and b >= 0:
                return False
            if a >= 0:
                enter = max(enter, a / (a - b))
            elif b >= 0:
                leave = min(leave, a / (a - b))
            if enter >= leave:
                return False
        return True

    def hits(start, end, radius, height):
        return any(inside_segment(start, end, radius, height, brush) for brush in solids)

    waypoints = {}
    for record in re.findall(rb'\{([^}]*)\}', lump(0)):
        fields = dict(re.findall(rb'"([^"]*)"\s*"([^"]*)"', record))
        if fields.get(b'classname') == b'waypoint':
            x, y, z = map(float, fields[b'origin'].split())
            waypoints[fields[b'targetname'].decode()] = ((x, y, z - 20), fields[b'target'].decode())
    graph = {name: set() for name in waypoints}
    blocked = []
    for name, (start, target) in waypoints.items():
        end = waypoints[target][0]
        assert not hits(start, start, 20, 48), name
        steps = max(1, math.ceil(math.dist(start, end) / 32))
        supported = True
        for i in range(steps + 1):
            x, y, z = (a + (b - a) * i / steps for a, b in zip(start, end))
            if not hits((x, y, z), (x, y, z - 48), 1, 1):
                supported = False
                break
        if supported and not hits(start, end, 60, 160):
            graph[name].add(target)
            graph[target].add(name)
        else:
            blocked.append([name, target])
    reachable, pending = set(), ['t38']
    while pending:
        point = pending.pop()
        if point not in reachable:
            reachable.add(point)
            pending.extend(graph[point] - reachable)
    assert len(reachable) == len(waypoints) == 18, (reachable, blocked)
    return dict(Waypoints=len(waypoints), ReachableFromSpawn=len(reachable), BlockedEdges=blocked)


siege_route = verify_rancor_route(data)
siege_graph = subprocess.run([str(args.build / 'jar_bsp_section_tests.exe'), siege_path, '--siege-korriban'],
                             capture_output=True, text=True, timeout=30, check=True)
(output / 'siege-korriban-graph.log').write_text(siege_graph.stdout + siege_graph.stderr, encoding='utf-8')
siege_rejections = []
for label, offset, replacement, reason in (
    ('swapped-objective', data.index(b'"objective" "2"', entity_start) + len(b'"objective" "'),
     b'3', 'stock arming or target'),
    ('wrong-crystal-checkpoint', data.index(b'"goaltarget" "redcrystaldelivery"', entity_start) + len(b'"goaltarget" "'),
     b'X', 'matching objective'),
    ('two-crystals-instead-of-three', counter_count, b'2', 'three-crystal counter'),
    ('changed-crusher-damage', data.index(b'"dmg" "999"', entity_start) + len(b'"dmg" "'),
     b'100', 'stock motion, sounds or targets'),
    ('redirected-pressure-pad', data.index(b'"target" "crusher_2"', entity_start) + len(b'"target" "crusher_'),
     b'1', 'stock crusher or wait'),
    ('redirected-rancor-waypoint', data.index(b'"target" "t38"', entity_start) + len(b'"target" "t3'),
     b'7', 'stock route'),
    ('changed-rancor-origin', data.index(b'"origin" "-1568 13088 -320"', entity_start) + len(b'"origin" "-156'),
     b'9', 'stock setup'),
    ('changed-scenery-cull-offset', data.index(b'"zoffset" "128"', entity_start) + len(b'"zoffset" "12'),
     b'9', 'stock scale or culling offset'),
    ('changed-lava-ambience', data.index(b'"artus_lava"', entity_start) + len(b'"artus_lav'),
     b'x', 'stock local loop'),
    ('missing-lava-plane', lump_offset(data, 9) + lava_side * 12, struct.pack('<i', -1),
     'plane reference is invalid'),
):
    mutated = output / f'siege-korriban-{label}.bsp'
    mutated.write_bytes(data[:offset] + replacement + data[offset + len(replacement):])
    report = inspect_siege_korriban(mutated)
    assert not report['ContentAdmitted'] and reason in report['Reason'], report
    siege_rejections.append(dict(Mutation=label, Reason=report['Reason']))

siege_definition = None
retail_assets = set()
for package in sorted((args.game_data / 'base').glob('assets*.pk3')):
    with zipfile.ZipFile(package) as archive:
        retail_assets.update(name.lower() for name in archive.namelist())
        if 'maps/mp/siege_korriban.siege' in archive.namelist():
            siege_definition = archive.read('maps/mp/siege_korriban.siege').decode('latin-1')
assert siege_definition
active = re.sub(r'//[^\r\n]*', '', siege_definition)
assert re.search(r'Objective1\s*\{[^}]*\btarget\s+"spawnrelay1"', active, re.S)
assert re.search(r'Objective6\s*\{[^}]*\bfinal\s+1\b', active, re.S)
assert re.search(r'\broundover_target\s+"siegeend"', active)
assert re.search(r'\bTimed\s+1800\b', active)
assert 'roundover_sound_wewon "sound/chars/protocol/misc/40mom011.mp3"' in active.lower()
assert 'roundover_sound_welost "sound/chars/protocol/misc/40mom012.mp3"' in active.lower()
assert {'music/goodsmall.mp3', 'music/badsmall.mp3'} <= retail_assets

scene_assets, effect_assets, offset_samples = set(), set(), []
for record in re.findall(rb'\{([^}]*)\}', data[entity_start:entity_start + entity_length]):
    fields = {key.lower(): val for key, val in re.findall(rb'"([^"]*)"\s*"([^"]*)"', record)}
    if fields.get(b'classname') == b'misc_model_static':
        scene_assets.add(fields[b'model'].decode().lower())
        if b'zoffset' in fields:
            x, y, z = map(float, fields[b'origin'].split())
            offset_samples.append(point_leaf(data, (x, y, z + float(fields[b'zoffset']) + 1))[0])
    if fields.get(b'classname') == b'fx_runner':
        path = fields[b'fxfile'].decode().lower().removeprefix('effects/').removesuffix('.efx')
        effect_assets.add('effects/' + path + '.efx')
audio_assets = {'sound/ambience/artus/artus_lava.wav', 'sound/effects/torch_burning_lp.wav'}
assert (scene_assets | effect_assets | audio_assets) <= retail_assets
vis_start, vis_length = struct.unpack_from('<ii', data, 8 + 8 * 16)
vis_clusters, vis_stride = struct.unpack_from('<ii', data, vis_start)
assert vis_clusters > 1 and vis_stride * 8 >= vis_clusters and vis_length == 8 + vis_clusters * vis_stride
assert len(offset_samples) == 11 and all(0 <= cluster < vis_clusters for cluster in offset_samples)
siege_media = dict(SceneryModels=len(scene_assets), EffectFiles=len(effect_assets), SoundFiles=len(audio_assets),
                   VisibilityClusters=vis_clusters, SceneryOffsetSamples=len(offset_samples))

result = dict(Maps=evidence, ContentAdmitted=len(profiles), HazardRejections=hazard_rejections,
              TatooineRejections=tatooine_rejections,
              TaspirRejections=taspir_rejections,
              FactoryRejections=factory_rejections,
              CoruscantRejections=coruscant_rejections,
              HilltopsRejections=hilltops_rejections,
              DropZoneRejections=dropzone_rejections,
              WastelandAdmission=wasteland, WastelandRejections=wasteland_rejections,
              SiegeKorribanAdmission=siege, SiegeKorribanGraph='siege-korriban-graph.log', SiegeKorribanRoute=siege_route,
              SiegeKorribanMedia=siege_media,
              SiegeKorribanRejections=siege_rejections,
              ShaftRejections=shaft_rejections,
              RiftRejections=rift_rejections,
              RiftColdAir=dict(ArenaArea=rift_area, CheckedEntryPoints=len(rift_entries)),
              KorribanFog=dict(ArenaArea=korriban_area, CheckedEntryPoints=len(korriban_entries)),
              KorribanRejections=korriban_rejections,
              SkyCameraRejection=sky_rejection['Reason'],
              HothSky=hoth_sky, YavinSky=yavin_sky, YavinRejections=yavin_rejections,
              BreakableRejections=breakable_rejections, GameLaunched=False, GPURendered=False,
              NativeSectionControllerPassed=True,
              Scope='Production content gate and section controller/parser callbacks on exact stock BSP bytes. Registration, native world collision, drawn rendering, full save relocation and gameplay remain separate.')
(output / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print(f'Retail portable BSP admission audit passed: {output}')
