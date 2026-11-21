// Native NPC loadout editing and reconstruction. GPL-2.0-only.
#pragma once
#include "g_local.h"
#include "qcommon/jar_duplicate.h"
#include "qcommon/jar_npc_weapon.h"
#include "wp_saber.h"

extern qboolean G_CanPickUpWeapons(const gentity_t*);
extern void ChangeWeapon(gentity_t*,int);
extern qboolean G_StandardHumanoid(gentity_t*);

struct WorkshopActorWeapon {
 int weapon=WP_NONE;
 bool alternate=false,matchPlayer=false;
};

inline WorkshopActorWeapon G_WorkshopReadActorWeapon(const gentity_t& actor){
 return {actor.client->ps.weapon,(actor.NPC->scriptFlags&SCF_ALT_FIRE)!=0,
         (actor.NPC->aiFlags&NPCAI_MATCHPLAYERWEAPON)!=0};
}

inline gitem_t* G_WorkshopNPCWeaponItem(int weapon){
 for(int i=1;i<bg_numItems;++i)
  if(bg_itemlist[i].giType==IT_WEAPON&&bg_itemlist[i].giTag==weapon)return &bg_itemlist[i];
 return nullptr;
}

inline uint32_t G_WorkshopActorWeaponChoices(gentity_t& actor){
 if(!actor.NPC||!actor.client||!G_CanPickUpWeapons(&actor)||!G_StandardHumanoid(&actor)||
    actor.client->NPC_class==CLASS_GALAKMECH||actor.weaponModel[1]>0)return 0;
 const auto& ps=actor.client->ps;
 if(actor.handRBolt<0||actor.s.weapon!=ps.weapon||ps.saberInFlight||
    (ps.eFlags&EF_LOCKED_TO_WEAPON)||!jar::npcWeapon::Name(ps.weapon))return 0;
 uint32_t choices=(1u<<WP_NONE)|(1u<<WP_MELEE);
 for(int i=1;i<bg_numItems;++i){
  const auto& item=bg_itemlist[i];
  if(item.giType!=IT_WEAPON||item.giTag<=WP_NONE||item.giTag>=WP_NATIVE_WEAPON_COUNT||
     !jar::npcWeapon::Name(item.giTag))continue;
  const auto& weapon=weaponData[item.giTag];
  if(weapon.weaponMdl[0]&&weapon.ammoIndex>=AMMO_NONE&&weapon.ammoIndex<AMMO_MAX)
   choices|=1u<<item.giTag;
 }
 return choices&(1u<<ps.weapon)?choices:0;
}

inline bool G_WorkshopApplyActorWeapon(gentity_t& actor,const WorkshopActorWeapon& next,
 uint32_t weapons,int pool,int ammo){
 auto* item=G_WorkshopNPCWeaponItem(next.weapon);
 if(!G_ReplaceG2HandWeapon(actor,next.weapon==WP_NONE||next.weapon==WP_MELEE?nullptr:weaponData[next.weapon].weaponMdl))
  return false;
 if(item)RegisterItem(item);

 auto& ps=actor.client->ps;
 ps.stats[STAT_WEAPONS]=weapons;
 if(pool!=AMMO_NONE)ps.ammo[pool]=ammo;
 if(next.alternate)actor.NPC->scriptFlags|=SCF_ALT_FIRE;
 else actor.NPC->scriptFlags&=~SCF_ALT_FIRE;
 if(next.matchPlayer)actor.NPC->aiFlags|=NPCAI_MATCHPLAYERWEAPON;
 else actor.NPC->aiFlags&=~NPCAI_MATCHPLAYERWEAPON;
 ChangeWeapon(&actor,next.weapon);
 actor.s.weapon=next.weapon;
 actor.NPC->last_ucmd.weapon=next.weapon;
 ps.weaponstate=WEAPON_READY;
 ps.weaponTime=ps.weaponChargeTime=0;
 actor.client->fireDelay=0;
 ps.eFlags&=~(EF_FIRING|EF_ALT_FIRING);
 actor.s.eFlags&=~(EF_FIRING|EF_ALT_FIRING);
 return true;
}

struct WorkshopActorFire {
 int weapon=WP_NONE,pool=AMMO_NONE,ammo=0;
 bool alternate=false,hasAlternate=false,manualAmmo=false;
};

inline bool G_WorkshopReadActorFire(const gentity_t& actor,WorkshopActorFire& fire){
 if(!actor.NPC||!actor.client||actor.m_pVehicle||!G_CanPickUpWeapons(&actor))return false;
 const auto& ps=actor.client->ps;
 if(ps.weapon<=WP_NONE||ps.weapon>=WP_NATIVE_WEAPON_COUNT)return false;
 const auto& weapon=weaponData[ps.weapon];
 if(weapon.ammoIndex<=AMMO_NONE||weapon.ammoIndex>=AMMO_MAX)return false;
 fire={ps.weapon,weapon.ammoIndex,ps.ammo[weapon.ammoIndex],
       (actor.NPC->scriptFlags&SCF_ALT_FIRE)!=0,weapon.altFireTime>0,(actor.NPC->aiFlags&NPCAI_MANUAL_AMMO)!=0};
 return fire.ammo>=-1&&fire.ammo<=32767;
}

