"""Graphical support checks for an unfrozen prop, bobbing platform and lift."""
import json
import re
import time

from mp_world_smoke import decode


def where(client):
    offset = len(client.text())
    client.do('where')
    until = time.monotonic() + 3
    while time.monotonic() < until:
        match = re.search(r'\((-?\d+) (-?\d+) (-?\d+)\)\s*$', client.text()[offset:], re.M)
        if match:
            return tuple(int(v) for v in match.groups())
        time.sleep(.05)
    raise AssertionError(f'{client.name}: no server position')


def place(client, x, y, z, yaw):
    # setviewpos adds a forward impulse. Position the test player in noclip,
    # measure the stopping point, and remove noclip before the actual ride.
    client.do('noclip')
    dx = dy = 0
    for _ in range(4):
        client.do(f'setviewpos {x+dx:.2f} {y+dy:.2f} {z:.2f} {yaw}')
        time.sleep(1)
        actual_x, actual_y, _ = where(client)
        if abs(actual_x-x) <= 3 and abs(actual_y-y) <= 3:
            break
        dx += x-actual_x
        dy += y-actual_y
    else:
        raise AssertionError(f'{client.name}: could not place player at ({x}, {y}); reached ({actual_x}, {actual_y})')
    client.do('noclip')


def camera(client):
    offset = len(client.text())
    client.do('viewpos')
    match = re.search(r'(?:maps/)?gm_flatgrass(?:\.bsp)? \((-?\d+) (-?\d+) (-?\d+)\)', client.text()[offset:], re.I)
    if not match:
        raise AssertionError(f'No rendered camera position from {client.name}: {client.text()[offset:][-300:]}')
    return int(match[3])


def select_from_above(host, x, y, height, catalog_id):
    host.do('developer 1')
    start = len(host.text())
    host.do(f'cmd jar_select ray_{x:.2f}_{y:.2f}_{height+72:.2f}_0_0_-1')
    until = time.monotonic() + 3
    while time.monotonic() < until:
        for row in re.findall(r'serverCommand: \d+ : jarselection ([^\r\n]+)', host.text()[start:]):
            fields = row.split()
            if len(fields) >= 6 and fields[5] == str(catalog_id):
                return fields[3]
        time.sleep(.05)
    raise AssertionError(f'{host.name} could not select catalog entry {catalog_id}')


def listen_players(mod, profile, run, stack, player, free_port):
    port = free_port()
    host = player('Host', mod, profile / 'host', run, port, 'devmap gm_flatgrass')
    stack.callback(host.close)
    host.ready()
    guest = player('Late', mod, profile / 'guest', run, free_port(), f'connect 127.0.0.1:{port}')
    stack.callback(guest.close)
    guest.ready()
    for client in (host, guest):
        client.do('ui_openmenu jarMPPause', 'ui_openmenu jar_probe_empty', 'togglemenu',
                  'cmd jar_equip 73557060', 'cg_thirdPerson 0', 'cg_draw2D 0', 'con_notifytime 0')

    return host, guest


def exercise_vehicle_lift(mod, profile, run, stack, player, free_port):
    host, guest = listen_players(mod, profile, run, stack, player, free_port)
    worlds = host.profile / 'jar/worlds'
    place(guest, -500, 350, 48, 0)
    place(host, -300, 0, 48, 0)
    host.do('+lookdown', 'wait 18', '-lookdown', 'cmd jar_spawn 93973657')
    time.sleep(1)
    host.do('jar_world_save deck')
    deck, = decode(worlds / 'deck.0.jrw')['nodes']
    assert deck[0] == 93973657
    x, y, z = deck[7:10]
    token = select_from_above(host, x, y, z + deck[18], deck[0])
    # The same aim point places the stock Swoop directly over the grate.
    host.do('cmd jar_spawn 60671213')
    time.sleep(2)
    host.do('jar_world_save parked')
    nodes = decode(worlds / 'parked.0.jrw')['nodes']
    swoop, = [node for node in nodes if node[0] == 60671213]
    vx, vy, vz = swoop[7:10]
    place(host, x, y-350, 48, 90)
    for client in (host, guest):
        client.record('swoop')
    # Landing on a stock Swoop automatically boards it. USE then dismounts.
    place(guest, vx, vy, vz+swoop[18]+60, 0)
    time.sleep(2)
    host.do('jar_undo')
    host.wait('Dismount or leave the supported section before Undo or cleanup. No objects removed.')
    for client in (host, guest):
        client.do('screenshot_png boarded')
    heights = []
    for i in range(16):
        if i == 2:
            host.do('cmd jar_property activate ' + token)
        heights.append(camera(guest))
        if i % 2 == 0:
            for client in (host, guest):
                client.do(f'screenshot_png ride{i}')
        time.sleep(.20)
    guest.do('+use', 'wait 4', '-use')
    time.sleep(1.5)
    host.do('jar_undo')
    host.wait('Undo: last spawned object or assembly removed.')
    time.sleep(1)
    for client in (host, guest):
        client.do('stoprecord')
    return dict(camera_z=heights, deck_origin=deck[7:10], parked_origin=swoop[7:10])


