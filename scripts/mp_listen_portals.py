"""Read the pixels of a live multiplayer portal on both supported renderers."""
import json
from itertools import product
import struct
import time

from mp_listen_platforms import place
from mp_world_smoke import checksum, decode


SURFACE = 88142876
CAMERA = 97890286


def camera_world(seed, width, height, pitch, active, second_camera=False):
    # Start with the real spawned pair, including its output link. Only the
    # pose and screen settings change. The camera is outside both eye views.
    data = bytearray(seed)
    assert data[:4] == b'JRWI' and struct.unpack_from('<II', data, 12) == (2, 0)
    stride = 284
    assert len(data) == 84 + 2 * stride + 4
    for start in (84, 84 + stride):
        catalog, = struct.unpack_from('<I', data, start)
        if catalog == SURFACE:
            struct.pack_into('<6f', data, start + 28, 0, 0, 100, 0, 180, 0)
            struct.pack_into('<i', data, start + 260, width)
            struct.pack_into('<i', data, start + 264, active)
            struct.pack_into('<i', data, start + 276, height)
            struct.pack_into('<6f', data, start + 52, -2, -width/2, -height/2, 2, width/2, height/2)
        else:
            assert catalog == CAMERA
            struct.pack_into('<6f', data, start + 28, 1400, 800, 400, pitch, 90, 0)
            housing = data[start:start + stride]
    if second_camera:
        # A second physical camera in the sky-facing lens's line of sight
        # must remain visible. Both player eyes are over 1,500 units away.
        struct.pack_into('<6f', housing, 28, 1400, 822, 460, 0, 0, 0)
        struct.pack_into('<I', data, 12, 3)
        data[-4:-4] = housing
    struct.pack_into('<I', data, len(data) - 4, checksum(data[:-4]))
    return data


def exercise_portals(mod, profile, run, stack, player, free_port):
    port = free_port()
    host = player('Host', mod, profile / 'host', run, port, 'devmap gm_flatgrass')
    stack.callback(host.close)
    host.ready()
    host.do('ui_openmenu jarMPPause', 'ui_openmenu jar_probe_empty', 'togglemenu',
            'cmd jar_equip 73557060', 'cg_thirdPerson 0', 'con_notifytime 0')
    place(host, -600, 0, 48, 0)
    host.do('+lookdown', 'wait 18', '-lookdown', 'cmd jar_spawn 88142876')
    time.sleep(.8)
    host.do('jar_world_save seed')
    worlds = host.profile / 'jar/worlds'
    path = worlds / 'seed.0.jrw'
    assert {node[0] for node in decode(path)['nodes']} == {SURFACE, CAMERA}
    seed = path.read_bytes()
    place(host, -250, -12, 48, 0)
    guest = None
    cases = {}
    sizes = (('', 128, 192), ('wide', 1152, 192), ('thin', 128, 12))
    directions = (('ground', 70, 1), ('sky', -70, 1), ('off', -70, 0))
    phases = list(product(sizes, directions)) + [(sizes[0], ('housing', -70, 1))]
    for (prefix, width, height), (direction, pitch, active) in phases:
        label = prefix + direction
        second_camera = direction == 'housing'
        (worlds / f'{label}.0.jrw').write_bytes(camera_world(seed, width, height, pitch, active, second_camera))
        host.do('jar_world_clear')
        host.do('jar_world_load ' + label)
        host.wait(f'Loaded {label} revision 1: {3 if second_camera else 2} objects')
        if guest is None:
            guest = player('Late', mod, profile / 'guest', run, free_port(),
                           f'connect 127.0.0.1:{port}', renderer='rdjar-mp-rend2')
            stack.callback(guest.close)
            guest.ready()
            guest.do('ui_openmenu jarMPPause', 'ui_openmenu jar_probe_empty', 'togglemenu',
                     'cmd jar_equip 73557060', 'cg_thirdPerson 0', 'con_notifytime 0')
            place(guest, -250, 28, 48, 0)
        time.sleep(1)
        for peer in (host, guest):
            peer.screenshot(label)
        if label == 'wideground':
            # In this renderer r_nocurves also selects the BSP's original
            # leaf faces. The camera sees only flat grass here.
            guest.do('r_nocurves 1', 'wait 30')
            guest.screenshot('originalground')
            guest.do('r_nocurves 0')
        cases[label] = dict(width=width, height=height, pitch=pitch, active=active)
    return cases


def compare_portals(reader, run):
    import numpy as np
    from PIL import Image

    results = {}
    for name in ('Host', 'Late'):
        for prefix in ('', 'wide', 'thin'):
            samples = {}
            poses = ('ground', 'sky', 'off') if prefix else ('ground', 'sky', 'off', 'housing')
            for pose in poses:
                with Image.open(run / f'{name}-{prefix}{pose}.png') as image:
                    w, h = image.size
                    samples[pose] = np.array(image.convert('RGB').crop((w*38//100, h//6, w*67//100, h*64//100)),
                                             dtype=np.int16)
            # The powered-off screen defines its own footprint, even for a thin screen.
            mask = samples['off'].max(axis=2) == 0
            assert mask.sum() > 150, (name, prefix, mask.sum())
            pixels = {pose: sample[mask] for pose, sample in samples.items()}
            change = np.abs(pixels['ground'] - pixels['sky']).mean(axis=0)
            grass_r, grass_g, grass_b = pixels['ground'].T
            grass_fraction = ((grass_g > grass_b + 6) & (grass_r > grass_b + 3)).mean()
            red, green, blue = pixels['sky'].T
            sky_fraction = ((blue > green + 12) & (green > red + 12)).mean()
            results[name + '-' + (prefix or 'default')] = dict(channel_change=change.tolist(),
                grass_fraction=float(grass_fraction), sky_fraction=float(sky_fraction), screen_pixels=int(mask.sum()))
            if name == 'Late' and prefix == 'wide':
                with Image.open(run / 'Late-originalground.png') as original:
                    original = np.array(original.convert('RGB').crop((w*38//100, h//6, w*67//100, h*64//100)))
                def color(pixels):
                    return pixels / np.maximum(1, pixels.sum(axis=1, keepdims=True))
                error = np.abs(color(original[mask]) - color(pixels['ground'])).mean()
                results['Late-wide']['unmerged_color_error'] = float(error)
            if not prefix:
                # Identify the dark stock camera body against the bright sky,
                # independent of the renderers' different exposure.
                second = pixels['housing']
                housing = (blue - second[:, 2] > 55) & (second.max(axis=1) - second.min(axis=1) < 25)
                results[name + '-default']['other_camera_pixels'] = int(housing.sum())
    (run / 'panel-pixels.json').write_text(json.dumps(results, indent=2) + '\n')
    for name, sample in results.items():
        assert max(sample['channel_change']) > 30, (name, sample)
        assert sample['sky_fraction'] > .98, (name, sample)
        assert sample['grass_fraction'] > .98, (name, sample)
        if 'unmerged_color_error' in sample:
            assert sample['unmerged_color_error'] < .025, (name, sample)
        if 'other_camera_pixels' in sample:
            assert sample['other_camera_pixels'] > 50, (name, sample)
    return dict(panels=results)
