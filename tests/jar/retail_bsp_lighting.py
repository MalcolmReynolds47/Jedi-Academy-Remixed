"""Run native lightmap decoders on retail BSP pairs without a window/GPU."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zipfile
from datetime import datetime

parser = argparse.ArgumentParser()
parser.add_argument('--game-data', required=True, type=Path)
parser.add_argument('--build', required=True, type=Path)
args = parser.parse_args()
output = args.build / ('retail-bsp-lighting-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
output.mkdir()
names = ['maps/mp/ffa3.bsp', 'maps/mp/duel4.bsp', 'maps/t1_inter.bsp']
data = {}
for package in sorted((args.game_data / 'base').glob('assets*.pk3')):
    with zipfile.ZipFile(package) as archive:
        available = {name.lower(): name for name in archive.namelist()}
        for name in names:
            if name in available:
                data[name] = archive.read(available[name])
assert set(data) == set(names), set(names) - set(data)
evidence = []
for index, name in enumerate(names):
    source = data[name]
    offset, size = struct.unpack_from('<ii', source, 8 + 14 * 8)
    assert source[:4] == b'RBSP' and size > 0 and size % (128 * 128 * 3) == 0
    assert offset >= 0 and offset + size <= len(source)
    path = output / f'map-{index}.bsp'
    path.write_bytes(source)
    evidence.append(dict(Name=name, SourceSHA256=hashlib.sha256(source).hexdigest(), Lightmaps=size//(128*128*3), File=str(path)))
pairs = [(0, 1), (1, 2)]
for left, right in pairs:
    assert evidence[left]['Lightmaps'] + evidence[right]['Lightmaps'] <= 256
    for renderer in ['classic', 'rend2']:
        exe = args.build / f'jar_bsp_lightmaps_{renderer}_tests.exe'
        result = subprocess.run([str(exe), evidence[left]['File'], evidence[right]['File']], capture_output=True, text=True, timeout=30)
        (output / f'{renderer}-{left}-{right}.log').write_text(result.stdout + result.stderr)
        assert result.returncode == 0, result.stdout + result.stderr
result = dict(GameLaunched=False, GPURendered=False, NativeLoadersPassed=2, Pairs=pairs, Maps=evidence,
              Scope='Production classic SP/MP and Rend2 lightmap loading; file decoders and image uploads are fixture boundaries.')
(output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
print(f'Retail native BSP lighting verification passed: {output}')
