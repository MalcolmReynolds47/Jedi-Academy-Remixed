// Native Sandbox push dispatch. Authored trigger callbacks remain separate.
#include "g_sandbox_push.h"
#include "g_logic_settings.h"
#include "g_contact_clocks.h"
#include "g_subbsp_ownership.h"
#include "g_workshop.h"
#include "g_sandbox_vehicle_transport.h"
#include "g_sandbox_vehicle_push.h"

bool G_PushVelocity(const gentity_t& source,const jar::push::Settings& settings,
 const vec3_t subject,const vec3_t current,vec3_t next){
 using namespace jar::push;
 vec3_t direction;
 if(settings.aim==Forward)AngleVectors(source.currentAngles,direction,nullptr,nullptr);
 else{
  const auto* target=G_PushTarget(source);
  if(!target)return false;
  vec3_t from;
  VectorCopy(settings.aim==Toward?subject:source.currentOrigin,from);
  if(settings.aim==Linked&&settings.kind==Volume){
   vec3_t axes[3];AnglesToAxis(source.currentAngles,axes);
   for(int i=0;i<3;++i)VectorMA(from,.5f*(source.pos1[i]+source.pos2[i]),axes[i],from);
  }
  VectorSubtract(target->currentOrigin,from,direction);
 }
 const float magnitude=VectorNormalize(direction);
 if(!std::isfinite(magnitude)||magnitude<.001f)return false;
 VectorMA(settings.additive?current:vec3_origin,settings.speed,direction,next);
 const float speed=VectorLength(next);
 if(!std::isfinite(speed))return false;
 if(speed>maxSpeed)VectorScale(next,maxSpeed/speed,next);
 return true;
}

static bool PushActorEligible(const gentity_t& actor,const jar::push::Settings& settings){
 using namespace jar::push;
 return G_LogicLiveActor(&actor)&&actor.client->ps.pm_type==PM_NORMAL&&
  !actor.m_pVehicle&&!actor.s.m_iVehicleNum&&
  !(actor.owner&&actor.owner->inuse&&actor.owner->m_pVehicle)&&!G_JAR_WorkshopControls(&actor)&&
  settings.subject!=Props&&(settings.subject!=Player||actor.s.number==0)&&
  (settings.subject!=NPCs||(actor.s.number!=0&&actor.NPC));
}

static void CommitPush(gentity_t& actor,const vec3_t velocity){
 auto& ps=actor.client->ps;
 VectorCopy(velocity,ps.velocity);
 if(velocity[2]>0){ps.groundEntityNum=ENTITYNUM_NONE;actor.s.groundEntityNum=ENTITYNUM_NONE;}
 ps.forceJumpZStart=0;
 ps.jumpZStart=ps.origin[2];
 ps.pm_flags|=PMF_TRIGGER_PUSHED;
}

static void PushActor(gentity_t& source,gentity_t& actor,const vec3_t origin,bool used){
 jar::push::Settings settings;
 if(!G_PushRead(source,settings)||!settings.enabled||!PushActorEligible(actor,settings))return;
 vec3_t velocity;
 if(!G_PushVelocity(source,settings,origin,actor.client->ps.velocity,velocity)||
    !G_ContactClockReady(source,actor,settings.interval))return;
 if(used){
  const auto life=G_EntityLifetime(&source),actorLife=G_EntityLifetime(&actor);
  G_ActivateBehavior(&source,BSET_USE);
  if(!source.inuse||G_EntityLifetime(&source)!=life||!PushActorEligible(actor,settings)||
     G_EntityLifetime(&actor)!=actorLife)return;
 }
 CommitPush(actor,velocity);
}

