"""Two isolated UDP clients using the source-built native codec, without graphics."""
from contextlib import ExitStack
import json
import math
from pathlib import Path
import queue
import re
import secrets
import socket
import struct
import subprocess
import threading
import time
import zipfile


OOB = b'\xff' * 4
SECTION_MAP = 'jar_wire_section'
IMPERIAL_RESOURCE = '#mp/duel4'
VJUN_RESOURCE = '#mp/duel10'
RANCOR_RESOURCE = '#mp/duel7'
GENERATOR_RESOURCE = '#mp/duel2'
TASPIR_RESOURCE = '#mp/duel5'
BESPIN_RESOURCE = '#mp/duel1'
CITY_RESOURCE = '#mp/duel8'
SENTINEL_RESOURCE = '#mp/ffa1'
HOTH_RESOURCE = '#mp/duel9'
RIFT_RESOURCE = '#mp/ffa4'
SHAFT_RESOURCE = '#mp/duel3'
KORRIBAN_RESOURCE = '#mp/ffa2'
YAVIN_RESOURCE = '#mp/duel6'
CITY_BRUSH_COUNT = 69
SECTION_BRUSHES = {CITY_RESOURCE: CITY_BRUSH_COUNT, YAVIN_RESOURCE: 30}
SECTION_MODEL_COUNTS = {IMPERIAL_RESOURCE: 1, VJUN_RESOURCE: 1, RANCOR_RESOURCE: 1,
                        GENERATOR_RESOURCE: 2, TASPIR_RESOURCE: 2, BESPIN_RESOURCE: 2,
                        CITY_RESOURCE: CITY_BRUSH_COUNT + 1, SENTINEL_RESOURCE: 17, HOTH_RESOURCE: 2,
                        RIFT_RESOURCE: 3, SHAFT_RESOURCE: 6, KORRIBAN_RESOURCE: 9, YAVIN_RESOURCE: 31}
# Retail model-0 bounds from the RBSP model lumps in assets0.pk3.
SECTION_BOUNDS = {
    IMPERIAL_RESOURCE: ((-3104, 1984, -832), (-1376, 4320, 704)),
    VJUN_RESOURCE: ((-736.001831, -976.000427, -192), (989.001770, 863.001587, 992)),
    RANCOR_RESOURCE: ((-960, -704, -736), (1920, 2048, 1472)),
    GENERATOR_RESOURCE: ((-7232, 1344, -1472), (-3712, 2816, 1152)),
    TASPIR_RESOURCE: ((-3648, -3648, -1728), (3648, 3648, 4032)),
    BESPIN_RESOURCE: ((-640, -672.001892, -32), (2400, 1952, 1568)),
    CITY_RESOURCE: ((-1280, -448, -64), (1088, 2368, 2048)),
    SENTINEL_RESOURCE: ((1856, -1376.000977, -352), (5664.000488, 2976.000244, 3392)),
    HOTH_RESOURCE: ((-3136, -2112, -1856), (5440, 5184, 2176)),
    RIFT_RESOURCE: ((4544, 896, -2240), (10304, 5696, 2176)),
    SHAFT_RESOURCE: ((-5248, 1280, -1408), (-3519.996338, 2816, 1504)),
    KORRIBAN_RESOURCE: ((-4736, -544, -640), (256, 4160, 1952)),
    YAVIN_RESOURCE: ((-4096, -4096, -1280), (4096, 4096, 2240)),
}
SECTION_FLOOR_TRACE_Z = .125
BOBBER_MODEL = 'models/map_objects/bespin/twinpodcc.md3'
BOBBER_ORIGIN = (720, -384, 120)
RANCOR_TORCHES = ((16, 528, 48), (16, 176, 48), (944, 176, 48),
                  (20, 1392, 80), (940, 1392, 80), (480, 1676, 80))
TORCH_EFFECT = 'env/small_fire'
TORCH_LOOP = 'sound/effects/torch_burning_lp.wav'
SECTION_TORCHES = {
    RANCOR_RESOURCE: RANCOR_TORCHES,
    YAVIN_RESOURCE: tuple((x, y, 528) for x, y in (
        (-2456, 320), (-2456, -320), (-1952, 856), (-1952, -856), (-320, -2456), (320, -2456),
        (-856, -1952), (856, -1952), (2456, -320), (2456, 320), (1952, -856), (1952, 856),
        (320, 2456), (-320, 2456), (856, 1952), (-856, 1952),
    )),
    KORRIBAN_RESOURCE: (
        (-4320, 2176, 408), (-1152, 2528, 292), (-632, 2528, 576), (-2656, 768, 208),
        (-2656, 1536, 208), (-2816, 1536, 208), (-3296, -112, 512), (-2176, 2528, 828),
        (-3072, 2528, 828), (-2624, 2528, 364), (-1536, 976, 464), (-1536, 1392, 464),
        (-672, 1472, 448), (-1024, 1184, 834), (-672, 896, 448), (-1024, -224, 80),
        (-1536, -368, 372), (-1536, 240, 372), (-2296, 2808, 80), (-2872, 2408, 144),
        (-2872, 2648, 144), (-2296, 2264, 80), (-3872, 2800, 592), (-2816, 768, 208),
        (-3872, 1184, 324), (-3832, 2776, 144), (-672, 1312, 56), (-1376, 1312, 56),
    ),
}
SECTION_SPEAKERS = {
    KORRIBAN_RESOURCE: tuple((origin, 'sound/ambience/prototype/windmix1.wav') for origin in (
        (-3872, -112, 1056), (-1024, -64, 1056), (-2624, -96, 992), (-3872, 1184, 1152),
        (-3200, 1184, 1024), (-2240, 1184, 896), (-1152, 2528, 1024),
    )),
    SHAFT_RESOURCE: (
        ((-4352, 2048, 640), 'sound/ambience/cairn/cairn_energy.wav'),
        ((-3680, 2048, 416), 'sound/ambience/cairn/cairn_circuits.wav'),
        ((-4352, 2048, -384), 'sound/ambience/prototype/busyroom.wav'),
    ),
    HOTH_RESOURCE: tuple((origin, 'sound/ambience/prototype/windmix1.wav') for origin in (
        (960, 2304, 960), (-64, 2688, 960), (-1344, 2304, 960), (-1408, 1472, 960),
        (-1024, -704, 1088), (0, -1472, 1088), (1216, -256, 1216), (-128, -64, 1024),
    )),
    SENTINEL_RESOURCE: tuple((origin, f'sound/ambience/{sound}.wav') for origin, sound in (
        ((3520, 832, 3264), 'cairn/cairn_reactor'), ((3520, 832, 448), 'cairn/cairn_reactor'),
        ((4608, -256, 1536), 'cairn/cairn_duct'), ((3520, 2304, 960), 'cairn/cairn_duct'),
        ((2368, 1056, 960), 'cairn/cairn_circuits'), ((2080, 672, 1984), 'cairn/cairn_circuits'),
        ((3520, -480, 1984), 'kejim/kejim_interior'), ((4992, 992, 1952), 'kejim/kejim_interior'),
        ((4768, 2336, 1952), 'kejim/kejim_interior'), ((3056, 1520, 2208), 'kejim/kejim_interior'),
        ((3520, 832, 1056), 'kejim/kejim_interior'), ((3520, 832, 3200), 'cairn/cairn_reactor'),
    )),
    IMPERIAL_RESOURCE: (
        ((-2944, 2528, 256), 'sound/ambience/kejim/kejim_interior.wav'),
        ((-1664, 2528, 256), 'sound/ambience/kejim/kejim_interior.wav'),
        ((-2304, 2176, 480), 'sound/ambience/cairn/cairn_circuits.wav'),
        ((-2304, 3072, 256), 'sound/ambience/kejim/kejim_interior.wav'),
    ),
    VJUN_RESOURCE: (
        ((-208, 0, 448), 'sound/ambience/cairn/cairn_reactor.wav'),
        ((528, 0, 832), 'sound/ambience/artus/artus_shaft.wav'),
        ((400, 0, 832), 'sound/ambience/artus/artus_shaft.wav'),
        ((-560, 0, 704), 'sound/ambience/artus/artus_shaft.wav'),
    ),
    GENERATOR_RESOURCE: (((-4448, 2080, -128), 'sound/ambience/artus/artus_shaft.wav'),),
    CITY_RESOURCE: (
        ((0, 1024, 1280), 'sound/ambience/prototype/windmix1.wav'),
        ((0, 1024, 1344), 'sound/ambience/prototype/windmix1.wav'),
    ),
}


def section_map_data():
    """Native RBSP with one flat solid floor and a single visible cluster."""
    extent = 8192
    entities = ['{ "classname" "worldspawn" }',
                '{ "classname" "info_player_intermission" "origin" "0 0 64" }']
    for y in (-128, 128):
        entities.append(f'{{ "classname" "info_player_deathmatch" "origin" "0 {y} 32" "angle" "0" }}')
    # qfiles.h RBSP lumps. Each axis-aligned plane is paired with its inverse.
    lumps = [b''] * 18
    lumps[0] = ('\n'.join(entities) + '\n\0').encode('ascii')
    lumps[1] = struct.pack('<64s2i', b'textures/common/caulk', 0, 1)
    planes = [(-1, 0, 0, extent), (1, 0, 0, extent), (0, -1, 0, extent),
              (0, 1, 0, extent), (0, 0, -1, 128), (0, 0, 1, 0)]
    lumps[2] = b''.join(struct.pack('<4f', *plane) + struct.pack('<4f', *(-v for v in plane))
                        for plane in planes)
    bounds = (-extent, -extent, -128, extent, extent, 4096)
    lumps[3] = struct.pack('<9i', 10, -1, -2, *bounds)
    leaf = struct.pack('<12i', 0, 0, *bounds, 0, 0, 0, 1)
    lumps[4] = leaf * 2
    lumps[6] = struct.pack('<i', 0)
    lumps[7] = struct.pack('<6f4i', *bounds, 0, 0, 0, 1)
    lumps[8] = struct.pack('<3i', 0, len(planes), 0)
    lumps[9] = b''.join(struct.pack('<3i', index * 2, 0, -1) for index in range(len(planes)))
    offset = 8 + 8 * len(lumps)
    header = bytearray(struct.pack('<4sI', b'RBSP', 1))
    for lump in lumps:
        header.extend(struct.pack('<2i', offset, len(lump)))
        offset += len(lump)
    return bytes(header) + b''.join(lumps)


def write_section_map(mod_directory):
    path = mod_directory / 'maps' / f'{SECTION_MAP}.bsp'
    path.parent.mkdir()
    path.write_bytes(section_map_data())


def write_section_map_pack(path):
    with zipfile.ZipFile(path, 'w') as archive:
        archive.writestr(f'maps/{SECTION_MAP}.bsp', section_map_data())


class WireTimeout(AssertionError):
    pass


