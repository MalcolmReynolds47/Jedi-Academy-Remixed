"""Derive MP choices from the qualified retail SP resource catalog."""
import csv
import pathlib
import re
import json
root = pathlib.Path(__file__).resolve().parents[1]
rows = []
entries = {}
aliases = []
legacy_labels = {}
items = (root / 'codemp/game/bg_misc.c').read_text()
presets = (root / 'code/qcommon/jar_entity_presets.h').read_text()
logic_markers = {name for name, flags in re.findall(r'^\s*\{"([^"]+)","[^"]*","",([^}]+)\}', presets, re.M)
                 if 'Marker' in flags.split('|')}
for line in (root / 'code/qcommon/jar_catalog_generated.h').read_text().splitlines():
    if not line.startswith(' {'):
        continue
    row = next(csv.reader([line.strip()[1:-2]], skipinitialspace=True))
    key, kind, tab = int(row[0].rstrip('u')), int(row[1]), int(row[2])
    name, model, icon = row[3:6]
    radius, height, bottom = [float(v.rstrip('f')) for v in row[10:13]]
    entries[key] = row
    if kind==5 and not row[14] and name in ('misc_portal_surface','misc_portal_camera','func_rotating','func_pendulum','func_static','func_group','trigger_space'):
        rows.append((key,kind,tab,name,model,icon,radius,height,bottom))
    if kind==5 and not row[14] and name in ('target_print','target_relay','target_delay','target_counter','target_position','target_push','target_teleporter','misc_teleporter_dest','target_speaker','fx_runner','target_activate','target_deactivate','target_laser','target_random','func_timer','info_null','info_notnull','info_player_start','info_player_deathmatch','target_give','target_score','target_kill','target_gravity_change','target_friction_change','trigger_multiple','trigger_once','trigger_hurt','trigger_push','trigger_teleport','misc_teleporter','misc_ammo_floor_unit','misc_shield_floor_unit','misc_model_shield_power_converter','misc_model_ammo_power_converter','func_usable','func_wall','func_door','func_plat','func_bobbing','func_button','func_breakable','func_glass','misc_model_breakable','object_cargo_barrel1','misc_exploding_crate','misc_gas_tank','shooter_plasma','shooter_grenade','shooter_rocket','misc_weapon_shooter','misc_model','misc_model_static','misc_model_ghoul','target_location','trigger_location','trigger_always','trigger_visible','trigger_entdist','trigger_bidirectional','misc_model_bomb_planted','misc_model_beacon','misc_model_cargo_small','misc_model_gun_rack','misc_model_ammo_rack','misc_model_jabba_cam','misc_model_welder','misc_spotlight','misc_crystal_crate','emplaced_gun','emplaced_eweb','light','misc_dlight','misc_cubemap','target_play_music','Imperial Control Room','misc_bsp','Rancor Pit','Generator Room','Taspir Landing','Bespin Courtyard','Abandoned City','Vjun Sentinel','Hoth Canyon','Rift Sanctuary','Imperial Shaft','Korriban FFA','Yavin Training Grounds'):
        if name == 'misc_bsp':
            name = 'Vjun Fuel Processing'
        if name == 'func_door':
            model = 'models/map_objects/factory/f_door_b.md3'
            radius, height, bottom = 96., 170., 0.
        if name == 'emplaced_gun':
            model = 'models/map_objects/mp/turret_chair.glm'
            radius, height, bottom = 30., 52., 8.
        if name == 'misc_weapon_shooter':
            icon = 'gfx/hud/w_icon_blaster'
        rows.append((key,kind,tab,name,model,icon,radius,height,bottom))
    if row[14] or kind == 5:
        continue
    # The four original static droid actors use exact stock geometry adapted
    # to Ghoul2. Native MP still parses their original behavior definitions.
    droids={'mouse':('mouse','jar_mouse'),'remote_sp':('remote_sp','jar_remote'),
            'remote':('remote_sp','jar_remote'),'seeker':('remote_sp','jar_remote')}
    if kind==2 and name in droids and model==f'models/players/{droids[name][0]}/lower.md3':
        model=f'models/players/{droids[name][1]}/model.glm'
    if kind in (2, 3) and not model.endswith('.glm'):
        continue
    # Actual per-frame MD3 bounds are validated at spawn; the rigid solver
    # supports large props. GLM uses shared renderer root poses and visible bounds.
    if kind == 1 and not model.endswith(('.md3', '.glm')):
        continue
    if kind == 4 and ('"' + name + '"') not in items:
        continue
    # MP stock NPC definitions are qualified again by the native MP parser on spawn.
    # SP stock icon aliases ship in the same sandbox icon PK3 for both clients.
    rows.append((key, kind, tab, name, model, icon, radius, height, bottom))
# Carry over the curated SP cards and their stock menu art. The game name of
# a native actor is independent of the label shown in the spawn menu.
source = (root / 'code/game/g_workshop.cpp').read_text(encoding='utf-8-sig')
overlay = (root / 'code/ui/ui_jar_overlay.inc').read_text(encoding='utf-8-sig')
cards = {int(ident): (int(tab), label, icon) for tab, ident, label, icon in
         re.findall(r'\{([014]),(\d+),"([^"]+)","([^"]+)",nullptr\}', overlay)}
