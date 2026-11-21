#pragma once
#include "g_space.h"
#include "qcommon/jar_movement.h"
#include <cmath>

constexpr char JAR_MOVEMENT_PORTABLE[]="@jar_movement";
inline jar::movement::Kind G_MovementMarkerClass(const char* name){
 using namespace jar::movement;
 if(name&&!strcmp(name,"target_gravity_change"))return Gravity;
 if(name&&!strcmp(name,"target_friction_change"))return Friction;
 return None;
}
inline bool G_MovementMarkerPortable(const gentity_t& e){
 return e.inuse&&G_MovementMarkerClass(e.classname)!=jar::movement::None&&e.model&&!strcmp(e.model,JAR_MOVEMENT_PORTABLE);
}
inline bool G_MovementMarkerRead(const gentity_t& e,jar::movement::Marker& state){
 using namespace jar::movement;
 if(!G_MovementMarkerPortable(e))return false;
 const Kind kind=G_MovementMarkerClass(e.classname);
 if(kind==Gravity&&!(e.speed>=0&&e.speed<=maxGravity&&e.speed==std::floor(e.speed)))return false;
 state={kind,kind==Gravity?int(e.speed):e.health,(e.svFlags&SVF_INACTIVE)==0};
 return Valid(state);
}
inline bool G_MovementMarkerSet(gentity_t& e,const jar::movement::Marker& state){
 using namespace jar::movement;
 if(!G_MovementMarkerPortable(e)||state.kind!=G_MovementMarkerClass(e.classname)||!Valid(state))return false;
 if(state.kind==Gravity)e.speed=state.value;else e.health=state.value;
 if(state.enabled)e.svFlags&=~SVF_INACTIVE;else e.svFlags|=SVF_INACTIVE;
 return true;
}
inline void G_MovementMarkerRestore(gentity_t& e){
 using namespace jar::movement;
 const Kind kind=G_MovementMarkerClass(e.classname);
 if(kind==None)return;
 if(!G_MovementMarkerPortable(e)){
  if(e.model&&gi.bIsFromZone(e.model,TAG_G_ALLOC))gi.Free(e.model);
  e.model=G_NewString(JAR_MOVEMENT_PORTABLE);
 }
 Marker marker;
 if(!G_MovementMarkerRead(e,marker))
  G_MovementMarkerSet(e,{kind,kind==Gravity?defaultGravity:defaultFriction,(e.svFlags&SVF_INACTIVE)==0});
}
inline bool G_MovementActorRead(const gentity_t& e,jar::movement::ActorState& state,bool workshopGrip=false){
 const int disallowed=EF_FORCE_DRAINED|EF_LOCKED_TO_WEAPON|EF_HELD_BY_RANCOR|EF_HELD_BY_WAMPA|EF_HELD_BY_SAND_CREATURE;
 if(!e.inuse||!e.client||e.health<=0||e.client->noclip||e.client->ps.pm_type!=PM_NORMAL||e.m_pVehicle||
    (e.s.m_iVehicleNum&&e.s.m_iVehicleNum<ENTITYNUM_WORLD)||
    (e.NPC&&e.client->moveType!=MT_WALK&&e.client->moveType!=MT_RUNJUMP)||
    (e.client->ps.eFlags&(disallowed|(workshopGrip?0:EF_FORCE_GRIPPED))))return false;
 state={e.client->ps.gravity,e.client->ps.friction,(e.svFlags&SVF_CUSTOM_GRAVITY)!=0};
 return true;
}
// The editor validates new values before calling this. Undo may restore values
// supplied by a mission script, including authored custom gravity.
inline bool G_MovementActorSet(gentity_t& e,const jar::movement::ActorState& state){
 jar::movement::ActorState current;
 if(!G_MovementActorRead(e,current))return false;
 e.client->ps.friction=state.friction;
 if(state.customGravity){
  e.svFlags|=SVF_CUSTOM_GRAVITY;
  e.client->ps.gravity=state.gravity;
 }else if(current.customGravity){
  e.svFlags&=~SVF_CUSTOM_GRAVITY;
  G_SpaceGravity(&e,g_gravity->value);
 }
 return true;
}
inline bool G_MovementMarkerActivate(const gentity_t& source,gentity_t* activator){
 using namespace jar::movement;
 Marker marker;
 ActorState actor;
 if(!G_MovementMarkerRead(source,marker)||!marker.enabled||!activator||!G_MovementActorRead(*activator,actor))return false;
 if(marker.kind==Gravity){actor.gravity=marker.value;actor.customGravity=true;}
 else actor.friction=marker.value;
 return G_MovementActorSet(*activator,actor);
}