inline bool G_WorkshopMergeActorFire(const WorkshopActorFire& current,const WorkshopActorFire& expected,
 const WorkshopActorFire& requested,WorkshopActorFire& merged){
 if(current.weapon!=expected.weapon||current.pool!=expected.pool)return false;
 merged=current;
 if(expected.ammo!=requested.ammo){
  if(current.ammo!=expected.ammo)return false;
  merged.ammo=requested.ammo;
 }
 if(expected.alternate!=requested.alternate){
  if(!current.hasAlternate||current.alternate!=expected.alternate)return false;
  merged.alternate=requested.alternate;
 }
 if(expected.manualAmmo!=requested.manualAmmo){
  if(current.manualAmmo!=expected.manualAmmo)return false;
  merged.manualAmmo=requested.manualAmmo;
 }
 return true;
}

inline void G_WorkshopApplyActorFire(gentity_t& actor,const WorkshopActorFire& before,const WorkshopActorFire& after){
 actor.client->ps.ammo[after.pool]=after.ammo;
 actor.NPC->currentAmmo=after.ammo;
 if(after.manualAmmo)actor.NPC->aiFlags|=NPCAI_MANUAL_AMMO;
 else actor.NPC->aiFlags&=~NPCAI_MANUAL_AMMO;
 if(after.alternate==before.alternate)return;
 if(after.alternate)actor.NPC->scriptFlags|=SCF_ALT_FIRE;
 else actor.NPC->scriptFlags&=~SCF_ALT_FIRE;
 // Use the same native cadence update as the script alternate-fire setter.
 ChangeWeapon(&actor,after.weapon);
}

inline void G_WorkshopFreeActorSaber(gentity_t& actor){
 if(!actor.NPC||!actor.client)return;
 auto& number=actor.client->ps.saberEntityNum;
 if(number<=0||number>=ENTITYNUM_WORLD)return;
 auto& saber=g_entities[number];
 if(&saber!=&actor&&saber.inuse&&saber.owner==&actor&&saber.classname&&!strcmp(saber.classname,"lightsaber")){
  number=ENTITYNUM_NONE;
  G_FreeEntity(&saber);
 }
}

inline bool G_WorkshopCaptureActorLoadout(const gentity_t& actor,jar::duplicate::ActorLoadout& loadout){
 if(!actor.NPC||!actor.client||actor.m_pVehicle||!G_CanPickUpWeapons(&actor))return false;
 const auto& ps=actor.client->ps;
 loadout.weapon=ps.weapon;
 loadout.weapons=uint32_t(ps.stats[STAT_WEAPONS]);
 std::copy(ps.ammo,ps.ammo+MAX_AMMO,loadout.ammo.begin());
 loadout.altFire=(actor.NPC->scriptFlags&SCF_ALT_FIRE)!=0;
 loadout.matchPlayer=(actor.NPC->aiFlags&NPCAI_MATCHPLAYERWEAPON)!=0;
 loadout.manualAmmo=(actor.NPC->aiFlags&NPCAI_MANUAL_AMMO)!=0;
 if(loadout.weapons&(1u<<WP_SABER)){
  loadout.saberStyle=ps.saberAnimLevel;
  loadout.saberStyles=ps.saberStylesKnown;
  for(int slot=0;slot<(ps.dualSabers?MAX_SABERS:1);++slot){
   const auto& saber=ps.saber[slot];
   if(!saber.name||saber.numBlades<1||saber.numBlades>MAX_BLADES)return false;
   auto& saved=loadout.sabers[slot];
   saved.name=saber.name;
   saved.bladeCount=saber.numBlades;
   for(int i=0;i<saber.numBlades;++i){
    const auto& blade=saber.blade[i];
    saved.blades[i]={blade.color,blade.radius,blade.lengthMax,blade.active!=qfalse};
   }
  }
 }
 return jar::duplicate::Valid(loadout);
}

