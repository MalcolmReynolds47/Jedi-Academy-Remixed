// Owned teleporter settings in native save fields. GPL-2.0-only.
#ifndef G_SANDBOX_TELEPORT_H
#define G_SANDBOX_TELEPORT_H
#include "g_volume_geometry.h"
#include "g_functions.h"
#include "g_logic_settings.h"
#include "g_bobbing_sandbox.h"
#include "g_sandbox_angular.h"
#include "g_sliding_door.h"
#include "g_rail_portable.h"
#include "g_bsp_section.h"
#include <vector>
namespace ojk {class SavedGameHelper;}

bool G_TeleportUse(gentity_t& source,gentity_t* actor);
bool G_TeleportTouchAt(gentity_t& source,gentity_t* actor,const vec3_t origin);
bool G_TeleportArrivalPending(const gentity_t& actor);
void G_TeleportMarkArrival(const gentity_t& actor);
void G_TeleportRefreshArrival(const gentity_t& actor,const vec3_t origin);
void G_TeleportArrivalsReset();
void G_TeleportArrivalsExport(bool autosave,ojk::SavedGameHelper& save);
bool G_TeleportArrivalsImport(bool autosave,ojk::SavedGameHelper& save);
void G_JAR_TeleportActor(gentity_t* actor,vec3_t origin,vec3_t angles,bool keepVelocity);
inline bool G_TeleportActorMatches(const gentity_t* actor,const jar::teleport::Settings& settings){
 using namespace jar::teleport;
 return G_LogicLiveActor(actor)&&actor->client->ps.pm_type==PM_NORMAL&&
  !(actor->svFlags&SVF_NO_TELEPORT)&&!actor->m_pVehicle&&!actor->s.m_iVehicleNum&&
  !(actor->owner&&actor->owner->inuse&&actor->owner->m_pVehicle)&&
  settings.subject!=NoActors&&(settings.subject!=Player||actor->s.number==0)&&
  (settings.subject!=NPCs||(actor->s.number!=0&&actor->NPC));
}
using TeleportObjectMove=bool(*)(gentity_t&,const vec3_t,const gentity_t&,const jar::teleport::Settings&,bool);
struct TeleportCrossing {float fraction;gentity_t* source;jar::teleport::Settings settings;};
std::vector<TeleportCrossing> G_TeleportCrossings(gentity_t& object,const vec3_t previous,const vec3_t end,
 bool clipped,bool assembly=false);
bool G_TeleportSweepObject(gentity_t& object,const vec3_t previous,const vec3_t end,bool clipped,bool rigid,TeleportObjectMove move);
bool G_TeleportSweepProjectile(gentity_t& projectile,const vec3_t previous,const vec3_t end,bool clipped,
 const float* rollingVelocity=nullptr);

inline bool G_TeleportRollingProjectile(const gentity_t& subject){
 return subject.s.weapon==WP_THERMAL&&subject.s.pos.trType==TR_INTERPOLATE;
}

inline bool G_TeleportRestingProjectile(const gentity_t& subject){
 return subject.s.pos.trType==TR_STATIONARY&&!(subject.s.eFlags&EF_MISSILE_STICK)&&
  ((subject.s.weapon==WP_THERMAL&&(subject.e_ThinkFunc==thinkF_WP_ThermalThink||
      subject.e_ThinkFunc==thinkF_thermalDetonatorExplode))||
   (subject.s.weapon==WP_FLECHETTE&&subject.e_ThinkFunc==thinkF_WP_flechette_alt_blow));
}

enum class TeleportMoverType { None, Bobbing, Slide, Rail, Brush, Train, Binary, Section, Angular };
inline bool G_TeleportBrushMover(TeleportMoverType type){
 return type==TeleportMoverType::Brush||type==TeleportMoverType::Train||type==TeleportMoverType::Binary;
}
inline TeleportMoverType G_TeleportMoverKind(const gentity_t& subject){
 if(subject.bmodel){
  if(jar::bspSection::Portable(subject))return TeleportMoverType::Section;
  if(!subject.inuse||subject.s.eType!=ET_MOVER||subject.s.modelindex<=0||!subject.model||subject.model[0]!='*'||
     !subject.classname)return TeleportMoverType::None;
  if(subject.e_ReachedFunc==reachedF_Reached_BinaryMover&&
     (!strcmp(subject.classname,"func_door")||!strcmp(subject.classname,"func_plat")||
      !strcmp(subject.classname,"func_button")))return TeleportMoverType::Binary;
  if((subject.flags&FL_TEAMSLAVE)||subject.teamchain||(subject.teammaster&&subject.teammaster!=&subject))
   return TeleportMoverType::None;
  if(!strcmp(subject.classname,"func_train")&&subject.e_ReachedFunc==reachedF_Reached_Train)
   return TeleportMoverType::Train;
  if(!strcmp(subject.classname,"func_bobbing")||!strcmp(subject.classname,"func_pendulum")||
     !strcmp(subject.classname,"func_rotating"))return TeleportMoverType::Brush;
  return TeleportMoverType::None;
 }
 if(G_BobbingPortable(subject))return TeleportMoverType::Bobbing;
 if(G_SandboxAngular(subject))return TeleportMoverType::Angular;
 if(G_SandboxSlidePortable(subject))return TeleportMoverType::Slide;
 if(G_RailPortable(subject)&&jar::rail::Class(subject.classname)==jar::rail::Mover)return TeleportMoverType::Rail;
 return TeleportMoverType::None;
}

