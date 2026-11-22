"""Exercise a rendered listen server and a late-joining client in isolated Windows profiles."""
import argparse
from collections import Counter
from contextlib import ExitStack
import ctypes
from ctypes import wintypes
import hashlib
from functools import partial
from itertools import groupby
import json
import math
import os
from pathlib import Path
import re
import shutil
import socket
import struct
import subprocess
import time

from mp_crowd_session import crowd_world, EF_DEAD, EF_FIRING, EF_NODRAW, PROP, REBEL, STORMTROOPER
from mp_world_smoke import checksum, decode, legacy_header


def link_asset(source, destination):
    # Windows hard links require both directories to be on the same volume.
    if source.resolve().drive.casefold() == destination.resolve().drive.casefold():
        os.link(source, destination)
    else:
        shutil.copy2(source, destination)


def remove_runtime(data):
    try:
        shutil.rmtree(data)
    except PermissionError:
        # A Windows process can still have an asset open. Leave the marker
        # where ordinary build retention can find it and preserve the test log.
        (data / '.jar-test-runtime').touch()
        print(f'Locked test runtime left for build retention: {data}')


def free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
        reservation.bind(('127.0.0.1', 0))
        return reservation.getsockname()[1]


class MemoryCounters(ctypes.Structure):
    _fields_ = [('size', wintypes.DWORD), ('faults', wintypes.DWORD)] + [
        (key, ctypes.c_size_t) for key in ('peak_working', 'working', 'peak_paged', 'paged',
                                           'peak_nonpaged', 'nonpaged', 'pagefile', 'peak_pagefile', 'private')]


def memory_mib(process):
    info = MemoryCounters()
    info.size = ctypes.sizeof(info)
    if not ctypes.windll.kernel32.K32GetProcessMemoryInfo(
            wintypes.HANDLE(int(process._handle)), ctypes.byref(info), info.size):
        raise ctypes.WinError()
    return round(info.private / (1024 ** 2), 1)


