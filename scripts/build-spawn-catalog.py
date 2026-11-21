"""Build the private retail spawn index; records contain metadata, never model bytes.
Stable path/name-derived IDs preserve saves when further assets are added.
Run with --check during release packaging to detect a stale generated index.
"""
from pathlib import Path
import argparse,json,re,struct,zipfile,math,collections
from jar_catalog_assets import catalog_asset_paths
ROOT=Path(__file__).resolve().parents[1]
def tokens(text):
 return [m.group(1) if m.group(1) is not None else m.group(2) for m in re.finditer(r'//[^\n]*|/\*[\s\S]*?\*/|"([^"\n]*)"|([^\s{}]+|[{}])',text) if not m.group(0).startswith(('//','/*'))]
def blocks(text,anonymous=False):
 t=tokens(text);i=0
 while i<len(t):
  name='' if anonymous else t[i];i+=0 if anonymous else 1
  if i>=len(t) or t[i]!='{':raise ValueError((name,t[i:i+5]))
  i+=1;body=[]
  while i<len(t) and t[i]!='}':body.append(t[i]);i+=1
  if i==len(t):raise ValueError('Unclosed '+name)
  i+=1
  # Values used here are single-token except min/max, parsed separately below.
  # Keys consumed here are independent of vector-valued NPC fields such as
  # customRGBA. Those must not shift the rest of a definition's key/value pairs.
  keys={'playermodel','headmodel','torsomodel','legsmodel','customskin','class','width','height','model','length','type','worldmodel','icon','classname','name','tag','itemname'}
  fields={body[j].lower():body[j+1] for j in range(len(body)-1) if body[j].lower() in keys}
  yield name.lower(),body,fields

def fnv(data):
 h=2166136261
 for b in data:h=((h^b)*16777619)&0xffffffff
 return h

def legacy_actor(d,files):
 """Match the SP native segmented renderer's model/skin/animation requirements."""
 lower=''
 for key,part in (('legsmodel','lower'),('torsomodel','upper'),('headmodel','head')):
  value=d.get(key,'')
  if not value or value.lower()=='none':
   if part=='lower':return lower,'Retail definition has no legs model.'
   continue
  pieces=value.split('/');name=pieces[0];skin=pieces[1] if len(pieces)==2 else 'default'
  if len(pieces)>2 or not all(re.fullmatch(r'[A-Za-z0-9_-]+',s) for s in (name,skin)):
   return lower,'Legacy model or skin name is unsupported.'
  base='models/players/'+name+'/'+part;skinpath=base+'_'+skin+'.skin'
  if len(skinpath)>=64 or (part=='lower' and len(f'models/players/{name}/animevents.cfg')>=64):
   return lower,'Legacy resource path exceeds the native limit.'
  model=base+'.mdr' if part!='head' and (base+'.mdr').lower() in files else base+'.md3'
  if part=='lower':lower=model
  if model.lower() not in files:return lower,'Retail definition references a missing legacy model.'
  if (part!='lower' or skin.lower()!='default') and skinpath.lower() not in files:return lower,'Retail definition references a missing legacy part skin.'
  if part=='lower' and ('models/players/'+name+'/animation.cfg').lower() not in files:
   return lower,'Retail definition references a missing legacy animation file.'
 return lower,''

def repaired_legacy_legs(name,d,files):
 """Same missing-only resource policy used before native NPC registration."""
 rules=re.findall(r'\{"([^"\n]+)","([^"\n]+)","([^"\n]+)"\}',(ROOT/'code/qcommon/jar_npc_resources.h').read_text(encoding='utf-8'))
 for npc,original,replacement in rules:
  if name.lower()!=npc or d.get('legsmodel','').lower()!=original:continue
  if any(f'models/players/{original}/lower.{ext}' in files for ext in ('md3','mdr')):continue
  if all(f'models/players/{replacement}/{part}' in files for part in ('lower.md3','animation.cfg')):
   return dict(d,legsmodel=replacement)
 return d