legacy_props = re.findall(r'\{(10\d{2}),"[^"]+","([^"]+)",\d+,\d+u,([\d.]+),([\d.]+)(?:,(-?[\d.]+))?\}', source)
assert {int(ident) for ident, *_ in legacy_props} == set(range(1001, 1021)), 'Legacy prop metadata changed'
for ident, model, radius, height, bottom in legacy_props:
    key = int(ident)
    tab, label, icon = cards[key]
    assert tab == 0 and model.endswith('.md3'), label
    rows.append((key, 1, tab, label, model, icon, float(radius), float(height), float(bottom or 0)))

# Keep map alias metadata separate from direct NPC/vehicle cards so aliases
# retain their original ID, label, art and native spawn route.
actors = {row[0]: row for row in rows if row[1] in (2, 3)}
legacy = re.findall(r'\{([34]\d{3}),"([^"]+)","([^"]+)",\d+u,\d+u,\d+u,([\d.]+),([\d.]+),(true|false)', source)
for ident, name, model, radius, height, vehicle in legacy:
    key = int(ident)
    kind, tab = (3, 4) if vehicle == 'true' else (2, 1)
    stock_icon = 'gfx/hud/w_icon_atst' if kind == 3 else 'gfx/hud/i_icon_ammodisp'
    actor = (key, kind, tab, name, model, stock_icon, float(radius), float(height), -24.)
    actors[key] = actor
    card_tab, label, icon = cards[key]
    assert card_tab == tab and model.endswith('.glm'), name
    rows.append((*actor[:5], icon, *actor[6:]))
    legacy_labels[key] = label
assert set(legacy_labels) == {*range(3001, 3013), 4001, 4002}, 'Legacy actor metadata changed'
legacy_labels[87961440] = 'Small cargo crate'
legacy_labels[96929416] = 'Gun rack'
legacy_labels[98056028] = 'Ammo rack'
legacy_labels[86873624] = 'Jabba door camera'
legacy_labels[89954407] = 'Welder'
legacy_labels[93844316] = 'Crystal crate'
legacy_labels[88708799] = 'Map model'
legacy_labels[98645765] = 'E-11 shooter'
legacy_labels[99343814] = 'Static map model'
legacy_labels[91157969] = 'Skeletal map prop'
legacy_labels[93292771] = 'Location volume'
legacy_labels[95482230] = 'Location marker'
legacy_labels[98286831] = 'Reflection probe'
legacy_labels[98040309] = 'Bobbing platform'
legacy_labels[92295601] = 'One-shot trigger'
legacy_labels[89857924] = 'Player start'
legacy_labels[88940811] = 'Deathmatch start'
legacy_labels[95709066] = 'Gravity change'
legacy_labels[92295820] = 'Friction change'
legacy_labels[89652619] = 'Directional trigger'
legacy_labels[95681750] = 'Searchlight'
legacy_labels[88142876] = 'Portal surface'
legacy_labels[97890286] = 'Portal camera'
legacy_labels[88599552] = 'Spinner'
legacy_labels[97415810] = 'Pendulum'
legacy_labels[87701002] = 'Space volume'
for key, kind, _, name, *_ in rows:
    if kind == 5 and name in ('func_static', 'func_group'):
        legacy_labels[key] = 'Static brush' if name == 'func_static' else 'Brush group'
for key, row in entries.items():
    if int(row[1]) != 5 or row[14] or not int(row[17].rstrip('u')):
        continue
    target = int(row[17].rstrip('u'))
    if target not in actors:
        # SP segmented MD3 actors are not supported by the MP player renderer.
        assert target in entries and entries[target][4].endswith('.md3'), row[3]
        continue
    actor = actors[target]
    rows.append((key, actor[1], 3, *actor[3:]))
    aliases.append((key, target, row[3]))
native_items=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/','',items)
native_items = native_items[native_items.index('bg_itemlist[]'):].split('};',1)[0]
for match in re.finditer(r'\{\s*"([^"]+)"\s*,(.*?)\b(IT_WEAPON|IT_AMMO|IT_HEALTH|IT_ARMOR|IT_HOLDABLE|IT_POWERUP|IT_TEAM)\s*,',native_items,re.S):
    name,body,item_type=match.groups()
    # These legacy neutral-flag/Harvester resources are absent from retail JA.
    # Kept open in the parity checklist; do not publish broken new menu cards.
    if name in ("team_CTF_neutralflag","item_redcube","item_bluecube"):continue
    if any(row[3]==name and row[1]==4 for row in rows):continue
    strings=re.findall(r'"([^"]*)"',body)
    model=next((s for s in strings if s.startswith('models/')), '')
    icon=next((s for s in strings if s.startswith('gfx/')), 'gfx/hud/i_icon_ammodisp')
    h=2166136261
    for b in name.encode():h=((h^b)*16777619)&0xffffffff
    rows.append(((4<<24)|(h&0xffffff),4,2 if item_type=="IT_WEAPON" else 3,name,model,icon,16.,40.,-24.))