class WireProbe:
    def __init__(self, executable, log):
        self.log = log
        self.child = subprocess.Popen(
            [str(executable), '--network-probe'], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=log, encoding='ascii', bufsize=1,
            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0),
        )
        self.lines = queue.Queue()

        def capture():
            for line in self.child.stdout:
                self.lines.put(line.strip())
            self.lines.put(None)

        self.reader = threading.Thread(target=capture, daemon=True)
        self.reader.start()

    def request(self, command):
        self.child.stdin.write(command + '\n')
        self.child.stdin.flush()
        try:
            reply = self.lines.get(timeout=5)
        except queue.Empty as exc:
            raise AssertionError(f'Native wire probe timed out on {command.split()[0]}') from exc
        assert reply is not None, (command.split()[0], self.child.poll(), self.log.name)
        return reply

    def packet(self, command):
        tag, data = self.request(command).split()
        assert tag == 'packet'
        return bytes.fromhex(data)

    def checksum(self, archive_headers):
        tag, value = self.request('blockchecksum ' + archive_headers.hex()).split()
        assert tag == 'blockchecksum'
        return int(value)

    def close(self):
        self.child.stdin.close()
        try:
            self.child.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.child.kill()
            self.child.wait(timeout=5)
        self.reader.join(timeout=5)
        self.child.stdout.close()


def receive(sock, predicate, timeout=6):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            packet = sock.recv(65535)
        except socket.timeout:
            continue
        value = predicate(packet)
        if value is not None:
            return value
    raise WireTimeout('Timed out waiting for the native server wire response')


def await_state(sock, probe, kind, accept=lambda fields: True, timeout=6):
    def decode(packet):
        if packet.startswith(OOB):
            assert not packet[4:].startswith(b'disconnect'), packet
            return None
        reply = probe.request('read ' + packet.hex())
        if reply.startswith('disconnect ') and kind != 'disconnect':
            raise AssertionError(reply)
        if kind == 'disconnect' and reply.startswith('snapshot '):
            raise AssertionError('Client entered the map with an invalid pure proof')
        if reply.startswith(kind + ' '):
            fields = reply.split()
            if accept(fields):
                return fields
        return None

    return receive(sock, decode, timeout)


def snapshot_entities(field):
    return [] if field == '-' else [int(entity) for entity in field.split(',')]


def snapshot_lights(fields):
    return snapshot_entities(fields[5])


def construct_furniture(fields):
    if fields[22] == '-':
        return {}
    furniture = {}
    for record in fields[22].split(';'):
        number, path, *vectors = record.split(',')
        path = wire_value(path)
        assert path not in furniture
        pose = tuple(map(float, vectors))
        assert len(pose) == 9 and all(math.isfinite(value) for value in pose)
        assert all(pose[3 + a] < pose[6 + a] for a in range(3))
        furniture[path] = dict(entity=int(number), origin=pose[:3], mins=pose[3:6], maxs=pose[6:9])
    return furniture


def check_construct_furniture(owner, observer, physgun_id):
    _, sock, probe, owner_state = owner
    _, other_sock, other_probe, _ = observer
    # The open interior west of the armchair sees all four imported models.
    # Face the chair so the native teleport impulse does not change the
    # firing angle between the first and subsequent user commands.
    start = (-3090, -1288, -72)
    sock.send(probe.packet('setviewpos ' + ' '.join(map(str, (*start, 0)))))
    models = {f'models/jg/gm_construct/p{i:03}.md3' for i in range(4)}
    fields = await_state(sock, probe, 'snapshot', lambda fields: construct_furniture(fields).keys() == models
                         and math.dist(tuple(map(float, fields[19].split(',')[:3])), start) < 24)
    initial = construct_furniture(fields)
    other_sock.send(other_probe.packet(f'follow {owner_state["slot"]}'))
    watched = await_state(other_sock, other_probe, 'snapshot', lambda fields:
                          fields[7] == '1' and construct_furniture(fields).keys() == models)
    assert {path: prop['entity'] for path, prop in initial.items()} == {
        path: prop['entity'] for path, prop in construct_furniture(watched).items()}
    settled = await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) >= int(watched[1]))
    # setviewpos applies a short native impulse. Consume it before aiming at
    # the chair so the test does not move into the table as it starts firing.
    end = int(settled[1]) + 400
    while int(settled[1]) < end:
        previous = int(settled[1])
        sock.send(probe.packet('move 0 0 0'))
        settled = await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) > previous)
    initial = construct_furniture(settled)
    chair = initial['models/jg/gm_construct/p002.md3']
    sock.send(probe.packet(f'select {chair["entity"]}'))
    selected = await_state(sock, probe, 'snapshot', lambda fields: int(fields[10]) == chair['entity'])
    assert float(selected[23]) == 700
    await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) >= int(selected[1]) + 250)
    sock.send(probe.packet(f'equip {physgun_id}'))
    equipped = await_state(sock, probe, 'snapshot', lambda fields: fields[24] != selected[24])
    assert int(equipped[25]) == 0
    held = equipped
    # Aim at the solid seat/base and allow repeated shots at each height.
    # Native weapon timing can defer a shot past the first command at a height.
    for target_height in (-10, -14, -6, 0) * 8:
        previous = int(held[1])
        sock.send(probe.packet(f'physgun {chair["entity"]} 1 0 {target_height}'))
        held = await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) > previous
                           and int(fields[29]) >= previous)
        if int(held[25]) == chair['entity'] + 1:
            break
    else:
        raise AssertionError(f'Native Physgun did not pick up Construct armchair from {held[19]} '
                             f'at pitch {held[6]} and yaw {held[8]} (held {held[25]})')
    sock.send(probe.packet('physgun -1 1 24'))
    def lifted(fields):
        furniture = construct_furniture(fields)
        pose = furniture.get('models/jg/gm_construct/p002.md3')
        return pose is not None and pose['origin'][2] > chair['origin'][2] + 8

    moved = await_state(sock, probe, 'snapshot', lifted)
    moved_other = await_state(other_sock, other_probe, 'snapshot', lifted)
    for _ in range(16):
        sock.send(probe.packet('physgun -1 3 0'))
        frozen = await_state(sock, probe, 'snapshot', lifted)
        if int(frozen[25]) == 0:
            break
    else:
        raise AssertionError('Alternate fire did not release and freeze Construct armchair')
    sock.send(probe.packet('physgun -1 0 0'))
    stopped = await_state(sock, probe, 'snapshot', lambda fields: lifted(fields)
                          and int(fields[1]) >= int(frozen[1]) + 300)
    assert math.dist(construct_furniture(stopped)['models/jg/gm_construct/p002.md3']['origin'],
                     construct_furniture(frozen)['models/jg/gm_construct/p002.md3']['origin']) < 1
    return dict(initial=initial, selected_armchair_mass_kg=float(selected[23]),
                moved_snapshot_time=int(moved[1]), observer_snapshot_time=int(moved_other[1]),
                alternate_fire_freeze=True)


def check_construct_ladder(owner, observer):
    _, sock, probe, _ = owner
    _, other_sock, other_probe, _ = observer
    # Approach from the native ladder's clear side and keep movement commands
    # tied to the server snapshot clock.
    start = (-2920, -1076, -72)
    def position(fields):
        return tuple(map(float, fields[19].split(',')[:3]))

    before = await_state(sock, probe, 'snapshot')
    before = await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) >= int(before[1]) + 1000)
    sock.send(probe.packet('setviewpos ' + ' '.join(map(str, (*start, 90)))))
    initial = await_state(sock, probe, 'snapshot', lambda fields: math.dist(position(fields), position(before)) > 100)
    assert math.dist(position(initial), start) < 10, (position(initial), start)

    def drive(command, condition, seconds=2):
        frame = await_state(sock, probe, 'snapshot')
        deadline = int(frame[29]) + seconds * 1000
        while int(frame[29]) < deadline:
            previous = int(frame[1])
            sock.send(probe.packet(command))
            frame = await_state(sock, probe, 'snapshot', lambda fields: int(fields[29]) >= previous)
            if condition(frame):
                return frame
        raise AssertionError(f'Construct ladder: {command} ended at {frame[19]} with {frame[26:29]}')

    climbed = drive('move 0 0 127', lambda fields: position(fields)[2] > start[2] + 22
                    and float(fields[26]) > 20 and fields[27:29] == ['hang', 'hang'])
    watched = await_state(other_sock, other_probe, 'snapshot', lambda fields: fields[7] == '1'
                          and position(fields)[2] > start[2] + 22 and float(fields[26]) > 20
                          and fields[27:29] == ['hang', 'hang'])
    stopped = drive('move 0 0 0', lambda fields: abs(float(fields[26])) < 1 and fields[27:29] == ['hang', 'hang'])
    idle_height = position(stopped)[2]
    settled = drive('move 0 0 0', lambda fields: int(fields[1]) > int(stopped[1]) + 200)
    assert abs(position(settled)[2] - idle_height) < 1, position(settled)
    descended = drive('move 0 0 -127', lambda fields: position(fields)[2] < idle_height - 16
                      and float(fields[26]) < -20)
    exited = drive('move -127 0 0', lambda fields: position(fields)[1] < -1095)
    landed = drive('move 0 0 0', lambda fields: position(fields)[2] < position(exited)[2] - 3
                   and abs(float(fields[26])) < 1 and fields[27:29] == ['-', '-'])

    # Reapproach from the lower landing, climb under the lip, then walk
    # through the shallow opening toward the Source upper dismount marker.
    drive('move 127 0 0', lambda fields: -1087 < position(fields)[1] < -1070)
    lip = drive('move 0 0 127', lambda fields: position(fields)[2] > 10
                and int(fields[19].split(',')[3]) < 20)
    upper = drive('move 127 0 0', lambda fields: position(fields)[1] > -1034
                  and position(fields)[2] > 7.5 and fields[27:29] == ['-', '-'])
    upper_observer = await_state(other_sock, other_probe, 'snapshot', lambda fields:
                                 fields[7] == '1' and position(fields)[1] > -1034
                                 and position(fields)[2] > 7.5 and fields[27:29] == ['-', '-'])
    reentered = drive('move -127 0 0', lambda fields: position(fields)[1] < -1070 and fields[27] == 'hang', seconds=3)
    returned = drive('move 0 0 -127', lambda fields: position(fields)[2] < -55 and float(fields[26]) < -20)
    lower_observer = await_state(other_sock, other_probe, 'snapshot', lambda fields:
                                 fields[7] == '1' and int(fields[29]) >= int(returned[29])
                                 and position(fields)[2] < -55 and float(fields[26]) < -20)
    return dict(initial=position(initial), climbed=position(climbed), observer=position(watched),
                stationary=position(settled), descended=position(descended),
                exited=position(exited), landed=position(landed), lip=position(lip),
                upper_dismount=position(upper), upper_observer=position(upper_observer),
                upper_reentry=position(reentered), returned=position(returned),
                lower_observer=position(lower_observer))


