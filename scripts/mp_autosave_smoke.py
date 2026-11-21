"""Automatic persistence through the actual private native server and filesystem."""
import json
import re
import struct
import time
from mp_world_smoke import legacy_header, checksum, decode, spawn_points


def latest(directory, slot):
    valid = []
    for path in directory.glob(slot + '.*.jrw'):
        try:
            saved = decode(path)
            valid.append((path, saved))
        except (AssertionError, ValueError, struct.error):
            pass
    assert valid, slot
    return max(valid, key=lambda value: value[1]['generation'])


def check_autosave(root, mod, game_data, rcon):
    directory = mod / 'jar/worlds'
    rcon('set jar_worldLoad ""')
    rcon('set jar_worldAutoSave 30')
    rcon('set jar_worldAutoLoad 1')
    assert 'world cleared' in rcon('jar_world_clear')
    rcon('map mp/ffa3')
    status = rcon('jar_world_status')
    assert 'World: 0 objects, 0 attachments' in status
    match = re.search(r'Autosave (auto_[0-9a-f]+_[0-9a-f]+) \(30s\)', status)
    assert match, status
    slot = match[1]
    baseline = latest(directory, slot)[1]['generation']
    seed = (directory / 'seed.0.jrw').read_bytes()
    catalog = (root / 'codemp/game/jar_mp_catalog.h').read_text(encoding='utf-8')
    match = re.search(r'\{(\d+),1,\d+,"models/chunks/crate/crate2_1.md3","[^"]*","[^"]*",([^,]+),([^,]+),([^}]+)\}', catalog)
    assert match
    ident = int(match[1]);radius, height, bottom = map(float, match.groups()[1:])
    point = spawn_points(game_data, 'maps/mp/ffa3.bsp')[0]
    def fixture(name, mass):
        data = legacy_header(seed);struct.pack_into('<2I', data, 12, 1, 1)
        data += struct.pack('<6I16f', ident, 1, 0, 1, 0, 0, mass, *point, 0., 25., 0., -radius, -radius, bottom, radius, radius, bottom+height, 0., 0., 0.)
        anchor = (*point[:2], point[2] + 64)
        data += struct.pack('<ii7f', 0, -1, 0., 0., 0., *anchor, 64.)
        data += struct.pack('<I', checksum(data))
        (directory / (name+'.0.jrw')).write_bytes(data)
    fixture('auto_fixture', 333.)
    assert 'Loaded auto_fixture revision 1' in rcon('jar_world_load auto_fixture')
    deadline = time.monotonic() + 45
    while time.monotonic() < deadline:
        saved = latest(directory, slot)[1]
        if saved['generation'] > baseline and len(saved['nodes']) == 1:
            break
        time.sleep(.25)
    assert saved['generation'] > baseline and saved['nodes'][0][6] == 333. and len(saved['ropes']) == 1, saved
    periodic_generation = saved['generation']
    fixture('auto_changed', 444.)
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded auto_changed revision 1' in rcon('jar_world_load auto_changed')
    # No timer wait: map shutdown must capture the changed world immediately.
    rcon('map mp/ffa1')
    saved = latest(directory, slot)[1]
    assert saved['generation'] > periodic_generation and saved['nodes'][0][6] == 444.
    status = rcon('jar_world_status')
    assert 'World: 0 objects, 0 attachments' in status and slot not in status
    rcon('map mp/ffa3')
    assert 'World: 1 objects, 1 attachments, 0 pending native members' in rcon('jar_world_status')
    assert 'Saved auto_observed revision 1' in rcon('jar_world_save auto_observed')
    observed = decode(directory / 'auto_observed.0.jrw')
    assert observed['nodes'][0][6] == 444. and observed['ropes'][0][1] == -1
    rcon('map mp/ffa1')
    corrupt, before = latest(directory, slot)
    # A torn latest revision must fall back to the previous checked revision.
    damaged = bytearray(corrupt.read_bytes());damaged[0] ^= 1;corrupt.write_bytes(damaged)
    fallback = latest(directory, slot)[1]
    assert fallback['generation'] < before['generation']
    rcon('map mp/ffa3')
    assert 'World: 1 objects, 1 attachments, 0 pending native members' in rcon('jar_world_status')
    assert 'Saved auto_recovered revision 1' in rcon('jar_world_save auto_recovered')
    recovered = decode(directory / 'auto_recovered.0.jrw')
    assert recovered['nodes'][0][6] == fallback['nodes'][0][6]
    evidence = dict(native_module=True, graphical_clients=0, map='mp/ffa3', slot=slot,
                    interval_seconds=30, periodic_generation=periodic_generation,
                    periodic_save=True, map_shutdown_save=True, map_specific_restore=True,
                    world_rope_restored=True, corrupt_latest_fallback=True,
                    recovered_generation=fallback['generation'], orderly_quit_saved_clear=False)
    (mod.parent / 'autosave-result.json').write_text(json.dumps(evidence, indent=2)+'\n', encoding='utf-8')
    return slot
