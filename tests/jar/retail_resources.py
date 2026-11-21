"""Exercise the production item/weapon loaders on exact retail definitions, no game."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from jar_catalog_assets import catalog_asset_paths
import subprocess
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--game-data', type=Path, required=True)
p.add_argument('--build', type=Path, required=True)
a = p.parse_args()
build = a.build.resolve()
run = build / ('retail-resources-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
run.mkdir()
paths, definitions, npcs = set(), {}, {}
for pack in catalog_asset_paths(a.game_data):
    with zipfile.ZipFile(pack) as z:
        for name in z.namelist():
            paths.add(name.lower())
            if name.lower().startswith('ext_data/npcs/') and name.lower().endswith('.npc'):
                npcs[name.lower()] = z.read(name)
            if name.lower() in ('ext_data/items.dat', 'ext_data/weapons.dat'):
                definitions[name.lower()] = z.read(name)
assert len(definitions) == 2 and npcs
definitions['ext_data/npcs.dat'] = b'\n'.join(npcs[name] for name in sorted(npcs))
(run / 'paths.txt').write_text('\n'.join(sorted(paths)) + '\n', encoding='utf-8')
for name, data in definitions.items():
    (run / Path(name).name).write_bytes(data)
for test, data in [('jar_item_resources_tests', 'items.dat'), ('jar_weapon_resources_tests', 'weapons.dat'), ('jar_actor_catalog_tests', 'npcs.dat')]:
    result = subprocess.run([str(build / (test + '.exe')), str(run / data), str(run / 'paths.txt')],
                            capture_output=True, text=True, timeout=30)
    (run / (test + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
    print(result.stdout, end='')
    if result.returncode:
        raise RuntimeError(f'{test} failed ({result.returncode}): {result.stderr}')
evidence = dict(DefinitionSHA256={name: hashlib.sha256(data).hexdigest() for name, data in definitions.items()},
                RetailPaths=len(paths), NativeLoadersPassed=3, GameLaunched=False)
(run / 'result.json').write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
print('Retail native resource verification passed: ' + str(run))
