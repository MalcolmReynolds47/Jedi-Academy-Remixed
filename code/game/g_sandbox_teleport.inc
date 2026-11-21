// Safe actor arrival and touch inhibition until the actor exits the volume.
#include "g_sandbox_teleport.h"
#include "g_contact_clocks.h"
#include "g_logic_settings.h"
#include "g_subbsp_ownership.h"
#include "g_workshop.h"
#include "g_sandbox_vehicle_teleport.h"
#include "qcommon/ojk_saved_game_helper.h"
#include <algorithm>
#include <map>
#include <vector>
#include <limits>

namespace {
struct TeleportArrival {uint64_t life;int frame;};
std::map<uint16_t,TeleportArrival> teleportArrivals;

bool TeleportedThisFrame(const gentity_t& actor){
 const auto entry=teleportArrivals.find(uint16_t(actor.s.number));
 return entry!=teleportArrivals.end()&&entry->second.frame==level.time&&
  entry->second.life==G_EntityLifetime(&actor);
}

bool TeleportSubjectLive(const gentity_t& subject){return G_LogicLiveActor(&subject)||G_TeleportObjectKind(subject);}

bool DestinationPosition(const gentity_t& target,const gentity_t& subject,vec3_t origin){
 if(&target==&subject||target.svFlags&SVF_INACTIVE)return false;
 VectorCopy(target.currentOrigin,origin);
 if(target.contents&(CONTENTS_SOLID|CONTENTS_BODY))origin[2]+=std::fmax(0.f,target.maxs[2]);
 origin[2]-=subject.mins[2];
 return std::isfinite(origin[0])&&std::isfinite(origin[1])&&std::isfinite(origin[2]);
}

bool TeleportClear(const gentity_t& subject,const vec3_t arrival,int mask){
 trace_t space{};
 gi.trace(&space,arrival,subject.mins,subject.maxs,arrival,subject.s.number,mask,(EG2_Collision)0,0);
 return !space.startsolid&&!space.allsolid&&space.fraction>=1;
}

bool TeleportPose(const gentity_t& source,const gentity_t& actor,const jar::teleport::Settings& settings,
                  vec3_t origin,vec3_t angles){
 const auto* target=G_TeleportDestination(source);
 if(!target||!DestinationPosition(*target,actor,origin))return false;
 VectorCopy(settings.keepView?actor.client->ps.viewangles:target->currentAngles,angles);
 for(int i=0;i<3;++i)if(!std::isfinite(angles[i]))return false;
 vec3_t arrival;VectorCopy(origin,arrival);arrival[2]+=1; // Native teleport offset.
 return TeleportClear(actor,arrival,MASK_PLAYERSOLID);
}

#include "g_sandbox_vehicle_teleport.inc"

bool TeleportActor(gentity_t& source,gentity_t* actor,bool use){
 jar::teleport::Settings settings;
 if(!G_TeleportRead(source,settings)||!settings.enabled||!G_TeleportActorMatches(actor,settings)||
    TeleportedThisFrame(*actor))return false;
 if(!G_JAR_WorkshopTransportAllowed(*actor))return !use&&G_JAR_WorkshopTeleportAssembly(source,*actor);
 vec3_t origin,angles;
 if(!TeleportPose(source,*actor,settings,origin,angles)||!G_ContactClockReady(source,*actor,settings.interval))return false;
 if(use&&source.behaviorSet[BSET_USE]){
  const auto sourceLife=G_EntityLifetime(&source),actorLife=G_EntityLifetime(actor);
  G_ActivateBehavior(&source,BSET_USE);
  if(G_EntityLifetime(&source)!=sourceLife||G_EntityLifetime(actor)!=actorLife||
     !G_TeleportRead(source,settings)||!settings.enabled||!G_TeleportActorMatches(actor,settings)||
     !G_JAR_WorkshopTransportAllowed(*actor)||
     TeleportedThisFrame(*actor)||!TeleportPose(source,*actor,settings,origin,angles))return false;
 }
 teleportArrivals[uint16_t(actor->s.number)]={G_EntityLifetime(actor),level.time};
 G_JAR_TeleportActor(actor,origin,angles,settings.keepVelocity);
 return true;
}
}

bool G_TeleportArrivalPending(const gentity_t& actor){
 if(actor.s.number<0||actor.s.number>=ENTITYNUM_WORLD||!TeleportSubjectLive(actor))return false;
 const auto entry=teleportArrivals.find(uint16_t(actor.s.number));
 return entry!=teleportArrivals.end()&&entry->second.life==G_EntityLifetime(&actor);
}

