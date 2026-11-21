// Completes Raven's unused trigger_bidirectional contract. GPL-2.0-only.
#include "g_bidirectional.h"
#include "g_volume_geometry.h"
#include <climits>
#include "../../shared/qcommon/jar_directional_trigger.h"
namespace {
bool bidirectionalClass(const gentity_t&e){return e.inuse&&e.classname&&!strcmp(e.classname,"trigger_bidirectional");}
int bidirectionalCrossing(const gentity_t&volume,const vec3_t previous,const vec3_t current){
 vec3_t center,forward;
 if(jar::volumeEntity::Portable(volume)){
  if(!jar::volume::Valid(volume.pos1,volume.pos2))return 0;
  vec3_t axis[3];AnglesToAxis(volume.currentAngles,axis);VectorCopy(volume.currentOrigin,center);
  for(int a=0;a<3;++a)VectorMA(center,(volume.pos1[a]+volume.pos2[a])*.5f,axis[a],center);
  VectorCopy(axis[0],forward);
 }else {
  for(int a=0;a<3;++a)center[a]=(volume.absmin[a]+volume.absmax[a])*.5f;
  VectorCopy(volume.movedir,forward);if(!VectorNormalize(forward))AngleVectors(volume.currentAngles,forward,nullptr,nullptr);
 }
 vec3_t crossing;
 const int direction=JAR_DirectionalCrossing(center,forward,previous,current,crossing);
 if(!direction)return 0;
 const bool inside=jar::volumeEntity::Portable(volume)?jar::volume::Contains(volume.pos1,volume.pos2,volume.currentOrigin,volume.currentAngles,crossing):G_PointInBounds(crossing,volume.absmin,volume.absmax)!=qfalse;
 return inside?direction:0;
}
void bidirectionalEnable(gentity_t&e,bool enabled){e.contents=enabled?CONTENTS_TRIGGER:jar::volumeEntity::Portable(e)?0:CONTENTS_ITEM;gi.linkentity(&e);}
}
void bidirectional_touch(gentity_t*self,gentity_t*other,trace_t*){
 if(!self||!bidirectionalClass(*self)||!self->linked||!(self->contents&CONTENTS_TRIGGER)||(self->svFlags&SVF_INACTIVE)||self->painDebounceTime>level.time||self->count<0)return;
 if(!other||!other->inuse||!other->client||other->health<=0||other->client->noclip)return;
 if((self->spawnflags&1)&&other->s.number!=0)return;
 if(self->NPC_targetname&&self->NPC_targetname[0]&&(!other->NPC||!other->script_targetname||Q_stricmp(self->NPC_targetname,other->script_targetname)))return;
 const int direction=bidirectionalCrossing(*self,other->lastOrigin,other->client->ps.origin);if(!direction)return;
 const char*target=direction>0?self->target:self->paintarget;if(!target||!*target)return;
 // Commit cooldown/exhaustion before callbacks, which may remove this entity.
 const int wait=std::isfinite(self->wait)?int(std::max(0.f,std::min(3600.f,self->wait))*1000):1000;
 self->painDebounceTime=int(std::min<int64_t>(INT_MAX,int64_t(level.time)+std::max(1,wait)));
 if(self->count>0&&!--self->count)self->count=-1;
 if(self->wait<0||self->count<0)bidirectionalEnable(*self,false);
 G_UseTargets2(self,other,target);
}
void bidirectional_use(gentity_t*self,gentity_t*,gentity_t*){
 if(!self||!bidirectionalClass(*self)||(self->svFlags&SVF_INACTIVE))return;
 const bool enable=!(self->contents&CONTENTS_TRIGGER);if(enable&&self->count<0)self->count=1;
 bidirectionalEnable(*self,enable);
}
/*QUAKED trigger_bidirectional (.1 .5 .1) ? PLAYER_ONLY x x x x x x INACTIVE
Cross its center plane along angle to fire target; reverse fires backwardstarget.
NPC_targetname restricts the matching script-named NPC. wait is seconds between
firings (negative: once); count optionally limits total firings. Use toggles it;
Activate rearms one exhausted count-limited firing. No stationary touch repeats.
*/
void SP_trigger_bidirectional(gentity_t*self){
 InitTrigger(self);if(jar::volumeEntity::Portable(*self))jar::volumeEntity::Initialize(*self);
 if(!std::isfinite(self->wait))self->wait=1;self->wait=std::max(-1.f,std::min(3600.f,self->wait));
 self->count=std::max(0,std::min(100000,self->count));self->painDebounceTime=0;
 self->e_TouchFunc=touchF_bidirectional_touch;self->e_UseFunc=useF_bidirectional_use;gi.linkentity(self);
}
bool G_BidirectionalHasVolumes(){for(int i=1;i<globals.num_entities;++i)if(bidirectionalClass(g_entities[i]))return true;return false;}
void G_BidirectionalRestore(){
 for(int i=1;i<globals.num_entities;++i){auto&e=g_entities[i];if(!bidirectionalClass(e))continue;
  if(e.e_TouchFunc!=touchF_bidirectional_touch||e.e_UseFunc!=useF_bidirectional_use||!std::isfinite(e.wait)||e.wait< -1||e.wait>3600||e.count< -1||e.count>100000)
   gi.Error(ERR_DROP,"Invalid saved bidirectional trigger");
  if(jar::volumeEntity::Portable(e)&&!jar::volumeEntity::Refresh(e))gi.Error(ERR_DROP,"Invalid saved bidirectional volume bounds");
  gi.linkentity(&e);
 }
}
