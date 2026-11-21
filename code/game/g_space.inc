// Native space callbacks, completed from g_trigger.cpp/g_active.cpp.
// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
#include "g_space.h"
#include "g_volume_geometry.h"
namespace {
constexpr int spaceGrace=5000;
void spaceClear(gentity_t*actor){if(actor&&actor->client){actor->client->inSpaceIndex=0;actor->client->inSpaceSuffocation=0;}}
bool spaceClass(const gentity_t&volume){return volume.inuse&&volume.classname&&!strcmp(volume.classname,"trigger_space");}
bool spaceVolume(int index){
 if(index<=0||index>=ENTITYNUM_WORLD)return false;
 const auto&v=g_entities[index];return spaceClass(v)&&v.linked&&(v.contents&CONTENTS_TRIGGER)&&!(v.svFlags&SVF_INACTIVE)&&v.e_TouchFunc==touchF_space_touch;
}
bool spaceRiderProtected(const gentity_t&actor){
 if(actor.client->NPC_class==CLASS_VEHICLE)return false;
 const int index=actor.s.m_iVehicleNum;if(index<=0||index>=ENTITYNUM_WORLD)return false;
 const auto&vehicle=g_entities[index];
 return vehicle.inuse&&vehicle.health>0&&vehicle.client&&vehicle.m_pVehicle&&vehicle.m_pVehicle->m_pVehicleInfo&&vehicle.m_pVehicle->m_pVehicleInfo->hideRider;
}
bool spaceActor(const gentity_t*actor){return actor&&actor->inuse&&actor->client&&actor->health>0&&!actor->client->noclip&&!spaceRiderProtected(*actor);}
bool spaceBreathes(const gentity_t&actor){
 switch(actor.client->NPC_class){
 case CLASS_ATST:case CLASS_GONK:case CLASS_INTERROGATOR:case CLASS_MARK1:case CLASS_MARK2:case CLASS_GALAKMECH:
 case CLASS_MOUSE:case CLASS_PROBE:case CLASS_PROTOCOL:case CLASS_R2D2:case CLASS_R5D2:case CLASS_REMOTE:
 case CLASS_SEEKER:case CLASS_SENTRY:case CLASS_SABER_DROID:case CLASS_ASSASSIN_DROID:case CLASS_VEHICLE:return false;
 default:return true;
 }
}
void spacePortableBounds(gentity_t*self){
 if(jar::volumeEntity::Portable(*self))jar::volumeEntity::Initialize(*self);
}
bool spaceContains(const gentity_t&volume,const vec3_t point){
 if(jar::volumeEntity::Portable(volume))return jar::volume::Contains(volume.pos1,volume.pos2,volume.currentOrigin,volume.currentAngles,point);
 return G_PointInBounds(point,volume.absmin,volume.absmax)!=qfalse;
}
bool spaceMembership(gentity_t*actor,int excluded=0){
 const int index=actor->client->inSpaceIndex;
 if(spaceActor(actor)&&index>0&&index<ENTITYNUM_WORLD){
  const float*point=actor->client->ps.origin;
  if(index!=excluded&&spaceVolume(index)&&spaceContains(g_entities[index],point))return true;
  gentity_t*nearby[MAX_GENTITIES];
  const int count=gi.EntitiesInBox(point,point,nearby,MAX_GENTITIES);
  for(int i=0;i<count;++i){
   const auto&volume=*nearby[i];
   const int slot=volume.s.number;
   if(slot==excluded||slot==index||!spaceVolume(slot)||!spaceContains(volume,point))continue;
   actor->client->inSpaceIndex=slot;
   return true;
  }
 }
 spaceClear(actor);
 return false;
}
}
bool G_SpaceActive(gentity_t*actor){
 return actor&&actor->client&&spaceMembership(actor);
}
void G_SpaceGravity(gentity_t*actor,float gravity){
 if(!actor||!actor->client)return;
 const bool active=G_SpaceActive(actor);
 if(!(actor->svFlags&SVF_CUSTOM_GRAVITY))actor->client->ps.gravity=active?0:gravity;
}
void space_touch(gentity_t*self,gentity_t*other,trace_t*){
 if(!self||!spaceVolume(self->s.number))return;
 if(!spaceActor(other)){spaceClear(other);return;}
 if(!spaceContains(*self,other->client->ps.origin))return;
 // Overlapping volumes share the current exposure timer instead of renewing
 // breathable time every time the native touch enumerator changes order.
 if(!G_SpaceActive(other))other->client->inSpaceSuffocation=level.time+spaceGrace;
 other->client->inSpaceIndex=self->s.number;
}
void G_SpaceEffects(gentity_t*actor){
 if(!G_SpaceActive(actor)||!spaceBreathes(*actor))return;
 auto*client=actor->client;if(client->inSpaceSuffocation>level.time)return;
 auto*volume=&g_entities[client->inSpaceIndex];
 client->inSpaceSuffocation=level.time+Q_irand(1000,2000);
 G_Damage(actor,volume,volume,nullptr,client->ps.origin,Q_irand(20,40),DAMAGE_NO_ARMOR,MOD_SUICIDE);
 // Death/scripts may remove the actor. Choke animations belong to humanoids.
 if(!actor->inuse||actor->client!=client||actor->health<=0){spaceClear(actor);return;}
 if(client->clientInfo.animFileIndex==0){G_SoundOnEnt(actor,CHAN_VOICE,va("*choke%d.wav",Q_irand(1,3)));NPC_SetAnim(actor,SETANIM_BOTH,BOTH_CHOKE1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);}
}
void G_SpaceForget(gentity_t*entity){
 if(!entity)return;spaceClear(entity);
 if(!spaceClass(*entity))return;
 for(int i=0;i<globals.num_entities;++i){auto&actor=g_entities[i];if(actor.inuse&&actor.client&&actor.client->inSpaceIndex==entity->s.number)spaceMembership(&actor,entity->s.number);}
}
bool G_SpacePortable(const gentity_t& volume){
 return spaceClass(volume)&&jar::volumeEntity::Portable(volume)&&volume.e_TouchFunc==touchF_space_touch;
}
void G_SpaceSet(gentity_t& volume,bool armed,bool enabled){
 if(!G_SpacePortable(volume))return;
 volume.contents=armed?CONTENTS_TRIGGER:0;
 if(enabled)volume.svFlags&=~SVF_INACTIVE;
 else volume.svFlags|=SVF_INACTIVE;
 if(!armed||!enabled)G_SpaceForget(&volume);
 gi.linkentity(&volume);
}
void space_use(gentity_t*self,gentity_t*,gentity_t*){
 if(!self||!spaceClass(*self)||(self->svFlags&SVF_INACTIVE))return;
 if(G_SpacePortable(*self)){G_SpaceSet(*self,!(self->contents&CONTENTS_TRIGGER),true);return;}
 self->contents=(self->contents&CONTENTS_TRIGGER)?CONTENTS_ITEM:CONTENTS_TRIGGER;
 if(!(self->contents&CONTENTS_TRIGGER))G_SpaceForget(self);
 gi.linkentity(self);
}
/*QUAKED trigger_space (.5 .5 .5) ? x x x x x x x INACTIVE
Zero gravity while an actor's origin is inside; breathing actors suffocate after
five seconds. Droids/vehicles do not breathe. Enclosed vehicles protect riders.
Use toggles the volume; target_activate/deactivate controls INACTIVE separately.
*/
void SP_trigger_space(gentity_t*self){
 InitTrigger(self);spacePortableBounds(self);self->contents=CONTENTS_TRIGGER;
 self->e_TouchFunc=touchF_space_touch;self->e_UseFunc=useF_space_use;gi.linkentity(self);
}
bool G_SpaceHasVolumes(){for(int i=1;i<globals.num_entities;++i)if(spaceClass(g_entities[i]))return true;return false;}
void G_SpaceRestore(bool legacyShape){
 // Saves made before the controller existed contain inert native callbacks.
 // Promote only that old state; current saves retain enabled/disabled contents.
 for(int i=1;i<globals.num_entities;++i){auto&v=g_entities[i];if(!spaceClass(v))continue;
  const bool inert=v.e_TouchFunc==touchF_NULL;
  if(inert){v.e_TouchFunc=touchF_space_touch;if(v.e_UseFunc==useF_NULL)v.e_UseFunc=useF_space_use;}
  if(jar::volumeEntity::Portable(v)){
   if(inert)jar::volumeEntity::Initialize(v);
   else if(legacyShape){VectorCopy(v.mins,v.pos1);VectorCopy(v.maxs,v.pos2);}
   if(!jar::volumeEntity::Refresh(v))gi.Error(ERR_DROP,"Invalid saved portable space-volume bounds");
   if(!(v.contents&CONTENTS_TRIGGER))v.contents=0; // Selection uses the exact oriented volume.
  }
  gi.linkentity(&v);
 }
 for(int i=0;i<globals.num_entities;++i)if(g_entities[i].inuse&&g_entities[i].client)G_SpaceActive(&g_entities[i]);
}
