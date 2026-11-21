// Direct Sandbox weapon grants use native inventory and weapon switching.
gitem_t* workshopWeaponItem(uint32_t catalog){
 if(catalog>=2002&&catalog<=2013)return FindItemForWeapon(weapon_t(catalog-2000));
 if(catalog>=2101&&catalog<=2103)return FindItemForWeapon(WP_SABER);
 const auto*entry=jar::catalog::Find(catalog);
 if(!entry||entry->tab!=2||entry->kind!=4||entry->reason[0])return nullptr;
 const auto*s=supply(catalog);return s?supplyItem(*s):nullptr;
}
const char* workshopMelee(uint32_t catalog){
 return catalog==2101?"jar_shortblade":catalog==2102?"jar_pike":catalog==2103?"sith_sword":nullptr;
}
gentity_t* workshopDropMelee(uint32_t catalog,vec3_t origin,vec3_t angles){
 const char*name=workshopMelee(catalog);
 if(!name||!saberValid(name)){message("Melee definition or model unavailable.");return nullptr;}
 const int oldVars=numSpawnVars;const qboolean oldSpawning=spawning;
 numSpawnVars=0;spawning=qtrue;
 auto*e=G_DropSaberItem(name,catalog==2101?SABER_PURPLE:SABER_YELLOW,origin,vec3_origin,angles);
 numSpawnVars=oldVars;spawning=oldSpawning;
 return e&&e->inuse?e:nullptr;
}
void weaponCard(uint32_t catalog){
 if(!sandboxReady())return;
 auto*p=workshopPlayer();auto&ps=p->client->ps;
 if(ps.saberInFlight||PM_InKnockDown(&ps)||PM_InRoll(&ps)){
  message("Return the saber or finish recovering before equipping a weapon.");return;
 }
 auto*item=workshopWeaponItem(catalog);
 if(!item||item->giType!=IT_WEAPON||item->giTag<=WP_NONE||item->giTag>=WP_NUM_WEAPONS){message("Weapon definition unavailable.");return;}
 const int weapon=item->giTag,ammo=weaponData[weapon].ammoIndex;
 if(ammo<AMMO_NONE||ammo>=AMMO_MAX){message("Weapon ammunition definition unavailable.");return;}
 if(const char*name=workshopMelee(catalog)){
  if(!saberValid(name)){message("Melee definition or model unavailable.");return;}
  ps.weaponTime=0;ps.weaponstate=WEAPON_READY;grant(name,true);return;
 }
 RegisterItem(item);CG_RegisterItemVisuals(item-bg_itemlist);CG_RegisterWeapon(weapon);
 stopTool();ps.stats[STAT_WEAPONS]|=1u<<weapon;
 if(ammo!=AMMO_NONE)Add_Ammo(p,weapon,std::max(1,std::min(1000,item->quantity>0?item->quantity:50)));
 // Selection must not use the previous snapshot's ammo (new grenades/mines).
 // Pmove performs the native draw and hand-model replacement on its next step.
 ps.weaponTime=0;ps.weaponstate=WEAPON_READY;
 cg.weaponSelect=weapon;cg.weaponSelectTime=cg.time;
 message("Weapon granted and selected.");list();
}
