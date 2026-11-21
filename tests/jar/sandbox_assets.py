"""Verify the packaged Sandbox aliases are byte-identical retail icons."""
from pathlib import Path
import argparse,hashlib,json,re,zipfile
parser=argparse.ArgumentParser();parser.add_argument('--game-data',type=Path,required=True);parser.add_argument('--pack',type=Path,required=True);args=parser.parse_args()
root=Path(__file__).resolve().parents[2];sources={};shaders=[]
with zipfile.ZipFile(args.pack) as packed:
 records=json.loads(packed.read('jar/icons/provenance.json'))
 wanted={r['source'].lower() for r in records}
 direct=set(re.findall(r'"((?:gfx/hud|models/players)/[^"\n]+)"',(root/'code/ui/ui_jar_overlay.inc').read_text()))
 generated=json.loads((root/'docs/JAR-spawn-catalog.json').read_text())['entries']
 direct.update(e['icon'] for e in generated if not e['icon'].startswith('jar/'))
 names=set()
 for path in sorted((args.game_data/'base').glob('assets*.pk3')):
  with zipfile.ZipFile(path) as stock:
   for name in stock.namelist():
    low=name.lower();names.add(low)
    if low in wanted:sources[low]=stock.read(name)
    if low.endswith('.shader'):shaders.append(stock.read(name).decode('utf-8','replace').lower())
 cloud=packed.read('shaders/jar_clouds.shader').decode('ascii')
 assert cloud==(root/'assets/jar/shaders/clouds.shader').read_text()
 assert 'textures/skies/cloudlayer2.png' in names
 marker=packed.read('shaders/jar_logic_marker.shader').decode('ascii')
 assert marker==(root/'assets/jar/shaders/logic_marker.shader').read_text()
 marker_textures=['gfx/menus/radar/LS_reticle']
 assert {name.lower()+'.tga' for name in marker_textures}<=names
 assert re.findall(r'(?m)^\s*clampmap\s+(\S+)',marker)==marker_textures
 assert 'rgbGen exactVertex' in marker and 'alphaGen vertex' in marker and 'nomipmaps' in marker
 assert 'blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA' in marker
 assert 'circle_base' not in marker and 'depthFunc' not in marker
 portal=packed.read('shaders/jar_portal_panel.shader').decode('ascii')
 assert portal==(root/'assets/jar/shaders/portal_panel.shader').read_text()
 assert all(word in portal for word in ('gfx/jar/portal_panel','blendFunc GL_ZERO GL_ONE','depthWrite'))
 for suffix in ('', '1', '2', '3'):
  monitor=portal.split('gfx/jar/portal_monitor'+suffix+'\n',1)[1].split('gfx/jar/',1)[0]
  assert 'map $jarCamera'+suffix+'\n' in monitor and 'sort opaque' in monitor
 assert 'alphaGen portal' not in portal
 speaker_cues=re.findall(r'"(sound/[^"\n]+)",(true|false)',(root/'code/qcommon/jar_sandbox_speaker.h').read_text())
 assert speaker_cues
 for path,loop in speaker_cues:
  assert path+'.wav' in names or (loop=='false' and path+'.mp3' in names),path
 assert set(re.findall(r'(?m)^jar/world/[^\s]+',cloud))=={'jar/world/cloud','jar/world/cloud_alt'}
 assert re.findall(r'(?m)^\s*map\s+(\S+)',cloud)==['textures/skies/cloudlayer2']*2
 for r in records:
  data=packed.read('jar/icons/'+r['icon']+Path(r['source']).suffix)
  assert r['editing']=='none' and data==sources[r['source'].lower()]
  assert hashlib.sha256(data).hexdigest()==r['sha256']
 for icon in direct:assert any(icon.lower()+ext in names for ext in ('.tga','.jpg','.png')),icon
 for shader in ('gfx/effects/blueline','gfx/effects/redline'):
  assert any(re.search(r'(?m)^\s*'+re.escape(shader)+r'\s*\{',text) for text in shaders),shader
 for asset in ('models/map_objects/imperial/crate1.md3','models/map_objects/mp/crystal_blue.md3','effects/thermal/explosion.efx','effects/chunks/metalexplode.efx','sound/weapons/explosions/cargoexplode.wav','models/map_objects/kejim/sec_panel.md3','models/map_objects/kejim/impcam_base.md3','models/map_objects/kejim/impcam.md3','sound/movers/objects/cameramove_lp2.wav','models/weapons2/demp2/demp2.md3','effects/sparks/spark_nosnd.efx','sound/effects/electric_beam_lp.wav'):
  assert asset in names,asset
 for material in ('gfx/misc/spark','gfx/misc/spark2','gfx/misc/steam'):
  assert any(material+ext in names for ext in ('.tga','.jpg','.png')) or any(re.search(r'(?m)^\s*'+re.escape(material)+r'\s*\{',text) for text in shaders),material
 for sound in ('sound/effects/metal_bounce.mp3','sound/effects/metal_bounce2.mp3','sound/weapons/bryar/fire.mp3'):assert sound in names,sound
print(f'PASS: {len(records)} unedited retail aliases, {len(direct)} direct stock icons, beam/cloud shaders, retail cloud texture, welder/camera/crystal-crate resources and collision sounds')
