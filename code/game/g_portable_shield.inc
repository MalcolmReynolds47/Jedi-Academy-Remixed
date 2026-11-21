// Retail portable-shield behavior adapted for native SP entities and saves.
// State lives in serialized entity fields; no process-local owner pointers or timers.
#include "g_portable_shield.h"
#include "qcommon/jar_shield_copy.h"
static void shieldLower(gentity_t*);

bool G_PortableShield(const gentity_t&e){
 return e.inuse&&e.classname&&!strcmp(e.classname,"jar_portable_shield")&&e.model&&!strcmp(e.model,"models/map_objects/mp/shield.md3");
}
bool G_PortableShieldClear(const gentity_t& shield){
 trace_t hit{};
 gi.trace(&hit,shield.currentOrigin,shield.mins,shield.maxs,shield.currentOrigin,
  shield.s.number,MASK_SOLID|CONTENTS_BODY,G2_NOCOLLIDE,0);
 return !hit.startsolid&&!hit.allsolid;
}
bool G_PortableShieldProperties(const gentity_t& shield,jar::shieldCopy::Properties& values){
 if(!G_PortableShield(shield)||shield.e_ThinkFunc!=thinkF_portable_shield_think||(shield.s.eFlags&EF_NODRAW)||
    shield.health<=0||shield.delay<=level.time||!jar::shieldCopy::ValidBounds(shield.pos1,shield.pos2))return false;
 values={shield.health,shield.delay,-shield.pos1[1],shield.pos2[1],shield.pos2[2]};
 return true;
}
bool G_PortableShieldSetProperties(gentity_t& shield,const jar::shieldCopy::Properties& values){
 jar::shieldCopy::Properties before;
 const vec3_t localMin={-4,-values.left,0},localMax={4,values.right,values.height};
 if(!G_PortableShieldProperties(shield,before)||values.health<1||values.health>jar::shieldCopy::maxHealth||
    values.deadline<=level.time||int64_t(values.deadline)-level.time>jar::shieldCopy::lifetime||
    !jar::shieldCopy::ValidBounds(localMin,localMax))return false;
 if(before.left!=values.left||before.right!=values.right||before.height!=values.height){
  vec3_t mins,maxs;
  jar::volume::Hull(localMin,localMax,shield.currentAngles,mins,maxs);
  trace_t hit{};
  gi.trace(&hit,shield.currentOrigin,mins,maxs,shield.currentOrigin,shield.s.number,MASK_SOLID|CONTENTS_BODY,G2_NOCOLLIDE,0);
  if(hit.startsolid||hit.allsolid)return false;
  VectorCopy(localMin,shield.pos1);VectorCopy(localMax,shield.pos2);
  VectorCopy(mins,shield.mins);VectorCopy(maxs,shield.maxs);
  gi.linkentity(&shield);
 }
 shield.health=values.health;shield.delay=values.deadline;
 return true;
}
bool G_PortableShieldMoved(gentity_t&e){
 if(!G_PortableShield(e)||!jar::volume::Valid(e.pos1,e.pos2))return false;
 for(int a=0;a<3;++a)if(!std::isfinite(e.currentAngles[a])||!std::isfinite(e.currentOrigin[a]))return false;
 jar::volume::Hull(e.pos1,e.pos2,e.currentAngles,e.mins,e.maxs);
 VectorCopy(e.currentOrigin,e.s.pos.trBase);VectorCopy(e.currentAngles,e.s.apos.trBase);
 if(e.count&&!G_PortableShieldClear(e))shieldLower(&e);
 return true;
}
static void shieldSound(gentity_t*e,const char*path){G_Sound(e,G_SoundIndex(path));}
static void shieldRemove(gentity_t*e){
 e->contents=0;e->takedamage=qfalse;e->s.loopSound=0;e->s.eFlags|=EF_NODRAW;
 e->e_TouchFunc=touchF_NULL;e->e_ThinkFunc=thinkF_G_FreeEntity;e->nextthink=level.time+100;
 shieldSound(e,"sound/movers/doors/forcefield_off.wav");gi.linkentity(e);
}
static void shieldLower(gentity_t*e){
 if(e->count)shieldSound(e,"sound/movers/doors/forcefield_off.wav");
 e->count=0;e->contents=CONTENTS_TRIGGER|CONTENTS_ITEM;e->takedamage=qfalse;e->s.loopSound=0;
 e->attackDebounceTime=level.time+200;e->nextthink=level.time+200;gi.linkentity(e);
}
void portable_shield_think(gentity_t*e){
 if(!G_PortableShield(*e))return;
 if(e->s.eFlags&EF_NODRAW){if(level.time>=e->nextthink)G_FreeEntity(e);return;}
 if(level.time>=e->delay||e->health<=0){shieldRemove(e);return;}
 if(level.time<e->nextthink)return;
 e->nextthink=level.time+100;
 if(!G_PortableShieldMoved(*e)){shieldRemove(e);return;}
 if(e->count)return;
 if(level.time<e->attackDebounceTime)return;
 if(!G_PortableShieldClear(*e))return;
 e->count=1;e->contents=CONTENTS_SOLID|CONTENTS_TRIGGER;e->takedamage=qtrue;
 e->s.loopSound=G_SoundIndex("sound/movers/doors/forcefield_lp.wav");
 shieldSound(e,"sound/movers/doors/forcefield_on.wav");gi.linkentity(e);
}
void portable_shield_touch(gentity_t*e,gentity_t*other,trace_t*){
 if(G_PortableShield(*e)&&other&&other->inuse&&other->client&&other->health>0&&
    other->client->playerTeam==TEAM_PLAYER&&level.time<e->delay)shieldLower(e);
}
void portable_shield_pain(gentity_t*e,gentity_t*,gentity_t*,const vec3_t,int,int,int){
 if(!G_PortableShield(*e)||!e->count)return;
 e->painDebounceTime=level.time+200;shieldSound(e,"sound/effects/bumpfield.wav");
}
void portable_shield_die(gentity_t*e,gentity_t*,gentity_t*,int,int,int,int){if(G_PortableShield(*e))shieldRemove(e);}