def exercise_rides(mod, profile, run, stack, player, free_port):
    host, guest = listen_players(mod, profile, run, stack, player, free_port)
    observed = {}
    worlds = host.profile / 'jar/worlds'
    for name, item in (('crate', 1001), ('bob', 98040309), ('lift', 93973657)):
        guest.do('setviewpos -500 350 48 0')
        place(host, -300, 0, 48, 0)
        host.do('+lookdown', 'wait 18', '-lookdown', f'cmd jar_spawn {item}')
        time.sleep(1.5)
        host.do(f'jar_world_save {name}')
        node, = decode(worlds / f'{name}.0.jrw')['nodes']
        assert node[0] == item
        if name == 'crate':
            assert not node[1] & 1, 'Test needs a simulated, unfrozen prop'
            host.do('jar_physics_status')
            assert 'Box3D active' in host.text()
        x, y, z = node[7:10]
        top = z + node[18]
        place(host, x-170, y, 44, 0)
        token = select_from_above(host, x, y, top, item) if name == 'lift' else None
        place(guest, x, y, top+60, 90)
        time.sleep(1.5)
        for client in (host, guest):
            client.record(name)
        heights = []
        for i in range(16):
            if name == 'lift' and i == 2:
                host.do('cmd jar_property activate ' + token)
            heights.append(camera(guest))
            if name != 'crate' and i % 2 == 0:
                host.do(f'screenshot_png {name}{i}')
            time.sleep(.28)
        if name == 'crate':
            host.do('screenshot_png crate_rider')
        guest.do('+moveup', 'wait 8', '-moveup')
        time.sleep(2.5)
        for client in (host, guest):
            client.do('stoprecord')
        host.do('jar_world_clear')
        if name != 'crate':
            time.sleep(1)
            host.do(f'screenshot_png {name}_empty')
        observed[name] = dict(catalog_id=item, camera_z=heights, placed_origin=[x,y,z])
    return observed


def compare_vehicle_lift(tool, run, read_demo):
    host, guest = [read_demo(tool, next(run.glob(f'{peer}-swoop.dm_*')), '--vehicle-demo')
                   for peer in ('Host', 'Late')]
    times = sorted(host.keys() & guest.keys())
    assert len(times) >= 100
    deck, = [n for n, row in host[times[0]].items() if row[0] == 'p']
    hull, = [n for n, row in host[times[0]].items() if row[0] == 'v']
    mounted = []
    removed = []
    max_peer_error = 0.
    for t in times:
        h, g = host[t], guest[t]
        assert h[deck] == g[deck], (t, h[deck], g[deck])
        observer, driver = h[1], g[1]
        assert observer[0] == 'r' and driver[0] == 's'
        assert observer[1] == driver[1], (t, observer, driver)
        assert (hull in h) == (hull in g)
        if hull not in h:
            removed.append(t)
        if driver[1] == hull:
            assert h[hull][1] == g[hull][1] == 2, (t, h[hull], g[hull])
            for local, remote in ((driver, observer), (g[hull], h[hull])):
                max_peer_error = max(max_peer_error, *(abs(a-b) for a,b in zip(local[2:], remote[2:])))
            mounted.append((t, h[deck][4], h[hull][4], g[hull][4], observer[4], driver[4]))
    assert mounted, 'The guest never boarded the Swoop'
    first, last = mounted[0][0], mounted[-1][0]
    assert times[0] < first < last < times[-1], 'Missing boarding or dismount'
    assert len(mounted) == sum(first <= t <= last for t in times), 'Guest lost the seat during the ride'
    assert max_peer_error <= 1.01, max_peer_error
    assert removed and removed[0] > last and removed == times[-len(removed):]
    points = list(zip(*mounted))
    spans = [max(axis) - min(axis) for axis in points[1:]]
    for axis, span in zip(points[1:], spans):
        assert 126 <= span <= 132 and abs(axis[-1]-axis[0]) < 3, (span, axis[0], axis[-1])
    hover_gap = [vehicle_z - deck_z for _, deck_z, _, vehicle_z, _, _ in mounted]
    assert max(hover_gap) - min(hover_gap) < 6, (min(hover_gap), max(hover_gap))
    heights = json.loads((run / 'observations.json').read_text())['camera_z']
    assert max(heights) - min(heights) >= 120, heights
    return dict(shared_snapshots=len(times), mounted_snapshots=len(mounted),
                seat_preserved_ms=last-first, boarded_and_dismounted=True,
                deck_travel=spans[0], host_vehicle_travel=spans[1], driver_vehicle_travel=spans[2],
                host_pilot_travel=spans[3], driver_pilot_travel=spans[4],
                driver_camera_travel=max(heights)-min(heights), maximum_peer_axis_error=max_peer_error,
                hover_gap_min=min(hover_gap), hover_gap_max=max(hover_gap),
                occupied_undo_refused=True, unoccupied_undo_seen_by_both=True,
                **projected_rider(run, 'ride'))


