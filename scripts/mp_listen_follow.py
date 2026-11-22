"""Record a Sandbox follower crossing a clear path and a barricade on Flatgrass."""
import json
import math
import struct
import time

from mp_crowd_session import EF_FIRING, REBEL, STORMTROOPER
from mp_listen_platforms import listen_players, place, where
from mp_world_smoke import checksum, decode, legacy_header


def exercise_follow(mod, profile, run, stack, player, free_port):
    host, guest = listen_players(mod, profile, run, stack, player, free_port)
    worlds = host.profile / 'jar/worlds'
    for client in (host, guest):
        client.do('god')
    place(guest, -350, -550, 48, 45)
    place(host, -400, 0, 48, 0)
    host.do('+lookdown', 'wait 18', '-lookdown', 'cmd jar_spawn 22609847')
    time.sleep(1)
    host.do('jar_world_save crate')
    seed = worlds / 'crate.0.jrw'
    crate, = decode(seed)['nodes']
    assert crate[0] == 22609847
    mins, maxs = crate[13:16], crate[16:19]
    width = maxs[1] - mins[1]
    assert width > 20 and maxs[2] - mins[2] > 64
    wall = legacy_header(seed.read_bytes())
    struct.pack_into('<2I', wall, 12, 7, 0)
    for i in range(-3, 4):
        wall.extend(struct.pack('<6I16f', crate[0], 1, 0, 0, 0, 0, crate[6],
                                0, i * width, .75 - mins[2], 0, 0, 0, *mins, *maxs, 0, 0, 0))
    wall.extend(struct.pack('<I', checksum(wall)))
    (worlds / 'barricade.0.jrw').write_bytes(wall)
    host.do('jar_world_clear')
    observations = {}
    for name, owner, witness in (('direct', host, guest), ('wallhost', host, guest), ('wallguest', guest, host)):
        blocked = name != 'direct'
        place(witness, 50, -650 if witness is guest else 650, 48, 90 if witness is guest else 270)
        place(owner, -370, 0, 48, 180)
        if blocked:
            host.do('jar_world_load barricade')
            host.wait('Loaded barricade revision 1: 7 objects')
        # Face away from the wall so that the NPC is always on the near side.
        owner.do('+lookdown', 'wait 18', '-lookdown', f'cmd jar_spawn {REBEL}')
        time.sleep(1.2)
        host.do('jar_world_save ' + name)
        nodes = decode(worlds / f'{name}.0.jrw')['nodes']
        follower, = [n for n in nodes if n[0] == REBEL]
        assert len(nodes) == (8 if blocked else 1) and follower[7] < -160
        for client in (host, guest):
            client.record(name)
        time.sleep(.5)
        place(owner, 370, 0, 48, 180)
        time.sleep(10)
        witness.do('screenshot_png ' + name)
        goal = where(owner)
        for client in (host, guest):
            client.do('stoprecord')
        observations[name] = dict(owner=owner.name, witness=witness.name, goal=goal,
                                  min_y=-3 * width + mins[1], max_y=3 * width + maxs[1], blocked=blocked)
        host.do('jar_world_clear')
    # Two opposing riflemen have a clear line past the end of the same
    # barricade. Both have scenery closer to them than the other NPC.
    for client in (host, guest):
        client.do('notarget')
    arena = bytearray(wall[:-4])
    struct.pack_into('<I', arena, 12, 9)
    y = 3 * width + maxs[1] + 90
    for catalog, x, yaw in ((REBEL, -160, 0), (STORMTROOPER, 160, 180)):
        arena.extend(struct.pack('<6I16f', catalog, 0, 0, 0, 500, 0, 80,
                                 x, y, 24.5, 0, yaw, 0, -16, -16, -24, 16, 16, 40, 0, 0, 0))
    arena.extend(struct.pack('<I', checksum(arena)))
    (worlds / 'fight.0.jrw').write_bytes(arena)
    place(host, 0, y + 420, 48, 270)
    place(guest, -360, y + 240, 48, 315)
    for client in (host, guest):
        client.record('fight')
    host.do('jar_world_load fight')
    host.wait('Loaded fight revision 1: 9 objects')
    time.sleep(8)
    host.do('jar_world_save afterfight', 'screenshot_png fight')
    survivors = decode(worlds / 'afterfight.0.jrw')['nodes']
    observations['fight'] = {str(n[0]): n[4] for n in survivors if n[0] in (REBEL, STORMTROOPER)}
    host.do('jar_world_clear')
    time.sleep(.6)
    for client in (host, guest):
        client.do('stoprecord')
    return observations


def compare_follow(tool, run, read_demo):
    output = {}
    cases = json.loads((run / 'observations.json').read_text())
    for name, case in cases.items():
        home, guest = [read_demo(tool, next(run.glob(f'{peer}-{name}.dm_*'))) for peer in ('Host', 'Late')]
        times = sorted(home.keys() & guest.keys())
        assert len(times) >= 50 and times[-1] - times[0] >= 8000, (name, len(times))
        assert all(home[t] == guest[t] for t in times), name
        if name == 'fight':
            shooters = {slot for t in times for slot, row in home[t].items() if row[0] == 'n' and row[1] & EF_FIRING}
            output[name] = dict(shooters=len(shooters), remaining_health=case, cleanup_seen=not home[times[-1]],
                                shared_snapshots=len(times), elapsed_ms=times[-1] - times[0])
            continue
        path = []
        fired = 0
        for tick in times:
            prop_count = sum(actor[0] == 'p' for actor in home[tick].values())
            assert prop_count == (7 if case['blocked'] else 0), (name, tick, prop_count)
            npc, = [actor for actor in home[tick].values() if actor[0] == 'n']
            path.append(npc[2:])
            fired += bool(npc[1] & EF_FIRING)
        first, last = path[0], path[-1]
        assert first[0] < -160 and math.dist(first[:2], case['goal'][:2]) > 500
        gap = math.dist(last[:2], case['goal'][:2])
        crossing = next((point for point in path if point[0] >= 0), None)
        output[name] = dict(shared_snapshots=len(times), start=first, finish=last,
                            owner_distance=gap, crossing=crossing, elapsed_ms=times[-1] - times[0],
                            maximum_side_travel=max(abs(p[1]) for p in path), firing_snapshots=fired)
    (run / 'route-measurements.json').write_text(json.dumps(output, indent=2) + '\n')
    for name, row in output.items():
        if name == 'fight':
            health = row['remaining_health']
            assert row['shooters'] == 2 and row['cleanup_seen'], row
            # Native overheal decays by one per second. Require damage that
            # exceeds decay throughout the entire recording.
            assert len(health) == 2 and 500 - min(health.values()) > 1 + row['elapsed_ms'] / 1000, row
            continue
        assert row['owner_distance'] <= 130 and row['crossing'] and row['firing_snapshots'] == 0, (name, row)
        if name != 'direct':
            case = cases[name]
            assert row['crossing'][1] < case['min_y'] or row['crossing'][1] > case['max_y'], (name, row)
    return output
