"""Exercise a real Physgun release from both sides of a graphical listen server."""
import json
import math
import subprocess
from statistics import linear_regression
import time
from itertools import groupby

from mp_listen_platforms import listen_players, place, select_from_above, where
from mp_world_smoke import decode


def exercise_throws(mod, profile, run, stack, player, free_port):
    host, guest = listen_players(mod, profile, run, stack, player, free_port)
    for client in (host, guest):
        client.do('god', 'notarget', 'cl_pitchspeed 120', 'cl_yawspeed 90')
    worlds = host.profile / 'jar/worlds'

    def trooper(slot):
        host.do('jar_world_save ' + slot)
        node, = decode(worlds / (slot + '.0.jrw'))['nodes']
        assert node[0] == 3001 and node[4] > 0, node
        return node

    observations = {}
    for name, thrower, observer in (('hostthrow', host, guest), ('guestthrow', guest, host),
                                    ('pacedthrow', guest, host)):
        if name == 'pacedthrow':
            guest.do('cl_lanForcePackets 0', 'cl_maxpackets 20')
        place(observer, -600, -500, 48, 90)
        place(thrower, -300, 0, 48, 0)
        thrower.do('+lookdown', 'wait 18', '-lookdown', 'cmd jar_spawn 3001')
        time.sleep(2)
        npc = trooper(name + 'base')
        x, y, _ = npc[7:10]
        place(observer, x, y-550, 48, 90)
        place(thrower, x-204, y, 48, 0)
        time.sleep(1)
        for client in (host, guest):
            client.record(name)
        time.sleep(.4)
        # Separate a harmless stationary hold from fall damage on the throw.
        thrower.do('+attack')
        time.sleep(1.4)
        thrower.do('screenshot_png ' + name + '_held')
        thrower.do('-attack')
        time.sleep(.5)
        released = trooper(name + 'rest')
        assert released[4] == npc[4], (name, npc[4], released[4])
        # A hard landing can kill a stock 30-HP trooper. Give it enough
        # health for the post-landing combat check with fall damage enabled.
        token = select_from_above(thrower, *released[7:10], released[0])
        thrower.do('wait 15', 'cmd jar_property health_500 ' + token, 'developer 0')
        combatant = trooper(name + 'ready')
        assert combatant[4] >= 490, (name, combatant[4])
        thrower.do('+attack')
        time.sleep(.7)
        pictures = []
        for i in range(24):
            pictures.extend((f'screenshot_png {name}{i:02}', 'wait 6'))
        pending = observer.queue(*pictures)
        thrower.do('+lookup', '+left', 'wait 17', '-attack', '-lookup', '-left')
        observer.wait(pending)
        landed = trooper(name + 'down')
        for attempt in range(12):
            time.sleep(.35)
            previous = landed
            landed = trooper(f'{name}settle{attempt}')
            if math.dist(landed[7:10], previous[7:10]) < 1:
                break
        else:
            raise AssertionError(f'{name}: trooper did not settle at its landing position')
        # Put a target inside the trooper's view after landing so that travel
        # distance and the initial camera placement do not decide the AI check.
        yaw = math.radians(landed[11])
        place(observer, landed[7]+170*math.cos(yaw), landed[8]+170*math.sin(yaw),
              landed[9]+24, landed[11]+180)
        observer.do('notarget')
        time.sleep(7)
        for client in (host, guest):
            client.do('stoprecord')
        final = trooper(name + 'land')
        observer.do('notarget')
        host.do('jar_world_clear')
        time.sleep(.5)
        observations[name] = dict(thrower=thrower.name, observer=observer.name,
                                  initial_health=npc[4], stationary_release_health=released[4],
                                  combat_test_health=combatant[4], final_health=final[4],
                                  input_packet_limit=20 if name == 'pacedthrow' else None)
    guest.do('cl_lanForcePackets 1', 'cl_maxpackets 63')
    host.do('jar_playerTools 1')
    for client in (host, guest):
        client.do('god')
    for name, holder, target in (('hostgrip', host, guest), ('guestgrip', guest, host)):
        place(target, 0, 0, 48, 90)
        place(holder, -180, 0, 48, 0)
        for client in (host, guest):
            client.record(name)
        time.sleep(.5)
        holder.do('+attack')
        time.sleep(.5)
        holder.do('screenshot_png ' + name + '_held')
        target.do('+forward', 'wait 25', '-forward')
        time.sleep(.5)
        escaped = None
        if holder is guest:
            start = where(target)
            target.do('cmd jar_allowTools 0')
            target.do('+forward', 'wait 25', '-forward')
            escaped = math.dist(start, where(target))
            assert escaped > 40, 'Player opt-out did not release the target while the trigger was held'
        holder.do('-attack')
        time.sleep(.3)
        target.do('+forward', 'wait 25', '-forward')
        time.sleep(.5)
        for client in (host, guest):
            client.do('stoprecord')
        target.do('cmd jar_allowTools 1')
        observations[name] = dict(holder=holder.name, target=target.name,
                                  release='opt-out' if holder is guest else 'trigger',
                                  movement_before_trigger_release=escaped)
    return observations