class Player:
    def __init__(self, name, mod, home, evidence, port, entry, renderer='rdjar-mp-vanilla'):
        self.name = name
        self.renderer = renderer
        self.evidence = evidence
        self.profile = home / mod.name
        self.profile.mkdir(parents=True)
        self.log = self.profile / 'qconsole.log'
        self.command_id = 1
        (self.profile / 'step1.cfg').write_text('// waiting\n')
        (self.profile / 'watch.cfg').write_text('vstr test_step\nwait 5\nexecq watch.cfg\n')
        settings = dict(fs_basepath=mod.parent, fs_homepath=home, fs_game=mod.name,
                        dedicated=0, net_ip='127.0.0.1', net_port=port, net_enabled=1,
                        r_fullscreen=0, r_mode=4, cl_renderer=renderer, logfile=2)
        command = [str(mod / 'jar_mp.x86_64.exe')]
        for key, value in settings.items():
            command += ['+set', key, str(value)]
        config = dict(sv_master1='', sv_master2='', sv_pure=0, sv_maxclients=4, sv_fps=40,
                      g_gametype=0, g_teamAutoJoin=1, g_spawnInvulnerability=0, bot_enable=0,
                      jar_sandbox=1, sv_floodProtect=0,
                      jar_worldAutoSave=0, jar_worldAutoLoad=0, fraglimit=0, timelimit=0,
                      com_maxfps=60, com_maxfpsUnfocused=60, s_volume=0, s_musicvolume=0,
                      cg_draw2D=0, cg_drawGun=0, cg_thirdPerson=0, name=name)
        lines = [f'set {key} "{value}"' for key, value in config.items()]
        lines += [entry, 'set test_step "execq step1.cfg"', 'echo LISTEN_TEST_ENTRY', 'exec watch.cfg']
        (self.profile / 'entry.cfg').write_text('\n'.join(lines) + '\n')
        with (evidence / f'{name}-stdout.log').open('wb') as log:
            self.process = subprocess.Popen(command + ['+exec', 'entry.cfg'], cwd=mod, stdout=log,
                                            stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)

    def text(self):
        return self.log.read_text(encoding='utf-8', errors='replace') if self.log.exists() else ''

    def wait(self, marker, timeout=45):
        until = time.monotonic() + timeout
        while time.monotonic() < until:
            if self.process.poll() is not None:
                raise AssertionError(f'{self.name} exited with {self.process.returncode} before {marker}')
            if marker in self.text():
                return
            time.sleep(.15)
        raise AssertionError(f'{self.name} did not log {marker}')

    def queue(self, *commands):
        index = self.command_id
        (self.profile / f'step{index+1}.cfg').write_text('// waiting\n')
        pending = self.profile / 'next.tmp'
        marker = f'LISTEN_STEP_{index}_DONE'
        lines = [f'set test_step "execq step{index+1}.cfg"', *commands, 'echo ' + marker]
        pending.write_text('\n'.join(lines) + '\n')
        # On Windows the game briefly holds this file open while polling it.
        # Advance the queue only after the command is actually available.
        deadline = time.monotonic()+3
        while True:
            try:
                pending.replace(self.profile / f'step{index}.cfg')
                break
            except PermissionError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(.02)
        self.command_id += 1
        return marker

    def do(self, *commands):
        self.wait(self.queue(*commands))

    def ready(self):
        self.wait('LISTEN_TEST_ENTRY', 60)
        self.wait('CL_InitCGame', 60)
        # CGame can be initialized while the late client is still waiting for
        # the first snapshot. Escape at that point would cancel its connection.
        until = time.monotonic() + 30
        while time.monotonic() < until:
            offset = len(self.text())
            self.do('clientinfo')
            if re.search(r'\bstate: 8\s*$', self.text()[offset:], re.M):  # CA_ACTIVE
                break
            time.sleep(.2)
        else:
            raise AssertionError(f'{self.name} did not enter the map')
        self.do('ui_openmenu jar_probe_empty', 'cg_draw2D 0', 'cg_drawGun 0')
        renderers = re.findall(r'Trying to load "(rd[^"]+\.dll)"', self.text())
        assert renderers and set(renderers) == {self.renderer + '_x86_64.dll'}, renderers

    def record(self, name):
        self.do('record ' + name)
        self.wait('recording to demos/' + name + '.dm_')

    def screenshot(self, name):
        self.do('screenshot_png ' + name)
        path = self.profile / 'screenshots' / (name + '.png')
        # The console acknowledgement precedes the renderer's actual write.
        until = time.monotonic() + 8
        while time.monotonic() < until:
            if path.is_file() and path.stat().st_size >= 12:
                with path.open('rb') as image:
                    image.seek(-8, 2)
                    if image.read() == b'IEND\xaeB`\x82':
                        return
            time.sleep(.1)
        raise AssertionError(f'{self.name} did not finish writing {name}.png')

    def close(self):
        try:
            if self.process.poll() is None:
                self.queue('quit')
                self.process.wait(timeout=15)
        finally:
            if self.log.is_file():
                shutil.copyfile(self.log, self.evidence / f'{self.name}-console.log')
            if self.name == 'Host' and (self.profile / 'games.log').is_file():
                shutil.copyfile(self.profile / 'games.log', self.evidence / 'games.log')
            for folder in ('demos', 'screenshots'):
                for path in (self.profile / folder).glob('*'):
                    if path.is_file():
                        shutil.copyfile(path, self.evidence / (self.name + '-' + path.name))


