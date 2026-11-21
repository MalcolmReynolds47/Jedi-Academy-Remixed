"""Qualify the SP curated IDs through the private native MP server."""
import csv
import json
import math
import re
import struct
import time
import zipfile

from mp_world_smoke import checksum, decode, legacy_header


PROP_IDS = set(range(1001, 1021))
NPC_IDS = set(range(3001, 3013))
VEHICLE_IDS = {4001, 4002}
ACTOR_HEALTH = 173
MAP_FILE = 'maps/mp/ffa3.bsp'
ACTOR_LINE = re.compile(
    r'Actor (\d+) entity (\d+) class (\d+) team (-?\d+) enemy (-?\d+) gravity (\d+) '
    r'health (\d+) ammo (\d+) model (\S+) gla (\S+) pose (\d+) frames (\d+) bounds ([^\r\n]+)'
)


def curated_cards(root, game_data):
    source = (root / 'code/game/g_workshop.cpp').read_text(encoding='utf-8-sig')
    props = {int(ident): (label, model, int(size), int(fingerprint))
             for ident, label, model, size, fingerprint in re.findall(
                 r'\{(10\d\d),"([^"]+)","([^"]+)",(\d+),(\d+)u,', source)}
    actors = {int(ident): (name, model, vehicle == 'true')
              for ident, name, model, vehicle in re.findall(
                  r'\{([34]\d{3}),"([^"]+)","([^"]+)",\d+u,\d+u,\d+u,[\d.]+,[\d.]+,(true|false)', source)}
    assert props.keys() == PROP_IDS and actors.keys() == NPC_IDS | VEHICLE_IDS
    rows = {}
    catalog = (root / 'codemp/game/jar_mp_catalog.h').read_text(encoding='utf-8')
    for line in catalog.split('jarMPCatalog[]', 1)[1].split('};', 1)[0].splitlines():
        if line.startswith(' {'):
            row = next(csv.reader([line.strip()[1:-2]]))
            ident = int(row[0])
            if ident in props or ident in actors:
                assert ident not in rows
                rows[ident] = row
    assert rows.keys() == props.keys() | actors.keys(), (props.keys() | actors.keys()) - rows.keys()

    assets = {}
    needed = {row[4].lower() for row in rows.values()}
    for pack in sorted((game_data / 'base').glob('assets*.pk3')):
        with zipfile.ZipFile(pack) as archive:
            for name in archive.namelist():
                if name.lower() in needed:
                    assets[name.lower()] = (pack.name, archive.read(name))
    assert assets.keys() == needed, needed - assets.keys()

    result = {}
    for ident, row in sorted(rows.items()):
        kind, tab = map(int, row[1:3])
        name, model = row[3:5]
        pack, data = assets[model.lower()]
        card = dict(id=ident, kind=kind, tab=tab, name=name, model=model, archive=pack)
        if ident in props:
            label, expected_model, size, fingerprint = props[ident]
            assert (kind, tab, model) == (1, 0, expected_model), (ident, row)
            assert len(data) == size and checksum(data) == fingerprint, (ident, model)
            assert struct.unpack_from('<2i', data) == (0x33504449, 15), model
            frames = struct.unpack_from('<i', data, 76)[0]
            offset = struct.unpack_from('<i', data, 92)[0]
            end = struct.unpack_from('<i', data, 104)[0]
            assert 1 <= frames <= 65536 and 108 <= offset < end <= len(data) and frames * 56 <= end - offset, model
            bounds = struct.unpack_from('<6f', data, offset)
            assert all(math.isfinite(v) for v in bounds) and all(bounds[i] < bounds[i+3] for i in range(3)), model
            card.update(label=label, stock_bytes=size, stock_fnv=fingerprint, stock_frames=frames, stock_bounds=bounds)
        else:
            expected_name, expected_model, vehicle = actors[ident]
            assert vehicle == (ident in VEHICLE_IDS)
            assert (kind, tab, name, model) == (3 if vehicle else 2, 4 if vehicle else 1, expected_name, expected_model), row
            assert data[:4] == b'2LGM', model
        result[ident] = card
    return result


def world_ready(rcon, count):
    expected = f'World: {count} objects, 0 attachments, 0 pending native members; map {MAP_FILE}.'
    deadline = time.monotonic() + 8
    while True:
        status = rcon('jar_world_status')
        if expected in status:
            return
        assert time.monotonic() < deadline, status


