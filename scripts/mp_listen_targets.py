"""Verify that passive props cannot hide a nearer player from MP enemy acquisition."""
import json
import math
import struct
import time

from mp_crowd_session import PROP, STORMTROOPER
from mp_listen_alerts import read_alert_demo
from mp_listen_platforms import listen_players, place
from mp_world_smoke import checksum, decode, legacy_header


def target_world(seed, count):
    arena = legacy_header(seed)
    struct.pack_into('<2I', arena, 12, count + 1, 0)
    arena.extend(struct.pack('<6I16f', STORMTROOPER, 0, 0, 0, 1000, 0, 80,
                             -300, -220, 24.5, 0, 0, 0, -16, -16, -24, 16, 16, 40, 0, 0, 0))
    # Flatgrass splits the server's spatial index at y=0. These stock
    # crate fragments overlap the split above both unobstructed sight lines.
    for i in range(count):
        x = (i % 32 - 16) * 18
        z = 200 + 14 * (i // 32)
        arena.extend(struct.pack('<6I16f', PROP, 1, 0, 0, 0, 0, 10,
                                 x, 0, z, 0, 0, 0, -7, -7, -4, 7, 7, 5, 0, 0, 0))
    arena.extend(struct.pack('<I', checksum(arena)))
    return arena


def exercise_targets(mod, profile, run, stack, player, free_port):
    host, guest = listen_players(mod, profile, run, stack, player, free_port)
    worlds = host.profile / 'jar/worlds'
    host.do('jar_world_save seed')
    seed = (worlds / 'seed.0.jrw').read_bytes()
    phases = {}
    # Also put the host in the nearer spot and ignore an otherwise eligible
    # nearer target. Both players keep their weapons idle throughout.
    for label, count, near, far, ignore in (
        ('empty', 0, guest, host, False), ('below', 192, guest, host, False),
        ('above', 320, guest, host, False), ('reversed', 512, host, guest, False),
        ('ignored', 512, guest, host, True),
    ):
        # Clear the old far position before swapping the two players.
        place(near, -850, -650, 48, 0)
        place(far, 220, 0, 48, 180)
        place(near, 0, -220, 48, 180)
        for peer in (near, far):
            peer.do('give armor', 'give health')
        if ignore:
            near.do('notarget')
        (worlds / f'{label}.0.jrw').write_bytes(target_world(seed, count))
        for peer in (host, guest):
            peer.record(label)
        host.do('jar_world_load ' + label)
        host.wait(f'Loaded {label} revision 1: {count + 1} objects')
        time.sleep(3.5)
        host.do('jar_world_save after' + label)
        nodes = decode(worlds / f'after{label}.0.jrw')['nodes']
        assert len(nodes) == count + 1 and sum(n[0] == PROP for n in nodes) == count
        for peer in (host, guest):
            peer.do('stoprecord')
        host.do('jar_world_clear')
        if ignore:
            near.do('notarget')
        phases[label] = dict(props=count, nearest=near.name, expected=far.name if ignore else near.name)
    return phases


def compare_targets(reader, run):
    cases = json.loads((run / 'observations.json').read_text())
    results = {}
    for label, case in cases.items():
        demos = {peer: read_alert_demo(reader, next(run.glob(f'{peer}-{label}.dm_*')))
                 for peer in ('Host', 'Late')}
        times = sorted(demos['Host'].keys() & demos['Late'].keys())
        assert len(times) >= 70 and all(demos['Host'][t].npc == demos['Late'][t].npc for t in times)
        aimed = dict(Host=0, Late=0)
        damage = {}
        for peer, frames in demos.items():
            assert all(not frames[t].firing for t in times)
            damage[peer] = max(frames[t].armor for t in times) - min(frames[t].armor for t in times)
        for t in times:
            npc = demos['Host'][t].npc
            if not npc.firing:
                continue
            errors = {}
            for peer, frames in demos.items():
                p = frames[t]
                bearing = math.degrees(math.atan2(p.y - npc.y, p.x - npc.x))
                errors[peer] = abs((npc.yaw - bearing + 180) % 360 - 180)
            aimed[min(errors, key=errors.get)] += 1
        results[label] = dict(**case, shared_snapshots=len(times), aimed_at=aimed, armor_lost=damage)
    (run / 'target-measurements.json').write_text(json.dumps(results, indent=2) + '\n')
    for label, row in results.items():
        expected = row['expected']
        other = 'Late' if expected == 'Host' else 'Host'
        assert row['aimed_at'][expected] and not row['aimed_at'][other], (label, row)
        assert row['armor_lost'][expected] and not row['armor_lost'][other], (label, row)
    return results