def check_construct_switch(owner, observer, join_late):
    _, sock, probe, _ = owner
    _, other_sock, other_probe, _ = observer
    origin = (-2698, -2598, -207.2)

    def lights(frame):
        return [wire_value(channel) for channel in frame[40:43]]

    before = await_state(sock, probe, 'snapshot')
    assert lights(before) == ['z'] * 3 and before[43] == '0', before
    # The observer already follows the owner, so it receives the local
    # speaker from the same PVS as the owner.
    await_state(other_sock, other_probe, 'snapshot', lambda fields: fields[7] == '1')
    before = await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) > int(before[1])+1100)
    # Approach from the room side of the switch's supporting wall.
    sock.send(probe.packet('setviewpos -2698 -2646 -229 90'))
    frame = await_state(sock, probe, 'snapshot', lambda fields:
                        math.dist(tuple(map(float, fields[19].split(',')[:3])), (-2698, -2646, -229)) < 15)
    positions = [frame[19]]
    samples = []
    for count, expected in ((1, 'a'), (2, 'z')):
        for _ in range(12):
            sock.send(probe.packet('use ' + ' '.join(map(str, origin))))
            frame = await_state(sock, probe, 'snapshot', lambda fields: int(fields[29]) >= int(frame[1]))
            positions.append(frame[19])
            if lights(frame) == [expected] * 3:
                break
        else:
            raise AssertionError((expected, positions, frame[6], frame[8], lights(frame)))
        frame = await_state(sock, probe, 'snapshot', lambda fields: fields[43] == str(count))
        seen = await_state(other_sock, other_probe, 'snapshot', lambda fields:
                           lights(fields) == [expected] * 3 and fields[43] == str(count))
        samples.append(dict(rgb=lights(frame), actor_clicks=int(frame[43]), observer_clicks=int(seen[43])))
        sock.send(probe.packet('move 0 0 0'))
        frame = await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) > int(frame[1])+450)
        assert lights(frame) == [expected] * 3 and frame[43] == str(count)
        if count == 1:
            _, late_sock, late_probe, late_state = join_late()
            assert late_state['initial_room_light'] == ['a'] * 3, late_state
            late_sock.send(late_probe.packet(f'follow {owner[3]["slot"]}'))
            late = await_state(late_sock, late_probe, 'snapshot', lambda fields: fields[7] == '1')
            assert lights(late) == ['a'] * 3 and late[43] == '0', late
        else:
            late = await_state(late_sock, late_probe, 'snapshot', lambda fields:
                               lights(fields) == ['z'] * 3 and fields[43] == '1')
    return dict(initial=lights(before), transitions=samples,
                late_join=dict(slot=late_state['slot'], initial=late_state['initial_room_light'],
                               replayed_clicks=0, restored=lights(late), new_clicks=int(late[43])))


def check_construct_mirror(owner, observer):
    _, sock, probe, _ = owner
    _, other_sock, other_probe, _ = observer
    # The mirror faces positive Y. Stand on that side in a non-solid leaf
    # so both connections receive the authored portal from the room PVS.
    frame = await_state(sock, probe, 'snapshot')
    await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) > int(frame[1])+1100)
    sock.send(probe.packet('setviewpos -2064 -2000 -300 270'))
    mirror = await_state(sock, probe, 'snapshot', lambda fields: fields[44] != '-')
    watched = await_state(other_sock, other_probe, 'snapshot', lambda fields:
                          fields[7] == '1' and fields[44] == mirror[44])
    number, *origin = mirror[44].split(',')
    position = tuple(map(float, origin))
    assert position == (-2064., -2067., -286.), mirror
    return dict(entity=int(number), origin=position, camera_origin=position,
                actor_snapshot=int(mirror[1]), observer_snapshot=int(watched[1]))


def wire_value(field):
    return '' if field == '-' else bytes.fromhex(field).decode('ascii')


def aim(sock, probe, yaw, pitch=35):
    before = await_state(sock, probe, 'snapshot')
    expected_yaw = float(before[8]) + yaw
    sock.send(probe.packet(f'aim {yaw} {pitch}'))
    return await_state(sock, probe, 'snapshot', lambda fields: abs(float(fields[6]) - pitch) < .1
                and abs((float(fields[8]) - expected_yaw + 180) % 360 - 180) < .1)


def spawn_body(sock, probe, kind, ident, visible):
    for yaw in (0, 90, -180, -90):
        aim(sock, probe, yaw)
        sock.send(probe.packet(f'spawn {ident}'))
        try:
            fields = await_state(sock, probe, 'snapshot', visible, timeout=1.3)
        except WireTimeout:
            continue
        entities = visible(fields)
        assert len(entities) == 1, fields
        return entities[0], int(fields[1])
    raise AssertionError(f'{kind} did not appear in any active snapshot')


def spawn_light(client, kind, ident):
    _, sock, probe, _ = client
    entity, server_time = spawn_body(sock, probe, kind, ident, snapshot_lights)
    return dict(entity=entity, packed_rgb_radius='0x28ffb27f', spawn_snapshot_time=server_time)


def activate_music(client, ident):
    _, sock, probe, state = client
    expected = 'music/hoth2/hoth2_explore.mp3 music/hoth2/hoth2_explore.mp3 @jar_music'
    baseline = state['initial_music']
    assert baseline != expected
    entity, _ = spawn_body(sock, probe, 'target_play_music', ident,
                           lambda fields: snapshot_entities(fields[13]))
    time.sleep(1.1)
    sock.send(probe.packet(f'select {entity}'))
    selected = await_state(sock, probe, 'snapshot', lambda fields: int(fields[10]) == entity
                           and int(fields[11]) > 0 and int(fields[12]) == ident)
    time.sleep(1.1)
    sock.send(probe.packet(f'activate {int(selected[11])}'))
    activated = await_state(sock, probe, 'snapshot', lambda fields: wire_value(fields[9]) == expected)
    return dict(entity=entity, baseline=baseline, activated_cue=expected,
                activation_snapshot_time=int(activated[1]))


def undo_music(owner, observer, music):
    time.sleep(1.1)
    _, sock, probe, _ = owner
    sock.send(probe.packet('undo'))
    restored = await_state(sock, probe, 'snapshot', lambda fields: wire_value(fields[9]) == music['baseline']
                           and music['entity'] not in snapshot_entities(fields[13]))
    _, other_sock, other_probe, _ = observer
    observed = await_state(other_sock, other_probe, 'snapshot', lambda fields:
                           int(fields[1]) >= int(restored[1]) and wire_value(fields[9]) == music['baseline'])
    music['owner_restoration_snapshot_time'] = int(restored[1])
    music['observer_restoration_snapshot_time'] = int(observed[1])


def check_movement_markers(clients):
    _, sock, probe, _ = clients[0]
    _, other_sock, other_probe, _ = clients[1]
    sock.send(probe.packet('move 0 0 0'))
    baseline = await_state(sock, probe, 'snapshot', lambda fields: int(fields[34]) > 0)
    default_gravity = int(baseline[34])
    assert default_gravity != 400 and baseline[35] == '0'

    def activate(kind, ident, observed):
        entity, _ = spawn_body(sock, probe, kind, ident, lambda fields: snapshot_entities(fields[13]))
        time.sleep(1.1)
        sock.send(probe.packet(f'select {entity}'))
        selected = await_state(sock, probe, 'snapshot', lambda fields:
                               int(fields[10]) == entity and int(fields[12]) == ident)
        time.sleep(1.1)
        sock.send(probe.packet(f'activate {selected[11]}'))
        active = await_state(sock, probe, 'snapshot', observed)
        time.sleep(1.1)
        sock.send(probe.packet('undo'))
        await_state(sock, probe, 'snapshot', lambda fields: entity not in snapshot_entities(fields[13]))
        return active

    gravity = activate('Gravity change', 95709066, lambda fields: fields[34] == '400')
    friction = activate('Friction change', 92295820, lambda fields: fields[34:36] == ['400', '3'])
    unchanged = await_state(other_sock, other_probe, 'snapshot', lambda fields: int(fields[1]) >= int(friction[1]))
    assert unchanged[34:36] == [str(default_gravity), '0'], unchanged
    return dict(gravity=int(gravity[34]), ground_friction=int(friction[35])-1,
                other_client_gravity=int(unchanged[34]), other_client_friction_override=False)


def check_space_volume(clients):
    _, sock, probe, _ = clients[0]
    _, other_sock, other_probe, _ = clients[1]
    baseline = await_state(sock, probe, 'snapshot', lambda fields: int(fields[34]) > 0)
    default_gravity = int(baseline[34])
    aimed = aim(sock, probe, round(math.remainder(-float(baseline[8]), 360)), pitch=45)
    x, y, z, viewheight = map(float, aimed[19].split(','))
    target = x + z + viewheight - SECTION_FLOOR_TRACE_Z
    sock.send(probe.packet('spawn 87701002'))
    spawned = await_state(sock, probe, 'snapshot', lambda fields: len(snapshot_entities(fields[13])) == 1)
    entity, = snapshot_entities(spawned[13])

    def wait_for_request_window(frame):
        return await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) >= int(frame[1]) + 150)

    wait_for_request_window(spawned)
    sock.send(probe.packet(f'select {entity}'))
    selected = await_state(sock, probe, 'snapshot', lambda fields: int(fields[10]) == entity and fields[12] == '87701002')
    wait_for_request_window(selected)
    sock.send(probe.packet('space_height 8'))
    shrunk = wait_for_request_window(await_state(sock, probe, 'snapshot'))

    # Native positioning gives the actor forward velocity, so start on the
    # near half of the volume to leave space for the subsequent movement step.
    point = (target - 32, y, 36)
    too_high = position_wire_client(sock, probe, point, 0, 20)
    assert int(too_high[1]) >= int(shrunk[1]) and int(too_high[34]) == default_gravity, too_high
    sock.send(probe.packet('space_height 81'))
    wait_for_request_window(await_state(sock, probe, 'snapshot'))
    position_wire_client(sock, probe, point, 0, 20)
    sock.send(probe.packet('move 0 0 0'))
    inside = await_state(sock, probe, 'snapshot', lambda fields: fields[34] == '0')
    outside = await_state(other_sock, other_probe, 'snapshot', lambda fields: int(fields[1]) >= int(inside[1]))
    assert int(outside[34]) == default_gravity, outside
    wait_for_request_window(inside)
    sock.send(probe.packet('undo'))
    restored = await_state(sock, probe, 'snapshot', lambda fields:
                           entity not in snapshot_entities(fields[13]) and int(fields[34]) == default_gravity)
    await_state(other_sock, other_probe, 'snapshot', lambda fields: entity not in snapshot_entities(fields[13]))
    return dict(entity=entity, gravity_after_shrink=int(too_high[34]), gravity_inside=int(inside[34]), gravity_outside=int(outside[34]),
                gravity_after_undo=int(restored[34]))


