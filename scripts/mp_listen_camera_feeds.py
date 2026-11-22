"""Exercise camera slot allocation through real MP snapshots and rendered screens."""
import json
import re
import statistics
import struct
import time

from mp_listen_platforms import place, select_from_above
from mp_listen_portals import CAMERA, SURFACE
from mp_world_smoke import checksum


# Left to right, top row then bottom row. The last screen uses the first lens.
LENSES = (0, 1, 2, 3, 4, 0)
ALL = (1, 1, 1, 1, 1, 1)
FOUR = (1, 1, 1, 1, 0, 0)
ONE = (1, 0, 0, 0, 0, 0)
OFF = (0,) * 6
CASES = {
    'first': (ALL, (1, 1, 0, 0, 0)),
    'second': (ALL, (1, 0, 1, 0, 1)),
    'third': (ALL, (0, 1, 1, 1, 0)),
    'promoted': ((1, 1, 0, 1, 1, 1), (0, 1, 1, 1, 0)),
    'restored': (ALL, (0, 1, 1, 1, 0)),
    'off': (OFF, (0,) * 5),
    'one': (ONE, (1,) * 5),
    'four': (FOUR, (1, 1, 0, 0, 0)),
}


def feed_world(seed, enabled, sky):
    assert seed[:4] == b'JRWI' and struct.unpack_from('<II', seed, 12) == (2, 0)
    stride = 284
    assert len(seed) == 84 + 2 * stride + 4
    originals = [seed[84+i*stride:84+(i+1)*stride] for i in range(2)]
    ids = [struct.unpack_from('<I', node)[0] for node in originals]
    panel, lens = originals[ids.index(SURFACE)], originals[ids.index(CAMERA)]
    assert struct.unpack_from('<ii', panel, 156) == (ids.index(CAMERA) + 1, 0)
    data = bytearray(seed[:84])
    struct.pack_into('<I', data, 12, len(enabled) + len(sky))
    for i, camera in enumerate(LENSES):
        node = bytearray(panel)
        width, height = (96, 60) if i < 4 else (48, 30)
        struct.pack_into('<6f', node, 28, 0, 120 - 120*(i % 3), 140 if i < 3 else 52, 0, 180, 0)
        struct.pack_into('<6f', node, 52, -2, -width/2, -height/2, 2, width/2, height/2)
        struct.pack_into('<i', node, 156, len(enabled) + camera + 1)
        struct.pack_into('<ii', node, 260, width, enabled[i])
        struct.pack_into('<i', node, 276, height)
        data.extend(node)
    for i, point_at_sky in enumerate(sky):
        node = bytearray(lens)
        struct.pack_into('<6f', node, 28, 1400+200*i, 800, 400, -70 if point_at_sky else 70, 90, 0)
        data.extend(node)
    return data + struct.pack('<I', checksum(data))


def open_view(peer, y):
    peer.ready()
    peer.do('ui_openmenu jarMPPause', 'ui_openmenu jar_probe_empty', 'togglemenu',
            'cmd jar_equip 73557060', 'cg_thirdPerson 0', 'con_notifytime 0')
    place(peer, -360, y, 48, 0)


def exercise_feeds(mod, profile, run, stack, player, free_port):
    port = free_port()
    host = player('Host', mod, profile / 'host', run, port, 'devmap gm_flatgrass')
    stack.callback(host.close)
    open_view(host, -24)
    host.do('+lookdown', 'wait 18', '-lookdown', 'cmd jar_spawn 88142876')
    time.sleep(.8)
    host.do('jar_world_save seed')
    worlds = host.profile / 'jar/worlds'
    seed = (worlds / 'seed.0.jrw').read_bytes()
    place(host, -360, -24, 48, 0)
    host.do('jar_worldEditing 1')

    def load(label, case):
        (worlds / f'{label}.0.jrw').write_bytes(feed_world(seed, *case))
        host.do('jar_world_clear')
        host.do('jar_world_load ' + label)
        host.wait(f'Loaded {label} revision 1: 11 objects')
        time.sleep(.65)

    load('first', CASES['first'])
    host.screenshot('beforejoin')
    guest = player('Late', mod, profile / 'guest', run, free_port(), f'connect 127.0.0.1:{port}',
                   renderer='rdjar-mp-rend2')
    stack.callback(guest.close)
    open_view(guest, 24)
    peers = (host, guest)
    for label, case in CASES.items():
        if label in ('promoted', 'restored'):
            # Use the player's ordinary Context action on the top-right panel
            # without recreating any entities or changing either player's view.
            token = select_from_above(host, 0, -120, 140, SURFACE)
            log_start = len(host.text())
            host.do('wait 18', 'cmd jar_property controller_portal_power ' + token,
                    'wait 18', 'cmd jar_select close', 'developer 0')
            assert 'Controller properties applied.' in host.text()[log_start:], label
            time.sleep(.65)
        elif label != 'first':
            load(label, case)
        for peer in peers:
            peer.screenshot(label)

    timings = {p.name: {} for p in peers}
    counter_starts = {p.name: {} for p in peers}
    for trial, order in enumerate((('off', 'one', 'four'), ('four', 'one', 'off'))):
        for label in order:
            tag = f'bench{trial}{label}'
            load(tag, CASES[label])
            started = {}
            for peer in peers:
                started[peer.name] = time.monotonic()
                peer.do('com_speeds 1', 'echo BEGIN_' + tag)
            time.sleep(4)
            for peer in peers:
                peer.do('echo END_' + tag, 'com_speeds 0')
                window = peer.text().split('BEGIN_' + tag)[-1].split('END_' + tag)[0]
                elapsed = (time.monotonic() - started[peer.name]) * 1000
                rows = re.findall(r'frame:\d+ all:\s*(\d+) sv:\s*(-?\d+) ev:\s*(\d+)', window)
                times = [int(work) for work, _, _ in rows]
                assert len(times) >= 45, (peer.name, tag, len(times))
                # A frame cannot take longer than the interval containing it.
                # Cover both event/command passes that can turn on the counters.
                peak = max(int(value) for row in rows for value in row)
                assert peak <= elapsed + 100, (peer.name, tag, peak, elapsed)
                counter_starts[peer.name][tag] = dict(first_ms=list(map(int, rows[0])),
                                                       peak_ms=peak, measurement_window_ms=round(elapsed))
                timings[peer.name].setdefault(label, []).append(times)
    (run / 'frame-work-ms.json').write_text(json.dumps(timings) + '\n')
    return dict(distinct_lenses=5, screens=6, cases=CASES, frame_cap=60, minimum_sample_seconds=4,
                counter_starts=counter_starts, counter_columns=['all', 'sv', 'ev'])


