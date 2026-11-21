"""Run independent native world scenarios; report every failure before exiting."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--game-data', required=True)
    parser.add_argument('--build', required=True)
    args = parser.parse_args()

    def check(name):
        proc = subprocess.run([sys.executable, str(Path(__file__).with_name('test-mp-server.py')),
                               '--game-data', args.game_data, '--build', args.build, '--'+name],
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace')
        return name, proc

    failures=[]
    with ThreadPoolExecutor(max_workers=2) as pool:
        for name, proc in pool.map(check, ('world','autosave','rigid','attachments','joints')):
            print(f'[{name}] exit {proc.returncode}\n{proc.stdout}', flush=True)
            if proc.returncode:
                failures.append(name)
    if failures:
        raise SystemExit('Failed native scenarios: '+', '.join(failures))


if __name__ == '__main__':
    main()
