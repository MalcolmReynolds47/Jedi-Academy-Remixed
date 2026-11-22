"""Check player gunfire behind a multiplayer NPC on listen and dedicated servers."""
import json
import math
import secrets
import struct
import subprocess
import time
from contextlib import ExitStack
from typing import NamedTuple

from mp_crowd_session import REBEL, STORMTROOPER
from mp_listen_platforms import listen_players, place
from mp_world_smoke import checksum, decode, legacy_header


def arena(worlds, command):
    command('jar_world_save seed')
    seed = worlds / 'seed.0.jrw'
    deadline = time.monotonic() + 5
    while not seed.exists() and time.monotonic() < deadline:
        time.sleep(.05)
    world = legacy_header(seed.read_bytes())
    struct.pack_into('<2I', world, 12, 1, 0)
    # Face east, away from the player. Neither the NPC nor a wall is in
    # the path of the player's westbound test shots.
    for name, catalog in (('sentry', STORMTROOPER), ('friendly', REBEL), ('barrier', STORMTROOPER)):
        state = world + struct.pack('<6I16f', catalog, 0, 0, 0, 500, 0, 80,
                                    0, 0, 24.5, 0, 0, 0, -16, -16, -24, 16, 16, 40, 0, 0, 0)
        if name == 'barrier':
            # Frozen Hoth crates. These bounds were saved by the stock
            # crate spawn; the row blocks a shot between x=-202 and x=0.
            struct.pack_into('<I', state, 12, 10)
            for offset in range(-4, 5):
                state.extend(struct.pack('<6I16f', 22609847, 1, 0, 0, 0, 0, 100,
                                         -100, offset * 75, 1.6, 0, 0, 0,
                                         -23.8283, -39.3393, -.8345, 21.7174, 37.1462, 92.9262, 0, 0, 0))
        state.extend(struct.pack('<I', checksum(state)))
        (worlds / (name + '.0.jrw')).write_bytes(state)


def trial(label, shooter, command, worlds, witness=None, ignore=False):
    place(shooter, -202, 0, 48, 180)
    shooter.do('cmd jar_equip 69736479', 'give ammo', 'give health', 'give armor')
    if ignore:
        shooter.do('notarget')
    players = [shooter] if witness is None else [shooter, witness]
    world = {'ally': 'friendly', 'cover': 'barrier'}.get(label, 'sentry')
    command('jar_world_load ' + world)
    for phase in ('quiet', 'shots'):
        for p in players:
            p.record(label + phase)
        if phase == 'shots':
            shooter.do('+attack')
        time.sleep(.65 if phase == 'quiet' else 2.4)
        if phase == 'shots':
            shooter.do('-attack', 'screenshot_png ' + label)
        for p in players:
            p.do('stoprecord')
    command('jar_world_save after' + label)
    after = worlds / ('after' + label + '.0.jrw')
    deadline = time.monotonic() + 5
    while not after.exists() and time.monotonic() < deadline:
        time.sleep(.05)
    nodes = decode(after)['nodes']
    assert len(nodes) == (10 if label == 'cover' else 1)
    npc, = [n for n in nodes if n[0] in (STORMTROOPER, REBEL)]
    assert npc[0] == (REBEL if label == 'ally' else STORMTROOPER) and npc[4] >= 480, npc[:5]
    command('jar_world_clear')
    if ignore:
        shooter.do('notarget')
    return dict(shooter=shooter.name, witness=witness.name if witness else None,
                npc_health=npc[4], ignored=ignore)