def check_directional_trigger(clients):
    _, sock, probe, _ = clients[0]
    _, other_sock, other_probe, _ = clients[1]
    start = await_state(sock, probe, 'snapshot')
    yaw = round(math.remainder(-float(start[8]), 360))
    aimed = aim(sock, probe, yaw, pitch=25)
    x, y, z, viewheight = map(float, aimed[19].split(','))
    # The root faces east. Its trigger is centered over the flat floor hit.
    plane_x = x + (z + viewheight - SECTION_FLOOR_TRACE_Z) / math.tan(math.radians(25))
    sock.send(probe.packet('spawn 89652619'))
    spawned = await_state(sock, probe, 'snapshot', lambda fields: len(snapshot_entities(fields[13])) == 3)
    ids = snapshot_entities(spawned[13])
    await_state(other_sock, other_probe, 'snapshot', lambda fields: snapshot_entities(fields[13]) == ids)

    def position(x, yaw):
        sock.send(probe.packet(f'setviewpos {round(x)} {round(y)} 24 {yaw}'))
        return await_state(sock, probe, 'snapshot', lambda fields: abs(float(fields[19].split(',')[0])-x) < 14)

    def cross(start, yaw, expected):
        frame = position(start, yaw)
        for _ in range(90):
            if frame[36:38] == expected:
                return frame
            time.sleep(.015)
            sock.send(probe.packet('move 127 0 0 25'))
            frame = await_state(sock, probe, 'snapshot')
        raise AssertionError((expected, frame[19], frame[36:38], plane_x))

    forward = cross(plane_x - 85, 0, ['1', '0'])
    # Let the shared one-second cooldown expire before the reverse pass.
    await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) > int(forward[1]) + 1100)
    reverse = cross(plane_x + 85, 180, ['1', '1'])
    observer = await_state(other_sock, other_probe, 'snapshot', lambda fields: int(fields[1]) >= int(reverse[1]))
    assert observer[36:38] == ['0', '0'], observer
    sock.send(probe.packet('undo'))
    await_state(sock, probe, 'snapshot', lambda fields: not snapshot_entities(fields[13]))
    await_state(other_sock, other_probe, 'snapshot', lambda fields: not snapshot_entities(fields[13]))
    return dict(forward=1, reverse=1, other_client_feedback=0, undo_removed=ids)


def check_searchlight(clients):
    _, sock, probe, _ = clients[0]
    _, other_sock, other_probe, _ = clients[1]
    start = await_state(sock, probe, 'snapshot')
    aim(sock, probe, round(math.remainder(-float(start[8]), 360)), pitch=25)
    sock.send(probe.packet('spawn 95681750'))
    clear = await_state(sock, probe, 'snapshot', lambda fields: fields[38] != '-')
    parts = clear[38].split(',')
    entity, packed, radius = map(int, parts[:3])
    origin = tuple(map(float, parts[3:6]))
    clear_endpoint = tuple(map(float, parts[6:9]))
    assert packed >> 24 == 8 and radius > 0 and clear_endpoint[0] > origin[0] + 500, clear
    ids = snapshot_entities(clear[13])
    assert entity in ids and len(ids) == 3, clear
    await_state(other_sock, other_probe, 'snapshot', lambda fields: fields[38] == clear[38])

    # TeleportPlayer adds forward velocity. Face away from the lamp so the
    # player stays in its cone while that velocity decays.
    sock.send(probe.packet(f'setviewpos {round(origin[0]+360)} {round(origin[1])} 24 0'))
    detected = await_state(sock, probe, 'snapshot', lambda fields: fields[39] == '1')
    clipped_endpoint = tuple(map(float, detected[38].split(',')[6:9]))
    assert origin[0] + 300 < clipped_endpoint[0] < origin[0] + 750, detected
    await_state(other_sock, other_probe, 'snapshot', lambda fields:
                int(fields[1]) >= int(detected[1]) and fields[38] != '-' and
                origin[0] + 300 < float(fields[38].split(',')[6]) < origin[0] + 750)
    stationary = await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) > int(detected[1]) + 1100)
    observer = await_state(other_sock, other_probe, 'snapshot', lambda fields: int(fields[1]) >= int(stationary[1]))
    assert stationary[39] == '1' and observer[39] == '0', (stationary, observer)

    sock.send(probe.packet('undo'))
    for client_sock, client_probe in ((sock, probe), (other_sock, other_probe)):
        await_state(client_sock, client_probe, 'snapshot', lambda fields:
                    fields[38] == '-' and not snapshot_entities(fields[13]))
    return dict(entity=entity, clear_endpoint=clear_endpoint, clipped_endpoint=clipped_endpoint,
                detected_notices=1, observer_notices=0, undo_removed=ids)


def check_portals(clients):
    _, sock, probe, _ = clients[0]
    _, other_sock, other_probe, _ = clients[1]
    start = await_state(sock, probe, 'snapshot')
    aim(sock, probe, round(math.remainder(-float(start[8]), 360)), pitch=18)
    sock.send(probe.packet('spawn 88142876'))
    active = await_state(sock, probe, 'snapshot', lambda fields: fields[40] != '-')
    panel, mode, width, height, *pose = active[40].split(',')
    panel = int(panel)
    assert (mode, float(width), int(height)) == ('1', 128, 192), active
    position = tuple(map(float, pose[:3]))
    camera = tuple(map(float, pose[3:]))
    assert math.isclose(math.dist(position, camera), 128, abs_tol=.01)
    ids = snapshot_entities(active[13])
    assert panel in ids and len(ids) == 2, active
    await_state(other_sock, other_probe, 'snapshot', lambda fields: fields[40] == active[40])
    # View the panel from beside the companion camera so the camera's
    # physical selection hull cannot intercept the selection ray.
    sock.send(probe.packet(f'setviewpos {round(position[0]-72)} {round(position[1]+80)} 24 90'))
    await_state(sock, probe, 'snapshot', lambda fields:
                abs(float(fields[19].split(',')[0])-position[0]+72)<8 and int(fields[1])>int(active[1])+250)
    sock.send(probe.packet(f'select {panel}'))
    selected = await_state(sock, probe, 'snapshot', lambda fields: int(fields[10]) >= 0)
    assert int(selected[10]) == panel, (active[40], selected[10], selected[19])
    await_state(sock, probe, 'snapshot', lambda fields: int(fields[1])>int(selected[1])+150)
    sock.send(probe.packet(f'activate {selected[11]}'))
    disabled = await_state(sock, probe, 'snapshot', lambda fields: fields[40].split(',')[1:2] == ['0'])
    await_state(other_sock, other_probe, 'snapshot', lambda fields: fields[40] == disabled[40])
    await_state(sock, probe, 'snapshot', lambda fields: int(fields[1])>int(disabled[1])+150)
    sock.send(probe.packet('undo'))
    for client_sock, client_probe in ((sock, probe), (other_sock, other_probe)):
        await_state(client_sock, client_probe, 'snapshot', lambda fields:
                    fields[40] == '-' and not snapshot_entities(fields[13]))
    return dict(panel=panel, dimensions=[128, 192], position=position, camera=camera,
                both_saw_active_and_disabled=True, undo_removed=ids)


def check_angular_movers(clients):
    _, sock, probe, _ = clients[0]
    _, other_sock, other_probe, _ = clients[1]

    def angles(fields):
        return {int(number): tuple(map(float, rotation))
                for number, *rotation in (record.split(',') for record in fields[41].split(';'))} if fields[41] != '-' else {}

    start = await_state(sock, probe, 'snapshot')
    aim(sock, probe, round(math.remainder(-float(start[8]), 360)), pitch=18)
    sock.send(probe.packet('spawn 97415810'))
    placed = await_state(sock, probe, 'snapshot', lambda fields: bool(angles(fields)))
    pendulum, = angles(placed)
    for client_sock, client_probe in ((sock, probe), (other_sock, other_probe)):
        await_state(client_sock, client_probe, 'snapshot', lambda fields: abs(angles(fields).get(pendulum, (0, 0, 0))[2]) > 4)
    await_state(sock, probe, 'snapshot', lambda fields: int(fields[1]) > int(placed[1])+250)
    sock.send(probe.packet('undo'))
    for client_sock, client_probe in ((sock, probe), (other_sock, other_probe)):
        await_state(client_sock, client_probe, 'snapshot', lambda fields: not angles(fields) and not snapshot_entities(fields[13]))
    return dict(pendulum=pendulum, both_saw_swing=True, undo_removed=True)


def check_console_npcs(clients, save):
    from mp_world_smoke import decode
    _, sock, probe, _ = clients[0]
    _, observer_sock, observer_probe, _ = clients[1]
    rcon, directory = save

    # Keep combat movement from changing the positions used to check overlap.
    # The dedicated server for this fixture enables cheats via devmap.
    sock.send(probe.packet('notarget'))
    observer_sock.send(observer_probe.packet('notarget'))

    def visible_actors(frame):
        if frame[43] == '-':
            return {}
        return {int(number): tuple(map(float, origin))
                for number, flags, *origin in (row.split(',') for row in frame[43].split(';'))
                if not int(flags) & (1 << 8)}

    position_wire_client(sock, probe, (0, 0, 24), 0, 32)
    sock.send(probe.packet('npcspawn'))
    first = await_state(sock, probe, 'snapshot', lambda f: f[42] == '1' and len(visible_actors(f)) == 1)
    await_state(sock, probe, 'snapshot', lambda f: int(f[1]) >= int(first[1])+250)
    # setviewpos applies an impulse. Reset the player's origin before each
    # command, then send the spawn without an intervening movement command.
    position_wire_client(sock, probe, (0, 0, 24), 0, 4)
    sock.send(probe.packet('npcspawn'))
    second = await_state(sock, probe, 'snapshot', lambda f: f[42] == '2' and len(visible_actors(f)) == 2)
    actors = visible_actors(second)
    assert math.dist(*actors.values()) < 32, actors
    await_state(observer_sock, observer_probe, 'snapshot', lambda f: set(visible_actors(f)) == set(actors))
    assert 'Saved console_npcs revision 1: 2 objects' in rcon('jar_world_save console_npcs')
    journal = decode(directory / 'console_npcs.0.jrw')
    assert [node[0] for node in journal['nodes']] == [36798905, 36798905]
    sock.send(probe.packet('undo'))
    undone = await_state(sock, probe, 'snapshot', lambda f: f[42] == '1' and set(visible_actors(f)) == set(visible_actors(first)))
    await_state(sock, probe, 'snapshot', lambda f: int(f[1]) >= int(undone[1])+250)
    sock.send(probe.packet('cleanup'))
    cleared = await_state(sock, probe, 'snapshot', lambda f: f[42] == '0' and not visible_actors(f))
    await_state(observer_sock, observer_probe, 'snapshot', lambda f: not visible_actors(f))
    await_state(sock, probe, 'snapshot', lambda f: int(f[1]) >= int(cleared[1])+250)
    menu_spawns = []
    for count in (1, 2):
        position_wire_client(sock, probe, (0, 0, 24), 0, 4)
        aim(sock, probe, 0, pitch=28)
        sock.send(probe.packet('spawn 36798905'))
        frame = await_state(sock, probe, 'snapshot', lambda f: int(f[42]) == count and len(visible_actors(f)) == count)
        menu_spawns.append(visible_actors(frame))
        await_state(sock, probe, 'snapshot', lambda f: int(f[1]) >= int(frame[1])+250)
    assert math.dist(*menu_spawns[-1].values()) < 32, menu_spawns
    sock.send(probe.packet('cleanup'))
    cleared = await_state(sock, probe, 'snapshot', lambda f: f[42] == '0' and not visible_actors(f))
    await_state(sock, probe, 'snapshot', lambda f: int(f[1]) >= int(cleared[1])+250)

    # Out-of-range troopers form separate patrol groups. A freed NPC's old
    # slot has origin zero. Leaving that slot in the second roster used to
    # make the next group update try to merge it into the group near zero.
    patrols = []
    for count, x in enumerate((0, 1024), start=1):
        position_wire_client(sock, probe, (x, 0, 24), 0, 4)
        sock.send(probe.packet('npcspawn'))
        frame = await_state(sock, probe, 'snapshot', lambda f: int(f[42]) == count and len(visible_actors(f)) == count)
        patrols.append(visible_actors(frame))
        await_state(sock, probe, 'snapshot', lambda f: int(f[1]) >= int(frame[1])+500)
    assert math.dist(*patrols[-1].values()) > 384, patrols
    sock.send(probe.packet('undo'))
    result = await_state(sock, probe, 'snapshot', lambda f: f[42] == '1' and visible_actors(f).keys() == patrols[0].keys())
    for peer in ((sock, probe), (observer_sock, observer_probe)):
        await_state(*peer, 'snapshot', lambda f: int(f[1]) >= int(result[1])+350 and visible_actors(f).keys() == patrols[0].keys())
    return dict(actors=actors, journal_count=2, count_updates=[1, 2, 1, 0], observer_saw_cleanup=True,
                menu_actors=menu_spawns[-1], distant_patrol_undo=patrols[-1])


