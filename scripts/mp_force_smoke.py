"""Actual native movement with server-owned actors; uses the existing private server."""
import json
import struct
import time
from mp_world_smoke import legacy_header, checksum, decode


def check_forces(mod, rcon, seed, position, entry):
    directory = mod / 'jar/worlds'
    results = []
    # A small positive gravity avoids the retail zero-G jump/shove branch.
    rcon('set g_gravity 1')
    rcon('set d_npcfreeze 1')
    try:
        cases = [('stormtrooper2', 'baseline'), ('stormtrooper2', 'thruster'), ('stormtrooper2', 'rope')]
        cases += [(name, 'thruster') for name in ('swoop_mp', 'tie-fighter', 'wildtauntaun', 'atst_vehicle')]
        for index, (name, mode) in enumerate(cases):
            gravity = 1 if mode == 'baseline' else 800
            rcon('set g_gravity ' + str(gravity))
            actor = entry(name)
            # No native freeze: position must change through real NPC/vehicle
            # movement and world collision, not an editor teleport or test hook.
            # Stay below native max health so overhealth decay is not a force effect.
            node = struct.pack('<6I16f', actor[0], 0, int(mode == 'thruster'), 0, 10, 0,
                               80., *position, 0., 0., 0., *actor[2], *actor[3], 0., 0., 0.)
            data = legacy_header(seed)
            struct.pack_into('<2I', data, 12, 1, int(mode == 'rope'))
            data += node
            if mode == 'rope':
                anchor = (position[0], position[1], position[2] + 300)
                data += struct.pack('<ii7f', 0, -1, 0., 0., 0., *anchor, 32.)
            data += struct.pack('<I', checksum(data))
            (directory / 'force.0.jrw').write_bytes(data)
            assert 'Loaded force revision 1' in rcon('jar_world_load force'), (name, mode)
            time.sleep(1)
            slot = 'force_result_' + str(index)
            assert '1 objects' in rcon('jar_world_save ' + slot), (name, mode)
            saved = decode(directory / (slot + '.0.jrw'))['nodes'][0]
            delta = [saved[7+a] - position[a] for a in range(3)]
            result = dict(name=name, mode=mode, gravity=gravity, health=saved[4], displacement=delta, velocity=saved[19:22])
            results.append(result)
            # Save intermediate evidence even when a native family exposes a gap.
            (mod.parent / 'force-result.json').write_text(json.dumps(dict(native_module=True, connected_players=0, map='mp/ffa5', ai_decisions_frozen=True, native_movement_and_collision=True, cases=results), indent=2)+'\n', encoding='utf-8')
            assert saved[4] == 10, result
            if mode == 'baseline':
                assert abs(delta[2]) < 16, result
            else:
                assert delta[2] > 40, result
            assert 'world cleared' in rcon('jar_world_clear')
    finally:
        rcon('set g_gravity 800')
        rcon('set d_npcfreeze 0')