void G_TeleportMarkArrival(const gentity_t& actor){
 teleportArrivals[uint16_t(actor.s.number)]={G_EntityLifetime(&actor),level.time};
}

void G_TeleportRefreshArrival(const gentity_t& actor,const vec3_t origin){
 if(actor.s.number<0||actor.s.number>=ENTITYNUM_WORLD)return;
 const auto entry=teleportArrivals.find(uint16_t(actor.s.number));
 if(entry==teleportArrivals.end())return;
 if(G_TeleportArrivalPending(actor)){
  if(entry->second.frame==level.time)return;
  vec3_t hullMin,hullMax,mins,maxs;G_TeleportHull(actor,hullMin,hullMax);
  VectorAdd(origin,hullMin,mins);VectorAdd(origin,hullMax,maxs);
  gentity_t* nearby[MAX_GENTITIES];
  const int count=gi.EntitiesInBox(mins,maxs,nearby,MAX_GENTITIES);
  for(int i=0;i<count;++i){
   const auto& volume=*nearby[i];
   if(G_TeleportKind(volume)==jar::teleport::Volume&&volume.contents&CONTENTS_TRIGGER&&
      jar::volume::Contact(volume.pos1,volume.pos2,volume.currentOrigin,volume.currentAngles,
                           origin,hullMin,hullMax))return;
  }
 }
 teleportArrivals.erase(entry);
}

bool G_TeleportTouchAt(gentity_t& source,gentity_t* actor,const vec3_t origin){
 if(!(actor&&G_TeleportKind(source)==jar::teleport::Volume&&(source.contents&CONTENTS_TRIGGER)&&
  !(source.svFlags&SVF_INACTIVE)&&!G_TeleportArrivalPending(*actor)&&
  jar::volume::Contact(source.pos1,source.pos2,source.currentOrigin,source.currentAngles,origin,actor->mins,actor->maxs)))return false;
 if(actor->m_pVehicle&&G_TransportVehicleHull(actor)==actor)return TeleportVehicle(source,*actor,actor,false);
 return TeleportActor(source,actor,false);
}

bool G_TeleportUse(gentity_t& source,gentity_t* actor){
 if(auto* hull=G_TransportVehicleHull(actor))return TeleportVehicle(source,*hull,actor,true);
 return TeleportActor(source,actor,true);
}

std::vector<TeleportCrossing> G_TeleportCrossings(gentity_t& object,const vec3_t previous,const vec3_t end,
 bool clipped,bool assembly){
 using namespace jar::teleport;
 const unsigned type=G_TeleportObjectKind(object);
 if(!type||G_JAR_WorkshopControls(&object))return {};
 G_TeleportRefreshArrival(object,previous);
 if(G_TeleportArrivalPending(object))return {};

 vec3_t hullMin,hullMax,lo,hi;
 G_TeleportHull(object,hullMin,hullMax);
 for(int i=0;i<3;++i){
  lo[i]=std::fmin(previous[i],end[i])+hullMin[i];
  hi[i]=std::fmax(previous[i],end[i])+hullMax[i];
 }
 gentity_t* candidates[MAX_GENTITIES];
 const int count=gi.EntitiesInBox(lo,hi,candidates,MAX_GENTITIES);
 std::vector<TeleportCrossing> crossings;
 for(int i=0;i<count;++i){
  auto& source=*candidates[i];Settings settings;
  if(G_TeleportKind(source)!=Volume||!(source.contents&CONTENTS_TRIGGER)||
     !G_TeleportRead(source,settings)||!settings.enabled)continue;
  if(type==Mover&&!(assembly&&settings.props&&settings.assemblies&&settings.movers))continue;
  if(type==Prop&&!(settings.props&&(!assembly||settings.assemblies)))continue;
  if(type==Projectile&&!settings.projectiles)continue;
  float fraction;
  if(jar::volume::Sweep(source.pos1,source.pos2,source.currentOrigin,source.currentAngles,
                         previous,end,hullMin,hullMax,fraction)&&(!clipped||fraction<.9999f))
   crossings.push_back({fraction,&source,settings});
 }
 return crossings;
}

