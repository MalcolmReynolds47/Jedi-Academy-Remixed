"""Package unmodified installed retail icons and the retained JA:R cursor.
No generated pictograms. The private package remains tied to the owner's assets.
"""
from pathlib import Path
import argparse,zipfile,json,hashlib
ICONS={
 'crate':'gfx/hud/i_icon_ammodisp.jpg','banded':'gfx/hud/i_icon_ammodisp.jpg',
 'barrels':'gfx/hud/i_icon_ammodisp.jpg','r2':'gfx/hud/i_icon_seeker.tga',
 'health':'gfx/hud/i_icon_medkit.tga','shield':'gfx/hud/i_icon_shieldwall.tga',
 'shield_large':'gfx/hud/i_icon_shieldwall.tga','battery':'gfx/hud/i_icon_battery.tga',
 'swoop':'gfx/hud/w_icon_atst.tga','tauntaun':'gfx/hud/w_icon_atst.tga',
 'shortblade':'gfx/hud/w_icon_lightsaber.tga','pike':'gfx/hud/w_icon_saberstaff.tga',
 'sword':'gfx/hud/w_icon_lightsaber.tga','physgun':'gfx/hud/w_icon_demp2.tga',
 'toolgun':'gfx/hud/w_icon_blaster_pistol.tga'}
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True)
 parser.add_argument('--game-data',type=Path,required=True);args=parser.parse_args()
 available={}
 for path in sorted((args.game_data/'base').glob('assets*.pk3')):
  with zipfile.ZipFile(path) as z:
   for name in z.namelist():
    if name.lower() in set(ICONS.values()):available[name.lower()]=z.read(name)
 missing=set(ICONS.values())-available.keys()
 if missing:raise RuntimeError('Missing retail icons: '+', '.join(sorted(missing)))
 args.output.parent.mkdir(parents=True,exist_ok=True);provenance=[]
 with zipfile.ZipFile(args.output,'w',zipfile.ZIP_DEFLATED) as pack:
  def put(name,data):
   info=zipfile.ZipInfo(name,(2026,10,9,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;pack.writestr(info,data)
  for alias,source in ICONS.items():
   data=available[source];put('jar/icons/'+alias+Path(source).suffix,data)
   provenance.append({'icon':alias,'source':source,'sha256':hashlib.sha256(data).hexdigest(),'editing':'none'})
  put('shaders/jar_clouds.shader',(Path(__file__).resolve().parent.parent/'assets/jar/shaders/clouds.shader').read_bytes())
  put('shaders/jar_wasteland_snow.shader',(Path(__file__).resolve().parent.parent/'assets/jar/shaders/wasteland_snow.shader').read_bytes())
  put('shaders/jar_logic_marker.shader',(Path(__file__).resolve().parent.parent/'assets/jar/shaders/logic_marker.shader').read_bytes())
  put('shaders/jar_portal_panel.shader',(Path(__file__).resolve().parent.parent/'assets/jar/shaders/portal_panel.shader').read_bytes())
  put('shaders/jar_saber_rgb.shader',(Path(__file__).resolve().parent.parent/'assets/jar/shaders/saber_rgb.shader').read_bytes())
  put('ext_data/sabers/jar_melee.sab',(Path(__file__).resolve().parent.parent/'assets/melee-definitions/jar_melee.sab').read_bytes())
  put('jar/icons/cursor.tga',(Path(__file__).resolve().parent.parent/'assets/jar/icons/cursor.tga').read_bytes())
  put('jar/icons/provenance.json',json.dumps(provenance,indent=2).encode())
  put('jar/icons/NOTICE.txt',b'Retail icon bytes are unmodified owner-installed game assets. Original retained JA:R cursor is GPL-2.0-only. No new pictograms.\n')
 print(args.output)
if __name__=='__main__':main()
