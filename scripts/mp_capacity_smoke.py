"""Native capacity and file-format checks in the smoke test's private world.

Frozen, separated objects in empty space test allocation/restore, not playability,
rendered crowds, useful map layout, active simulation rate or network performance.
"""
import json
import re
import struct
import time
from mp_world_smoke import checksum, decode, legacy_header


def check_capacity(root, mod, game_data, rcon):
    rcon('set jar_worldLoad ""');rcon('set jar_worldAutoSave 0')
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Saved capacity_seed revision 1' in rcon('jar_world_save capacity_seed')
    directory = mod / 'jar/worlds'
    seed = (directory / 'capacity_seed.0.jrw').read_bytes()
    catalog = (root / 'codemp/game/jar_mp_catalog.h').read_text()
    audit = []
    for name, limit, health in [('models/chunks/crate/crate2_1.md3', 4096, 0), ('stormtrooper2', 513, 173)]:
        match = re.search(r'\{(\d+),(\d+),\d+,"' + re.escape(name) + r'","[^"]*","[^"]*",([^,]+),([^,]+),([^}]+)\}', catalog)
        assert match
        ident, kind = map(int, match.groups()[:2]);radius, height, bottom = map(float, match.groups()[2:])
        slot = 'capacity_props' if kind == 1 else 'capacity_actors'

        def fixture(count, target):
            data = legacy_header(seed);struct.pack_into('<2I', data, 12, count, 0)
            for i in range(count):
                position = (-12000 + 96*(i % 64), -12000 + 96*(i // 64), 12000.)
                data += struct.pack('<6I16f', ident, 1, 0, 1, health, 0, 80., *position, 0., 0., 0.,
                                    -radius, -radius, bottom, radius, radius, bottom+height, 0., 0., 0.)
            data += struct.pack('<I', checksum(data));(directory / (target+'.0.jrw')).write_bytes(data)

        if kind == 1:
            fixture(limit+1, slot+'_overflow')
            rejected = rcon('jar_world_load '+slot+'_overflow')
            assert 'Not enough entity, NPC, attachment or model capacity' in rejected, rejected
            assert 'World: 0 objects, 0 attachments' in rcon('jar_world_status')
        fixture(limit, slot)
        assert f'Loaded {slot} revision 1: {limit} objects' in rcon('jar_world_load '+slot)
        time.sleep(4)
        assert f'World: {limit} objects, 0 attachments, 0 pending native members' in rcon('jar_world_status')
        saved = slot+'_saved'
        assert f'Saved {saved} revision 1: {limit} objects' in rcon('jar_world_save '+saved)
        readback = decode(directory / (saved+'.0.jrw'))
        assert len(readback['nodes']) == limit and all(n[0]==ident and n[1]==1 and n[4]==health for n in readback['nodes'])
        assert 'world cleared' in rcon('jar_world_clear')
        assert f'Loaded {saved} revision 1: {limit} objects' in rcon('jar_world_load '+saved)
        time.sleep(4)
        assert f'World: {limit} objects, 0 attachments, 0 pending native members' in rcon('jar_world_status')
        assert 'world cleared' in rcon('jar_world_clear')
        audit.append(dict(kind=kind, count=limit, overflow_rejected_atomically=kind == 1, native_save_reload=True, world_version=readback['version']))
    (mod.parent / 'capacity-result.json').write_text(json.dumps(dict(
        cases=audit, frozen_separated_objects=True, native_module=True,
        graphical_clients=0, active_physics_performance_tested=False,
        human_multiplayer_tested=False), indent=2)+'\n')