def exercise_alerts(mod, profile, run, stack, player, free_port):
    results = {}
    with ExitStack() as session:
        host, guest = listen_players(mod, profile / 'listen', run, session, player, free_port)
        worlds = host.profile / 'jar/worlds'
        host.do('g_forcerespawn 0', 'notarget')
        place(host, 1500, 800, 48, 180)
        arena(worlds, host.do)
        results['alive'] = trial('alive', guest, host.do, worlds, host)
        host.do('kill')
        results['dead'] = trial('dead', guest, host.do, worlds, host)
        results['ignored'] = trial('ignored', guest, host.do, worlds, host, ignore=True)
        results['ally'] = trial('ally', guest, host.do, worlds, host)
        results['cover'] = trial('cover', guest, host.do, worlds, host)

    # A listen server ends with its host. Use a dedicated server to check
    # what happens when slot zero actually leaves and slot one remains.
    port = free_port()
    address = f'127.0.0.1:{port}'
    password = secrets.token_hex(12)
    server_home = profile / 'dedicated'
    args = [str(mod / 'jar_server.x86_64.exe')]
    settings = dict(fs_basepath=mod.parent, fs_homepath=server_home, fs_game=mod.name,
                    dedicated=1, net_ip='127.0.0.1', net_port=port, net_enabled=1, sv_pure=0,
                    rconPassword=password, sv_fps=40, sv_maxclients=4, sv_master1='', sv_master2='',
                    g_gametype=0, g_teamAutoJoin=1, bot_enable=0, jar_sandbox=1, sv_floodProtect=0,
                    jar_worldAutoSave=0, jar_worldAutoLoad=0, g_spawnInvulnerability=0, logfile=2)
    for key, value in settings.items():
        args += ['+set', key, str(value)]
    with (run / 'dedicated.log').open('wb') as log:
        server = subprocess.Popen(args + ['+devmap', 'gm_flatgrass'], cwd=mod, stdout=log,
                                  stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            with ExitStack() as session:
                first = player('First', mod, profile / 'first', run, free_port(), f'connect {address}')
                session.callback(first.close)
                first.ready()
                guest = player('Joined', mod, profile / 'second', run, free_port(), f'connect {address}')
                session.callback(guest.close)
                guest.ready()
                guest.do('ui_openmenu jar_probe_empty', 'togglemenu', 'rconPassword ' + password)
                first.do('disconnect')
                def command(line):
                    guest.do('rcon ' + line)
                    time.sleep(.65)
                worlds = server_home / mod.name / 'jar/worlds'
                arena(worlds, command)
                results['vacant'] = trial('vacant', guest, command, worlds)
                command('quit')
            server.wait(timeout=10)
        finally:
            if server.poll() is None:
                server.terminate()
                server.wait(timeout=10)
    return results


class Actor(NamedTuple):
    slot: int
    firing: int
    yaw: float
    x: float
    y: float


class Frame(NamedTuple):
    slot: int
    health: int
    armor: int
    firing: int
    blaster: int
    x: float
    y: float
    npc: Actor


def read_alert_demo(tool, path):
    lines = subprocess.check_output([str(tool), '--alert-demo', str(path)], text=True).splitlines()
    frames = {}
    for line in lines:
        t, slot, health, armor, firing, blaster, x, y, actor = line.split()
        number, shooting, yaw, nx, ny = actor.split(',')
        npc = Actor(int(number), int(shooting), float(yaw), float(nx), float(ny))
        frames[int(t)] = Frame(int(slot), int(health), int(armor), int(firing), int(blaster), float(x), float(y), npc)
    return frames


def aim_error(frame):
    bearing = math.degrees(math.atan2(frame.y - frame.npc.y, frame.x - frame.npc.x))
    return abs((frame.npc.yaw - bearing + 180) % 360 - 180)


def barrier_blocks(frame):
    # The middle of each crate is solid. Leave a margin at the two
    # ends for the shaped hull and at the muzzle end of the sight line.
    n = frame.npc
    if n.x < -74 or frame.x >= -124:
        return False
    fraction = (-100 - n.x) / (frame.x - n.x)
    return abs(n.y + fraction * (frame.y - n.y)) < 290


def compare_alerts(tool, run):
    observations = json.loads((run / 'observations.json').read_text())
    server_log = (run / 'dedicated.log').read_text(errors='replace')
    depart = server_log.index('ClientDisconnect: 0')
    assert 'ClientBegin: 1' in server_log[:depart]
    assert 'jar_world_load sentry' in server_log[depart:] and 'ClientBegin: 0' not in server_log[depart:]
    output = {}
    for name, case in observations.items():
        metrics = {}
        for phase in ('quiet', 'shots'):
            recording = name + phase
            frames = read_alert_demo(tool, next(run.glob(f"{case['shooter']}-{recording}.dm_*")))
            assert {row.slot for row in frames.values()} == {1}
            times = sorted(frames)
            if case['witness']:
                other = read_alert_demo(tool, next(run.glob(f"{case['witness']}-{recording}.dm_*")))
                times = sorted(frames.keys() & other.keys())
                assert all(frames[t].npc == other[t].npc for t in times)
                assert all((other[t].health <= 0) == (name != 'alive') for t in times)
            assert len(times) >= 20
            npc_fire = [t for t in times if frames[t].npc.firing]
            player_fire = [t for t in times if frames[t].firing]
            assert bool(player_fire) == (phase == 'shots') and all(frames[t].blaster for t in times)
            row = dict(snapshots=len(times), firing=len(npc_fire), player_firing=len(player_fire),
                       armor_lost=frames[times[0]].armor - min(frames[t].armor for t in times),
                       minimum_player_health=min(frames[t].health for t in times))
            if player_fire:
                first = player_fire[0]
                row['angle_to_shooter_at_first_shot'] = aim_error(frames[first])
                row['final_angle_to_shooter'] = aim_error(frames[times[-1]])
                if npc_fire:
                    row['first_return_fire_ms'] = npc_fire[0] - first
                if name == 'cover':
                    screened = [t for t in times if t >= first and barrier_blocks(frames[t])]
                    row['screened_snapshots_after_shot'] = len(screened)
                    row['firing_while_screened'] = sum(frames[t].npc.firing for t in screened)
                    row['maximum_lateral_travel'] = max(abs(frames[t].npc.y - frames[first].npc.y) for t in times)
            metrics[phase] = row
        output[name] = metrics
    (run / 'alert-measurements.json').write_text(json.dumps(output, indent=2) + '\n')
    for name, m in output.items():
        shots = m['shots']
        assert not m['quiet']['firing'], (name, m)
        if name != 'ally':
            assert shots['angle_to_shooter_at_first_shot'] > 90, (name, m)
        if name == 'cover':
            assert shots['screened_snapshots_after_shot'] >= 20 and not shots['firing_while_screened'], m
            assert shots['maximum_lateral_travel'] > 100 and shots['final_angle_to_shooter'] < 55, m
        elif name in ('ignored', 'ally'):
            assert not shots['firing'] and not shots['armor_lost'], m
        else:
            assert shots['firing'] and shots['first_return_fire_ms'] > 0, m
    return output