inline bool G_WorkshopRestoreActorSabers(gentity_t& actor,const jar::duplicate::ActorLoadout& loadout){
 struct ParsedSabers {
  saberInfo_t data[MAX_SABERS]{};
  ~ParsedSabers(){for(auto& saber:data)WP_SaberFreeStrings(saber);}
 } parsed;
 const int count=loadout.sabers[1].bladeCount?2:1;
 for(int slot=0;slot<count;++slot){
  const auto& saved=loadout.sabers[slot];
  auto& saber=parsed.data[slot];
  if(!WP_SaberParseParms(saved.name.c_str(),&saber,qfalse)||saber.numBlades!=saved.bladeCount||!saber.model)return false;
  const size_t length=std::strlen(saber.model);
  if(length<5||length>=64||Q_stricmp(saber.model+length-4,".glm")||
     (count>1&&(saber.saberFlags&SFL_TWO_HANDED)))return false;
 }
 auto& ps=actor.client->ps;
 for(int slot=0;slot<MAX_SABERS;++slot){
  WP_SaberFreeStrings(ps.saber[slot]);
  ps.saber[slot]=parsed.data[slot];
  parsed.data[slot]={};
 }
 ps.dualSabers=count==2?qtrue:qfalse;
 ps.saberAnimLevel=loadout.saberStyle;
 ps.saberStylesKnown=loadout.saberStyles;
 if(!WP_SaberInitSandboxBladeData(&actor))return false;
 ps.saberInFlight=qfalse;
 for(int slot=0;slot<count;++slot){
  const auto& saved=loadout.sabers[slot];
  for(int i=0;i<saved.bladeCount;++i){
   auto& blade=ps.saber[slot].blade[i];
   blade.color=static_cast<saber_colors_t>(saved.blades[i].color);
   blade.radius=saved.blades[i].radius;
   blade.lengthMax=saved.blades[i].lengthMax;
   blade.active=saved.blades[i].active?qtrue:qfalse;
  }
 }
 if(loadout.weapon==WP_SABER){
  WP_SaberAddG2SaberModels(&actor);
  for(int slot=0;slot<count;++slot)if(actor.weaponModel[slot]<=0)return false;
 }
 return true;
}

inline bool G_WorkshopApplyActorLoadout(gentity_t& actor,const jar::duplicate::ActorLoadout& loadout){
 if(!actor.NPC||!actor.client||actor.m_pVehicle||!G_CanPickUpWeapons(&actor)||!jar::duplicate::Valid(loadout))return false;
 auto& ps=actor.client->ps;
 const bool changeWeapon=ps.weapon!=loadout.weapon;
 const auto& weapon=weaponData[loadout.weapon];
 const bool usesModel=loadout.weapon!=WP_NONE&&loadout.weapon!=WP_MELEE&&loadout.weapon!=WP_SABER;
 if(weapon.ammoIndex<0||weapon.ammoIndex>=MAX_AMMO||(changeWeapon&&usesModel&&!weapon.weaponMdl[0]))return false;
 const auto stockWeapons=uint32_t(ps.stats[STAT_WEAPONS]);
 // Reuse the loaded item table and validate before changing native weapon state.
 uint32_t itemsNeeded=loadout.weapons&~stockWeapons&~(1u<<WP_MELEE);
 std::array<gitem_t*,WP_NATIVE_WEAPON_COUNT> items{};
 for(int i=1;i<bg_numItems&&itemsNeeded;++i){
  auto& item=bg_itemlist[i];
  if(item.giType==IT_WEAPON&&item.giTag>WP_NONE&&item.giTag<WP_NATIVE_WEAPON_COUNT&&
     (itemsNeeded&(1u<<item.giTag))){
   items[item.giTag]=&item;
   itemsNeeded&=~(1u<<item.giTag);
  }
 }
 if(itemsNeeded)return false;
 if(changeWeapon){
  G_RemoveWeaponModels(&actor);
  if(usesModel){
   G_CreateG2AttachedWeaponModel(&actor,weapon.weaponMdl,actor.handRBolt,0);
   if(actor.weaponModel[0]<=0)return false;
  }
 }
 if((loadout.weapons&(1u<<WP_SABER))&&!G_WorkshopRestoreActorSabers(actor,loadout))return false;
 for(auto* item:items)if(item)RegisterItem(item);
 if(loadout.altFire)actor.NPC->scriptFlags|=SCF_ALT_FIRE;
 else actor.NPC->scriptFlags&=~SCF_ALT_FIRE;
 if(loadout.matchPlayer)actor.NPC->aiFlags|=NPCAI_MATCHPLAYERWEAPON;
 else actor.NPC->aiFlags&=~NPCAI_MATCHPLAYERWEAPON;
 if(loadout.manualAmmo)actor.NPC->aiFlags|=NPCAI_MANUAL_AMMO;
 else actor.NPC->aiFlags&=~NPCAI_MANUAL_AMMO;
 ps.stats[STAT_WEAPONS]=loadout.weapons;
 std::copy(loadout.ammo.begin(),loadout.ammo.end(),ps.ammo);
 ChangeWeapon(&actor,loadout.weapon);
 actor.s.weapon=loadout.weapon;
 ps.weaponstate=WEAPON_READY;
 ps.weaponTime=0;
 return true;
}
