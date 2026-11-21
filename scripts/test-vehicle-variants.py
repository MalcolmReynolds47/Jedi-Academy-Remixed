"""Exercise both production loaders with locally installed retail vehicle data."""
import argparse,json,re,subprocess,tempfile,zipfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--game-data',type=Path,required=True);p.add_argument('--build',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];files={}
for path in sorted((a.game_data/'base').glob('assets*.pk3')):
 z=zipfile.ZipFile(path)
 for name in z.namelist():files[name.lower()]=(z,name)
def data(name):z,key=files[name];return z.read(key)
variants=dict(re.findall(r'\{"(swoop_[^"]+)","([^"]+)"\}',(root/'shared/qcommon/jar_vehicle_variants.h').read_text()))
assert len(variants)==7
entries={e['name']:e for e in json.loads((root/'docs/JAR-spawn-catalog.json').read_text())['entries'] if e['kind']==3}
for name,skin in variants.items():
 assert not entries[name]['reason'] and entries[name]['model']=='models/players/swoop/model.glm'
 text=data('models/players/swoop/model_'+skin+'.skin').decode('latin1')
 textures=[]
 for line in text.splitlines():
  if ',' not in line:continue
  surface,texture=line.split(',',1);texture=texture.strip().lower()
  if not texture or texture in ('*off','*default') or surface.strip().lower().startswith('tag_'):continue
  stem=str(Path(texture).with_suffix('')).replace('\\','/')
  assert any(stem+ext in files for ext in ('.jpg','.tga','.png')),(name,texture)
  textures.append(texture)
 assert textures,(name,'skin has no textures')
with tempfile.TemporaryDirectory(prefix='jar-vehicle-variants-',dir=root/'build') as directory:
 for source in ('swoop.veh','swoop_red.veh'):
  path=Path(directory)/source;path.write_bytes(data('ext_data/vehicles/'+source))
  for mode in ('sp','mp'):
   subprocess.run([str(a.build/f'jar_vehicle_variants_{mode}_tests.exe'),str(path)],check=True)
print('PASS: seven original color IDs, visible retail skins/textures, both stock Swoop definitions, actual SP/MP loaders and 64-slot exhaustion')