template<class Move>
static bool SweepTeleportObject(gentity_t& object,const vec3_t previous,const vec3_t end,bool clipped,Move move){
 auto crossings=G_TeleportCrossings(object,previous,end,clipped);
 std::sort(crossings.begin(),crossings.end(),[](const TeleportCrossing& a,const TeleportCrossing& b){
  return a.fraction!=b.fraction?a.fraction<b.fraction:a.source->s.number<b.source->s.number;
 });
 for(const auto& crossing:crossings){
  const auto* target=G_TeleportDestination(*crossing.source);vec3_t arrival;
  if(!target||!DestinationPosition(*target,object,arrival))continue;
  arrival[2]+=1;
  const int mask=G_TeleportObjectKind(object)==jar::teleport::Projectile?(MASK_SHOT|object.clipmask):(MASK_SOLID|CONTENTS_BODY);
  if(!TeleportClear(object,arrival,mask)||
     !G_ContactClockReady(*crossing.source,object,crossing.settings.interval)||
     !move(object,arrival,*target,crossing.settings))continue;
  G_TeleportMarkArrival(object);
  return true;
 }
 return false;
}

bool G_TeleportSweepObject(gentity_t& object,const vec3_t previous,const vec3_t end,bool clipped,bool rigid,TeleportObjectMove move){
 return SweepTeleportObject(object,previous,end,clipped,[&](gentity_t& traveler,const vec3_t arrival,
  const gentity_t& target,const jar::teleport::Settings& settings){return move(traveler,arrival,target,settings,rigid);});
}

static bool MoveTeleportedProjectile(gentity_t& projectile,const vec3_t origin,const gentity_t& target,
 const jar::teleport::Settings& settings,const float* rollingVelocity){
 const bool atRest=G_TeleportRestingProjectile(projectile);
 vec3_t velocity;
 if(rollingVelocity)VectorCopy(rollingVelocity,velocity);
 else if(atRest)VectorClear(velocity);
 else EvaluateTrajectoryDelta(&projectile.s.pos,level.time,velocity);
 const float speed=VectorLength(velocity);
 if(!std::isfinite(speed)||(!atRest&&!rollingVelocity&&speed<=.001f))return false;
 if(!settings.keepView&&speed>.001f){
  for(float angle:target.currentAngles)if(!std::isfinite(angle))return false;
  AngleVectors(target.currentAngles,velocity,nullptr,nullptr);
  VectorScale(velocity,speed,velocity);
 }
 VectorCopy(origin,projectile.currentOrigin);
 VectorCopy(origin,projectile.s.pos.trBase);
 VectorCopy(velocity,projectile.s.pos.trDelta);
 projectile.s.pos.trTime=level.time;
 if(rollingVelocity||atRest)projectile.s.groundEntityNum=ENTITYNUM_NONE;
 if(projectile.e_ThinkFunc==thinkF_rocketThink&&speed>.001f)VectorScale(velocity,1.f/speed,projectile.movedir);
 projectile.s.eFlags^=EF_TELEPORT_BIT;
 gi.linkentity(&projectile);
 return true;
}

bool G_TeleportSweepProjectile(gentity_t& projectile,const vec3_t previous,const vec3_t end,bool clipped,
 const float* rollingVelocity){
 if(G_TeleportObjectKind(projectile)!=jar::teleport::Projectile||
    G_TeleportRollingProjectile(projectile)!=(rollingVelocity!=nullptr))return false;
 return SweepTeleportObject(projectile,previous,end,clipped,[&](gentity_t& traveler,const vec3_t arrival,
  const gentity_t& target,const jar::teleport::Settings& settings){
   return MoveTeleportedProjectile(traveler,arrival,target,settings,rollingVelocity);
 });
}

void G_TeleportArrivalsReset(){teleportArrivals.clear();}

void G_TeleportArrivalsExport(bool autosave,ojk::SavedGameHelper& save){
 std::vector<uint16_t> live;
 if(!autosave)for(const auto& entry:teleportArrivals){
  if(entry.first<globals.num_entities&&G_TeleportArrivalPending(g_entities[entry.first]))live.push_back(entry.first);
 }
 save.write<uint32_t>(uint32_t(live.size()));
 for(const auto slot:live)save.write<uint16_t>(slot);
}

bool G_TeleportArrivalsImport(bool autosave,ojk::SavedGameHelper& save){
 uint32_t count;save.read<uint32_t>(count);
 if(count>ENTITYNUM_WORLD||(autosave&&count))return false;
 decltype(teleportArrivals) candidate;
 for(uint32_t i=0;i<count;++i){
  uint16_t slot;save.read<uint16_t>(slot);
  if(slot>=ENTITYNUM_WORLD||slot>=globals.num_entities||g_entities[slot].s.number!=slot||
     !TeleportSubjectLive(g_entities[slot])||
     !candidate.emplace(slot,TeleportArrival{G_EntityLifetime(&g_entities[slot]),std::numeric_limits<int>::min()}).second)return false;
 }
 teleportArrivals.swap(candidate);
 return true;
}
