"""Independent coverage checks against installed retail file and definition names."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from jar_catalog_assets import catalog_asset_paths
import argparse,json,re,zipfile,math,struct
p=argparse.ArgumentParser();p.add_argument('--game-data',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[2];report=json.loads((root/'docs/JAR-spawn-catalog.json').read_text());entries=report['entries'];byid={e['id']:e for e in entries}
files={}
for path in catalog_asset_paths(a.game_data):
 z=zipfile.ZipFile(path)
 for name in z.namelist():files[name.lower()]=(z,name)
button_model='models/map_objects/imperial/switch.md3'
assert byid[96653372]['name']=='func_button' and byid[96653372]['model']==button_model
archive,key=files[button_model];button=archive.read(key)
assert button[:4]==b'IDP3' and struct.unpack_from('<i',button,76)[0]==5
frame_offset=struct.unpack_from('<i',button,92)[0]
frames=[struct.unpack_from('<6f',button,frame_offset+i*56) for i in range(5)]
assert tuple(min(f[i] for f in frames) for i in range(3))==(-17.5,-14,0)
assert tuple(max(f[i] for f in frames) for i in range(3,6))==(14.71875,14,26.390625)
for sound in ('button_08.mp3','button_09.mp3'):
 archive,key=files['sound/movers/switches/'+sound];assert archive.getinfo(key).file_size>128
print('PASS: portable button uses the five retail Imperial switch frames, their combined hull and both stock sounds')
lift_model='models/map_objects/rocky_ruins/floor_grate.md3'
assert byid[93973657]['name']=='func_plat' and byid[93973657]['model']==lift_model
archive,key=files[lift_model];lift=archive.read(key)
assert lift[:4]==b'IDP3' and struct.unpack_from('<i',lift,76)[0]==1
offset=struct.unpack_from('<i',lift,92)[0]
assert struct.unpack_from('<6f',lift,offset)==(-32.03125,-32.03125,.03125,32.03125,32.046875,4.921875)
for sound in ('platform_start.mp3','platform_move_lp.wav','platform_stop.mp3'):
 archive,key=files['sound/movers/platforms/'+sound];assert archive.getinfo(key).file_size>128
print('PASS: portable lift uses the stock floor grate, exact model hull and three stock platform sounds')
bob_model='models/map_objects/imperial/crate1.md3'
assert byid[98040309]['name']=='func_bobbing' and byid[98040309]['model']==bob_model
archive,key=files[bob_model];bob=archive.read(key)
assert bob[:4]==b'IDP3' and struct.unpack_from('<i',bob,76)[0]==1
offset=struct.unpack_from('<i',bob,92)[0]
assert struct.unpack_from('<6f',bob,offset)==(-16.75,-10.921875,.0625,16.828125,10.921875,21.90625)
print('PASS: portable bobbing platform uses the stock Imperial crate and exact model hull')
def text(name):z,key=files[name];return z.read(key).decode('latin1')
def clean(s):return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/','',s)
def names(extension):
 result=set()
 for name in files:
  if name.startswith('ext_data/') and name.endswith(extension):result.update(n.lower() for n in re.findall(r'(?m)^\s*([^\s{}"]+)\s*\{',clean(text(name))))
 return result
source=(root/'code/game/g_workshop.cpp').read_text()
legacyprops=set(re.findall(r'\{10\d\d,"[^"]+","([^"]+)"',source))
legacyactors={m.group(2).lower():int(m.group(1)) for m in re.finditer(r'\{([34]\d{3}),"([^"]+)"',source)}
models={name for name in files if name.endswith(('.md3','.glm'))}
excluded=report['ownerExcluded'];excluded_names={e['name'] for e in excluded}
policy=json.loads((root/'docs/JAR-catalog-exclusions.json').read_text(encoding='utf-8'))
assert len(excluded)==len(excluded_names)==21 and excluded_names==set(policy['models'])
assert not excluded_names&{e['model'] for e in entries if e['kind']==1}
assert not {e['id'] for e in excluded}&byid.keys()
for e in excluded:
 z,key=files[e['name']];b=z.read(key)
 if e['name'].endswith('.glm'):
  assert e['name']=='models/weapons2/noweap/noweap.glm' and b[:4]==b'2LGM' and len(b)==e['bytes']
  surfaces,lod=struct.unpack_from('<i',b,152)[0],struct.unpack_from('<i',b,148)[0]
  # Independently inspect every default-visible surface, including disconnected ones.
  triangles=sum(struct.unpack_from('<i',b,lod+4+struct.unpack_from('<i',b,lod+4+i*4)[0]+12)[0] for i in range(surfaces) if struct.unpack_from('<I',b,164+struct.unpack_from('<i',b,164+i*4)[0]+64)[0]==0)
  assert triangles==e['visibleTriangles']==0
  continue
 assert e['name'].endswith('.md3') and b[:4]==b'IDP3' and struct.unpack_from('<i',b,4)[0]==15
 frames,tags,surfaces=struct.unpack_from('<3i',b,76)
 assert frames==e['frames'] and tags==e['tags'] and frames>0 and tags>0 and surfaces==e['surfaces']==0
 tag_offset,end=(struct.unpack_from('<i',b,i)[0] for i in (96,104))
 assert tag_offset>=108 and tag_offset+frames*tags*112<=end==len(b)==e['bytes']
assert models<=legacyprops|excluded_names|{e['model'].lower() for e in entries if e['kind']==1}
assert 'models/weapons2/noweap/noweap.glm' in excluded_names
print('PASS: twenty owner-approved MD3 helpers and one empty GLM hidden; remaining retail model coverage preserved')
actors=names('.npc');vehicles=names('.veh');indexed={e['name'].lower() for e in entries if e['kind'] in (2,3)}|legacyactors.keys()
excluded_content=report['ownerExcludedContent']
expected_content={(2,'test')}|{(5,'NPC_Monster_'+n) for n in ('Claw','Fish','Flier2','Lizard','Murjj','Swamp')}
assert len(excluded_content)==7 and {(e['kind'],e['name']) for e in excluded_content}==expected_content
assert {(e['kind'],e['name']) for e in policy['unavailableEntries']}==expected_content
assert not {e['id'] for e in excluded_content}&byid.keys()
assert not expected_content&{(e['kind'],e['name']) for e in entries}
assert all(e['reason'] and e['ownerDecision']=='Exclude and hide these seven' for e in excluded_content)
assert 'models/players/test/model.glm' not in files
assert not {'claw','fish','flier2','lizard','murjj','swamp'}&actors
assert actors<=indexed|{'test'},actors-indexed-{'test'}
assert vehicles<=indexed,vehicles-indexed
items=set(re.findall(r'(?mi)^\s*classname\s+([^\s"]+)',clean(text('ext_data/items.dat'))))
olditems={'item_medpak_instant','item_shield_sm_instant','item_battery','item_shield_lrg_instant','weapon_blaster_pistol','weapon_blaster','weapon_disruptor','weapon_bowcaster','weapon_repeater','weapon_demp2','weapon_flechette','weapon_rocket_launcher','weapon_thermal','weapon_trip_mine','weapon_det_pack','weapon_concussion_rifle'}
assert items<=olditems|{e['name'] for e in entries if e['kind']==4}
spawnsource=(root/'code/game/g_spawn.cpp').read_text().split('spawn_t\tspawns[] = {')[1].split('{NULL, NULL}')[0]
classes=set(re.findall(r'^\s*\{\s*"([^"]+)"',clean(spawnsource),re.M))
disabled=report['ownerExcludedDisabled']
assert len(disabled)==1 and disabled[0]['kind']==5 and disabled[0]['name']=='terrain' and disabled[0]['ownerDecision']=='Exclude and hide terrain'
assert disabled[0]['id'] not in byid and not any(e['name']=='terrain' for e in entries)
section_profiles=re.findall(r'\{"([^"\n]+)","#([^"\n]+)","([^"\n]+)",Mode::\w+\}',(root/'code/game/g_bsp_section.h').read_text())
section_cards={name for name,resource,title in section_profiles if name!='misc_bsp'}
assert classes=={e['name'] for e in entries if e['kind']==5 and e['name'] not in section_cards}|{name for kind,name in expected_content if kind==5}|{'terrain'}
for name,resource,title in section_profiles:
 matches=[e for e in entries if e['kind']==5 and e['name']==name]
 assert len(matches)==1 and matches[0]['model']=='maps/'+resource+'.bsp' and not matches[0]['reason'] and not matches[0]['alias']
 assert matches[0]['tab']==5 and matches[0]['icon']=='levelshots/'+resource
assert {e['name'] for e in entries if e['tab']==5}=={name for name,_,_ in section_profiles}
print('PASS: seven exact owner-approved unavailable cards hidden; all other actor and entity classes retained')
assert len(byid)==len(entries)
# Native SP legacy droids: qualify the real frames, visible surfaces, embedded
# textures and animation ranges. Registration/loaded-definition failures are
# separately exercised by the production C++ actor/catalog fixtures.
byname={e['name']:e for e in entries if e['kind'] in (2,5)}

for actor,alias in (('glider','NPC_Monster_Glider'),('mark1','NPC_Droid_Mark1'),('mark2','NPC_Droid_Mark2'),('minemonster','NPC_MineMonster')):
 e=byname[actor];assert not e['reason'] and e['model']==f'models/players/{actor}/model.glm'
 assert not byname[alias]['reason'] and byname[alias]['alias']==e['id']
 assert any(p['kind']==1 and p['model']==e['model'] and not p['reason'] for p in entries)
print('PASS: four original legacy NPC bodies, native actor cards, four map aliases and four prop cards')

for actor,alias in (('mouse','NPC_Droid_Mouse'),('remote_sp','NPC_Droid_Remote')):
 e=byname[actor];assert not e['reason'] and e['model']==f'models/players/{actor}/lower.md3'
 assert not byname[alias]['reason'] and byname[alias]['alias']==e['id']
 z,key=files[e['model']];b=z.read(key)
 assert b[:4]==b'IDP3' and struct.unpack_from('<i',b,4)[0]==15
 frames,surfaces,off,end=(struct.unpack_from('<i',b,i)[0] for i in (76,84,100,104))
 assert frames>0 and surfaces>0 and end==len(b)
 for i in range(surfaces):
  assert b[off:off+4]==b'IDP3'
  sf,shaders,verts,tris,triangle_offset,shader_offset,_,vertex_offset,size=struct.unpack_from('<9i',b,off+72)
  assert sf==frames and shaders>0 and verts>0 and tris>0 and 108<=size<=len(b)-off
  assert triangle_offset>=108 and triangle_offset+tris*12<=size
  assert vertex_offset>=108 and vertex_offset+frames*verts*8<=size
  assert all(0<=v<verts for v in struct.unpack_from('<'+str(tris*3)+'i',b,off+triangle_offset))
  for j in range(shaders):
   assert 108<=shader_offset and shader_offset+(j+1)*68<=size
   shader=b[off+shader_offset+j*68:off+shader_offset+j*68+64].split(b'\0',1)[0].decode('ascii').lower()
   # R_LoadImage retries other supported extensions when an authored .tga is
   # shipped as JPEG; both retail droid models use this native path.
   assert any(str(Path(shader).with_suffix(ext)).replace('\\','/') in files for ext in ('.tga','.jpg','.png')),(actor,shader)
  off+=size
 assert off==end
 animations=re.findall(r'(?m)^\s*(\w+)\s+(\d+)\s+(\d+)\s+(-?\d+)\s+(-?\d+)',clean(text(f'models/players/{actor}/animation.cfg')))
 assert animations and any(a[0]=='BOTH_STAND1' for a in animations)
 for name,first,count,loop,fps in animations:
  assert 0<=int(first)<frames and 0<int(count)<=frames-int(first) and int(fps)!=0
  assert -1<=int(loop)<=int(count)
# The missing retail seeker body now uses the qualified remote_sp geometry;
# the original Seeker definition (AI/allegiance/weapon) is still fingerprinted.
seeker=byname['seeker'];assert not seeker['reason'] and seeker['model']==byname['remote_sp']['model']
assert seeker['npcHash'] and seeker['npcHash']!=byname['remote_sp']['npcHash']
assert not byname['NPC_Droid_Seeker']['reason'] and byname['NPC_Droid_Seeker']['alias']==seeker['id']
remote=byname['remote'];assert not remote['reason'] and remote['model']==byname['remote_sp']['model']
assert remote['npcHash']!=byname['remote_sp']['npcHash'] and 'models/players/remote/model.glm' not in files
assert 'models/players/remote/lower.md3' not in files
assert 'models/players/remote/lower.mdr' not in files
assert 'sound/chars/seeker/misc/hiss.wav' in files
assert 'effects/blaster/muzzle_flash.efx' in files
assert 'effects/env/small_explode.efx' in files
import importlib.util
spec=importlib.util.spec_from_file_location('catalog_builder',root/'scripts/build-spawn-catalog.py');builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
original={'legsmodel':'remote','class':'CLASS_SEEKER'}
assert builder.repaired_legacy_legs('seeker',original,files)==dict(original,legsmodel='remote_sp') and original['legsmodel']=='remote'
for suffix in ('md3','mdr'):
 assert builder.repaired_legacy_legs('seeker',original,set(files)|{f'models/players/remote/lower.{suffix}'})==original
for part in ('lower.md3','animation.cfg'):
 assert builder.repaired_legacy_legs('seeker',original,set(files)-{f'models/players/remote_sp/{part}'})==original
assert builder.repaired_legacy_legs('remote',original,files)==original
remote_def={'playermodel':'remote','class':'CLASS_SEEKER','customskin':'default'}
expected=dict(remote_def);expected.pop('playermodel');expected.update(headmodel='none',torsomodel='none',legsmodel='remote_sp')
assert builder.repaired_legacy_body('remote',remote_def,files)==expected
assert builder.repaired_legacy_body('remote',remote_def,set(files)|{'models/players/remote/model.glm'})==remote_def
for part in ('lower.md3','animation.cfg'):
 assert builder.repaired_legacy_body('remote',remote_def,set(files)-{f'models/players/remote_sp/{part}'})==remote_def
assert builder.repaired_legacy_body('remote',dict(remote_def,customskin='blue'),files)==dict(remote_def,customskin='blue')

assert builder.repaired_legacy_legs('seeker',{'legsmodel':'remote/blue'},files)=={'legsmodel':'remote/blue'}
assert 'models/weapons2/noweap/noweap.md3' in files
print('PASS: mouse/remote_sp/seeker native MD3 resources, missing-only fallback, seeker sounds and entity aliases')
restored_pickups={e['name']:e for e in entries if e['kind']==4}
force_models={'heal':'lt_heal','levitation':'force_jump','speed':'force_speed','push':'force_push','pull':'force_pull','telepathy':'lt_telepathy','grip':'dk_grip','lightning':'dk_lightning','saberthrow':'saber_throw'}
restored={**{'holocron_force_'+name:'models/map_objects/mp/'+model+'.md3' for name,model in force_models.items()},'item_bacta':'models/map_objects/mp/bacta.md3','weapon_stun_baton':'models/weapons2/stun_baton/baton_w.glm','weapon_jawa':'models/weapons2/briar_pistol/briar_pistol_w.glm'}
for name,model in restored.items():
 e=restored_pickups[name];assert not e['reason'] and e['model']==model and model in files
 assert any(e['icon']+ext in files for ext in ('.tga','.jpg','.png')),(name,e['icon'])
 # These models already appear in the geometry-qualified prop catalog.
 assert any(prop['kind']==1 and prop['model']==model and not prop['reason'] for prop in entries)
 if model.endswith('.md3'):
  z,key=files[model];b=z.read(key);off=struct.unpack_from('<i',b,100)[0]
  for i in range(struct.unpack_from('<i',b,84)[0]):
   shader_count=struct.unpack_from('<i',b,off+76)[0];shader_offset=struct.unpack_from('<i',b,off+92)[0]
   assert shader_count>0
   for j in range(shader_count):
    shader=b[off+shader_offset+j*68:off+shader_offset+j*68+64].split(b'\0',1)[0].decode('ascii').lower()
    assert any(str(Path(shader).with_suffix(ext)).replace('\\','/') in files for ext in ('.tga','.jpg','.png')),(name,shader)
   off+=struct.unpack_from('<i',b,off+104)[0]
assert any('sound/player/holocron'+ext in files for ext in ('.wav','.mp3'))
for name in ('models/weapons2/stun_baton/baton.md3','models/weapons2/stun_baton/baton_barrel.md3','models/weapons2/stun_baton/baton_barrel2.md3','models/weapons2/stun_baton/baton_barrel3.md3'):assert name in files
assert not restored_pickups['weapon_jawa']['reason'] and not restored_pickups['item_enviro']['reason']
assert 'models/weapons2/jawa/jawa_gun.md3' not in files
assert not restored_pickups['item_datapad']['reason'] and restored_pickups['item_datapad']['icon']=='models/items/datapad_screen'
assert 'models/items/datapad.md3' in files and 'models/items/datapad_screen.jpg' in files
for asset in ('models/map_objects/mp/shield.md3','gfx/hud/i_icon_shieldwall.tga','gfx/effects/mp_b_forcefield','gfx/effects/mp_b_forcefield_d','gfx/effects/mp_b_forcefield_d1'):
 assert asset in files or any(asset+ext in files for ext in ('.tga','.jpg','.png')),asset
# Both native view/hand paths have the muzzle and weapon tags used by the renderer.
for model in ('briar_pistol.md3','briar_pistol_hand.md3'):
 z,key=files['models/weapons2/briar_pistol/'+model];b=z.read(key)
 frames,tags,surfaces=struct.unpack_from('<3i',b,76);off=struct.unpack_from('<i',b,96)[0]
 assert b[:4]==b'IDP3' and frames>0 and tags>=2 and (surfaces>0 if model=='briar_pistol.md3' else True)
 names={b[off+i*112:off+i*112+64].split(b'\0',1)[0] for i in range(tags)}
 assert {b'tag_flash',b'tag_weapon'}<=names
for asset in ('gfx/hud/w_icon_briar','gfx/hud/w_icon_briar_na','models/weapons2/briar_pistol/briar_pistol'):
 assert any(asset+ext in files for ext in ('.jpg','.tga','.png')),asset
assert any('sound/weapons/bryar/select'+ext in files for ext in ('.wav','.mp3'))
for effect in ('muzzle_flash','shot','NPCshot','wall_impact','flesh_impact'):
 assert ('effects/bryar/'+effect+'.efx').lower() in files
print('PASS: 12 repaired retail pickups, Jawa world/view/hand model tags, stock icons and Bryar effects')
legacyids=set(legacyactors.values())
walker=byname['misc_atst_drivable'];native_walker=byid[walker['alias']]
assert not walker['reason'] and native_walker['name']=='atst_vehicle' and native_walker['kind']==3
assert not native_walker['reason'] and native_walker['npcHash'] and native_walker['vehicleHash']
for e in entries:
 assert 0<e['id']<0x7fffffff
 if e['alias']:assert e['alias'] in legacyids or (byid[e['alias']]['kind'] in (2,3) and not byid[e['alias']]['reason'])
 if e['kind']==1 and not e['reason']:
  assert e['model'].lower() in files and all(math.isfinite(e[k]) for k in ('radius','height','bottom'))
  assert 0<e['radius']<=8192 and e['bottom']<e['height']
 if e['kind']==5 and not e['reason'] and not e['alias']:assert e['model'].lower() in files
assert len(entries)>1000 and len(actors)>190 and len(models)>700
presets=set(re.findall(r'\{"([^"\n]+)","[^"\n]*","[^"\n]*",[^}]+\}',(root/'code/qcommon/jar_entity_presets.h').read_text()))
portable={e['name']:e for e in entries if e['kind']==5 and e['name'] in presets}
assert len(portable)==len(presets)
turret=portable['misc_ns_turret']
assert not turret['reason'] and turret['model']=='models/map_objects/imp_mine/turret_canon.glm'
assert (turret['radius'],turret['bottom'],turret['height'])==(10,-30,0)
assert 'models/map_objects/nar_shaddar/turret/turret.glm' not in files
z,key=files[turret['model']];body=z.read(key)
animation=body[72:136].split(b'\0',1)[0].decode('ascii')+'.gla'
z,key=files[animation];rig=z.read(key);assert rig[:4]==b'2LGA'
bones=struct.unpack_from('<i',rig,84)[0]
bone_names={rig[100+struct.unpack_from('<i',rig,100+i*4)[0]:164+struct.unpack_from('<i',rig,100+i*4)[0]].split(b'\0',1)[0].decode('ascii') for i in range(bones)}
assert 'Bone_body' in bone_names
surfaces=struct.unpack_from('<i',body,152)[0];muzzle=False
for i in range(surfaces):
 offset=164+struct.unpack_from('<i',body,164+i*4)[0]
 if body[offset:offset+64].split(b'\0',1)[0]==b'*flash03':
  lod=struct.unpack_from('<i',body,148)[0];surface=lod+4+struct.unpack_from('<i',body,lod+4+i*4)[0]
  assert struct.unpack_from('<i',body,surface+12)[0]==3 and struct.unpack_from('<i',body,surface+20)[0]==1
  triangle=surface+struct.unpack_from('<i',body,surface+24)[0]
  assert set(struct.unpack_from('<3i',body,triangle))=={0,1,2}
  muzzle=True
assert muzzle
for sound in ('startup','shutdown','ping','move'):assert any(f'sound/chars/turret/{sound}'+ext in files for ext in ('.wav','.mp3'))
for asset in ('models/map_objects/imp_mine/turret_damage.md3','effects/turret/explode.efx','effects/sparks/spark_exp_nosnd.efx'):assert asset in files
print('PASS: NS turret portable route, stock body/skeleton, pitch bone, muzzle triangle, damaged body, sounds and effects')
assert not portable["misc_skyportal"]["reason"]
assert not portable["misc_cubemap"]["reason"] and portable["misc_cubemap"]["model"]=="models/map_objects/imperial/crate1.md3"
assert 'target_level_change' in portable and not portable['target_level_change']['reason']
for name in ('ref_tag','waypoint_navgoal','waypoint','waypoint_small'):assert name in portable and not portable[name]['reason']
assert 'maps/hoth2.bsp' in files and 'maps/t1_surprise.bsp' in files
for name,e in portable.items():
 assert not e['reason'],(name,e['reason'])
 assert e['model'].lower() in files
 assert math.isfinite(e['radius']) and e['radius']>0 and e['bottom']<e['height']
for name in ('effects/sparks/spark.efx','effects/env/targ_beam.efx','effects/env/targ_beam_impact.efx','sound/weapons/bryar/fire.mp3','models/items/key.md3',
 'models/map_objects/kejim/impcam_base.md3','sound/movers/switches/button_08.mp3','sound/movers/switches/button_09.mp3',
 'models/map_objects/kejim/sec_panel.md3','models/map_objects/imperial/barrels.md3','effects/thermal/explosion.efx'):
 assert name in files,name
assert any('sound/movers/objects/cameramove_lp2'+ext in files for ext in ('.mp3','.wav'))
for name in ('misc_model_gun_rack','misc_model_ammo_rack','misc_camera','trigger_entdist','misc_maglock','object_cargo_barrel1'):
 assert name in portable and not portable[name]['reason']
for name in ('misc_camera_focus','misc_camera_track'):
 assert name in portable and not portable[name]['reason'] and (portable[name]['radius'],portable[name]['bottom'],portable[name]['height'])==(8,-8,8)
assert 'models/map_objects/kejim/impcam.md3' in files
assert (portable['point_combat']['radius'],portable['point_combat']['bottom'],portable['point_combat']['height'])==(20,-24,40)
for name in ('trigger_space','trigger_bidirectional'):assert (portable[name]['radius'],portable[name]['bottom'],portable[name]['height'])==(64,0,128)
for name in ('fx_rain','fx_snow','fx_puff','fx_wind','fx_wind_zone','misc_weather_zone'):assert name in portable and not portable[name]['reason']
for name in ('gfx/world/rain.jpg','gfx/effects/snowflake1.tga','gfx/effects/alpha_smoke2b.tga'):assert name in files,name
assert 'fx_cloudlayer' in portable and 'textures/skies/cloudlayer2.png' in files
assert 'trigger_shipboundary' in portable and (portable['trigger_shipboundary']['radius'],portable['trigger_shipboundary']['bottom'],portable['trigger_shipboundary']['height'])==(512,0,512)
rack_pickups={'weapon_blaster','weapon_repeater','weapon_rocket_launcher','ammo_blaster','ammo_metallic_bolts','ammo_powercell','item_medpak_instant'}
for body in re.findall(r'\{([^{}]*)\}',clean(text('ext_data/items.dat'))):
 fields=dict((k.lower(),v.strip('"')) for k,v in re.findall(r'(?m)^\s*(\w+)\s+("[^"\n]*"|[^\s]+)',body))
 if fields.get('classname') in rack_pickups:
  assert fields['worldmodel'].lower() in files,fields
  rack_pickups.remove(fields['classname'])
assert not rack_pickups,rack_pickups
music_paths=set(re.findall(r'^ "([^"\n]+)",$',(root/'code/qcommon/jar_music_tracks.h').read_text(),re.M))
assert music_paths=={name for name in files if name.startswith('music/') and name.endswith(('.mp3','.wav'))}
assert len(music_paths)>100 and 'music/hoth2/hoth2_explore.mp3' in music_paths
script_paths=set(re.findall(r'^ "([^"\n]+)",$',(root/'code/qcommon/jar_script_catalog.h').read_text(),re.M))
assert script_paths=={name[8:-4] for name in files if name.startswith('scripts/') and name.endswith('.ibi')}
assert len(script_paths)>1000 and 'common/dooropenedelsewhere' in script_paths
z,key=files['scripts/common/dooropenedelsewhere.ibi'];default_script=z.read(key)
assert default_script[:8]==b'IBI\0'+struct.pack('<f',1.57) and len(default_script)==80
assert b'CHAN_VOICE\0' in default_script and b'sound/chars/jaden_male/12jak001.mp3\0' in default_script
assert 'sound/chars/jaden_male/12jak001.mp3' in files
assert 'target_scriptrunner' in portable
print(f'PASS: {len(script_paths)} retail script paths, native IBI default voice-line asset and portable runner route')
assert 'target_play_music' in portable and 'target_change_parm' in portable
print(f'PASS: {len(music_paths)} retail music paths and portable music controller route')
assert not next(e for e in entries if e['name']=='item_security_key')['reason']
print(f'PASS: complete retail name coverage: {len(models)} models, {len(actors)} NPC definitions, {len(vehicles)} vehicles, {len(items)} pickups, {len(classes)} entity classes; alias targets and placement metadata checked')
print(f'PASS: {len(portable)} portable entity presets, helper key, stock models, effect and sound resources')
# Ghoul2 candidates must have the native mesh/skeleton structure required by
# the posed-bounds adapter. This checks assets, not rendered/gameplay poses.
ghoul_count=0
for e in entries:
 if e['kind']!=1 or not e['model'].endswith('.glm') or e['reason']:continue
 z,key=files[e['model']];b=z.read(key);assert b[:4]==b'2LGM' and e['rigid']
 bones,lods,off_lods,surfaces,off_hierarchy,end=struct.unpack_from('<6i',b,140)
 assert 0<bones<=512 and lods>0 and 0<surfaces<=512 and end==len(b)
 anim=b[72:136].split(b'\0',1)[0].decode('ascii').lower()
 assert anim=='*default' or anim+'.gla' in files,(e['model'],anim)
 pending=[0];visited=set();vertices=0
 while pending:
  index=pending.pop();assert 0<=index<surfaces and index not in visited;visited.add(index)
  off=164+struct.unpack_from('<i',b,164+index*4)[0]
  flags=struct.unpack_from('<I',b,off+64)[0];children=struct.unpack_from('<i',b,off+140)[0]
  assert 0<=children<=surfaces
  if not flags:
   surf=off_lods+4+struct.unpack_from('<i',b,off_lods+4+index*4)[0]
   _,surface_index,_,count,off_verts,_,_,refs,off_refs,_=struct.unpack_from('<10i',b,surf)
   assert surface_index==index and 0<=count<=250000-vertices and 0<refs<=32
   references=struct.unpack_from('<'+str(refs)+'i',b,surf+off_refs)
   assert all(0<=r<bones for r in references)
   for j in range(count):
    pos=surf+off_verts+j*32;packed=struct.unpack_from('<I',b,pos+24)[0]
    assert all(math.isfinite(x) for x in struct.unpack_from('<3f',b,pos+12))
    assert all(((packed>>(5*k))&31)<refs for k in range((packed>>30)+1))
   vertices+=count
  if not flags&256:pending.extend(struct.unpack_from('<'+str(children)+'i',b,off+144))
 assert vertices>0,(e['model'],'No visible default surfaces')
 ghoul_count+=1
print(f'PASS: {ghoul_count} Ghoul2 candidates have visible geometry, bounded surface/vertex counts, valid bone references and stock/built-in skeletons')