def undo_light(owner, light, observer=None):
    # Native reliable client command flood protection is enabled.
    time.sleep(1.1)
    _, sock, probe, _ = owner
    sock.send(probe.packet('undo'))
    removed = await_state(sock, probe, 'snapshot', lambda fields: not snapshot_lights(fields))
    light['undo_removed_from_snapshot'] = True
    if observer:
        _, other_sock, other_probe, _ = observer
        observed = await_state(other_sock, other_probe, 'snapshot', lambda fields:
                               fields[7] == '1' and fields[2] == removed[2]
                               and int(fields[1]) >= int(removed[1]) and not snapshot_lights(fields))
        light['observer_removal_snapshot_time'] = int(observed[1])


def bsp_slots(field):
    if field == '-':
        return {}
    records = (record.split(',') for record in field.split(';'))
    return {int(slot): wire_value(resource) for slot, resource in records}


def inline_resources(slots):
    # The open fixture has one inline model. Each mounted BSP consumes its
    # complete retail model count, including any child brushes.
    first = 1
    resources = {}
    for slot, resource in sorted(slots.items()):
        assert slot == len(resources) + 1, slots
        resources[first] = resource
        first += SECTION_MODEL_COUNTS[resource]
    return resources


def section_frame(fields):
    resources = inline_resources(bsp_slots(fields[14]))
    roots = {}
    if fields[15] != '-':
        for record in fields[15].split(';'):
            entity, model, x, y, z, yaw = record.split(',')
            model = int(model)
            assert model in resources, ('Portable BSP root arrived before its resource slot', model, resources)
            roots[int(entity)] = dict(model_index=model, resource=resources[model],
                                      origin=tuple(map(float, (x, y, z))), yaw=float(yaw))
    speakers = {}
    if fields[16] != '-':
        for record in fields[16].split(';'):
            entity, owner, sound, path, x, y, z = record.split(',')
            speakers[int(entity)] = dict(owner=int(owner), sound_index=int(sound), sound=wire_value(path),
                                         origin=tuple(map(float, (x, y, z))))
    torches = {}
    if fields[17] != '-':
        for record in fields[17].split(';'):
            entity, owner, effect, effect_path, sound, sound_path, x, y, z, dx, dy, dz = record.split(',')
            torches[int(entity)] = dict(owner=int(owner), effect_index=int(effect), effect=wire_value(effect_path),
                                        sound_index=int(sound), sound=wire_value(sound_path),
                                        origin=tuple(map(float, (x, y, z))), direction=tuple(map(float, (dx, dy, dz))))
    bobbers = {}
    if fields[20] != '-':
        for record in fields[20].split(';'):
            entity, owner, brush, model, path, start, period, x, y, z, dx, dy, dz, yaw = record.split(',')
            bobbers[int(entity)] = dict(owner=int(owner), model_index=int(brush),
                                        visual_model_index=int(model), visual_model=wire_value(path),
                                        trajectory='TR_SINE', solid_brush=True, trajectory_time=int(start),
                                        period_ms=int(period), origin=tuple(map(float, (x, y, z))),
                                        delta=tuple(map(float, (dx, dy, dz))), yaw=float(yaw))
    brushes = {}
    if fields[21] != '-':
        for record in fields[21].split(';'):
            entity, owner, model, x, y, z, yaw = record.split(',')
            brushes[int(entity)] = dict(owner=int(owner), model_index=int(model), solid_brush=True,
                                        origin=tuple(map(float, (x, y, z))), yaw=float(yaw))
    return roots, speakers, torches, bobbers, brushes


def section_frame_matches(fields, slots, count, baseline):
    # Validate the slot order against the configstrings seen at the instant
    # each snapshot arrived, including snapshots earlier than the desired one.
    roots, speakers, torches, bobbers, brushes = section_frame(fields)
    unknown = () if fields[18] == '-' else fields[18].split(';')
    assert set(unknown) <= set(baseline), ('Unexpected non-client section entity', unknown, baseline)
    if bsp_slots(fields[14]) != slots or len(roots) != count:
        return False
    return (len(speakers) == sum(len(SECTION_SPEAKERS.get(root['resource'], ())) for root in roots.values())
            and len(torches) == sum(len(SECTION_TORCHES.get(root['resource'], ())) for root in roots.values())
            and len(bobbers) == sum(root['resource'] == BESPIN_RESOURCE for root in roots.values())
            and len(brushes) == sum(SECTION_BRUSHES.get(root['resource'], 0) for root in roots.values()))


def sentinel_children(fields):
    def records(field):
        return [] if field == '-' else [record.split(',') for record in field.split(';')]

    lifts = {int(row[0]): (int(row[1]), int(row[2]), int(row[3]), int(row[4]), tuple(map(float, row[5:])))
             for row in records(fields[30])}
    pickups = {int(row[0]): (int(row[1]), int(row[2]), int(row[3]), tuple(map(float, row[4:])))
               for row in records(fields[31])}
    return lifts, pickups


def section_scenery(fields):
    scenery = {}
    if fields[32] != '-':
        for record in fields[32].split(';'):
            entity, owner, model, x, y, z, sx, sy, sz, yaw = record.split(',')
            scenery[int(entity)] = dict(owner=int(owner), model=bytes.fromhex(model).decode(), origin=tuple(map(float, (x, y, z))),
                                        scale=tuple(map(float, (sx, sy, sz))), yaw=float(yaw))
    return scenery


def section_point(root, local):
    x, y, z = local
    cosine, sine = math.cos(math.radians(root['yaw'])), math.sin(math.radians(root['yaw']))
    ox, oy, oz = root['origin']
    return (ox + cosine*x - sine*y, oy + sine*x + cosine*y, oz + z)


def assert_section_children(roots, speakers, torches, bobbers, brushes, snapshot_time):
    owners = {child['owner'] for child in (*speakers.values(), *torches.values(), *bobbers.values(), *brushes.values())}
    assert set(roots) - owners == {
        entity for entity, root in roots.items() if root['resource'] in (TASPIR_RESOURCE, RIFT_RESOURCE)}
    for entity, root in roots.items():
        ox, oy, oz = root['origin']
        yaw = math.radians(root['yaw'])
        cosine, sine = math.cos(yaw), math.sin(yaw)
        own_speakers = [speaker for speaker in speakers.values() if speaker['owner'] == entity]
        expected_speakers = SECTION_SPEAKERS.get(root['resource'], ())
        assert len(own_speakers) == len(expected_speakers), own_speakers
        for (x, y, z), sound in expected_speakers:
            expected = (ox + cosine*x - sine*y, oy + sine*x + cosine*y, oz + z)
            assert sum(speaker['sound'] == sound and math.dist(speaker['origin'], expected) < .125
                       for speaker in own_speakers) == 1, (entity, expected, own_speakers)

        own_torches = [torch for torch in torches.values() if torch['owner'] == entity]
        expected_torches = SECTION_TORCHES.get(root['resource'], ())
        assert len(own_torches) == len(expected_torches), own_torches
        for x, y, z in expected_torches:
            expected = (ox + cosine*x - sine*y, oy + sine*x + cosine*y, oz + z)
            assert sum(torch['effect'] == TORCH_EFFECT and torch['sound'] == TORCH_LOOP
                       and math.dist(torch['origin'], expected) < .125
                       and (root['resource'] == YAVIN_RESOURCE or math.dist(torch['direction'], (0, 0, 1)) < .001)
                       for torch in own_torches) == 1, (entity, expected, own_torches)

        own_bobbers = [bobber for bobber in bobbers.values() if bobber['owner'] == entity]
        assert len(own_bobbers) == (root['resource'] == BESPIN_RESOURCE), own_bobbers
        if own_bobbers:
            bobber, = own_bobbers
            x, y, z = BOBBER_ORIGIN
            expected = (ox + cosine*x - sine*y, oy + sine*x + cosine*y, oz + z)
            assert math.dist(bobber['origin'], expected) < .125, (expected, bobber)
            assert bobber['model_index'] == root['model_index'] + 1, bobber
            assert bobber['visual_model'] == BOBBER_MODEL and bobber['period_ms'] == 3000, bobber
            assert math.dist(bobber['delta'], (0, 0, 20)) < .001, bobber
            assert abs((bobber['yaw'] - root['yaw'] + 180) % 360 - 180) < .01, bobber
            assert 0 <= bobber['trajectory_time'] <= snapshot_time, bobber

        own_brushes = [brush for brush in brushes.values() if brush['owner'] == entity]
        if root['resource'] in SECTION_BRUSHES:
            count = SECTION_BRUSHES[root['resource']]
            assert len(own_brushes) == count
            assert {brush['model_index'] for brush in own_brushes} == set(
                range(root['model_index'] + 1, root['model_index'] + 1 + count))
            for brush in own_brushes:
                assert math.dist(brush['origin'], root['origin']) < .125, brush
                assert abs((brush['yaw'] - root['yaw'] + 180) % 360 - 180) < .01, brush
        else:
            assert not own_brushes, own_brushes


def section_placement(entity, root, aimed):
    low, high = SECTION_BOUNDS[root['resource']]
    x, y, z, viewheight = map(float, aimed[19].split(','))
    pitch, view_yaw, room_yaw = map(math.radians, (float(aimed[6]), float(aimed[8]), root['yaw']))
    distance = (z + viewheight - SECTION_FLOOR_TRACE_Z) / math.tan(pitch)
    hit = (x + distance * math.cos(view_yaw), y + distance * math.sin(view_yaw))
    center_x, center_y = ((low[axis] + high[axis]) * .5 for axis in (0, 1))
    ox, oy, oz = root['origin']
    center = (ox + center_x * math.cos(room_yaw) - center_y * math.sin(room_yaw),
              oy + center_x * math.sin(room_yaw) + center_y * math.cos(room_yaw))
    bottom = oz + low[2]
    assert math.dist(center, hit) < .5, (root, center, hit, aimed[19])
    # Placement keeps four units beyond the padded collision hull.
    assert abs(bottom - SECTION_FLOOR_TRACE_Z - 5) < .02, (root, bottom)
    return dict(entity=entity, aimed_floor=(*hit, SECTION_FLOOR_TRACE_Z),
                stock_center_xy=center, stock_bottom_z=bottom)