def write_fixture(directory, seed, cards):
    out = legacy_header(seed)
    struct.pack_into('<2I', out, 12, len(cards), 0)
    for index, (ident, card) in enumerate(cards.items()):
        # Frozen instances in separated airspace give native constructors room
        # to run. Tiny placeholder hulls require a fresh native model/hull read.
        point = (-12000 + 4000 * (index % 6), -12000 + 4000 * (index // 6), 12000)
        health = 0 if card['kind'] == 1 else ACTOR_HEALTH
        out += struct.pack('<6I16f', ident, 1, 0, 0, health, 0, 200., *point,
                           0., 0., 0., -1., -1., -1., 1., 1., 1., 0., 0., 0.)
        card['origin'] = point
    out += struct.pack('<I', checksum(out))
    (directory / 'curated_input.0.jrw').write_bytes(out)


def load_catalog(rcon, slot, count):
    reply = rcon('jar_world_load ' + slot)
    assert f'Loaded {slot} revision 1: {count} objects, 0 attachments' in reply, reply
    assert 'Skin name exceeds MAX_QPATH' not in reply, reply


def capture(rcon, directory, slot, cards):
    world_ready(rcon, len(cards))
    assert f'Saved {slot} revision 1: {len(cards)} objects, 0 attachments' in rcon('jar_world_save ' + slot)
    saved = decode(directory / (slot + '.0.jrw'))
    assert saved['map'] == MAP_FILE and not saved['ropes'], saved
    nodes = {node[0]: node for node in saved['nodes']}
    assert nodes.keys() == cards.keys() and len(saved['nodes']) == len(cards), saved
    for ident, card in cards.items():
        node = nodes[ident]
        assert node[1] == 1 and math.dist(node[7:10], card['origin']) < .01, (ident, node)
        bounds = node[13:19]
        assert all(math.isfinite(v) for v in bounds) and all(bounds[i] < bounds[i+3] for i in range(3)), (ident, bounds)
        if card['kind'] == 1:
            assert node[33] == 0 and math.dist(bounds, card['stock_bounds']) < .01, (ident, bounds, card['stock_bounds'])
        else:
            assert node[4] == ACTOR_HEALTH, (ident, node[4])
    return nodes


def check_native_actors(root, rcon, cards):
    teams = (root / 'codemp/game/teams.h').read_text(encoding='utf-8')
    classes = list(dict.fromkeys(re.findall(r'\bCLASS_\w+\b', re.sub(r'//[^\n]*|/\*[\s\S]*?\*/', '', teams))))
    vehicle_class = classes.index('CLASS_VEHICLE')
    entities = set()
    for ident in sorted(NPC_IDS | VEHICLE_IDS):
        card = cards[ident]
        reply = rcon(f'jar_actor_info {ident}')
        match = ACTOR_LINE.search(reply)
        assert match and int(match[1]) == ident, (card, reply)
        entity, npc_class = map(int, match.groups()[1:3])
        assert entity not in entities and 0 <= npc_class < classes.index('CLASS_NUM_CLASSES'), (ident, entity, npc_class)
        entities.add(entity)
        assert (npc_class == vehicle_class) == (ident in VEHICLE_IDS), (ident, classes[npc_class])
        assert int(match[7]) == ACTOR_HEALTH and match[9].lower() == card['model'].lower(), (card, reply)
        assert match[10].startswith('models/') and int(match[11]) == 1 and int(match[12]) > 0, (card, reply)
        bounds = tuple(map(float, match[13].split()))
        assert len(bounds) == 6 and all(math.isfinite(v) for v in bounds) and all(bounds[i] < bounds[i+3] for i in range(3)), (ident, bounds)
        card.update(native_entity=entity, native_class=classes[npc_class], native_health=int(match[7]),
                    native_gla=match[10], native_frames=int(match[12]), native_pose_bounds=bounds)
        if ident in VEHICLE_IDS:
            rig = rcon(f'jar_vehicle_rig {ident}')
            driver = re.search(rf'Rig {ident} phase 0 \*driver at (.*?) dir ([^\r\n]+)', rig)
            assert driver and 'Vehicle rig copy released' in rig, (card, rig)
            point = tuple(map(float, driver[1].split()))
            direction = tuple(map(float, driver[2].split()))
            assert len(point) == len(direction) == 3 and all(math.isfinite(v) for v in point + direction), rig
            assert abs(math.sqrt(sum(v * v for v in direction)) - 1) < .05, rig
            card['native_driver_bolt'] = point
    return entities


def check_curated_catalog(root, mod, game_data, rcon):
    cards = curated_cards(root, game_data)
    rcon('set jar_worldAutoSave 0')
    assert 'world cleared' in rcon('jar_world_clear')
    world_ready(rcon, 0)
    directory = mod / 'jar/worlds'
    assert 'Saved curated_seed revision 1: 0 objects, 0 attachments' in rcon('jar_world_save curated_seed')
    write_fixture(directory, (directory / 'curated_seed.0.jrw').read_bytes(), cards)
    load_catalog(rcon, 'curated_input', len(cards))
    observed = capture(rcon, directory, 'curated_observed', cards)
    live_actors = check_native_actors(root, rcon, cards)

    assert 'world cleared' in rcon('jar_world_clear')
    world_ready(rcon, 0)
    entity_list = rcon('entitylist').replace('\ufffd' * 4 + 'print\n', '')
    remaining = {int(num) for num in re.findall(r'^\s*(\d+):', entity_list, re.M)}
    assert remaining and not live_actors & remaining and 'jar_prop' not in entity_list, entity_list
    load_catalog(rcon, 'curated_observed', len(cards))
    restored = capture(rcon, directory, 'curated_restored', cards)
    for ident, card in cards.items():
        assert math.dist(observed[ident][13:19], restored[ident][13:19]) < .01, ident
        card['native_collision_hull'] = observed[ident][13:19]

    assert 'world cleared' in rcon('jar_world_clear')
    world_ready(rcon, 0)
    assert 'Saved curated_cleared revision 1: 0 objects, 0 attachments' in rcon('jar_world_save curated_cleared')
    cleared = decode(directory / 'curated_cleared.0.jrw')
    assert not cleared['nodes'] and not cleared['ropes'], cleared
    report = dict(native_module=True, map='mp/ffa3', stock_props=len(PROP_IDS), npcs=len(NPC_IDS),
                  vehicles=len(VEHICLE_IDS), spawn_path='native world restore via SpawnAtPose',
                  native_round_trip=True, cleanup=True, graphical_clients=0, cards=list(cards.values()))
    evidence = mod.parent / 'curated-catalog-result.json'
    evidence.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'Native MP curated catalog: {len(cards)} cards spawned, saved, restored and cleared: {evidence}', flush=True)