inline void G_TeleportHull(const gentity_t& subject,vec3_t mins,vec3_t maxs){
 if(subject.bmodel&&(jar::bspSection::Portable(subject)||G_TeleportBrushMover(G_TeleportMoverKind(subject))))
  jar::volume::Hull(subject.mins,subject.maxs,subject.currentAngles,mins,maxs);
 else {VectorCopy(subject.mins,mins);VectorCopy(subject.maxs,maxs);}
}

inline unsigned G_TeleportObjectKind(const gentity_t& subject){
 if(!subject.inuse||subject.client||subject.freeAfterEvent||subject.svFlags&SVF_NO_TELEPORT)return 0;
 if(subject.classname&&!strcmp(subject.classname,"jar_sandbox_prop"))return jar::teleport::Prop;
 if(G_TeleportMoverKind(subject)!=TeleportMoverType::None)return jar::teleport::Mover;
 if(subject.s.eType!=ET_MISSILE||subject.s.weapon==WP_SABER||subject.s.eFlags&EF_HELD_BY_SAND_CREATURE)return 0;
 const bool ballistic=subject.s.pos.trType==TR_LINEAR||subject.s.pos.trType==TR_GRAVITY;
 if(subject.s.eFlags&EF_MISSILE_STICK){
  const bool thrownCharge=(subject.s.weapon==WP_DET_PACK&&subject.e_TouchFunc==touchF_charge_stick)||
   (subject.s.weapon==WP_TRIP_MINE&&subject.e_TouchFunc==touchF_touchLaserTrap);
  return ballistic&&thrownCharge?jar::teleport::Projectile:0;
 }
 return ballistic||G_TeleportRollingProjectile(subject)||G_TeleportRestingProjectile(subject)?jar::teleport::Projectile:0;
}

inline jar::teleport::Kind G_TeleportKind(const gentity_t& source){
 using namespace jar::teleport;
 if(!source.inuse||source.bmodel||!source.model)return None;
 const auto kind=Class(source.classname);
 if(kind==Volume&&!strcmp(source.model,volumeModel))return Volume;
 if(kind==Marker&&!strcmp(source.model,markerModel))return Marker;
 return None;
}
inline gentity_t* G_TeleportDestination(const gentity_t& source){
 auto* target=source.target&&*source.target?G_Find(nullptr,FOFS(targetname),source.target):nullptr;
 return target&&target->inuse&&target!=&source?target:nullptr;
}
inline bool G_TeleportRead(const gentity_t& source,jar::teleport::Settings& settings){
 using namespace jar::teleport;
 const auto kind=G_TeleportKind(source);
 if(!kind||(kind==Volume&&source.e_TouchFunc!=touchF_trigger_teleporter_touch)||
    (kind==Marker&&source.e_UseFunc!=useF_target_teleporter_use))return false;
 settings={kind,source.delay,source.count,bool(source.spawnflags&1),bool(source.spawnflags&2),
  !(source.svFlags&SVF_INACTIVE),bool(source.spawnflags&4),bool(source.spawnflags&8),bool(source.spawnflags&16),
  bool(source.spawnflags&32),bool(source.spawnflags&64),bool(source.spawnflags&128),bool(source.spawnflags&256)};
 return Valid(settings);
}
inline bool G_TeleportSet(gentity_t& source,const jar::teleport::Settings& settings){
 if(settings.kind!=G_TeleportKind(source)||!jar::teleport::Valid(settings))return false;
 source.delay=settings.interval;source.count=settings.subject;
 source.spawnflags=(settings.keepView?1:0)|(settings.keepVelocity?2:0)|(settings.props?4:0)|
  (settings.projectiles?8:0)|(settings.vehicles?16:0)|(settings.assemblies?32:0)|
  (settings.frozenProps?64:0)|(settings.movers?128:0)|(settings.worldAnchors?256:0);
 if(settings.enabled)source.svFlags&=~SVF_INACTIVE;
 else source.svFlags|=SVF_INACTIVE;
 return true;
}
inline bool G_TeleportInitialize(gentity_t& source){
 using namespace jar::teleport;
 const auto kind=Class(source.classname);
 if(!kind||source.bmodel||!source.model)return false;
 if(kind==Volume&&(strcmp(source.model,"@jar_box")||!jar::volume::Valid(source.mins,source.maxs)))return false;
 Settings settings{kind,defaultInterval};
 if(kind==Volume&&(source.spawnflags&4))settings.subject=Player;
 settings.enabled=!(source.svFlags&SVF_INACTIVE);
 if(gi.bIsFromZone(source.model,TAG_G_ALLOC))gi.Free(source.model);
 source.model=G_NewString(kind==Volume?volumeModel:markerModel);
 source.nextthink=0;source.e_ThinkFunc=thinkF_NULL;source.lastEnemy=nullptr;
 if(kind==Volume){
  VectorCopy(source.mins,source.pos1);VectorCopy(source.maxs,source.pos2);
  source.e_TouchFunc=touchF_trigger_teleporter_touch;source.contents=CONTENTS_TRIGGER;
  if(!jar::volumeEntity::Refresh(source))return false;
 }else source.e_UseFunc=useF_target_teleporter_use;
 if(!G_TeleportSet(source,settings))return false;
 gi.linkentity(&source);
 return true;
}
inline bool G_TeleportRestore(gentity_t& source){
 if(!jar::teleport::Class(source.classname))return true;
 if(!G_TeleportKind(source))return G_TeleportInitialize(source);
 jar::teleport::Settings settings;
 if(!G_TeleportRead(source,settings)||source.e_ThinkFunc!=thinkF_NULL||
    (settings.kind==jar::teleport::Volume&&!jar::volumeEntity::Refresh(source)))return false;
 gi.linkentity(&source);
 return true;
}
#endif