def spawn_sections(client, ids):
    _, sock, probe, state = client
    assert not state['initial_section_resources']
    before = await_state(sock, probe, 'snapshot')
    assert before[14:18] == ['-', '-', '-', '-'] and before[20:22] == ['-', '-'], before
    baseline = [] if before[18] == '-' else before[18].split(';')
    slots, roots, speakers, torches, bobbers, brushes = {}, {}, {}, {}, {}, {}
    spawn_order, placement_proof = [], []
    # Let the player settle on the fixture floor before aiming shallowly.
    aim(sock, probe, 0, pitch=20)
    aimed = aim(sock, probe, 0, pitch=25)
    if YAVIN_RESOURCE in ids:
        scenario = 'yavin'
        placements = ((YAVIN_RESOURCE, 4900, 0),)
    elif KORRIBAN_RESOURCE in ids:
        scenario = 'korriban'
        placements = ((KORRIBAN_RESOURCE, 3500, 0),)
    elif SHAFT_RESOURCE in ids:
        scenario = 'shaft'
        placements = ((SHAFT_RESOURCE, 3000, 0),)
    elif RIFT_RESOURCE in ids:
        scenario = 'rift'
        placements = ((RIFT_RESOURCE, 3500, 0),)
    elif HOTH_RESOURCE in ids:
        scenario = 'hoth'
        # Its east-west bounds include the detached sky chamber. The larger
        # section aim range lets the player place it from outside its hull.
        placements = ((HOTH_RESOURCE, 4800, 0),)
    elif SENTINEL_RESOURCE in ids:
        scenario = 'sentinel'
        placements = ((SENTINEL_RESOURCE, 3500, 0),)
    elif CITY_RESOURCE in ids:
        scenario = 'city_breakables'
        placements = ((CITY_RESOURCE, 3300, 0), (VJUN_RESOURCE, -3300, 0))
    elif GENERATOR_RESOURCE in ids:
        scenario = 'hazard_brushes'
        # Taspir occupies the east half. Keep the others in three separate
        # lanes west of its hull and within the 4,096-unit aim range.
        placements = ((TASPIR_RESOURCE, 3860, 0), (GENERATOR_RESOURCE, -3850, 0),
                      (RANCOR_RESOURCE, -2050, 3375), (BESPIN_RESOURCE, -2050, -3375))
    else:
        scenario = 'mixed'
        placements = ((IMPERIAL_RESOURCE, 2600, 0), (IMPERIAL_RESOURCE, -2600, 0),
                      (VJUN_RESOURCE, 0, 3000), (RANCOR_RESOURCE, 0, -3400))
    for resource, dx, dy in placements:
        if resource not in slots.values():
            slots[len(slots) + 1] = resource
        _, _, z, viewheight = map(float, aimed[19].split(','))
        yaw = math.degrees(math.atan2(dy, dx))
        yaw_delta = round(math.remainder(yaw - float(aimed[8]), 360))
        pitch = math.degrees(math.atan2(z + viewheight - SECTION_FLOOR_TRACE_Z, math.hypot(dx, dy)))
        aimed = aim(sock, probe, yaw_delta, pitch=pitch)
        sock.send(probe.packet(f'spawn {ids[resource]}'))
        try:
            fields = await_state(sock, probe, 'snapshot', lambda fields:
                                 section_frame_matches(fields, slots, len(spawn_order) + 1, baseline))
        except WireTimeout as exc:
            raise AssertionError(f'Section {resource} did not appear from {aimed[19]}, pitch {aimed[6]}, yaw {aimed[8]}') from exc
        next_roots, next_speakers, next_torches, next_bobbers, next_brushes = section_frame(fields)
        assert all(next_roots[entity] == root for entity, root in roots.items()), next_roots
        assert all(next_speakers[entity] == speaker for entity, speaker in speakers.items()), next_speakers
        assert all(next_torches[entity] == torch for entity, torch in torches.items()), next_torches
        assert all(next_bobbers[entity] == bobber for entity, bobber in bobbers.items()), next_bobbers
        assert all(next_brushes[entity] == brush for entity, brush in brushes.items()), next_brushes
        entity, = next_roots.keys() - roots.keys()
        assert next_roots[entity]['resource'] == resource, next_roots
        spawn_order.append(entity)
        placement_proof.append(section_placement(entity, next_roots[entity], aimed))
        roots, speakers, torches, bobbers, brushes = next_roots, next_speakers, next_torches, next_bobbers, next_brushes
        assert_section_children(roots, speakers, torches, bobbers, brushes, int(fields[1]))
        time.sleep(1.1)
    if scenario == 'mixed':
        first, second = spawn_order[:2]
        assert math.dist(roots[first]['origin'], roots[second]['origin']) > 2000, roots
    result = dict(scenario=scenario, resources=slots, inline_model_resources=inline_resources(slots),
                resources_precede_root_snapshots=True, baseline_non_player_entities=baseline,
                spawn_order=spawn_order, placements=placement_proof, roots=roots, speakers=speakers, torches=torches,
                bobbers=bobbers, brushes=brushes, spawn_snapshot_time=int(fields[1]))
    if scenario == 'sentinel':
        lifts, pickups = sentinel_children(fields)
        root = roots[entity]
        assert len(lifts) == 10 and len(pickups) == 45, (len(lifts), len(pickups))
        assert {row[1] - root['model_index'] for row in lifts.values()} == {1, 2, 3, 4, 6, 11, 13, 14, 15, 16}
        assert all(row[0] == entity and row[2] == 0 and row[3] > 0 for row in lifts.values())
        assert {row[0] for row in pickups.values()} == {entity}
        result.update(lifts=lifts, pickups=pickups)
    if scenario in ('rift', 'korriban'):
        lifts, pickups = sentinel_children(fields)
        assert not lifts and len(pickups) == (38 if scenario == 'rift' else 47)
        assert {row[0] for row in pickups.values()} == {entity}
        result['pickups'] = pickups
    if scenario == 'shaft':
        lifts, pickups = sentinel_children(fields)
        assert len(lifts) == 2 and not pickups
        root = roots[entity]
        assert {row[1] - root['model_index'] for row in lifts.values()} == {2, 3}
        assert all(row[0] == entity and row[2] == 0 and math.dist(row[4], section_point(root, (0, 0, -266))) < .125
                   for row in lifts.values())
        scenery = section_scenery(fields)
        expected = [('lava_ring', 584, (1, 1, 1), yaw) for yaw in (0, 90, 180, 270)]
        expected += [('lava_tube', 832, (.76, .76, .76), 0), ('tank_top', 1096, (1, 1, .5), 0)]
        assert len(scenery) == len(expected)
        for model, z, scale, yaw in expected:
            assert sum(row['owner'] == entity and row['model'] == f'models/map_objects/taspir/{model}.md3'
                       and math.dist(row['origin'], section_point(root, (-4352, 2048, z))) < .125
                       and math.dist(row['scale'], scale) < .001
                       and abs(math.remainder(row['yaw'] - root['yaw'] - yaw, 360)) < .01
                       for row in scenery.values()) == 1, (model, yaw, scenery)
        result.update(lifts=lifts, scenery=scenery)
    if scenario == 'yavin':
        scenery = section_scenery(fields)
        assert len(scenery) == 51 and {row['owner'] for row in scenery.values()} == {entity}
        result['scenery'] = scenery
    return result


def position_wire_client(sock, probe, point, yaw, tolerance):
    command = 'setviewpos %d %d %d %d' % (*map(round, point), yaw)
    latest = []

    def arrived(frame):
        latest[:] = map(float, frame[19].split(',')[:3])
        return math.dist(latest, point) < tolerance

    for _ in range(3):
        sock.send(probe.packet(command))
        try:
            return await_state(sock, probe, 'snapshot', arrived, timeout=2)
        except WireTimeout:
            continue
    raise AssertionError(f'Positioning failed for {point}, last position {latest}')


def check_sentinel_interaction(sock, probe, section, save):
    root, = section['roots'].values()
    lift = next(entity for entity, row in section['lifts'].items() if row[1] == root['model_index'] + 1)
    yaw = math.radians(root['yaw'])
    cosine, sine = math.cos(yaw), math.sin(yaw)
    x, y, z = root['origin']
    # Pad *7 activates lift *1 and its four matching stock brush pieces.
    # The native positioning command applies a forward impulse. Start toward
    # the west side of the pad and aim along its wider axis to retain contact.
    pad = (x + cosine * 3654 - sine * -880, y + sine * 3654 + cosine * -880, z + 1387)
    time.sleep(1.1)
    position_wire_client(sock, probe, pad, round(root['yaw']), 32)
    for _ in range(32):
        sock.send(probe.packet('move 0 0 0'))
        frame = await_state(sock, probe, 'snapshot')
        if sentinel_children(frame)[0][lift][2] == 3:
            break
    else:
        raise AssertionError(f'Sentinel pad did not trigger from {frame[19]}, target {pad}')
    rising = frame
    # The pad test establishes activation. Clear the actor from the lift's
    # path before checking an unobstructed cycle and its saved timers.
    position_wire_client(sock, probe, (0, 0, 32), 0, 64)
    raised = await_state(sock, probe, 'snapshot', lambda frame:
                         sentinel_children(frame)[0][lift][2] == 0 and
                         sentinel_children(frame)[0][lift][4] != section['lifts'][lift][4])
    moved_group = {entity for entity, row in sentinel_children(raised)[0].items()
                   if row[4] != section['lifts'][entity][4]}
    assert {section['lifts'][entity][1] - root['model_index'] for entity in moved_group} == {1, 2, 3, 4, 6}
    # The concussion rifle has a five-second stock respawn. Move away after
    # collecting it so it remains available in the next snapshot.
    rifle_xy = (x + cosine * 4224 - sine * 832, y + sine * 4224 + cosine * 832)
    rifle = next(entity for entity, row in section['pickups'].items() if math.dist(row[3][:2], rifle_xy) < 1)
    rx, ry, rz = section['pickups'][rifle][3]
    sock.send(probe.packet('setviewpos %d %d %d 0' % tuple(map(round, (rx, ry, rz + 24)))))
    sock.send(probe.packet('move 0 0 0'))
    collected = await_state(sock, probe, 'snapshot', lambda frame:
                            sentinel_children(frame)[1].get(rifle, (0, 0, 0))[2] & (1 << 23))
    rcon, directory = save
    from mp_world_smoke import decode
    assert 'Saved sentinel_active revision 1: 1 objects' in rcon('jar_world_save sentinel_active')
    saved = decode(directory / 'sentinel_active.0.jrw')
    state, = saved['sentinels']
    assert saved['version'] == 20 and sum(timer > 0 for timer in state['pickups']) == 1
    assert sum(phase == 1 for phase, _ in state['lifts']) == 5 and any(timer for _, timer in state['pads'])
    sock.send(probe.packet('setviewpos 0 0 32 0'))
    respawned = await_state(sock, probe, 'snapshot', lambda frame:
                            sentinel_children(frame)[1].get(rifle) == section['pickups'][rifle], timeout=8)
    elapsed = int(respawned[1]) - int(collected[1])
    assert 4900 <= elapsed <= 5150, elapsed
    assert sentinel_children(respawned)[0] == section['lifts']
    section['interaction'] = dict(lift_group=[1, 2, 3, 4, 6],
                                  lift_ascent_ms=int(raised[1]) - int(rising[1]),
                                  lift_returned=True, concussion_respawn_ms=elapsed)