# Give the matching stock MP pickups the SP catalog IDs. Keep the original
# MP IDs loadable for existing worlds and console requests.
holdables = {
    'item_bacta': ('item_medpac', 'Bacta Canister', 'HI_MEDPAC'),
    'item_la_goggles': ('item_binoculars', 'Binoculars', 'HI_BINOCULARS'),
    'item_enviro': ('item_shield', 'Portable Shield', 'HI_SHIELD'),
}
holdable_aliases = []
for name, (native, label, tag) in holdables.items():
    native_card = next(row for row in rows if row[1] == 4 and row[3] == native)
    key, row = next((key, row) for key, row in entries.items() if row[1] == '4' and row[3] == name)
    assert not row[14] and row[4] == native_card[4], name
    assert re.search(r'"'+native+r'"[^}]+\}[^{}]+\bIT_HOLDABLE\s*,\s*'+tag+r'\b', native_items), native
    rows.append((key, 4, 3, name, *native_card[4:]))
    holdable_aliases.append((key, native_card[0], native))
    legacy_labels[key] = label
# MP also provides a native health station; reuse its qualified stock model.
name='misc_model_health_power_converter'
h=2166136261
for b in name.encode():h=((h^b)*16777619)&0xffffffff
rows.append(((5<<24)|(h&0xffffff),5,3,name,'models/items/power_converter.md3','gfx/hud/i_icon_ammodisp',24.,64.,-24.))
# Internal definitions give native furniture and its duplicates stable IDs.
# The optional converted-map pack can be absent, so these have no menu cards.
construct_furniture = ('Construct companion doll', 'Construct table',
                       'Construct armchair', 'Construct sofa')
for i, label in enumerate(construct_furniture):
    rows.append((1101 + i, 1, -1, label, f'models/jg/gm_construct/p{i:03}.md3',
                 'gfx/hud/i_icon_ammodisp', 16., 32., -16.))
assert len({row[0] for row in rows}) == len(rows), 'MP stable catalog ID collision'
text = '// Generated retail MP resource choices. GPL-2.0-only.\n#pragma once\n'
text += 'typedef struct { unsigned id; int kind, tab; const char *name,*model,*icon; float radius,height,bottom; } jarMPEntry_t;\n'
text += 'static const jarMPEntry_t jarMPCatalog[] = {\n'
for row in rows:
    text += ' {' + ','.join(json.dumps(v) if isinstance(v,str) else str(v) for v in row) + '},\n'
text += '};\n#define JAR_MP_CATALOG_COUNT (sizeof(jarMPCatalog)/sizeof(jarMPCatalog[0]))\n'
text += 'static inline const char *JAR_MPCatalogPickupClass(const jarMPEntry_t *entry) {\n switch (entry->id) {\n'
for key, _, native in holdable_aliases:
    text += f'  case {key}: return {json.dumps(native)};\n'
text += '  default: return entry->name;\n }\n}\n'
text += 'static inline int JAR_MPCatalogVisible(const jarMPEntry_t *entry) {\n switch (entry->id) {\n'
for _, old_key, _ in holdable_aliases:
    text += f'  case {old_key}: return 0;\n'
text += '  default: return entry->tab >= 0 && !(entry->tab == 3 && entry->kind == 2);\n }\n}\n'
text += 'static inline int JAR_MPCatalogMinMass(const jarMPEntry_t *entry) {\n'
text += ' return entry->tab < 0 ? 1 : 5;\n}\n'
text += 'typedef struct { unsigned id, target; const char *name; } jarMPAlias_t;\n'
text += 'static const jarMPAlias_t jarMPAliases[] = {\n'
for key, target, name in aliases:
    text += ' {' + f'{key},{target},{json.dumps(name)}' + '},\n'
text += '};\n#define JAR_MP_ALIAS_COUNT (sizeof(jarMPAliases)/sizeof(jarMPAliases[0]))\n'
text += 'static const char *JAR_MPCatalogName(const jarMPEntry_t *entry) {\n'
text += ' switch (entry->id) {\n'
for key, label in sorted(legacy_labels.items()):
    text += f'  case {key}: return {json.dumps(label)};\n'
text += ' }\n'
text += ' for (unsigned i=0;i<JAR_MP_ALIAS_COUNT;++i) if (jarMPAliases[i].id==entry->id) return jarMPAliases[i].name;\n'
text += ' return entry->name;\n}\n'
text += 'static inline int JAR_MPCatalogLogicMarker(const jarMPEntry_t *entry) {\n switch (entry->id) {\n'
for key, kind, _, name, model, *_ in rows:
    if kind == 5 and name in logic_markers and model == 'models/map_objects/imperial/crate1.md3':
        text += f'  case {key}:\n'
text += '   return 1;\n  default: return 0;\n }\n}\n'
destination = root / 'codemp/game/jar_mp_catalog.h'
if not destination.exists() or destination.read_text() != text:
    destination.write_text(text, newline='\n')
print(f'MP catalog: {len(rows)} entries, including {len(aliases)} map spawner aliases')