def repaired_legacy_body(name,d,files):
 """Resolve only the exact missing stock Ghoul2 body and default skin."""
 rules=re.findall(r'\{"([^"\n]+)","([^"\n]+)","([^"\n]+)","([^"\n]+)"\}',(ROOT/'code/qcommon/jar_npc_resources.h').read_text(encoding='utf-8'))
 for npc,original,skin,replacement in rules:
  if (name.lower(),d.get('playermodel','').lower(),d.get('customskin','default').lower())!=(npc,original,skin):continue
  if f'models/players/{original}/model.glm' in files:continue
  if all(f'models/players/{replacement}/{part}' in files for part in ('lower.md3','animation.cfg')):
   result=dict(d,headmodel='none',torsomodel='none',legsmodel=replacement);result.pop('playermodel');return result
 return d


def main():
 p=argparse.ArgumentParser();p.add_argument('--game-data',type=Path,required=True);p.add_argument('--check',action='store_true');a=p.parse_args()
 files={};packs=[]
 for path in catalog_asset_paths(a.game_data):
  z=zipfile.ZipFile(path);packs.append(z)
  for n in z.namelist():files[n.lower()]=(z,n)
 def data(n):z,k=files[n.lower()];return z.read(k)
 def icon(n,fallback):return n if any(n.lower()+ext in files for ext in ('.tga','.jpg','.png')) else fallback
 source=(ROOT/'code/game/g_workshop.cpp').read_text(encoding='utf-8-sig')
 oldprops=set(re.findall(r'\{10\d\d,"[^"]+","([^"]+)"',source))
 oldactors=set(re.findall(r'\{[34]00\d,"([^"]+)"|\{301[012],"([^"]+)"',source));oldactors={n for pair in oldactors for n in pair if n}
 olditems={'item_medpak_instant','item_shield_sm_instant','item_battery','item_shield_lrg_instant','weapon_blaster_pistol','weapon_blaster','weapon_disruptor','weapon_bowcaster','weapon_repeater','weapon_demp2','weapon_flechette','weapon_rocket_launcher','weapon_thermal','weapon_trip_mine','weapon_det_pack','weapon_concussion_rifle'}
 policy=json.loads((ROOT/'docs/JAR-catalog-exclusions.json').read_text(encoding='utf-8'))
 excluded_names=set(policy['models']);excluded=[]
 assert policy['schema']==1 and len(excluded_names)==len(policy['models'])==21
 assert excluded_names<=files.keys() and not excluded_names&oldprops,'Approved helper set requires requalification'
 entries=[]
 def add(kind,tab,name,**kw):
  e=dict(id=(kind<<24)|(fnv(name.lower().encode())&0xffffff),kind=kind,tab=tab,name=name,model='',icon='gfx/hud/i_icon_ammodisp',bytes=0,hash=0,npcHash=0,vehicleHash=0,npcAlternate=0,radius=16,height=40,bottom=-24,rigid=False,reason='',itemType='IT_BAD',alias=0)
  e.update(kw);entries.append(e);return e
 for n in sorted(files):
  if not n.endswith(('.md3','.glm')) or n in oldprops:continue
  b=data(n);reason='';rigid=False;radius=16;height=40;bottom=-24
  if n in excluded_names and n.endswith('.md3'):
   assert n.endswith('.md3') and len(b)>=108 and b[:4]==b'IDP3' and struct.unpack_from('<i',b,4)[0]==15,n
   frames,tags,surfaces=struct.unpack_from('<3i',b,76)
   tag_offset=struct.unpack_from('<i',b,96)[0];end=struct.unpack_from('<i',b,104)[0]
   assert frames>0 and tags>0 and surfaces==0 and end==len(b) and tag_offset>=108 and tag_offset+frames*tags*112<=end,'Approved helper now has different geometry: '+n
   excluded.append(dict(id=(1<<24)|(fnv(n.encode())&0xffffff),name=n,bytes=len(b),hash=fnv(b),frames=frames,tags=tags,surfaces=surfaces,reason='Owner-excluded tag-only attachment helper; no visible surfaces.'))
   continue
  if n.endswith('.md3'):
   try:
    if len(b)<108 or b[:4]!=b'IDP3':raise ValueError()
    frames=struct.unpack_from('<i',b,76)[0];off=struct.unpack_from('<i',b,92)[0]
    if frames<1 or off<108 or off+56>len(b):raise ValueError()
    lo=struct.unpack_from('<3f',b,off);hi=struct.unpack_from('<3f',b,off+12)
    if not all(math.isfinite(x) and abs(x)<=8192 for x in lo+hi) or any(lo[i]>hi[i] for i in range(3)) or struct.unpack_from('<i',b,84)[0]<1:raise ValueError()
    radius=max(.5,*(abs(x) for x in lo[:2]+hi[:2]));bottom=lo[2];height=max(bottom+1,hi[2])
    rigid=all(0<=hi[i]-lo[i]<=1024 and abs((hi[i]+lo[i])*.5)<=512 for i in range(3))
   except (ValueError,struct.error):reason='Model has no visible surfaces or usable world-space frame bounds (tag/viewmodel helper).'
  else:
   # Ghoul2 candidates use visible posed vertices at runtime; oversized models
   # retain native motion instead of entering the rigid solver.
   rigid=True
   if len(b)<164 or b[:4]!=b'2LGM':reason='Model has no valid Ghoul2 header.'
   else:
    # A native no-weapon placeholder has no visible geometry. Do not advertise
    # an invisible handle as a successfully spawnable decorative prop.
    try:
     surfaces=struct.unpack_from('<i',b,152)[0];lod=struct.unpack_from('<i',b,148)[0]
     pending=[0];seen=set();visible=0
     while pending:
      index=pending.pop()
      if not 0<=index<surfaces or index in seen:raise ValueError()
      seen.add(index);off=164+struct.unpack_from('<i',b,164+index*4)[0]
      flags=struct.unpack_from('<I',b,off+64)[0];children=struct.unpack_from('<i',b,off+140)[0]
      if not 0<=children<=512:raise ValueError()
      if not flags:
       surf=lod+4+struct.unpack_from('<i',b,lod+4+index*4)[0];visible+=struct.unpack_from('<i',b,surf+12)[0]
      if not flags&256:pending.extend(struct.unpack_from('<'+str(children)+'i',b,off+144))
     if visible<=0:reason='Empty no-weapon/helper model has no visible default-pose geometry.';rigid=False
    except (ValueError,struct.error):reason='Model has no usable Ghoul2 surface hierarchy.';rigid=False
  if n in excluded_names:
   assert n=='models/weapons2/noweap/noweap.glm' and b[:4]==b'2LGM' and visible==0 and reason=='Empty no-weapon/helper model has no visible default-pose geometry.',n
   excluded.append(dict(id=(1<<24)|(fnv(n.encode())&0xffffff),name=n,bytes=len(b),hash=fnv(b),surfaces=surfaces,visibleTriangles=visible,reason='Owner-excluded empty no-weapon placeholder; no visible default-pose geometry.'))
   continue
  add(1,0,n,model=n,icon=icon(n.removesuffix('/model.glm')+'/icon_default','gfx/hud/i_icon_ammodisp'),bytes=len(b),hash=fnv(b),radius=radius,height=height,bottom=bottom,rigid=rigid,reason=reason)
 defs={};vehicles={};npcAlternates={}
 for ext,out in (('.npc',defs),('.veh',vehicles)):
  for n in sorted(files):
   if n.endswith(ext) and n.startswith('ext_data/'):
    for name,body,d in blocks(data(n).decode('latin1')):
     fingerprint=fnv(b''.join(t.encode('latin1')+b'\0' for t in body))
     if ext=='.npc' and name in out and out[name][1]!=fingerprint:npcAlternates[name]=fingerprint
     out.setdefault(name,(d,fingerprint))
 # The native resolver fills only missing retail color definitions, retaining
 # actual stock geometry/skins and either qualified retail Swoop behavior block.
 variants=dict(re.findall(r'\{"(swoop_[^"]+)","([^"]+)"\}',(ROOT/'shared/qcommon/jar_vehicle_variants.h').read_text()))
 assert len(variants)==7
 if vehicles.get('swoop',({},0))[1] in (710687386,140738443):
  for name,skin in variants.items():
   if name not in vehicles and ('models/players/swoop/model_'+skin+'.skin') in files:
    vehicles[name]=vehicles['swoop']
 def num(d,key,default):
  try:return float(d.get(key,default))
  except ValueError:return default
 for name,(d,h) in sorted(defs.items()):
  veh=name in vehicles or d.get('class')=='CLASS_VEHICLE'
  if name in oldactors:continue
  if not veh:d=repaired_legacy_body(name,d,files)
  vd,vh=vehicles.get(name,({},0));model=vd.get('model',d.get('playermodel','')) if veh else d.get('playermodel','')
  if model.lower()=='player':model='kyle'
  model='models/players/'+model+'/model.glm' if model else ('models/players/kyle/model.glm' if name=='player' else '')
  reason=''
  if not veh and not model and d.get('legsmodel'):model,reason=legacy_actor(repaired_legacy_legs(name,d,files),files)
  elif not model or model.lower() not in files:reason='Retail definition references a missing model.'
  elif veh and not vh:reason='Vehicle has no matching retail vehicle definition.'
  elif name=='test':reason='Native test template relocates to the player; use player instead.'
  radius=max(1,min(2048,num(d,'width',16)));height=max(1,min(4096,num(d,'height',64)))
  if d.get('class') in ('CLASS_GALAKMECH','CLASS_GALAK_MECH'):radius=max(radius,60);height=max(height,104)
  if veh:radius=max(radius,num(vd,'width',radius*2)/2,num(vd,'length',radius*2)/2);height=max(height,num(vd,'height',height))
  portrait=icon(model.removesuffix('/model.glm')+'/icon_'+d.get('customskin','default'),'jar/icons/r2' if not veh else 'gfx/hud/w_icon_atst')
  add(3 if veh else 2,4 if veh else 1,name,model=model,npcHash=h,npcAlternate=npcAlternates.get(name,0),vehicleHash=vh,radius=radius,height=height,bottom=-24,icon=portrait,reason=reason)
 for name,(d,h) in sorted(vehicles.items()):
  if name in defs or name in oldactors:continue
  model='models/players/'+d.get('model','')+'/model.glm'
  add(3,4,name,model=model,vehicleHash=h,reason='Retail vehicle has no matching NPC definition.',icon='gfx/hud/w_icon_atst')
 itemFallbacks={name:(original,model,kind,tag) for name,original,model,kind,tag in re.findall(r'\{"([^"]+)","([^"]+)","([^"]+)",(\w+),(\w+)\}',(ROOT/'code/qcommon/jar_item_resources.h').read_text())}
 for name,body,d in blocks(data('ext_data/items.dat').decode('latin1'),True):
  name=d.get('classname','')
  if not name or name in olditems:continue
  model=d.get('worldmodel','');replacement=itemFallbacks.get(name)
  tag={'ITM_SHIELD_PICKUP':'INV_PORTABLE_SHIELD','ITM_DATAPAD_PICKUP':'INV_DATAPAD'}.get(d.get('tag'),d.get('tag'))
  if model.lower() not in files and replacement and (model,d.get('type'),tag)==(replacement[0],replacement[2],replacement[3]) and replacement[1] in files:model=replacement[1]
  reason='' if model.lower() in files else 'Pickup references a missing retail model.'
  if name=='item_enviro' and (d.get('type'),d.get('tag'))!=('IT_HOLDABLE','ITM_SHIELD_PICKUP'):reason='Shield pickup has an incompatible type or inventory tag.'
  if d.get('type')=='IT_HOLDABLE' and tag not in {'INV_ELECTROBINOCULARS','INV_BACTA_CANISTER','INV_SEEKER','INV_LIGHTAMP_GOGGLES','INV_SENTRY','INV_GOODIE_KEY','INV_SECURITY_KEY','INV_PORTABLE_SHIELD','INV_DATAPAD'}:reason='Pickup has no usable singleplayer inventory slot or controller.'
  add(4,2 if d.get('type')=='IT_WEAPON' else 3,name,model=model,icon=icon(d.get('icon') or ('models/items/datapad_screen' if name=='item_datapad' else ''),'gfx/hud/i_icon_ammodisp'),itemType=d.get('type','IT_BAD'),reason=reason)
 boon=dict(re.findall(r'constexpr char (\w+)\[\]="([^"\n]+)";',(ROOT/'code/qcommon/jar_force_boon.h').read_text()))
 assert boon['model'] in files and boon['sound'] in files and icon(boon['icon'],'')
 add(4,3,boon['classname'],model=boon['model'],icon=boon['icon'],itemType='IT_POWERUP')
 enlightenment=(ROOT/'code/qcommon/jar_force_enlightenment.h').read_text()
 pickups=re.findall(r'\{"(item_force_enlighten_\w+)", "[^"\n]+", "([^"\n]+)", "([^"\n]+)", \w+Pickup\}',enlightenment)
 assert len(pickups)==2
 assert re.search(r'constexpr char sound\[\] = "([^"\n]+)";',enlightenment)[1] in files
 for name,model,image in pickups:
  assert model in files and icon(image,'')
  add(4,3,name,model=model,icon=image,itemType='IT_POWERUP')
 legacyActors={m.group(2).lower():int(m.group(1)) for m in re.finditer(r'\{([34]\d{3}),"([^"]+)"',source)}
 actorEntries={e['name']:e for e in entries if e['kind'] in (2,3)}
 npcSource=(ROOT/'code/game/NPC_spawn.cpp').read_text()
 spawnsource=(ROOT/'code/game/g_spawn.cpp').read_text();table=spawnsource.split('spawn_t\tspawns[] = {',1)[1].split('{NULL, NULL}',1)[0]
 # These point entities have complete native defaults and do not require brush geometry, targets or map scripts.
 standalone={
 'misc_exploding_crate':'models/map_objects/nar_shaddar/crate_xplode.md3',
 'misc_gas_tank':'models/map_objects/imp_mine/tank.md3',
 'misc_crystal_crate':'models/map_objects/imp_mine/crate_open.md3',
 'misc_model_shield_power_converter':'models/items/psd_big.md3',
 'misc_model_ammo_power_converter':'models/items/power_converter.md3',
 'misc_shield_floor_unit':'models/items/a_shield_converter.md3',
 'misc_ammo_floor_unit':'models/items/a_pwr_converter.md3',
 'misc_model_bomb_planted':'models/map_objects/factory/bomb_new_deact.md3',
 'misc_model_beacon':'models/map_objects/wedge/beacon.md3',
 'misc_model_welder':'models/map_objects/cairn/welder.glm',
 'misc_model_jabba_cam':'models/map_objects/nar_shaddar/jabacam/jabacam.glm',
 'emplaced_eweb':'models/map_objects/hoth/eweb_model.glm',
 'emplaced_gun':'models/map_objects/imp_mine/turret_chair.glm'}
 presetSource=(ROOT/'code/qcommon/jar_entity_presets.h').read_text()
 presets={m[0]:m[2] or 'models/map_objects/imperial/crate1.md3' for m in re.findall(r'\{"([^"\n]+)","([^"\n]*)","([^"\n]*)",[^}]+\}',presetSource)}
 for name,func in re.findall(r'^\s*\{"([^"]+)",\s*(\w+)\s*\}',table,re.M):
  reason='No portable controller yet: requires map rendering, navigation, scripts or an owned multi-entity adapter.'
  if name in standalone:reason='' if standalone[name] in files else 'Native entity references a model absent from retail Jedi Academy.'
  if name=='trigger_bidirectional':reason='Native implementation is unfinished; needs a functional replacement controller.'
  if name in ('misc_ns_turret','misc_spotlight','misc_model_welder','misc_model_jabba_cam','misc_crystal_crate'):reason='Native entity references a model absent from retail Jedi Academy.'
  if name in ('misc_camera','misc_model_gun_rack','misc_model_ammo_rack','misc_laser_arm'):reason='Native assembly needs ownership for its extra parts and delayed spawns before portable spawning.'
  alias=0
  if name.startswith('NPC_'):
   reason='Spawner alias requires a missing NPC definition or an explicit type; use the NPC/vehicle tab.'
   match=re.search(r'void\s+'+re.escape(func)+r'\s*\([^)]*\)\s*\{',npcSource)
   if match:
    begin=match.end();depth=1;end=begin
    while end<len(npcSource) and depth:
     depth+=(npcSource[end]=='{')-(npcSource[end]=='}');end+=1
    candidates=re.findall(r'NPC_type\s*=\s*"([^"\n]+)"',npcSource[begin:end])
    for candidate in reversed(candidates):
     candidate=candidate.lower();definition=actorEntries.get(candidate)
     if candidate in legacyActors:alias=legacyActors[candidate];break
     if definition and not definition['reason']:alias=definition['id'];break
   if alias:reason=''
  model=standalone.get(name,'');radius=24;height=64;bottom=-24
  if name in presets:
   model=presets[name];reason='' if model in files else 'Portable setup model is absent from retail Jedi Academy.'
   if not reason and model.endswith('.md3'):
    b=data(model);off=struct.unpack_from('<i',b,92)[0];lo=struct.unpack_from('<3f',b,off);hi=struct.unpack_from('<3f',b,off+12)
    radius=max(abs(x) for x in lo[:2]+hi[:2]);bottom=lo[2];height=hi[2]
   if name in ('fx_wind_zone','misc_weather_zone'):radius=256;bottom=0;height=256
   if name=='point_combat':radius=20;bottom=-24;height=40
   if name in ('waypoint','waypoint_small'):radius=8;bottom=-24;height=16
   if name in ('misc_camera_focus','misc_camera_track'):radius=8;bottom=-8;height=8
   if name in ('misc_turret','misc_ns_turret'):radius=10;bottom=-30;height=0
   if name=='misc_ion_cannon':radius=148;bottom=0;height=245
   if name=='misc_panel_turret':radius=8;bottom=-12;height=0
   if name=='misc_sentry_turret':radius=8;bottom=0;height=18
   if name=='misc_security_panel':radius=8;bottom=-8;height=8
   if name=='misc_maglock':radius=12;bottom=-12;height=12
   if name in ('trigger_space','trigger_bidirectional'):radius=64;bottom=0;height=128
   if name=='trigger_shipboundary':radius=512;bottom=0;height=512
   if name=='misc_camera':radius=8;bottom=-12;height=16
   if name=='misc_crystal_crate':radius=17;bottom=0;height=38
   if name=='misc_model_jabba_cam':radius=34;bottom=-34;height=34
   if name=='misc_model_welder':radius=16;bottom=-16;height=16
   if name=='misc_spotlight':radius=12;bottom=-12;height=12
   if name=='misc_laser_arm':radius=32;bottom=-32;height=16
   if name=='misc_portal_surface':radius=64;bottom=-96;height=96
   if name=='misc_portal_camera':radius=12;bottom=-96;height=12
  # Native aliases with explicit type selection now have complete retail presets.
  # Use the native vehicle controller for the old player-transform walker.
  # This avoids the legacy transform's model/inventory restoration path.
  fallback={'NPC_spawner':'stormtrooper','NPC_Galak':'galak','NPC_StormtrooperOfficer':'stofficer','misc_atst_drivable':'atst_vehicle'}
  if name in fallback:
   candidate=fallback[name];alias=legacyActors.get(candidate,actorEntries.get(candidate,{}).get('id',0))
   if alias:reason=''
  add(5,3,name,alias=alias,model=model,radius=radius,height=height,bottom=bottom,reason=reason,icon='gfx/mp/siegeicons/korriban/blue_crystal' if name in ('misc_crystal_crate','misc_siege_item') else 'gfx/hud/i_icon_zoom' if name=='misc_model_jabba_cam' else 'gfx/hud/w_icon_demp2' if name=='misc_model_welder' else 'gfx/hud/w_icon_atst' if 'emplaced' in name else 'gfx/hud/i_icon_ammodisp')
 # Additional cards are distinct qualified stock map sections, not native class aliases.
 section_profiles=re.findall(r'\{"([^"\n]+)","#([^"\n]+)","([^"\n]+)",Mode::\w+\}',(ROOT/'code/game/g_bsp_section.h').read_text())
 assert section_profiles and section_profiles[0][:2]==('misc_bsp','mp/duel10')
 for name,resource,title in section_profiles:
  model='maps/'+resource+'.bsp';icon='levelshots/'+resource
  assert model in files and any(icon+ext in files for ext in ('.jpg','.tga','.png'))
  if name=='misc_bsp':
   existing=next(e for e in entries if e['kind']==5 and e['name']==name)
   existing.update(tab=5,model=model,icon=icon)
  else:
   assert name==title
   add(5,5,name,model=model,icon=icon)
 ids=[e['id'] for e in entries]
 if len(set(ids))!=len(ids):raise ValueError('Stable catalog ID collision')
 assert len(excluded)==21
 # Exact owner-approved unavailable content only. Keep its identity and evidence
 # in the audit while removing menu cards; never filter missing assets broadly.
 unavailable=policy['unavailableEntries'];excluded_content=[]
 keys={(e['kind'],e['name']) for e in unavailable}
 assert len(unavailable)==len(keys)==7
 bykey={(e['kind'],e['name']):e for e in entries}
 for approved in unavailable:
  e=bykey[(approved['kind'],approved['name'])]
  assert e['reason'],'Approved unavailable entry now needs requalification: '+e['name']
  if 'missingNpc' in approved:
   assert e['kind']==5 and approved['missingNpc'] not in defs and not e['alias'],e['name']
  else:
   assert e['kind']==2 and e['model']==approved['missingModel'] and e['model'].lower() not in files,e['name']
  excluded_content.append(dict(e,ownerDecision=policy['unavailableContentDecision'],decisionDate=policy['unavailableContentDecisionDate']))
 entries=[e for e in entries if (e['kind'],e['name']) not in keys]
 # Keep the separately approved disabled system in the audit, not the menu.
 disabled=policy['disabledEntries'];assert disabled==[dict(kind=5,name='terrain',reason='Disabled legacy native terrain system; landscape dependencies and retail terrain definitions/heightmaps are absent.')]
 excluded_disabled=[]
 for approved in disabled:
  e=bykey[(approved['kind'],approved['name'])];assert e['reason'] and not e['model'] and not e['alias']
  excluded_disabled.append(dict(e,reason=approved['reason'],ownerDecision=policy['disabledSystemDecision'],decisionDate=policy['disabledSystemDecisionDate']))
 entries=[e for e in entries if (e['kind'],e['name']) not in {(d['kind'],d['name']) for d in disabled}]
 entries.sort(key=lambda e:e['id'])
 def q(s):return json.dumps(s,ensure_ascii=True)
 def f(v):
  value=format(v,'.8g');return value+('f' if '.' in value or 'e' in value else '.0f')
 out=['// Generated by scripts/build-spawn-catalog.py from installed retail metadata. GPL-2.0-only.','#ifndef JAR_CATALOG_GENERATED_H','#define JAR_CATALOG_GENERATED_H','#include <cstdint>','#include <cstddef>','namespace jar { namespace catalog {','struct Entry {uint32_t id; int kind,tab; const char *name,*model,*icon; int bytes; uint32_t hash,npcHash,vehicleHash; float radius,height,bottom; bool rigid; const char *reason,*itemType; uint32_t npcAlternate,alias;};','static const Entry entries[]={']
 for e in entries:
  out.append(' {'+','.join([str(e['id'])+'u',str(e['kind']),str(e['tab']),q(e['name']),q(e['model']),q(e['icon']),str(e['bytes']),str(e['hash'])+'u',str(e['npcHash'])+'u',str(e['vehicleHash'])+'u',f(e['radius']),f(e['height']),f(e['bottom']),str(e['rigid']).lower(),q(e['reason']),q(e['itemType']),str(e['npcAlternate'])+'u',str(e['alias'])+'u'])+'},')
 out+=['};','inline const Entry* Find(uint32_t id){size_t lo=0,hi=sizeof(entries)/sizeof(entries[0]);while(lo<hi){const size_t m=(lo+hi)/2;if(entries[m].id<id)lo=m+1;else hi=m;}return lo<sizeof(entries)/sizeof(entries[0])&&entries[lo].id==id?&entries[lo]:nullptr;}','}}','#endif','']
 output='\n'.join(out);dest=ROOT/'code/qcommon/jar_catalog_generated.h'
 report={'schema':1,'legacyCards':53,'additionalCards':len(entries),'additionalByKind':dict(collections.Counter(e['kind'] for e in entries)),'ownerExcluded':excluded,'ownerExcludedContent':excluded_content,'ownerExcludedDisabled':excluded_disabled,'blocked':[{k:e[k] for k in ('id','name','reason')} for e in entries if e['reason']],'entries':entries}
 reporttext=json.dumps(report,indent=2)+'\n';reportpath=ROOT/'docs/JAR-spawn-catalog.json'
 if a.check:
  assert dest.read_text()==output,'Generated header is stale'
  assert reportpath.read_text()==reporttext,'Catalog coverage report is stale'
 else:dest.write_text(output,encoding='utf-8');reportpath.write_text(reporttext,encoding='utf-8')
 music=sorted(n for n in files if n.startswith('music/') and n.endswith(('.mp3','.wav')))
 assert music and all(len(n)<64 and re.fullmatch(r'[a-z0-9_/.-]+',n) for n in music)
 assert all(n.startswith('music/') and n.endswith('.mp3') and len(n)<=39 for n in music), 'Music cue storage needs requalification'
 musictext='\n'.join(['// Generated retail music paths; no audio bytes. GPL-2.0-only.','#ifndef JAR_MUSIC_TRACKS_H','#define JAR_MUSIC_TRACKS_H','static const char* const jarMusicTracks[]={']+[' '+q(n)+',' for n in music]+['};','#endif',''])
 musicpath=ROOT/'code/qcommon/jar_music_tracks.h'
 if a.check:assert musicpath.read_text()==musictext,'Music path catalog is stale'
 else:musicpath.write_text(musictext,encoding='utf-8')
 scripts=sorted(n[8:-4] for n in files if n.startswith('scripts/') and n.endswith('.ibi'))
 assert scripts and all(len(n)<=127 and re.fullmatch(r'[a-z0-9_/-]+',n) for n in scripts)
 scripttext='\n'.join(['// Generated retail script paths; no compiled script bytes. GPL-2.0-only.','#ifndef JAR_SCRIPT_CATALOG_H','#define JAR_SCRIPT_CATALOG_H','namespace jar { namespace scriptRunner {','static const char* const scripts[]={']+[' '+q(n)+',' for n in scripts]+['};','constexpr unsigned scriptCount=sizeof(scripts)/sizeof(scripts[0]);','}}','#endif',''])
 scriptpath=ROOT/'code/qcommon/jar_script_catalog.h'
 if a.check:assert scriptpath.read_text()==scripttext,'Script path catalog is stale'
 else:scriptpath.write_text(scripttext,encoding='utf-8')
 print(f"Catalog: {len(entries)+53} cards; additional kinds {report['additionalByKind']}; {len(report['blocked'])} entries have explicit setup/missing-asset reasons; {len(excluded)} owner-excluded empty helpers; {len(excluded_content)} owner-excluded unavailable content entries")
if __name__=='__main__':main()
