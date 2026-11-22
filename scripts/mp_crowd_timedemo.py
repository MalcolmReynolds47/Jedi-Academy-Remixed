"""Time the same recorded multiplayer fight in the Classic renderer."""
import hashlib
import math
import re
import statistics
import time


def replay(mod, profile, run, demo, player, free_port, link_asset):
    if not demo.is_file() or not re.fullmatch(r'\.dm_\d+', demo.suffix, re.I):
        raise ValueError(f'Expected a multiplayer demo file: {demo}')
    (mod / 'demos').mkdir()
    link_asset(demo, mod / 'demos' / ('crowd-render' + demo.suffix.lower()))
    start = 'CROWD_TIME_BEGIN'
    observer = player('Replay', mod, profile, run, free_port(), 'echo REPLAY_READY')
    try:
        observer.wait('LISTEN_TEST_ENTRY', 60)
        observer.queue('con_notifytime 0', 'com_maxfps 0', 'com_maxfpsUnfocused 0', 'timedemo 1',
                       'demo crowd-render', 'wait 120', 'screenshot_png crowd', 'com_speeds 1', 'echo ' + start)
        # The engine counts rendered demo frames and advances by a fixed 50 ms
        # per frame. End at its own completion message, before drawing the menu.
        limit = time.monotonic() + 300
        while True:
            log = observer.text()
            match = re.search(r'(\d+) frames, ([\d.]+) seconds: ([\d.]+) fps', log)
            if match:
                break
            if "couldn't open demos/crowd-render" in log or 'Demo file was truncated' in log:
                raise ValueError('The multiplayer client could not read this recording')
            if observer.process.poll() is not None or time.monotonic() >= limit:
                raise RuntimeError('Timedemo did not complete; see the Replay console log')
            time.sleep(.15)
        if start not in log[:match.start()]:
            raise ValueError('The recording ended before the warm-up finished')
        sample = log[:match.start()].rsplit(start, 1)[1]
        rows = re.findall(r'frame:\d+ all:\s*(\d+).*?rf:\s*(\d+) bk:\s*(\d+)', sample)
        if not rows:
            raise ValueError('No complete frames after warm-up')
        loaded = re.findall(r'Trying to load "(rd[^"]+\.dll)"', log)
        renderer = 'rdjar-mp-vanilla_x86_64.dll'
        assert set(loaded) == {renderer}, loaded
        result = dict(frames=int(match[1]), seconds=float(match[2]), fps=float(match[3]), sampled_frames=len(rows))
        for key, values in zip(('engine_work', 'renderer_frontend', 'renderer_backend'), zip(*rows)):
            ms = sorted(map(int, values))
            result[key] = dict(mean_ms=statistics.mean(ms), median_ms=statistics.median(ms),
                               p95_ms=ms[math.ceil(.95*len(ms))-1])
        for key, path in [('demo', demo), ('renderer', mod/renderer), ('client', mod/'jar_mp.x86_64.exe')]:
            with path.open('rb') as source:
                result[key+'_sha256'] = hashlib.file_digest(source, 'sha256').hexdigest()
        return result
    finally:
        try:
            observer.close()
        finally:
            # A failed map load can erase the queued quit command. This is
            # the process created above, running under the disposable profile.
            if observer.process.poll() is None:
                observer.process.terminate()
                observer.process.wait(timeout=10)