def panel_boxes(image):
    """Find the six powered-off rectangular panels in the middle of the image."""
    import numpy as np
    from PIL import Image, ImageDraw

    rgb = np.asarray(image)
    h, w = rgb.shape[:2]
    dark = (rgb.max(axis=2) == 0)
    dark[:h//6] = dark[3*h//4:] = False
    dark[:, :w//8] = dark[:, 7*w//8:] = False
    mask = Image.fromarray(dark.astype('uint8')).copy()
    boxes = []
    while (remaining := mask.getbbox()):
        x0, y0, x1, y1 = remaining
        row = np.asarray(mask.crop((x0, y0, x1, y0+1)))[0]
        point = (x0 + int(np.flatnonzero(row)[0]), y0)
        ImageDraw.floodfill(mask, point, 2)
        region = mask.point(lambda p: 255 if p == 2 else 0)
        x, y, right, bottom = region.getbbox()
        mask.paste(0, (0, 0), region)
        if right-x >= 20 and bottom-y >= 12 and region.histogram()[255] >= (right-x)*(bottom-y)*.9:
            boxes.append((x, y, right, bottom))
    assert len(boxes) == 6, boxes
    boxes.sort(key=lambda b: (b[1]+b[3])/2)
    return sorted(boxes[:3]) + sorted(boxes[3:])


def compare_feeds(reader, run):
    import numpy as np
    from PIL import Image

    results = {}
    times = json.loads((run / 'frame-work-ms.json').read_text())
    for viewer in ('Host', 'Late'):
        with Image.open(run / f'{viewer}-off.png') as image:
            boxes = panel_boxes(image.convert('RGB'))
        phases = {}
        checks = dict(CASES)
        if viewer == 'Host':
            checks['beforejoin'] = checks['first']
        for label, (enabled, sky) in checks.items():
            with Image.open(run / f'{viewer}-{label}.png') as image:
                rgb = np.asarray(image.convert('RGB'), dtype=np.int16)
            phases[label] = []
            for i, (x, y, right, bottom) in enumerate(boxes):
                dx, dy = (right-x)//5, (bottom-y)//5
                px = rgb[y+dy:bottom-dy, x+dx:right-dx]
                r, g, b = px.transpose(2, 0, 1)
                if not enabled[i] or (i == 4 and all(enabled[:4])):
                    expected, match = 'off', px.max(axis=2) == 0
                elif sky[LENSES[i]]:
                    expected, match = 'sky', (b > g+12) & (g > r+12)
                else:
                    expected, match = 'grass', (g > b+6) & (r > b+3)
                fraction = float(match.mean())
                assert fraction > .98, (viewer, label, i+1, expected, fraction, boxes)
                phases[label].append(dict(expected=expected, fraction=fraction))
        perf = {}
        for label, trials in times[viewer].items():
            samples = [ms for group in trials for ms in group]
            perf[label] = dict(samples=len(samples), mean_ms=statistics.mean(samples),
                               p95_ms=float(np.percentile(samples, 95)),
                               trial_mean_ms=[statistics.mean(group) for group in trials])
        results[viewer] = dict(panel_boxes=boxes, pictures=phases, engine_work_per_frame=perf)
    return results
