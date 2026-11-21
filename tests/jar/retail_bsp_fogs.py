"""Exercise classic and Rend2 fog loaders against every installed stock BSP."""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('--game-data', type=Path, required=True)
parser.add_argument('--build', type=Path, required=True)
args = parser.parse_args()
output = args.build / ('retail-bsp-fogs-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
output.mkdir()
maps = {}
for package in sorted((args.game_data / 'base').glob('assets*.pk3')):
    with zipfile.ZipFile(package) as archive:
        for name in archive.namelist():
            if name.lower().startswith('maps/') and name.lower().endswith('.bsp'):
                maps[name.lower()] = archive.read(name)
assert maps and 'maps/mp/ffa2.bsp' in maps
# This main map has both global and bounded fog. Subsequent children cover
# no fog (inherit), their own global fog, and multiple bounded volumes.
names = ['maps/mp/ffa2.bsp'] + sorted(set(maps) - {'maps/mp/ffa2.bsp'})
evidence = []
for index, name in enumerate(names):
    data = maps[name]
    assert data[:4] == b'RBSP'
    offset, size = struct.unpack_from('<ii', data, 8 + 12 * 8)
    assert size % 72 == 0 and 0 <= offset <= offset + size <= len(data)
    fogs = [struct.unpack_from('<64sii', data, n) for n in range(offset, offset + size, 72)]
    path = output / f'map-{index}.bsp'
    path.write_bytes(data)
    evidence.append(dict(Name=name, SHA256=hashlib.sha256(data).hexdigest(), File=str(path),
                         Fogs=len(fogs), GlobalFog=any(f[1] == -1 for f in fogs),
                         BoundedFog=any(f[2] != -1 for f in fogs)))
for mode in ['sp', 'mp', 'rend2']:
    exe = args.build / f'jar_bsp_fog_{mode}_tests.exe'
    result = subprocess.run([str(exe)] + [m['File'] for m in evidence], capture_output=True, text=True, timeout=60)
    (output / f'{mode}.log').write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
for mode in ['sp', 'mp', 'rend2']:
    exe = args.build / f'jar_bsp_grid_{mode}_tests.exe'
    result = subprocess.run([str(exe)] + [m['File'] for m in evidence], capture_output=True, text=True, timeout=60)
    (output / f'grid-{mode}.log').write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
result = dict(NativeLoadersPassed=3, NativeGridLoadersPassed=3, Maps=evidence, GameLaunched=False, GPURendered=False,
              Scope='Production classic SP/MP and Rend2 fog loaders and surface references for all retail BSPs. Also exercises shared light-grid loading, native worldspawn parsing and sampling in all three renderer modes. Shader parameters, grid color shifting and GL drawing are fixture boundaries.')
(output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
print(f'Retail native BSP fog verification passed: {output}')
print(f'Retail native BSP grid verification passed: {output}')
