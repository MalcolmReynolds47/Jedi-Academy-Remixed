"""Every map actor alias through the real MP module, in frozen batches of 16."""
import csv
import json
import math
import re
import struct
import time
from mp_world_smoke import checksum, decode


def check_aliases(root, mod, game_data, rcon):
    source = (root / 'codemp/game/jar_mp_catalog.h').read_text()
    actors = {}
    for line in source.splitlines():
        if not line.startswith(' {'):
            continue
        row = next(csv.reader([line.strip()[1:-2]]))
        if len(row) == 9:
            actors[int(row[0])] = row
    aliases = [(int(key), int(target), name) for key, target, name in
               re.findall(r'\{(\d+),(\d+),"([^"]+)"\}', source)]
    assert len(aliases) == 75 and len(actors) == 948
    rcon('set jar_worldLoad ""');rcon('set jar_worldAutoSave 0')
    assert 'world cleared' in rcon('jar_world_clear')
    rcon('map mp/ffa3');assert 'world cleared' in rcon('jar_world_clear')
    directory = mod / 'jar/worlds'
    assert 'Saved alias_seed revision 1' in rcon('jar_world_save alias_seed')
    seed = (directory / 'alias_seed.0.jrw').read_bytes()
    assert struct.unpack_from('<I', seed)[0] == 0x3657524a
    audit = []

    def readback(slot, batch):
        time.sleep(3.5)  # Native NPC begin/readiness must finish before capture.
        assert 'Saved '+slot+' revision 1' in rcon('jar_world_save '+slot)
        nodes = decode(directory / (slot+'.0.jrw'))['nodes']
        assert len(nodes) == len(batch), (slot, len(nodes), batch)
        for i, (node, (key, target, name)) in enumerate(zip(nodes, batch)):
            expected = (-12000+3000*(i % 4), -12000+3000*(i // 4), 12000)
            assert node[0] == key and node[1] == 1 and node[4] == 173, (name, node)
            assert math.dist(node[7:10], expected) < .05, (name, node[7:10])
        return nodes

    for start in range(0, len(aliases), 16):
        batch = aliases[start:start+16];slot = 'alias_'+str(start)
        out = bytearray(seed[:84]);struct.pack_into('<I',out,0,0x3557524a);struct.pack_into('<2I', out, 12, len(batch), 0)
        for i, (key, target, name) in enumerate(batch):
            row = actors[key];assert int(row[1]) in (2, 3) and int(row[2]) == 3
            radius, height, bottom = map(float, row[6:9])
            point = (-12000+3000*(i % 4), -12000+3000*(i // 4), 12000)
            out += struct.pack('<6I19fI7fI', key, 1, 0, 0, 173, 0, 200., *point, 0., 0., 0.,
                               -radius, -radius, bottom, radius, radius, bottom+height,
                               0., 0., 0., 0., 0., 0., 0, 3000., 0., 0., 0., 0., 0., 1., 0)
        out += struct.pack('<I', checksum(out));(directory / (slot+'.0.jrw')).write_bytes(out)
        assert 'Loaded '+slot in rcon('jar_world_load '+slot)
        readback(slot+'_seen', batch)
        assert 'world cleared' in rcon('jar_world_clear')
        assert 'Loaded '+slot+'_seen' in rcon('jar_world_load '+slot+'_seen')
        readback(slot+'_restored', batch)
        assert 'world cleared' in rcon('jar_world_clear')
        audit.extend(dict(id=key, target=target, map_class=name, native_definition=actors[key][3],
                          vehicle=int(actors[key][1]) == 3, spawned=True, restored=True)
                     for key, target, name in batch)
        (mod.parent / 'alias-audit.json').write_text(json.dumps(audit, indent=2)+'\n')
        print(f'Native map aliases: {len(audit)}/{len(aliases)} spawned/restored', flush=True)
    (mod.parent / 'alias-result.json').write_text(json.dumps(dict(native_module=True,
        map_spawner_aliases=len(audit), frozen_batches=5, largest_batch=16,
        original_ids_retained=True, all_spawned=True, all_restored=True, cleanup=True,
        grid_above_retail_map=True, graphical_clients=0, npc_behavior_test=False,
        vehicle_boarding_test=False, load_performance_test=False), indent=2)+'\n')