def distribution(values):
    ordered = sorted(values)
    return dict(samples=len(ordered), median=ordered[len(ordered)//2],
                p95=ordered[math.ceil(len(ordered)*.95)-1], maximum=ordered[-1])


def measure(host, guests, tag, seconds):
    host.do('com_speeds 1', 'echo MEASURE_BEGIN_' + tag)
    usage = {p.name: [] for p in (host, *guests)}
    until = time.monotonic() + seconds
    while time.monotonic() < until:
        for p in (host, *guests):
            assert p.process.poll() is None
            usage[p.name].append(memory_mib(p.process))
        time.sleep(.5)
    host.do('echo MEASURE_END_' + tag, 'com_speeds 0')
    sample = host.text().split('MEASURE_BEGIN_' + tag)[-1].split('MEASURE_END_' + tag)[0]
    times = [int(sv) + int(game) for sv, game in re.findall(r'frame:\s*\d+ all:\s*\d+ sv:\s*(-?\d+).*?gm:\s*(\d+)', sample)]
    assert len(times) >= seconds * 2, (tag, len(times))
    return dict(wall_seconds=seconds, server_work_ms_per_host_frame=distribution(times),
                private_mib={name: dict(first=row[0], last=row[-1], peak=max(row)) for name, row in usage.items()})


def crowd_scale(mod, profile, run, stack, rate_limited_guest=False):
    port = free_port()
    host = Player('Host', mod, profile / 'host', run, port, 'devmap gm_flatgrass')
    stack.callback(host.close)
    host.ready()
    if rate_limited_guest:
        # The host uses engine loopback. The guest connects over UDP and must
        # continue to honor its advertised rate when the LAN override is off.
        host.do('sv_lanForceRate 0', 'sv_ratePolicy 2', 'sv_minRate 1000', 'sv_maxRate 25000')
    guest = Player('Late', mod, profile / 'guest', run, free_port(), f'connect 127.0.0.1:{port}')
    stack.callback(guest.close)
    guest.ready()
    if rate_limited_guest:
        guest.do('rate 25000', 'snaps 40')
    for client, x in ((host, 0), (guest, 40)):
        client.do('notarget', 'god', f'setviewpos {x} -1100 48 90',
                  'ui_openmenu jarMPPause', 'ui_openmenu jar_probe_empty', 'togglemenu')
    host.do('jar_world_save seed')
    worlds = host.profile / 'jar/worlds'
    seed = (worlds / 'seed.0.jrw').read_bytes()
    results = {'rate_limited_guest': rate_limited_guest}
    for count in (192, 384):
        name = f'crowd{count}'
        (worlds / f'{name}.0.jrw').write_bytes(crowd_world(seed, count//2, 16))
        host.do('jar_world_load ' + name)
        host.wait(f'Loaded {name} revision 1: {count+16} objects')
        time.sleep(3)
        for client in (host, guest):
            client.record(name)
            client.do('screenshot_png ' + name)
        results[name] = measure(host, [guest], name, 35)
        # Exercise the same client command as the spawn card and the Z binding.
        host.do(f'cmd jar_spawn {REBEL}')
        time.sleep(1.5)
        host.do('jar_undo')
        time.sleep(1.5)
        slot = 'survivors' + str(count)
        host.do('jar_world_save ' + slot)
        nodes = decode(worlds / (slot + '.0.jrw'))['nodes']
        assert Counter(n[0] for n in nodes) == {REBEL: count//2, STORMTROOPER: count//2, PROP: 16}
        host.do('jar_world_clear')
        time.sleep(2)
        for client in (host, guest):
            client.do('stoprecord')
    return results


def exercise(mod, profile, run, stack):
    port = free_port()
    host = Player('Host', mod, profile / 'host', run, port, 'devmap gm_flatgrass')
    stack.callback(host.close)
    host.ready()
    host.do('notarget', 'god', 'setviewpos 0 -1100 48 90', 'jar_world_save seed')
    worlds = host.profile / 'jar/worlds'
    seed = (worlds / 'seed.0.jrw').read_bytes()
    assert not decode(worlds / 'seed.0.jrw')['nodes']
    for pairs, name in ((16, 'small'), (48, 'large')):
        (worlds / (name + '.0.jrw')).write_bytes(crowd_world(seed, pairs, 16))
    phases = {'empty': measure(host, [], 'empty', 6)}
    host.do('jar_world_load small')
    host.wait('Loaded small revision 1: 48 objects')
    phases['32_npcs'] = measure(host, [], 'small', 10)
    host.do('jar_world_clear', 'jar_world_load large')
    host.wait('Loaded large revision 1: 112 objects')
    time.sleep(2)
    host.record('fight')
    host.do('screenshot_png beforejoin')
    phases['96_npcs'] = measure(host, [], 'beforejoin', 8)
    late = Player('Late', mod, profile / 'late', run, free_port(), f'connect 127.0.0.1:{port}')
    stack.callback(late.close)
    late.ready()
    late.do('notarget', 'god', 'setviewpos 16 -1100 48 90')
    late.record('fight')
    time.sleep(2)
    for p in (host, late):
        p.do('ui_openmenu jar_probe_empty', 'screenshot_png fight')
    phases['96_with_guest'] = measure(host, [late], 'together', 35)
    host.do('jar_world_save survivors')
    nodes = decode(worlds / 'survivors.0.jrw')['nodes']
    assert Counter(n[0] for n in nodes) == {REBEL:48, STORMTROOPER:48, PROP:16}
    # Exercise the client-to-server spawn and Undo commands during the same live fight.
    host.do('cmd jar_spawn 1001', 'wait 20', 'jar_world_save plusone')
    assert len(decode(worlds / 'plusone.0.jrw')['nodes']) == 113
    host.do('jar_undo', 'wait 20', 'jar_world_save minusone')
    assert Counter(n[0] for n in decode(worlds / 'minusone.0.jrw')['nodes']) == Counter(n[0] for n in nodes)
    host.do('jar_world_clear')
    time.sleep(2)
    for p in (host, late):
        p.do('screenshot_png cleared', 'stoprecord')
    return phases


def interactions(mod, profile, run, stack):
    port = free_port()
    host = Player('Host', mod, profile / 'host', run, port, 'devmap gm_flatgrass')
    stack.callback(host.close)
    host.ready()
    host.do('notarget', 'god', 'developer 1', 'setviewpos -420 -20 40 0', 'jar_world_save seed')
    worlds = host.profile / 'jar/worlds'
    for name, height in (('medium', 650), ('lethal', 1800)):
        data = legacy_header((worlds / 'seed.0.jrw').read_bytes())
        struct.pack_into('<2I', data, 12, 1, 0)
        data += struct.pack('<6I16f', STORMTROOPER, 0, 0, 0, 50, 0, 80, 64, 0, height,
                            0, 0, 0, -16, -16, -24, 16, 16, 40, 0, 0, 0)
        data += struct.pack('<I', checksum(data))
        (worlds / f'{name}.0.jrw').write_bytes(data)

    def landing(mode, label):
        host.do('jar_world_load ' + mode)
        host.wait(f'Loaded {mode} revision 1: 1 objects')
        time.sleep(3)
        if mode == 'medium':
            host.do('jar_world_save ' + label)
            node, = decode(worlds / (label + '.0.jrw'))['nodes']
            assert 0 < node[4] < 35, node
            return node[4]

    host.record('falls')
    injury_before_join = landing('medium', 'initial_injury')
    late = Player('Late', mod, profile / 'late', run, free_port(), f'connect 127.0.0.1:{port}')
    stack.callback(late.close)
    late.ready()
    late.do('god', 'notarget', 'developer 1', 'setviewpos -220 -20 40 180')
    late.record('falls')
    time.sleep(2)
    host.do('jar_world_clear')
    time.sleep(.7)
    injury_after_join = landing('medium', 'shared_injury')
    host.do('jar_world_clear')
    time.sleep(.7)
    landing('lethal', '')
    for p in (host, late):
        p.do('stoprecord')
    host.do('jar_world_clear', 'jar_playerTools 1')

    def selection(client, command, expected):
        offset = len(client.text())
        client.do('cmd jar_' + command)
        end = time.monotonic() + 6
        while time.monotonic() < end:
            rows = re.findall(r'serverCommand: \d+ : jarselection ([^\r\n]+)', client.text()[offset:])
            for row in rows:
                fields = row.split()
                if expected(fields):
                    return fields
            time.sleep(.1)
        raise AssertionError((command, client.name, rows))

    def select(client, target, slot):
        return selection(client, 'select ' + target,
                         lambda row: len(row) == 21 and row[0] == str(slot) and row[4] == '4')

    def armor(client, selected, amount):
        return selection(client, f'property armor_{amount} {selected[3]}',
                         lambda row: len(row) == 21 and row[0] == selected[0] and row[20] == str(amount))

    ray = 'ray_-420_-20_55_1_0_0'
    armor(late, select(late, 'self', 1), 32)
    armor(host, select(host, ray, 1), 57)
    assert select(late, 'self', 1)[20] == '57'
    late.do('cmd jar_allowTools 0')
    time.sleep(.4)
    selection(host, 'select ' + ray, lambda r: r[0] == '-1')
    host.wait('That player blocked tools on themselves.')
    late.do('cmd jar_allowTools 1')
    time.sleep(.4)
    armor(host, select(host, ray, 1), 46)
    host.do('jar_playerTools 0')
    selection(host, 'select ' + ray, lambda r: r[0] == '-1')
    host.wait('The server disabled tools on other players.')
    assert select(late, 'self', 1)[20] == '46'
    armor(late, select(late, 'self', 1), 28)
    return dict(injured_before_late_join=injury_before_join, medium_landing_health=injury_after_join,
                initial_health=50, guest_observed_host_edit=True,
                guest_opt_out_denied_host=True, server_disabled_remote_edit=True,
                self_edit_with_server_remote_editing_off=True)


def read_demo(tool, path, option='--crowd-demo'):
    # Recordings contain ordinary delta snapshots from the two original graphical clients.
    text = subprocess.check_output([str(tool), option, str(path)], text=True)
    frames = {}
    for line in text.splitlines():
        stamp, slot, health, *entries = line.split()
        entities = {}
        for entry in entries:
            kind, number, flags, x, y, z = entry.split(',')
            entities[int(number)] = (kind, int(flags), float(x), float(y), float(z))
        frames[int(stamp)] = entities
    return frames


def roster(frame):
    return {n for n, row in frame.items() if row[0] == 'n' and not row[1] & (EF_NODRAW | EF_DEAD)}


def props(frame):
    return {n for n, row in frame.items() if row[0] == 'p'}


def compare_crowd_scale(tool, run):
    results = {}
    limited = json.loads((run / 'observations.json').read_text()).get('rate_limited_guest', False)
    for count in (192, 384):
        name = f'crowd{count}'
        demos = [next(run.glob(f'{peer}-{name}.dm_*')) for peer in ('Host', 'Late')]
        tracks = [read_demo(tool, demo) for demo in demos]
        host, guest = tracks
        together = sorted(host.keys() & guest.keys())
        assert together and all(host[t] == guest[t] for t in together), f'{name}: clients disagree'
        stable = []
        initial_visibility = {}
        for track in tracks:
            segments = [(key, list(ticks)) for key, ticks in groupby(track, key=lambda t:
                        (frozenset(roster(track[t])), frozenset(props(track[t]))))]
            assert len(segments) >= 4, (name, len(segments))
            steady, addition, undo, cleanup = segments[-4:]
            before, added, undone, cleared = (phase[0] for phase in (steady, addition, undo, cleanup))
            npcs, objects = before
            assert len(npcs) == count and len(objects) == 16
            assert added[0] > npcs and len(added[0]) == count+1 and added[1] == objects
            assert before == undone and cleared == (set(), set())
            # Flatgrass can cull actors below the observers' platform. Only
            # admit a brief opening interval if the same live actors reappear
            # below ground. A missing prop, death or later roster change fails.
            start = steady[1][0]
            opening_ms = start - segments[0][1][0]
            assert opening_ms <= 3000, (name, 'crowd took too long to become visible', opening_ms)
            hidden = set()
            for (visible, seen_props), ticks in segments[:-4]:
                assert visible <= npcs and seen_props == objects
                for slot in npcs - visible:
                    assert all(slot not in track[t] for t in ticks)
                    assert track[start][slot][4] < 0, (name, 'missing actor above platform', slot)
                    hidden.add(slot)
            peer = 'host' if track is host else 'guest'
            initial_visibility[peer] = dict(ms=opening_ms, below_platform_slots=sorted(hidden))
            stable.append(steady[1])
        common = sorted(set(stable[0]) & set(stable[1]))
        fired = {n for t in common for n in npcs if host[t][n][1] & EF_FIRING}
        assert fired and common[-1] - common[0] >= 30000
        gaps = {peer: distribution([b-a for a,b in zip(ticks,ticks[1:])])
                for peer, ticks in zip(('host','guest'), stable)}
        delayed = {}
        for peer, demo, ticks in zip(('host', 'guest'), demos, stable):
            output = subprocess.check_output([str(tool), '--timing-demo', str(demo)], text=True)
            flags = dict(tuple(map(int, line.split())) for line in output.splitlines())
            delayed[peer] = sum(flags[t] for t in ticks)
        assert delayed['host'] == 0 and bool(delayed['guest']) == limited, (name, delayed)
        results[name] = dict(npcs=count, props=16, shared_crowd_snapshots=len(common), initial_visibility=initial_visibility,
                             steady_crowd_ms=common[-1]-common[0], distinct_npcs_firing=len(fired),
                             snapshot_gaps_server_ms=gaps, rate_delayed_snapshots=delayed,
                             extra_npc_undone=True, cleanup_seen_by_both=True)
    return results


def compare(tool, run):
    host, late = [read_demo(tool, next(run.glob(name + '-fight.dm_*'))) for name in ('Host', 'Late')]
    expected = next(f for f in host.values() if len(roster(f)) == 96 and len(props(f)) == 16)
    npcs, objects = roster(expected), props(expected)
    assert any(any(f[n][1] & EF_FIRING for n in roster(f)) for t, f in host.items() if t < min(late))
    common = sorted(host.keys() & late.keys())
    for t in common:
        assert host[t] == late[t], f'Different entity states at server time {t}'
    stable = []
    for track in (host, late):
        segments = [(key, list(ticks)) for key, ticks in groupby(track, key=lambda t:
                    (frozenset(roster(track[t])), frozenset(props(track[t]))))]
        assert len(segments) == 4, [(len(a), len(b)) for (a,b), _ in segments]
        before, added, undone, cleared = (key for key, _ in segments)
        assert before == undone == (npcs, objects) and cleared == (set(), set())
        assert added[0] == npcs and added[1] > objects and len(added[1]) == len(objects) + 1
        stable.append(set(segments[0][1]))
    together = sorted(stable[0] & stable[1])
    assert together and together[-1] - together[0] >= 30000, (len(common), len(together))
    fired = {n for t in together for n in npcs if host[t][n][1] & EF_FIRING}
    assert fired
    def gaps(track, start):
        stamps = sorted(t for t in track if start <= t <= together[-1])
        return distribution([y-x for x,y in zip(stamps,stamps[1:])])
    tracks = {'host': host, 'late': late}
    return dict(matching_server_times=len(common), matching_crowd_times=len(together),
                crowd_overlap_ms=together[-1]-together[0], matching_firing_npcs=len(fired),
                cleanup_and_undo_seen_by_both=True,
                snapshot_gap_server_ms={name: gaps(track, together[0]) for name, track in tracks.items()},
                last_30s_snapshot_gap_server_ms={name: gaps(track, together[-1]-30000) for name, track in tracks.items()})


def compare_falls(tool, run):
    assert 'killed stormtrooper2 by MOD_FALLING' in (run / 'games.log').read_text(errors='replace')
    host, late = [read_demo(tool, next(run.glob(name + '-falls.dm_*'))) for name in ('Host', 'Late')]
    # The guest starts recording while the original injured NPC is still present.
    first = next(iter(late.values()))
    assert len(first) == 1 and next(iter(first.values()))[0] == 'n'
    common = sorted(host.keys() & late.keys())
    passages = []
    previous = False
    for t in common:
        assert host[t] == late[t], t
        present = bool(host[t])
        if present:
            if not previous:
                passages.append([])
            passages[-1].append(next(iter(host[t].values())))
        previous = present
    assert len(passages) == 3, [len(p) for p in passages]
    existing, medium, lethal = passages
    ground = existing[0][4]
    assert all(not row[1] & EF_DEAD and abs(row[4] - ground) < 1 for row in existing)
    for rows, start, dead in ((medium, 500, False), (lethal, 1200, True)):
        assert any(r[4] > start for r in rows)
        assert any(abs(r[4] - ground) < 4 and bool(r[1] & EF_DEAD) == dead for r in rows)
    return dict(common_snapshots=len(common), existing_npc_seen_by_late_joiner=True,
                matching_injured_landing=True, matching_lethal_landing=True, lethal_fall_logged=True,
                ground_height=ground)


def main():
    from mp_listen_alerts import compare_alerts, exercise_alerts
    from mp_listen_camera_feeds import compare_feeds, exercise_feeds
    from mp_listen_portals import compare_portals, exercise_portals
    from mp_listen_follow import compare_follow, exercise_follow
    from mp_listen_platforms import compare_rides, compare_vehicle_lift, exercise_rides, exercise_vehicle_lift
    from mp_listen_targets import compare_targets, exercise_targets
    from mp_listen_throws import compare_throws, exercise_throws

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-data', type=Path, required=True)
    parser.add_argument('--package', type=Path, required=True, help='Staged JediAcademyRemixedMP folder')
    parser.add_argument('--reader', type=Path, required=True, help='jar_mp_snapshot_tests.exe')
    parser.add_argument('--results', type=Path, required=True)
    scenarios = parser.add_mutually_exclusive_group()
    parser.set_defaults(scenario=(exercise, compare))
    scenarios.add_argument('--camera-feeds', dest='scenario', action='store_const',
                           const=(partial(exercise_feeds, player=Player, free_port=free_port), compare_feeds),
                           help='Check four simultaneous multiplayer cameras, feed sharing, fallback and frame time')
    scenarios.add_argument('--portal-screens', dest='scenario', action='store_const',
                           const=(partial(exercise_portals, player=Player, free_port=free_port), compare_portals),
                           help='Compare actual camera-panel pixels in Classic and Rend2 on a listen server')
    scenarios.add_argument('--npc-targets', dest='scenario', action='store_const',
                           const=(partial(exercise_targets, player=Player, free_port=free_port), compare_targets),
                           help='Check NPC target priority with hundreds of nearby props and two visible players')
    scenarios.add_argument('--npc-alerts', dest='scenario', action='store_const',
                           const=(partial(exercise_alerts, player=Player, free_port=free_port), compare_alerts),
                           help='Check NPC reactions to a joined player firing behind them')
    scenarios.add_argument('--npc-follow', dest='scenario', action='store_const',
                           const=(partial(exercise_follow, player=Player, free_port=free_port),
                                  partial(compare_follow, read_demo=read_demo)),
                           help='Check friendly NPCs following the host and guest around a wall of props')
    scenarios.add_argument('--physgun-throws', dest='scenario', action='store_const',
                           const=(partial(exercise_throws, player=Player, free_port=free_port), compare_throws),
                           help='Check NPC throws, post-landing combat, player holds and opt-out on a listen server')
    scenarios.add_argument('--interactions', dest='scenario', action='store_const', const=(interactions, compare_falls),
                           help='Check fall damage, late join and player editing permissions')
    scenarios.add_argument('--platforms', dest='scenario', action='store_const',
                           const=(partial(exercise_rides, player=Player, free_port=free_port),
                                  partial(compare_rides, read_demo=read_demo)),
                           help='Check a rider on an unfrozen crate, a bobbing platform and a vertical lift')
    scenarios.add_argument('--vehicle-lift', dest='scenario', action='store_const',
                           const=(partial(exercise_vehicle_lift, player=Player, free_port=free_port),
                                  partial(compare_vehicle_lift, read_demo=read_demo)),
                           help='Check a connected driver riding a Swoop on a vertical lift')
    scenarios.add_argument('--crowd-scale', dest='scenario', action='store_const', const=(crowd_scale, compare_crowd_scale),
                           help='Measure 192 and 384 fighting NPCs and undo a spawn at each population')
    scenarios.add_argument('--crowd-scale-paced', dest='scenario', action='store_const',
                           const=(partial(crowd_scale, rate_limited_guest=True), compare_crowd_scale),
                           help='Repeat crowd scaling with LAN rate override disabled and a 25000 byte/s UDP guest')
    args = parser.parse_args()
    scenario, verify = args.scenario
    run = args.results.resolve()
    run.mkdir(parents=True, exist_ok=False)
    package = args.package.resolve()
    with ExitStack() as stack:
        data = run / 'game'
        data.mkdir()
        (data / '.jar-test-runtime').touch()
        stack.callback(remove_runtime, data)
        mod = data / package.name
        mod.mkdir()
        (data / 'base').mkdir()
        for i in range(4):
            name = f'assets{i}.pk3'
            link_asset(args.game_data / 'base' / name, data / 'base' / name)
        for path in package.iterdir():
            if path.suffix.lower() in ('.pk3', '.dll', '.exe'):
                link_asset(path, mod / path.name)
        phases = scenario(mod, data / 'profiles', run, stack)
        (run / 'observations.json').write_text(json.dumps(phases, indent=2))
    result = verify(args.reader.resolve(), run)
    result['graphical_players'] = 2
    result['machines'] = 1
    result['phases'] = phases
    result['module_sha256'] = {}
    modules = ['jar_mp.x86_64.exe', 'jampgamex86_64.dll', 'cgamex86_64.dll', 'rdjar-mp-vanilla_x86_64.dll']
    if verify in (compare_portals, compare_feeds):
        modules.append('rdjar-mp-rend2_x86_64.dll')
    for name in modules:
        with (package / name).open('rb') as src:
            result['module_sha256'][name] = hashlib.file_digest(src, 'sha256').hexdigest()
    (run / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(run / 'result.json', flush=True)


if __name__ == '__main__':
    main()