static void PushVehicle(gentity_t& source,gentity_t& hull,gentity_t& activator,const vec3_t origin,bool used){
 using namespace jar::push;
 Settings settings;
 vec3_t velocity,drift;
 const auto prepare=[&](const vec3_t from){
  if(!G_PushRead(source,settings)||!settings.enabled||!settings.vehicles||!G_TransportVehicleReady(hull))return false;
  const auto& vehicle=*hull.m_pVehicle;
  for(const auto* member:{&hull,vehicle.m_pPilot,vehicle.m_pDroidUnit}){
   if(member&&(member->client->ps.eFlags&JAR_VEHICLE_IMMOBILE_FLAGS))return false;
  }
  const auto& ps=hull.client->ps;
  return G_PushVelocity(source,settings,from,ps.velocity,velocity)&&
   G_VehiclePushDrift(ps,vehicle.jarPushDrift,velocity,settings.additive,drift);
 };
 if(!prepare(origin)||!G_ContactClockReady(source,hull,settings.interval))return;
 if(used&&source.behaviorSet[BSET_USE]){
  const auto sourceLife=G_EntityLifetime(&source),hullLife=G_EntityLifetime(&hull);
  const auto activatorLife=G_EntityLifetime(&activator);
  const auto* pilot=hull.m_pVehicle->m_pPilot;
  const auto* droid=hull.m_pVehicle->m_pDroidUnit;
  const auto pilotLife=pilot?G_EntityLifetime(pilot):0;
  const auto droidLife=droid?G_EntityLifetime(droid):0;
  G_ActivateBehavior(&source,BSET_USE);
  if(G_EntityLifetime(&source)!=sourceLife||G_EntityLifetime(&hull)!=hullLife||
     G_EntityLifetime(&activator)!=activatorLife||G_TransportVehicleHull(&activator)!=&hull||
     hull.m_pVehicle->m_pPilot!=pilot||hull.m_pVehicle->m_pDroidUnit!=droid||
     (pilot&&G_EntityLifetime(pilot)!=pilotLife)||(droid&&G_EntityLifetime(droid)!=droidLife)||
     !prepare(hull.currentOrigin))return;
 }
 VectorCopy(drift,hull.m_pVehicle->jarPushDrift);
 if(!settings.additive)hull.client->ps.speed=0;
 CommitPush(hull,velocity);
 VectorCopy(velocity,hull.s.pos.trDelta);
}

void G_PushTouchAt(gentity_t& source,gentity_t* actor,const vec3_t origin){
 if(!actor||G_PushKind(source)!=jar::push::Volume||!(source.contents&CONTENTS_TRIGGER)||
    source.svFlags&SVF_INACTIVE)return;
 if(!jar::volume::Contact(source.pos1,source.pos2,source.currentOrigin,source.currentAngles,
                         origin,actor->mins,actor->maxs))return;
 if(actor->m_pVehicle&&G_TransportVehicleHull(actor)==actor)PushVehicle(source,*actor,*actor,origin,false);
 else PushActor(source,*actor,origin,false);
}

void G_PushUse(gentity_t& source,gentity_t* activator){
 if(!activator)return;
 if(auto* hull=G_TransportVehicleHull(activator))PushVehicle(source,*hull,*activator,hull->currentOrigin,true);
 else PushActor(source,*activator,activator->currentOrigin,true);
}

void G_PushPropSweep(gentity_t& prop,const vec3_t previous,bool rigid,
 void(*apply)(gentity_t&,const vec3_t,bool)){
 using namespace jar::push;
 vec3_t lo,hi;
 for(int i=0;i<3;++i){
  lo[i]=std::fmin(previous[i],prop.currentOrigin[i])+prop.mins[i];
  hi[i]=std::fmax(previous[i],prop.currentOrigin[i])+prop.maxs[i];
 }
 gentity_t* candidates[MAX_GENTITIES];
 const int count=gi.EntitiesInBox(lo,hi,candidates,MAX_GENTITIES);
 for(int i=0;i<count;++i){
  auto& volume=*candidates[i];Settings settings;
  if(!(volume.contents&CONTENTS_TRIGGER)||G_PushKind(volume)!=Volume||!G_PushRead(volume,settings)||
     !settings.enabled||settings.subject<Props)continue;
  float fraction;
  if(!jar::volume::Sweep(volume.pos1,volume.pos2,volume.currentOrigin,volume.currentAngles,
                        previous,prop.currentOrigin,prop.mins,prop.maxs,fraction))continue;
  vec3_t travel,point,velocity;
  VectorSubtract(prop.currentOrigin,previous,travel);VectorMA(previous,fraction,travel,point);
  if(G_PushVelocity(volume,settings,point,prop.s.pos.trDelta,velocity)&&
     G_ContactClockReady(volume,prop,settings.interval))apply(prop,velocity,rigid);
 }
}