bool G_PortableShieldCopyState(const gentity_t& e,jar::shieldCopy::State& state){
 if(!G_PortableShield(e)||(e.s.eFlags&EF_NODRAW)||e.e_ThinkFunc!=thinkF_portable_shield_think||e.count<0||e.count>1)return false;
 const auto remaining=[](int absolute){return std::max<int64_t>(0,int64_t(absolute)-level.time);};
 const auto expiry=remaining(e.delay);
 if(!expiry||expiry>jar::shieldCopy::lifetime)return false;
 VectorCopy(e.pos1,state.mins);VectorCopy(e.pos2,state.maxs);
 state.health=e.health;state.capacity=e.max_health;state.raised=e.count==1;
 state.remaining=int(expiry);
 state.nextCheck=int(std::min<int64_t>(500,remaining(e.nextthink)));
 state.rearm=int(std::min<int64_t>(500,remaining(e.attackDebounceTime)));
 state.damageFlash=int(std::min<int64_t>(200,remaining(e.painDebounceTime)));
 return jar::shieldCopy::Valid(state);
}

gentity_t* G_PortableShieldSpawnCopy(const jar::shieldCopy::State& state,const vec3_t origin,const vec3_t angles){
 if(!jar::shieldCopy::Valid(state)||int64_t(level.time)+state.remaining+500>INT_MAX)return nullptr;
 for(int axis=0;axis<3;++axis)if(!std::isfinite(origin[axis])||std::fabs(origin[axis])>65535||!std::isfinite(angles[axis]))return nullptr;
 auto* e=G_TrySpawn();if(!e)return nullptr;
 e->classname=G_NewString("jar_portable_shield");e->model=G_NewString("models/map_objects/mp/shield.md3");
 e->s.eType=ET_GENERAL;e->s.modelindex=G_ModelIndex(e->model);e->mass=40;
 if(!e->s.modelindex){G_FreeEntity(e);return nullptr;}
 G_SetOrigin(e,origin);G_SetAngles(e,angles);
 VectorCopy(state.mins,e->pos1);VectorCopy(state.maxs,e->pos2);
 jar::volume::Hull(e->pos1,e->pos2,angles,e->mins,e->maxs);
 e->health=state.health;e->max_health=state.capacity;e->delay=level.time+state.remaining;
 e->attackDebounceTime=level.time+state.rearm;e->painDebounceTime=level.time+state.damageFlash;
 e->count=state.raised?1:0;e->takedamage=state.raised?qtrue:qfalse;
 e->contents=state.raised?CONTENTS_SOLID|CONTENTS_TRIGGER:CONTENTS_TRIGGER|CONTENTS_ITEM;
 e->s.loopSound=state.raised?G_SoundIndex("sound/movers/doors/forcefield_lp.wav"):0;
 e->clipmask=MASK_SOLID|CONTENTS_BODY;
 e->e_ThinkFunc=thinkF_portable_shield_think;e->nextthink=std::max(1,level.time+state.nextCheck);
 e->e_TouchFunc=touchF_portable_shield_touch;e->e_PainFunc=painF_portable_shield_pain;e->e_DieFunc=dieF_portable_shield_die;
 return e;
}

