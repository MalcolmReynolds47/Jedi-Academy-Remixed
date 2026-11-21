"""JRW6 split vehicle momentum through the real, private dedicated server."""
import csv
import json
import math
import struct
from mp_world_smoke import checksum, decode


def check_vehicle_motion(root, mod, game_data, rcon):
    catalog = {}
    for line in (root / 'codemp/game/jar_mp_catalog.h').read_text().splitlines():
        if line.startswith(' {'):
            row = next(csv.reader([line.strip()[1:-2]]))
            if len(row) == 9 and row[1] == '3':
                catalog[row[3]] = row
    rcon('set jar_worldLoad ""')
    rcon('set jar_worldAutoSave 0')
    rcon('set jar_worldAutoLoad 0')
    assert 'world cleared' in rcon('jar_world_clear')
    rcon('devmap mp/ffa3')
    rcon('set g_gravity 1')
    rcon('set d_npcfreeze 1')
    directory = mod / 'jar/worlds'
    assert 'Saved motion_seed revision 1' in rcon('jar_world_save motion_seed')
    seed = (directory / 'motion_seed.0.jrw').read_bytes()
    assert struct.unpack_from('<I', seed)[0] == 0x3657524a
    results = []
    try:
        for family, name in enumerate(('swoop_mp', 'tie-fighter', 'wildtauntaun', 'atst_vehicle')):
            row = catalog[name]
            radius, height, bottom = map(float, row[6:9])
            point = (-12000., -12000., 12000.)
            data = bytearray(seed[:84])
            struct.pack_into('<2I', data, 12, 1, 0)
            data += struct.pack('<6I19fI7fII4f', int(row[0]), 0, 0, 0, 317, 0, 200.,
                                *point, 0., 0., 0., -radius, -radius, bottom,
                                radius, radius, bottom + height, 80., 25., 40., 0., 0., 0.,
                                0, 3000., 0., 0., 0., 0., 0., 1., 0, 1, 0., 25., 0., 111.5)
            data += struct.pack('<I', checksum(data))
            slot = 'motion_' + str(family)
            (directory / (slot + '.0.jrw')).write_bytes(data)
            assert 'Loaded ' + slot + ' revision 1' in rcon('jar_world_load ' + slot)

            def capture(suffix):
                target = slot + suffix
                assert 'Saved ' + target + ' revision 1' in rcon('jar_world_save ' + target)
                world = decode(directory / (target + '.0.jrw'))
                assert world['version'] == 6 and len(world['nodes']) == 1
                node = world['nodes'][0]
                assert node[0] == int(row[0]) and 0 < node[4] <= 317 and node[34] == 1, (name, node)
                assert 0 < math.hypot(*node[35:37]) <= 25 and node[37] == 0, (name, node)
                assert all(math.isfinite(x) for x in (*node[19:22], node[38]))
                return node

            first = capture('_seen')
            assert math.dist(first[7:10], point) > .1, (name, first)
            assert 'world cleared' in rcon('jar_world_clear')
            assert 'Loaded ' + slot + '_seen revision 1' in rcon('jar_world_load ' + slot + '_seen')
            restored = capture('_restored')
            assert 0 < restored[36] < first[36], (name, first, restored)
            assert math.dist(restored[7:10], first[7:10]) > .1
            results.append(dict(name=name, first_velocity=first[19:22], restored_velocity=restored[19:22],
                                first_drift=first[35:38], restored_drift=restored[35:38],
                                first_speed=first[38], restored_speed=restored[38],
                                first_health=first[4], restored_health=restored[4]))
            assert 'world cleared' in rcon('jar_world_clear')
    finally:
        rcon('set g_gravity 800')
        rcon('set d_npcfreeze 0')
        rcon('set jar_worldAutoLoad 1')
    evidence = dict(world_write_version=6, native_module=True, empty_vehicle_families=results,
                    resumed_horizontal_drift=True, native_controller_speed_recorded=True,
                    exact_immediate_state_boundary='production fixture', graphical_clients=0,
                    occupied_human_driving=False, isolated_airspace=True)
    (mod.parent / 'vehicle-motion-result.json').write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print('Native JRW6 vehicle motion: four families saved, restored and continued coasting', flush=True)
