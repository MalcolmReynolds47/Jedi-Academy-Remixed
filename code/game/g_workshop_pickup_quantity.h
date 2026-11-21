// Quantity edits use the native count field and stock pickup callbacks.
#pragma once
#include "g_local.h"
#include "g_workshop.h"
#include "qcommon/jar_pickup_quantity.h"
#include "qcommon/jar_force_enlightenment.h"

inline bool G_WorkshopPickupQuantityEditable(const gentity_t& e){
 if(!e.item||e.s.eType!=ET_ITEM||!(e.flags&FL_DROPPED_ITEM)||
    (e.s.eFlags&EF_NODRAW)||e.freeAfterEvent||!G_JAR_WorkshopOwns(&e))return false;
 switch(e.item->giType){
 case IT_HEALTH:
 case IT_ARMOR:
 case IT_AMMO:
 case IT_BATTERY:return true;
 case IT_POWERUP:return e.item->giTag==PW_FORCE_BOON||jar::enlightenment::PickupSide(e.item->giTag)!=jar::enlightenment::None;
 case IT_WEAPON:{
  const int weapon=e.item->giTag;
  if(weapon<=WP_NONE||weapon>=WP_NATIVE_WEAPON_COUNT||weapon==WP_SABER)return false;
  const int ammo=weaponData[weapon].ammoIndex;
  return ammo>AMMO_NONE&&ammo<AMMO_MAX;
 }
 default:return false;
 }
}

inline int G_WorkshopArmorPickupQuantity(const gentity_t& e){
 return e.count>0&&jar::pickupQuantity::Valid(e.count)&&G_WorkshopPickupQuantityEditable(e)?e.count:e.item->quantity;
}