def throw_track(tool, path):
    frames = {}
    output = subprocess.check_output([str(tool), '--throw-demo', str(path)], text=True)
    for line in output.splitlines():
        tick, slot, held, actor = line.split()
        number, grounded, posed, flags, x, y, z = actor.split(',')
        frames[int(tick)] = (int(held), int(number), bool(int(grounded)), bool(int(posed)),
                             int(flags), (float(x), float(y), float(z)))
    return frames


def player_grip(tool, run, name, case):
    def track(peer):
        path, = run.glob(f'{peer}-{name}.dm_*')
        rows = {}
        for line in subprocess.check_output([str(tool), '--player-grip-demo', str(path)], text=True).splitlines():
            tick, slot, held, health, frozen, *actors = line.split()
            poses = {}
            for actor in actors:
                number, posed, *position = actor.split(',')
                poses[int(number)] = (bool(int(posed)), tuple(map(float, position)))
            rows[int(tick)] = (int(slot), int(held), int(health), bool(int(frozen)), poses)
        return rows
    holder, target = track(case['holder']), track(case['target'])
    shared = sorted(holder.keys() & target.keys())
    victim = target[shared[0]][0]
    phases = [(held, list(ticks)) for held, ticks in groupby(shared, lambda t: holder[t][1] == victim)]
    assert [held for held, _ in phases] == [False, True, False], (name, phases)
    before, held, after = (ticks for _, ticks in phases)
    assert held[-1]-held[0] >= 1000
    health = target[before[-1]][2]
    origin = target[held[0]][4][victim][1]
    for t in held:
        posed, position = target[t][4][victim]
        remote_pose, remote_position = holder[t][4][victim]
        assert posed and remote_pose and target[t][3] and target[t][2] == health, (name, t, target[t])
        assert math.dist(position, origin) < 1 and math.dist(remote_position, position) < 2
    assert all(not target[t][3] for t in after)
    assert not holder[after[-1]][4][victim][0] and not target[after[-1]][4][victim][0]
    distance = math.dist(target[after[-1]][4][victim][1], origin)
    assert distance > 40, (name, distance)
    return dict(hold_ms=held[-1]-held[0], shared_hold_snapshots=len(held), health=health,
                release=case['release'], movement_after_release=distance)


