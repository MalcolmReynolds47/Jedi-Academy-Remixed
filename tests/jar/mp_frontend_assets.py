"""Check the native MP menu hooks against current retail assets, not copied menu art."""
import argparse,re,zipfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--game-data',required=True,type=Path);a=p.parse_args();root=Path(__file__).resolve().parents[2]
files={};archives=[]
for path in sorted((a.game_data/'base').glob('assets*.pk3')):
 z=zipfile.ZipFile(path);archives.append(z)
 for name in z.namelist():files[name.lower()]=(z,name)
def text(name):
 z,key=files[name];return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/','',z.read(key).decode('latin1'))
menus=set()
for name in files:
 if name.startswith('ui/jamp/') and name.endswith('.menu'):menus.update(re.findall(r'\bmenuDef\s*\{\s*name\s+"?([^"\s]+)',text(name),re.I))
src=(root/'codemp/ui/ui_jar_frontend.c').read_text(encoding='utf-8-sig');styled=set(re.findall(r'"([^"]+)"',re.search(r'static const char\*names\[\]=\{([^}]+)\}',src)[1]))
assert styled==menus-{'main'},(styled-menus,menus-styled)
main=text('ui/jamp/main.menu')
for name in ['playbutton','profilebutton','controlsbutton','setupbutton','exitgamebutton']:assert re.search(r'\bname\s+"?'+name+r'\b',main,re.I)
for name in ['jarMPSettings','jarMPHost','jarMPPause','jarMPBindings','jarMPConsole','jarMPInterface','jarMPGraphics','jarMPTextures','jarMPDetail','jarMPDisplay','jarMPDisplayConfirm','jarMPEffects']:assert name in src and name not in menus
assert len(menus)+12<128
assert 'ui/jahud.txt' in files
assert 'fonts/arialnb.fontdat' in files and (root/'assets/jar/icons/cursor.tga').is_file()
for z in archives:z.close()
print(f'PASS: {len(styled)} native MP menu names, main controls, retail font and retained cursor; native actions remain runtime-owned')

# Native weapon catalog must include every actual MP weapon classname; comments do not count.
items=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/','',(root/'codemp/game/bg_misc.c').read_text())
weapons={m.group(1):m.group(2) for m in re.finditer(r'\{\s*"(weapon_[^"]+)"\s*,(.*?)IT_WEAPON\s*,\s*(WP_\w+)',items,re.S)}
catalog=(root/'codemp/game/jar_mp_catalog.h').read_text()
for name in weapons:assert re.search(r'\{\d+,4,2,"'+name+r'",',catalog),name
for name,expected in [('weapon_physgun','concussion/c_rifle'),('weapon_toolgun','blaster_pistol/blaster_pistol')]:
 body=weapons[name]
 for path in [f'models/weapons2/{expected}_w.glm',f'models/weapons2/{expected}.md3',f'models/weapons2/{expected}_hand.md3']:
  assert path in files,path
 assert f'models/weapons2/{expected}_w.glm' in body and f'models/weapons2/{expected}.md3' in body
for sound in ['sound/weapons/bryar/fire.mp3','sound/weapons/bryar/select.wav','sound/weapons/concussion/select.wav']:assert sound in files or sound.removesuffix('.wav')+'.mp3' in files,sound
print(f'PASS: all {len(weapons)} native MP weapon cards, shared SP tool models, hand resources and sounds')

# Media pickers reference unmodified retail files; typed paths use runtime validation.
media=(root/'codemp/game/jar_mp_media.h').read_text()
for group,prefix,suffix in [('jarSpeakerChoices','',''),('jarEffectChoices','effects/','.efx')]:
 values=re.findall(r'"([^"\n]+)"',re.search(group+r'\[\]=\{([^}]+)\}',media)[1])
 assert len(values)==5
 for path in values:assert prefix+path+suffix in files,prefix+path+suffix
print('PASS: ten sound/effect picker assets exist in retail archives')
