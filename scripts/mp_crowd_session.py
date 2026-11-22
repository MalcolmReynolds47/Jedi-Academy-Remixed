"""Exercise rival squads, active props and a late join on the native MP wire."""
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import json
import math
import re
import struct
import time

from mp_network_smoke import await_state, snapshot_entities
from mp_world_smoke import checksum, decode, legacy_header


REBEL = 3002
STORMTROOPER = 36798905
PROP = 16940136
SQUAD_COUNTS = {REBEL: 48, STORMTROOPER: 48, PROP: 16}
NPC_COUNT = SQUAD_COUNTS[REBEL] + SQUAD_COUNTS[STORMTROOPER]
PROP_COUNT = SQUAD_COUNTS[PROP]
TOTAL = NPC_COUNT + PROP_COUNT
EF_DEAD = 1 << 1
EF_NODRAW = 1 << 8
EF_FIRING = 1 << 9


def living(frame):
    """Read entity numbers and flags from the open fixture's NPC snapshot field."""
    result = {}
    if frame[43] != '-':
        for row in frame[43].split(';'):
            number, flags, _ = row.split(',', 2)
            flags = int(flags)
            if not flags & (EF_DEAD | EF_NODRAW):
                result[int(number)] = flags
    return result


def prop_slots(frame):
    return set(snapshot_entities(frame[13]))


def crowd_world(seed, pairs=SQUAD_COUNTS[REBEL], props=PROP_COUNT):
    data = legacy_header(seed)
    struct.pack_into('<2I', data, 12, pairs * 2 + props, 0)

    def add(catalog_id, health, origin, yaw, bounds):
        data.extend(struct.pack('<6I16f', catalog_id, 0, 0, 0, health, 0,
                                100., *origin, 0., yaw, 0., *bounds, 0., 0., 0.))

    for i in range(pairs):
        for side, actor in ((-1, REBEL), (1, STORMTROOPER)):
            origin = (side * (224 + 80 * (i // 12)), (i % 12 - 5.5) * 80, 24.5)
            # High health keeps the same two opposing rosters alive throughout the sample.
            add(actor, 20000, origin, 0 if side < 0 else 180, (-16, -16, -24, 16, 16, 40))
    for i in range(props):
        add(PROP, 0, (0, (i - 7.5) * 64, 140 + 3 * i), 0, (-7, -7, -4, 7, 7, 5))

    data.extend(struct.pack('<I', checksum(data)))
    return data


def load_crowd(peer, world):
    _, sock, codec, _ = peer
    rcon, directory = world
    sock.send(codec.packet('notarget'))
    assert '0 objects, 0 attachments' in rcon('jar_world_save crowd_seed')
    (directory / 'crowd.0.jrw').write_bytes(crowd_world((directory / 'crowd_seed.0.jrw').read_bytes()))
    result = rcon('jar_world_load crowd')
    assert f'Loaded crowd revision 1: {TOTAL} objects' in result, result

    def fighting(frame):
        actors = living(frame)
        return (len(actors) == NPC_COUNT and len(prop_slots(frame)) == PROP_COUNT and
                any(flags & EF_FIRING for flags in actors.values()))

    frame = await_state(sock, codec, 'snapshot', fighting, timeout=20)
    return set(living(frame)), prop_slots(frame), int(frame[1])


def check_crowd(clients, world, initial, run):
    actors_expected, props_expected, combat_time = initial
    rcon, directory = world
    sample_after = max(combat_time, clients[1][3]['snapshot_time']) + 2000

    def sample(peer):
        _, sock, codec, _ = peer
        frame = await_state(sock, codec, 'snapshot', lambda f: int(f[1]) >= sample_after, timeout=20)
        start = previous = int(frame[1])
        gaps = []
        firing = set()
        last_ack = 0
        deadline = time.monotonic() + 65

        while True:
            actors = living(frame)
            assert actors.keys() == actors_expected, (frame[1], actors_expected ^ actors.keys())
            assert prop_slots(frame) == props_expected, (frame[1], props_expected ^ prop_slots(frame))
            firing.update(number for number, flags in actors.items() if flags & EF_FIRING)
            if previous - start >= 30000:
                break
            if time.monotonic() > deadline:
                raise AssertionError('Crowd did not complete 30 simulated seconds')
            if previous - last_ack >= 200:
                sock.send(codec.packet('move 0 0 0'))
                last_ack = previous
            frame = await_state(sock, codec, 'snapshot', lambda f: int(f[1]) > previous)
            stamp = int(frame[1])
            gaps.append(stamp - previous)
            previous = stamp

        assert firing and len(gaps) > 1
        return dict(start_ms=start, end_ms=previous, snapshot_intervals=len(gaps),
                    p95_snapshot_gap_ms=sorted(gaps)[math.ceil(.95 * len(gaps)) - 1],
                    max_snapshot_gap_ms=max(gaps), observed_firing_npcs=len(firing))

    def physics_steps():
        status = rcon('jar_physics_status')
        match = re.search(r'Box3D active; \d+ triangles; (\d+) fixed steps', status)
        assert match, status
        return int(match[1])

    clients[1][1].send(clients[1][2].packet('notarget'))
    before = physics_steps()
    with ThreadPoolExecutor(max_workers=len(clients)) as pool:
        metrics = dict(zip((peer[0] for peer in clients), pool.map(sample, clients)))
    steps = physics_steps() - before
    assert steps > 0

    assert f'{TOTAL} objects' in rcon('jar_world_save crowd_finished')
    nodes = decode(directory / 'crowd_finished.0.jrw')['nodes']
    assert Counter(node[0] for node in nodes) == SQUAD_COUNTS
    fallen = sum(node[0] == PROP and node[9] < 100 for node in nodes)
    assert fallen == PROP_COUNT, (fallen, PROP_COUNT)
    assert 'world cleared' in rcon('jar_world_clear')
    for name, sock, codec, _ in clients:
        await_state(sock, codec, 'snapshot', lambda f: int(f[1]) > metrics[name]['end_ms']
                    and not living(f) and not prop_slots(f))

    result = dict(npcs=NPC_COUNT, props=PROP_COUNT, already_fighting_before_second_join=True,
                  box3d_steps_during_window=steps, fallen_props=fallen, cleanup_seen_by_both=True,
                  loopback_only=True, graphical_clients=0, snapshot_mode='full (wire probe requests no delta)',
                  client_rate=25000, requested_snaps=20, clients=metrics)
    (run / 'crowd-session.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