bool G_UsePortableShield(gentity_t*player){
 if(!player||!player->inuse||player->s.number!=0||!player->client||player->health<1||in_camera||
    player->client->ps.inventory[INV_PORTABLE_SHIELD]<=0||player->s.m_iVehicleNum)return false;
 // Resolve the floor and full wall before allocating or consuming inventory.
 vec3_t angles={0,player->client->ps.viewangles[YAW],0},forward,dest,base;
 AngleVectors(angles,forward,nullptr,nullptr);VectorMA(player->currentOrigin,64,forward,dest);
 vec3_t smallLo={-4,-4,0},smallHi={4,4,4};trace_t tr{};
 gi.trace(&tr,player->currentOrigin,smallLo,smallHi,dest,player->s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);
 if(tr.startsolid||tr.allsolid||tr.fraction<1)return false;
 VectorCopy(dest,base);dest[2]-=128;
 gi.trace(&tr,base,smallLo,smallHi,dest,player->s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);
 if(tr.startsolid||tr.allsolid||tr.fraction>=1||tr.plane.normal[2]<.7f||tr.entityNum!=ENTITYNUM_WORLD)return false;
 VectorCopy(tr.endpos,base);base[2]+=.5f;
 // Match the retail cardinal-axis wall, and sweep its entire cross-section.
 const int widthAxis=std::fabs(forward[0])>std::fabs(forward[1])?1:0;
 VectorCopy(base,dest);dest[2]+=254;
 gi.trace(&tr,base,smallLo,smallHi,dest,player->s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);
 if(tr.startsolid||tr.allsolid)return false;
 const float height=std::floor(254*tr.fraction)-1;if(height<32)return false;
 smallHi[2]=height;float widths[2];
 for(int side=0;side<2;++side){VectorCopy(base,dest);dest[widthAxis]+=(side?1:-1)*251;
  gi.trace(&tr,base,smallLo,smallHi,dest,player->s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);
  if(tr.startsolid||tr.allsolid)return false; widths[side]=std::floor(251*tr.fraction)+3;
 }
 if(widths[0]+widths[1]<32)return false;
 if(widthAxis==0)std::swap(widths[0],widths[1]);
 vec3_t localLo={-4,-widths[0],0},localHi={4,widths[1],height},lo,hi;
 VectorSet(angles,0,widthAxis==1?0:90,0);jar::volume::Hull(localLo,localHi,angles,lo,hi);
 gi.trace(&tr,base,lo,hi,base,player->s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);
 if(tr.startsolid||tr.allsolid)return false;
 jar::shieldCopy::State state;
 VectorCopy(localLo,state.mins);VectorCopy(localHi,state.maxs);
 state.health=state.capacity=250;state.remaining=jar::shieldCopy::lifetime;state.nextCheck=state.rearm=500;
 auto* e=G_PortableShieldSpawnCopy(state,base,angles);if(!e)return false;
 gi.linkentity(e);shieldSound(e,"sound/weapons/detpack/stick.wav");
 --player->client->ps.inventory[INV_PORTABLE_SHIELD];return true;
}
bool G_PortableShieldRestore(){
 for(int i=1;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_PortableShield(e))continue;
  if(!jar::shieldCopy::ValidBounds(e.pos1,e.pos2)||e.count<0||e.count>1||e.delay<0||
     e.health>jar::shieldCopy::maxHealth||!G_PortableShieldMoved(e))return false;
  // Re-register after a map/load: sound and model indices are map-local.
  e.s.modelindex=G_ModelIndex(e.model);
  if(!(e.s.eFlags&EF_NODRAW)){e.e_ThinkFunc=thinkF_portable_shield_think;e.e_TouchFunc=touchF_portable_shield_touch;
   e.e_PainFunc=painF_portable_shield_pain;e.e_DieFunc=dieF_portable_shield_die;
   e.contents=e.count?CONTENTS_SOLID|CONTENTS_TRIGGER:CONTENTS_TRIGGER|CONTENTS_ITEM;
   e.takedamage=e.count?qtrue:qfalse;e.s.loopSound=e.count?G_SoundIndex("sound/movers/doors/forcefield_lp.wav"):0;
  }
  gi.linkentity(&e);
 }
 return true;
}
void G_PortableShieldCleanup(){
 for(int i=1;i<globals.num_entities;++i)if(G_PortableShield(g_entities[i]))G_FreeEntity(&g_entities[i]);
}
