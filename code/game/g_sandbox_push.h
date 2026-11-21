// Owned push sources retain their settings in native saved fields.
// GPL-2.0-only.
#ifndef G_SANDBOX_PUSH_H
#define G_SANDBOX_PUSH_H
#include "g_volume_geometry.h"
#include "g_functions.h"

void G_PushTouchAt(gentity_t& source,gentity_t* actor,const vec3_t origin);
void G_PushUse(gentity_t& source,gentity_t* activator);
bool G_PushVelocity(const gentity_t& source,const jar::push::Settings& settings,
 const vec3_t subject,const vec3_t current,vec3_t next);
void G_PushPropSweep(gentity_t& prop,const vec3_t previous,bool rigid,
 void(*apply)(gentity_t&,const vec3_t,bool));

inline jar::push::Kind G_PushKind(const gentity_t& source){
 using namespace jar::push;
 if(!source.inuse||source.bmodel||!source.model)return None;
 const auto kind=Class(source.classname);
 if(kind==Volume&&!strcmp(source.model,volumeModel))return Volume;
 if(kind==Marker&&!strcmp(source.model,markerModel))return Marker;
 return None;
}
inline gentity_t* G_PushTarget(const gentity_t& source){
 auto* target=source.target&&*source.target?G_Find(nullptr,FOFS(targetname),source.target):nullptr;
 return target&&target->inuse&&target!=&source?target:nullptr;
}
inline bool G_PushRead(const gentity_t& source,jar::push::Settings& settings){
 using namespace jar::push;
 const auto kind=G_PushKind(source);
 if(!kind||(kind==Volume&&source.e_TouchFunc!=touchF_trigger_push_touch)||
    (kind==Marker&&source.e_UseFunc!=useF_Use_target_push)||!std::isfinite(source.speed)||
    source.speed<1||source.speed>maxSpeed||!std::isfinite(source.wait)||
    source.wait<minInterval||source.wait>maxInterval)return false;
 settings={kind,int(std::lround(source.speed)),int(std::lround(source.wait)),source.count,
  source.damage,bool(source.spawnflags&1),!(source.svFlags&SVF_INACTIVE),bool(source.spawnflags&2)};
 return Valid(settings);
}
inline bool G_PushSet(gentity_t& source,const jar::push::Settings& settings){
 if(settings.kind!=G_PushKind(source)||!jar::push::Valid(settings))return false;
 source.speed=settings.speed;source.wait=settings.interval;
 source.count=settings.subject;source.damage=settings.aim;
 source.spawnflags=(settings.additive?1:0)|(settings.vehicles?2:0);
 if(settings.enabled)source.svFlags&=~SVF_INACTIVE;
 else source.svFlags|=SVF_INACTIVE;
 return true;
}
inline bool G_PushInitialize(gentity_t& source){
 using namespace jar::push;
 const auto kind=Class(source.classname);
 if(!kind||source.bmodel||!source.model)return false;
 if(kind==Volume&&(strcmp(source.model,"@jar_box")||!jar::volume::Valid(source.mins,source.maxs)))return false;
 Settings settings{kind,kind==Volume?1000:600,kind==Volume?1000:minInterval};
 if(std::isfinite(source.speed)&&source.speed>=1&&source.speed<=maxSpeed)settings.speed=int(std::lround(source.speed));
 if(kind==Volume&&std::isfinite(source.wait)&&source.wait>=minInterval&&source.wait<=maxInterval)
  settings.interval=int(std::lround(source.wait));
 settings.aim=source.target&&*source.target?Linked:Forward;
 settings.enabled=!(source.svFlags&SVF_INACTIVE);
 if(gi.bIsFromZone(source.model,TAG_G_ALLOC))gi.Free(source.model);
 source.model=G_NewString(kind==Volume?volumeModel:markerModel);
 source.nextthink=0;source.e_ThinkFunc=thinkF_NULL;
 if(kind==Volume){
  VectorCopy(source.mins,source.pos1);VectorCopy(source.maxs,source.pos2);
  source.e_TouchFunc=touchF_trigger_push_touch;source.contents=CONTENTS_TRIGGER;
  if(!jar::volumeEntity::Refresh(source))return false;
 }else source.e_UseFunc=useF_Use_target_push;
 if(!G_PushSet(source,settings))return false;
 gi.linkentity(&source);
 return true;
}
inline bool G_PushRestore(gentity_t& source){
 if(!jar::push::Class(source.classname))return true;
 if(!G_PushKind(source))return G_PushInitialize(source);
 jar::push::Settings settings;
 if(!G_PushRead(source,settings)||source.e_ThinkFunc!=thinkF_NULL||
    (settings.kind==jar::push::Volume&&!jar::volumeEntity::Refresh(source)))return false;
 gi.linkentity(&source);
 return true;
}
#endif