def check_shaft_interaction(sock, probe, section, save):
    from mp_world_smoke import decode

    root, = section['roots'].values()
    rcon, directory = save
    for inline, local_y in ((2, 1510), (3, 2585)):
        lift = next(entity for entity, row in section['lifts'].items() if row[1] == root['model_index'] + inline)
        pad = section_point(root, (-4398, local_y, 24))
        position_wire_client(sock, probe, pad, round(root['yaw']), 36)
        for _ in range(32):
            time.sleep(.025)
            sock.send(probe.packet('move 0 0 0'))
            rising = await_state(sock, probe, 'snapshot')
            if sentinel_children(rising)[0][lift] != section['lifts'][lift]:
                break
        else:
            raise AssertionError(f'Imperial Shaft pad {inline} did not trigger from {rising[19]}, target {pad}')
        raised = await_state(sock, probe, 'snapshot', lambda frame:
                             sentinel_children(frame)[0][lift][2] == 0 and
                             sentinel_children(frame)[0][lift][4] != section['lifts'][lift][4])
        moved = {entity for entity, row in sentinel_children(raised)[0].items() if row != section['lifts'][entity]}
        assert moved == {lift}, moved
        if inline == 2:
            assert 'Saved shaft_active revision 1: 1 objects' in rcon('jar_world_save shaft_active')
            state = decode(directory / 'shaft_active.0.jrw')
            timers, = state['shafts']
            assert state['version'] == 21 and timers['lifts'][0][0] == 1 and timers['pads'][0][1] > 0, timers
        position_wire_client(sock, probe, (0, 0, 32), 0, 64)
        await_state(sock, probe, 'snapshot', lambda frame: sentinel_children(frame)[0] == section['lifts'], timeout=8)
    section['interaction'] = dict(independent_lifts=[2, 3], both_returned=True)


def check_section_weapon(sock, probe, section, save):
    from mp_world_smoke import decode

    root, = section['roots'].values()
    name = section['scenario']
    target = section_point(root, (7164, 1058, 8) if name == 'rift' else (-2624, 2528, 528))
    weapon = next(entity for entity, row in section['pickups'].items() if math.dist(row[3][:2], target[:2]) < 1)
    rx, ry, rz = section['pickups'][weapon][3]
    time.sleep(1.1)
    # Bring the last usercmd up to date before teleporting. A stale command
    # otherwise spends most of the 160 ms impulse in one movement update.
    frame = await_state(sock, probe, 'snapshot')
    for _ in range(8):
        if int(frame[1]) - int(frame[29]) <= 75:
            break
        sock.send(probe.packet('move 0 0 0'))
        frame = await_state(sock, probe, 'snapshot')
    # Position inside the touch region. Native setviewpos pushes at 400 u/s;
    # use short usercmd steps so the first touch check stays in range.
    arrival = (rx, ry, rz + 24)
    sock.send(probe.packet('setviewpos %d %d %d 90' % tuple(map(round, arrival))))
    collected = await_state(sock, probe, 'snapshot', lambda frame:
                            math.dist(tuple(map(float, frame[19].split(',')[:3])), arrival) < 12)
    path = []
    for _ in range(48):
        if sentinel_children(collected)[1].get(weapon, (0, 0, 0))[2] & (1 << 23):
            break
        # Snapshot spacing can put the first movement update beyond the
        # pickup. Steer back toward it once the teleport impulse wears off.
        dy = ry - float(collected[19].split(',')[1])
        forward = 50 if dy > 6 else -50 if dy < -6 else 0
        time.sleep(.025)
        sock.send(probe.packet(f'move {forward} 0 0 25'))
        collected = await_state(sock, probe, 'snapshot')
        path.append((collected[1], collected[29], collected[19]))
    else:
        raise AssertionError(f'{name} weapon was not collected, pickup {rx, ry, rz}, path {path[:12]}')
    rcon, directory = save
    assert f'Saved {name}_active revision 1: 1 objects' in rcon(f'jar_world_save {name}_active')
    state = decode(directory / f'{name}_active.0.jrw')
    timers, = state[name + 's']
    assert state['version'] == (21 if name == 'rift' else 22)
    # Weapon-only timers are five seconds. Walking over adjacent ammo or
    # armor during the same pass may legitimately start longer timers.
    assert sum(0 < timer <= 5000 for timer in timers) == 1, timers
    sock.send(probe.packet('setviewpos 0 0 32 0'))
    respawned = await_state(sock, probe, 'snapshot', lambda frame:
                            sentinel_children(frame)[1].get(weapon) == section['pickups'][weapon], timeout=8)
    elapsed = int(respawned[1]) - int(collected[1])
    assert 4900 <= elapsed <= 5150, elapsed
    section['interaction'] = {'repeater_respawn_ms' if name == 'rift' else 'disruptor_respawn_ms': elapsed}


def check_section_late_join_and_undo(owner, observer, section, section_save):
    _, sock, probe, _ = owner
    _, other_sock, other_probe, state = observer
    slots = section['resources']
    order = section['spawn_order']
    baseline = section['baseline_non_player_entities']
    assert state['initial_section_resources'] == slots, state
    observed = await_state(other_sock, other_probe, 'snapshot', lambda fields:
                           section_frame_matches(fields, slots, len(order), baseline))
    assert section_frame(observed) == tuple(section[k] for k in ('roots', 'speakers', 'torches', 'bobbers', 'brushes')), observed
    if section['scenario'] == 'sentinel':
        assert sentinel_children(observed) == (section['lifts'], section['pickups'])
        check_sentinel_interaction(sock, probe, section, section_save)
    if section['scenario'] in ('rift', 'korriban'):
        assert sentinel_children(observed) == ({}, section['pickups'])
        check_section_weapon(sock, probe, section, section_save)
    if section['scenario'] == 'shaft':
        assert sentinel_children(observed) == (section['lifts'], {})
        assert section_scenery(observed) == section['scenery']
        check_shaft_interaction(sock, probe, section, section_save)
    if section['scenario'] == 'yavin':
        assert section_scenery(observed) == section['scenery']
        dust = await_state(other_sock, other_probe, 'snapshot', lambda frame: frame[33] != '-', timeout=11)
        assert all(int(event.split(',')[1]) in section['roots'] for event in dust[33].split(';'))
        section['dust_snapshot_time'] = int(dust[1])
        rcon, _ = section_save
        assert 'Saved yavin_active revision 1: 1 objects' in rcon('jar_world_save yavin_active')
    section['late_join_snapshot_time'] = int(observed[1])
    section['late_join_gamestate_resources'] = state['initial_section_resources']
    undo = []
    for remaining in range(len(order) - 1, -1, -1):
        time.sleep(1.1)
        sock.send(probe.packet('undo'))
        removed = await_state(sock, probe, 'snapshot', lambda fields:
                              section_frame_matches(fields, slots, remaining, baseline))
        observed = await_state(other_sock, other_probe, 'snapshot', lambda fields: int(fields[1]) >= int(removed[1])
                               and section_frame_matches(fields, slots, remaining, baseline))
        roots, speakers, torches, bobbers, brushes = section_frame(removed)
        if section['scenario'] in ('sentinel', 'rift', 'shaft', 'korriban'):
            assert sentinel_children(observed) == sentinel_children(removed) == ({}, {})
        if section['scenario'] in ('shaft', 'yavin'):
            assert section_scenery(observed) == section_scenery(removed) == {}
        assert section_frame(observed) == (roots, speakers, torches, bobbers, brushes)
        assert roots == {entity: section['roots'][entity] for entity in order[:remaining]}, roots
        assert speakers == {entity: speaker for entity, speaker in section['speakers'].items()
                            if speaker['owner'] in roots}, speakers
        assert torches == {entity: torch for entity, torch in section['torches'].items()
                           if torch['owner'] in roots}, torches
        assert bobbers == {entity: bobber for entity, bobber in section['bobbers'].items()
                           if bobber['owner'] in roots}, bobbers
        assert brushes == {entity: brush for entity, brush in section['brushes'].items()
                           if brush['owner'] in roots}, brushes
        undo.append(dict(roots_remaining=list(roots), speakers_remaining=list(speakers),
                         torches_remaining=list(torches), bobbers_remaining=list(bobbers),
                         brushes_remaining=list(brushes), owner_snapshot_time=int(removed[1]), observer_snapshot_time=int(observed[1]),
                         retained_resources=bsp_slots(observed[14])))
    section['undo'] = undo
    if section['scenario'] == 'hazard_brushes':
        section['hidden_hazard_brushes_excluded'] = True


def open_wire_client(stack, port, executable, run, protocol, name, rejection_checks=False,
                     map_name='mp/ffa3', appearance=None):
    sock = stack.enter_context(socket.socket(socket.AF_INET, socket.SOCK_DGRAM))
    sock.bind(('127.0.0.1', 0))
    sock.connect(('127.0.0.1', port))
    sock.settimeout(.25)
    log = stack.enter_context((run / f'{name}-wire.log').open('wb'))
    probe = WireProbe(executable, log)
    stack.callback(probe.close)
    nonce = secrets.randbelow(2**31 - 1) + 1
    sock.send(OOB + f'getchallenge {nonce}'.encode('ascii'))

    def challenge_reply(packet):
        match = re.fullmatch(OOB + rb'challengeResponse (-?\d+) (\d+)\x00?', packet)
        return match if match and int(match[2]) == nonce else None

    challenge = int(receive(sock, challenge_reply)[1])
    qport = sock.getsockname()[1]
    if rejection_checks:
        # Use the same compressed lowercase connect wire format as accepted clients.
        sock.send(probe.packet(f'connect {protocol - 1} {challenge} {qport} {name}'))
        mismatch = f'Server uses protocol version {protocol} (yours is {protocol - 1})'.encode('ascii')
        receive(sock, lambda packet: packet if packet.startswith(OOB) and mismatch in packet else None)
        sock.send(probe.packet(f'connect {protocol} {challenge ^ 1} {qport} {name}'))
        receive(sock, lambda packet: packet if packet.startswith(OOB) and b'Incorrect challenge' in packet else None)

    options = ' ' + ' '.join(appearance) if appearance else ''
    sock.send(probe.packet(f'connect {protocol} {challenge} {qport} {name}{options}'))
    response = receive(sock, lambda packet: packet if packet.startswith(OOB) else None)
    assert response.rstrip(b'\x00') == OOB + b'connectResponse', response
    sock.send(probe.packet('prime'))
    _, mapname, gamedir, slot, baselines, initial_music, initial_section, *room_light = await_state(sock, probe, 'gamestate')
    assert (mapname, gamedir) == (map_name, 'JediAcademyRemixedMP'), (mapname, gamedir)
    # Flatgrass starts with BSP scenery and spawn points only.
    if map_name not in (SECTION_MAP, 'gm_flatgrass'):
        assert int(baselines) > 0
    state = dict(slot=int(slot), baselines=int(baselines), initial_music=wire_value(initial_music),
                 initial_section_resources=bsp_slots(initial_section))
    if room_light:
        state['initial_room_light'] = list(map(wire_value, room_light))
    return sock, probe, state