def projected_rider(run, name):
    from PIL import Image

    rows = {}
    for i in range(0, 16, 2):
        path = run / f'Host-{name}{i}.png'
        with Image.open(path) as source:
            picture = source.convert('RGB')
        width, height = picture.size
        # The stationary camera faces the rider against the bright Flatgrass
        # sky. Find the first foreground row in the central strip above the horizon.
        pixels = picture.load()
        for y in range(height // 12, height // 2):
            if sum(max(pixels[x, y]) < 125 for x in range(9*width//20, 11*width//20)) >= 4:
                rows[path.name] = y
                break
    assert len(rows) == 8, (name, rows)
    low = max(rows, key=rows.get)
    high = min(rows, key=rows.get)
    displacement = rows[low] - rows[high]
    assert displacement >= (35 if name == 'bob' else 100), (name, rows)
    return dict(low_image=low, high_image=high, foreground_vertical_pixels=displacement)


def compare_rides(tool, run, read_demo):
    observations = json.loads((run / 'observations.json').read_text())
    result = {}
    for name in ('crate', 'bob', 'lift'):
        host, guest = [read_demo(tool, next(run.glob(f'{peer}-{name}.dm_*')), '--rider-demo')
                       for peer in ('Host', 'Late')]
        ticks = sorted(host.keys() & guest.keys())
        assert len(ticks) >= 100, (name, len(ticks))
        deck, = [n for n, row in host[ticks[0]].items() if row[0] == 'p']
        grounded = []
        airborne = []
        for t in ticks:
            h, g = host[t], guest[t]
            assert h[deck] == g[deck], (t, h[deck], g[deck])
            local, remote = g[1], h[1]
            assert local[0] == 's' and remote[0] == 'r'
            assert local[1] == remote[1], (name, t, local, remote)
            # The ordinary player entity is snapped to integers on the wire.
            assert max(abs(a-b) for a,b in zip(local[2:], remote[2:])) <= 1.01
            if local[1] == deck:
                grounded.append((t, g[deck][4], local[4]))
            else:
                airborne.append((t, local[4]))
        assert grounded and airborne
        assert grounded[0][0] < airborne[0][0] < airborne[-1][0] < grounded[-1][0]
        assert airborne[0][0] - grounded[0][0] >= 4000
        takeoff = next(rider for tick, _, rider in reversed(grounded) if tick < airborne[0][0])
        assert max(z for _,z in airborne) > takeoff + 12
        separation = [rider-platform for _,platform,rider in grounded]
        travel = max(h for _,h,_ in grounded) - min(h for _,h,_ in grounded)
        assert max(separation) - min(separation) <= 2, (name, min(separation), max(separation))
        if name == 'bob':
            assert travel >= 22, travel
        elif name == 'lift':
            assert travel >= 127, travel
            assert abs(grounded[-1][1] - grounded[0][1]) < 1
        result[name] = dict(common_frames=len(ticks), grounded_frames=len(grounded),
                            supported_before_jump_ms=airborne[0][0]-grounded[0][0],
                            deck_vertical_travel=travel, min_rider_offset=min(separation),
                            max_rider_offset=max(separation), jumped_and_relanded=True)
        heights = observations[name]['camera_z']
        excursion = max(heights) - min(heights)
        if name == 'crate':
            assert excursion <= 2, heights
        else:
            assert excursion >= (19 if name == 'bob' else 120), heights
            result[name].update(projected_rider(run, name))
        result[name]['rendered_camera_vertical_travel'] = excursion
    return result