def rendered_arc(run, observer, name):
    from PIL import Image
    tops = []
    for path in sorted(run.glob(f'{observer}-{name}[0-9][0-9].png')):
        with Image.open(path) as picture:
            width, height = picture.size
            # Only the stock trooper crosses the clear sky above the thrower.
            sky = picture.convert('RGB').crop((width//8, height//10, 7*width//8, 487*height//1000))
            pixels = sky.load()
            for y in range(sky.height):
                if sum(1 for x in range(sky.width) if max(pixels[x,y]) < 180 and
                       max(pixels[x,y])-min(pixels[x,y]) < 35) >= 3:
                    tops.append((path.name, y))
                    break
    assert len(tops) >= 6, (name, tops)
    apex = min(range(len(tops)), key=lambda i: tops[i][1])
    rise = tops[0][1]-tops[apex][1]
    descent = tops[-1][1]-tops[apex][1]
    assert rise > 15 and descent > 15, (name, tops)
    return dict(visible_frames=len(tops), ascent_pixels=rise, descent_pixels=descent,
                apex_image=tops[apex][0])


def compare_throws(tool, run):
    observations = json.loads((run / 'observations.json').read_text())
    results = {}
    for name, case in observations.items():
        if 'holder' in case:
            results[name] = player_grip(tool, run, name, case)
            continue
        owner, other = [throw_track(tool, next(run.glob(f'{peer}-{name}.dm_*')))
                        for peer in (case['thrower'], case['observer'])]
        shared = sorted(owner.keys() & other.keys())
        for t in shared:
            assert owner[t][1:] == other[t][1:], (name, t, owner[t], other[t])
            assert other[t][0] == -1, (name, t)
        holds = [(held, list(times)) for held, times in groupby(shared, key=lambda t: owner[t][0] >= 0)]
        assert [held for held, _ in holds] == [False, True, False, True, False], (name, holds)
        stationary, moving, released = holds[1][1], holds[3][1], holds[4][1]
        assert stationary[-1] - stationary[0] >= 1000
        assert all(owner[t][0] == owner[t][1] and owner[t][3] for t in stationary + moving)
        airborne = []
        for t in released:
            if owner[t][2]:
                break
            airborne.append(t)
        assert len(airborne) >= 12 and len(airborne) < len(released), (name, airborne)
        z = [owner[t][5][2] for t in airborne]
        apex = max(range(len(z)), key=z.__getitem__)
        assert 1 < apex < len(z)-2 and z[apex] > z[0]+8
        assert all(a <= b for a,b in zip(z[:apex], z[1:apex+1]))
        assert all(a >= b for a,b in zip(z[apex:], z[apex+1:]))
        landing = released[len(airborne)]
        distance = math.dist(owner[airborne[0]][5][:2], owner[landing][5][:2])
        assert distance > 50, (name, distance)
        times = [(t-airborne[0])*.001 for t in airborne]
        # Under default gravity, removing the quadratic term leaves three
        # straight lines. Check every received sample, including the guest's.
        errors = []
        slopes = []
        for axis in range(3):
            values = [owner[t][5][axis]+(400*s*s if axis == 2 else 0) for t,s in zip(airborne,times)]
            fit = linear_regression(times, values)
            slopes.append(fit.slope)
            errors.append(max(abs(value-(fit.intercept+fit.slope*s)) for s,value in zip(times,values)))
        start = next(i for i,t in enumerate(moving) if math.dist(owner[t][5],owner[moving[0]][5]) > 2)
        first, last = moving[start-1], moving[-1]
        swing = [(owner[last][5][axis]-owner[first][5][axis])*1000/(last-first) for axis in (0,1)]
        # The scripted swing continues until trigger release. A noticeably
        # weaker launch can still trace a mathematically perfect parabola.
        retained = sum(a*b for a,b in zip(swing,slopes))/sum(a*a for a in swing)
        assert .5 < retained < 2, (name, swing, slopes, retained)
        gap = max(b-a for a,b in zip(airborne,airborne[1:]))
        assert max(errors) < 2 and gap <= 150, (name, errors, gap)
        assert any(owner[t][4] & (1 << 9) for t in released if t > landing), 'NPC did not resume fire'
        results[name] = dict(shared_snapshots=len(shared), stationary_hold_ms=stationary[-1]-stationary[0],
                             flight_ms=landing-airborne[0], airborne_snapshots=len(airborne),
                             apex_above_release=z[apex]-z[0], horizontal_travel=distance,
                             max_flight_gap_ms=gap, max_ballistic_fit_error=max(errors),
                             swing_horizontal_speed=math.hypot(*swing), flight_horizontal_speed=math.hypot(*slopes[:2]),
                             velocity_fraction_along_swing=retained,
                             rendered=rendered_arc(run, case['observer'], name),
                             fired_after_landing=True, unchanged_health_during_stationary_hold=True)
    return results