class LocalPureArchives:
    def __init__(self, game_data):
        self.archives = {}
        for folder in ('base', 'JediAcademyRemixedMP'):
            for path in (game_data / folder).glob('*.pk3'):
                stem = path.stem.lower()
                assert stem not in self.archives, f'Duplicate archive basename: {stem}'
                with zipfile.ZipFile(path) as archive:
                    members = archive.infolist()
                    files = {item.filename.lower() for item in members}
                    # FS_LoadZipFile hashes nonempty entries' CRCs in central-directory order.
                    checksums = b''.join(struct.pack('<I', item.CRC) for item in members if item.file_size)
                    self.archives[stem] = (folder, files, checksums)

    def proof(self, probe, map_name, section_resources=(), wrong=False):
        _, pure, feed, names, sums = probe.request('pureinfo').split()
        assert pure == '1', 'Dedicated server did not advertise sv_pure 1'
        feed = int(feed)
        names = wire_value(names).lower().split()
        advertised = [int(value) & 0xffffffff for value in wire_value(sums).split()]
        assert len(names) == len(advertised) == len(self.archives) and set(names) == set(self.archives), names
        ordinary = [probe.checksum(self.archives[name][2]) for name in names]
        assert ordinary == advertised, 'The local unseeded archive checksums differ from the server allowlist'
        seeded = {name: probe.checksum(struct.pack('<I', feed) + self.archives[name][2]) for name in names}

        def containing(path):
            return next(name for name in names if path in self.archives[name][1])

        def qualified(name):
            return f'{self.archives[name][0]}/{name}'

        # Match the archive references a native load makes in this clean layout.
        cgame, ui = containing('cgamex86.dll'), containing('uix86.dll')
        map_pack = containing(f'maps/{map_name}.bsp')
        section_packs = {containing(f'maps/{model[1:]}.bsp') for model in section_resources}
        references = [name for name in names
                      if name == map_pack or name in section_packs or self.archives[name][0] != 'base']
        checksums = [seeded[name] for name in references]
        if wrong:
            # Keep the count and XOR valid while replacing one known archive checksum.
            checksums[0] = next(checksums[0] ^ (1 << bit) for bit in range(32)
                                if (checksums[0] ^ (1 << bit)) not in seeded.values())
        combined = feed ^ len(checksums)
        for checksum in checksums:
            combined ^= checksum

        def signed(value):
            return value if value < 2**31 else value - 2**32

        proof = f'{signed(seeded[cgame])} {signed(seeded[ui])} @ ' + ' '.join(
            str(signed(value)) for value in (*checksums, combined))
        return proof, dict(checksum_feed=feed, ordinary_archive_count=len(ordinary),
                           cgame_archive=qualified(cgame), ui_archive=qualified(ui), map_archive=qualified(map_pack),
                           section_archives=[qualified(name) for name in names if name in section_packs],
                           general_archives=[qualified(name) for name in references])


def check_network_clients(port, executable, run, protocol, light_ids=None, music_id=None, pure_game_data=None,
                          section_ids=None, *, map_name='mp/ffa3', physgun_id=None, section_save=None,
                          movement_markers=False, directional_trigger=False, searchlight=False, portals=False, angular=False,
                          console_npcs=False, space_volume=False):
    assert Path(executable).is_file(), f'Missing native wire probe: {executable}'
    clients = []
    lights = {}
    music = {}
    section = {}
    furniture = {}
    ladder = {}
    room_switch = {}
    room_mirror = {}
    extra_clients = []
    first_light = next(iter(light_ids.items()), None) if light_ids else None
    local_paks = LocalPureArchives(pure_game_data) if pure_game_data else None
    pure_evidence = {}
    client_proofs = {}
    if local_paks:
        with ExitStack() as rejected:
            sock, probe, _ = open_wire_client(rejected, port, executable, run, protocol, 'JARWireRejected',
                                              map_name=map_name)
            proof, pure_evidence = local_paks.proof(probe, map_name, wrong=True)
            sock.send(probe.packet('pure ' + proof))
            reason = ' '.join(await_state(sock, probe, 'disconnect')[1:]).strip('"')
            assert reason == 'Unpure client detected. Invalid .PK3 files referenced!', reason
            pure_evidence['wrong_proof_rejected'] = reason

    with ExitStack() as stack:
        for i, name in enumerate(('JARWireA', 'JARWireB')):
            sock, probe, state = open_wire_client(stack, port, executable, run, protocol, name,
                                                  rejection_checks=(i == 0), map_name=map_name)
            if local_paks:
                proof, verification = local_paks.proof(probe, map_name, state['initial_section_resources'].values())
                assert verification['checksum_feed'] == pure_evidence['checksum_feed']
                client_proofs[name] = verification
                sock.send(probe.packet('pure ' + proof))
            sock.send(probe.packet('enter'))
            _, server_time, player, entities, health, _, _, following, *_ = await_state(sock, probe, 'snapshot')
            assert state['slot'] == int(player) and int(health) > 0
            if not section_ids and map_name != 'gm_flatgrass':
                assert int(entities) > 0
            assert following == '0'
            state.update(snapshot_time=int(server_time), snapshot_entities=int(entities), health=int(health))
            clients.append((name, sock, probe, state))
            if i == 0 and section_ids:
                section = spawn_sections(clients[0], section_ids)
            if i == 0 and music_id:
                music = activate_music(clients[0], music_id)
                if first_light:
                    time.sleep(1.1)
            if i == 0 and first_light:
                kind, ident = first_light
                lights[kind] = spawn_light(clients[0], kind, ident)

        assert clients[0][3]['slot'] != clients[1][3]['slot']
        if movement_markers:
            result = check_movement_markers(clients)
            (run / 'movement-markers.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
        if space_volume:
            result = check_space_volume(clients)
            (run / 'space-volume.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
        if directional_trigger:
            result = check_directional_trigger(clients)
            (run / 'directional-trigger.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
        if searchlight:
            result = check_searchlight(clients)
            (run / 'searchlight.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
        if portals:
            result = check_portals(clients)
            (run / 'portals.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
        if angular:
            result = check_angular_movers(clients)
            (run / 'angular-movers.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
        if console_npcs:
            result = check_console_npcs(clients, section_save)
            (run / 'console-npcs.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
        if map_name == 'gm_construct':
            furniture = check_construct_furniture(clients[0], clients[1], physgun_id)
            ladder = check_construct_ladder(clients[0], clients[1])

            def join_construct_late():
                name = 'JARWireLate'
                sock, probe, state = open_wire_client(stack, port, executable, run, protocol, name, map_name=map_name)
                if local_paks:
                    proof, verification = local_paks.proof(probe, map_name)
                    assert verification['checksum_feed'] == pure_evidence['checksum_feed']
                    client_proofs[name] = verification
                    sock.send(probe.packet('pure ' + proof))
                sock.send(probe.packet('enter'))
                entered = await_state(sock, probe, 'snapshot')
                assert int(entered[2]) == state['slot'] and int(entered[4]) > 0
                client = (name, sock, probe, state)
                extra_clients.append(client)
                return client

            room_switch = check_construct_switch(clients[0], clients[1], join_construct_late)
            room_mirror = check_construct_mirror(clients[0], clients[1])
        if section:
            check_section_late_join_and_undo(clients[0], clients[1], section, section_save)
        if music:
            assert clients[1][3]['initial_music'] == music['activated_cue'], clients[1][3]
            music['late_join_initial_cue'] = clients[1][3]['initial_music']
        if first_light:
            kind, _ = first_light
            light = lights[kind]
            assert light['spawn_snapshot_time'] <= clients[1][3]['snapshot_time']
            _, sock, probe, _ = clients[1]
            slot = clients[0][3]['slot']
            # Follow through the real spectator command so both snapshots use
            # the same PVS even when the second player spawns on another floor.
            sock.send(probe.packet(f'follow {slot}'))
            observed = await_state(sock, probe, 'snapshot', lambda fields:
                                   fields[7] == '1' and int(fields[2]) == slot
                                   and snapshot_lights(fields) == [light['entity']])
            light['late_join_observer_slot'] = clients[1][3]['slot']
            light['late_join_snapshot_time'] = int(observed[1])
            undo_light(clients[0], light, clients[1])
            for kind, ident in list(light_ids.items())[1:]:
                time.sleep(1.1)
                lights[kind] = spawn_light(clients[0], kind, ident)
                undo_light(clients[0], lights[kind])
        if music:
            undo_music(clients[0], clients[1], music)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as status:
            status.connect(('127.0.0.1', port))
            status.settimeout(.25)
            def server_status(nonce, predicate):
                deadline = time.monotonic() + 6
                query = OOB + b'getstatus ' + nonce
                while time.monotonic() < deadline:
                    status.send(query)
                    try:
                        return receive(status, lambda packet: packet if packet.startswith(OOB + b'statusResponse')
                                       and nonce in packet and predicate(packet) else None, timeout=.5)
                    except WireTimeout:
                        pass
                raise WireTimeout('Timed out waiting for the expected client list')

            connected = clients + extra_clients
            both = server_status(b'jar-wire-clients', lambda packet:
                                 all(name.encode('ascii') in packet for name, *_ in connected))
            assert all(name.encode('ascii') in both for name, *_ in connected), both
            assert b'JARWireRejected' not in both, both
            for _, sock, probe, _ in connected:
                sock.send(probe.packet('disconnect'))
            server_status(b'jar-wire-closed', lambda packet:
                          all(name.encode('ascii') not in packet for name, *_ in connected))

    evidence = dict(loopback_only=True, pure_server=bool(local_paks), graphical_clients=0,
                    native_codecs=True, native_client_executable=False, protocol=protocol,
                    wrong_protocol_rejected=True, wrong_challenge_rejected=True,
                    reliable_disconnect=True, clients={name: values for name, _, _, values in clients},
                    portable_lights=lights, portable_music=music)
    if section:
        evidence['portable_section'] = section
    if furniture:
        evidence['construct_furniture'] = furniture
        evidence['construct_ladder'] = ladder
        evidence['construct_room_switch'] = room_switch
        evidence['construct_room_mirror'] = room_mirror
    if local_paks:
        pure_evidence['authenticated_active_clients'] = [name for name, *_ in clients]
        pure_evidence['client_proofs'] = client_proofs
        if extra_clients:
            pure_evidence['authenticated_late_clients'] = [name for name, *_ in extra_clients]
        if section:
            assert pure_evidence['map_archive'] == 'JediAcademyRemixedMP/jar-wire-section'
            assert not client_proofs['JARWireA']['section_archives']
            assert client_proofs['JARWireB']['section_archives'] == ['base/assets0']
        evidence['pure_proof'] = pure_evidence
    (run / 'network-client-result.json').write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print(f'Two UDP clients passed compressed connect, native gamestate, active snapshots and disconnect: {run}')
    if first_light:
        print('Late-joining spectator received the existing light and both clients saw it removed after Undo.')
    if music:
        print('Late join received the active portable music cue and both clients received the map cue after Undo.')
    if local_paks:
        print('Pure dedicated server rejected the invalid archive proof and accepted both independently computed proofs.')
    if section:
        print('The late joiner received all BSP resources and their replicated children. Undo retained the resource slots.')
