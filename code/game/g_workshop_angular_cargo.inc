// Single-player platform adapters. Included beside the vertical lift helpers.
#include "g_angular_cargo.h"
#include "g_sandbox_angular.h"

AngularCargoMember angularCargoMember(gentity_t& e,int vehicle){
 AngularCargoMember member{};
 member.entity=&e;member.vehicle=vehicle;member.linked=e.linked!=qfalse;
 VectorCopy(e.mins,member.mins);VectorCopy(e.maxs,member.maxs);
 VectorCopy(e.currentAngles,member.angles);
 return member;
}

bool angularCargoAdd(AngularCargo& cargo,const AngularCargoMember& member){
 if(cargo.members.size()>=JAR_LIFT_MAX_RIDERS)return false;
 for(const auto& other:cargo.members)if(other.entity==member.entity)return false;
 for(int axis=0;axis<3;++axis)
  if(!std::isfinite(member.destination[axis])||std::fabs(member.destination[axis])>65535||
     !std::isfinite(member.angles[axis])||!std::isfinite(member.mins[axis])||
     !std::isfinite(member.maxs[axis])||member.mins[axis]>member.maxs[axis])return false;
 cargo.members.push_back(member);
 return true;
}

void angularCargoPosition(const jarLiftBody_t& deck,const jarLiftBody_t& next,const vec3_t before,const vec3_t after,
 const gentity_t& e,AngularCargoMember& member,float contactGap=0){
 vec3_t offset,rotated;
 VectorSubtract(e.currentOrigin,deck.origin,offset);
 if(VectorCompare(before,after))VectorCopy(offset,rotated);
 else jar::angular::RebaseVector(offset,before,after,rotated);
 VectorAdd(next.origin,rotated,member.destination);
 if(G_JAR_PortableDeck(g_entities[deck.entity])&&VectorCompare(before,after))return;
 if(member.geometry){
  jar::physics::NativePropHull hull;
  if(hull.Build(*member.geometry,pv(member.angles))){
   float nextGap;
   // A tip outside the deck cannot determine the supported height.
   // With no destination contact, keep the rotated pose and let physics settle it.
   if(platformPropGap(next,member.destination,hull,nextGap))
    member.destination[2]+=std::max(.125f,contactGap)-nextGap;
   return;
  }
 }
 const auto body=liftBody(e);
 member.destination[2]=next.origin[2]+next.maxs[2]-member.mins[2]+std::max(.125f,JAR_LiftGap(&deck,&body));
}

bool angularCargoVehicle(gentity_t& hull,const jarLiftBody_t& deck,const jarLiftBody_t& next,
 const vec3_t before,const vec3_t after,AngularCargo& cargo){
 jarLiftBody_t seats[JAR_LIFT_MAX_RIDERS+1];int count=0;
 if(!liftVehicleMembers(hull,seats,count))return false;
 auto body=angularCargoMember(hull,hull.s.number);
 angularCargoPosition(deck,next,before,after,hull,body);

 // Native vehicles stay upright relative to their own suspension. Carry
 // their heading and attached seat frame by the projected deck turn.
 const auto& vehicle=*hull.m_pVehicle;
 const vec3_t heading={0,vehicle.m_vOrientation[YAW],0};
 vec3_t forward,turned;
 AngleVectors(heading,forward,nullptr,nullptr);
 jar::angular::RebaseVector(forward,before,after,turned);
 if(std::hypot(turned[0],turned[1])>1e-4f)
  body.yaw=AngleSubtract(float(std::atan2(turned[1],turned[0])*180/M_PI),heading[YAW]);
 body.angles[YAW]=AngleNormalize180(body.angles[YAW]+body.yaw);
 if(!angularCargoAdd(cargo,body))return false;
 const vec3_t yaw={0,body.yaw,0};
 for(int i=0;i<count;++i){
  auto& e=g_entities[seats[i].entity];
  if(e.owner!=&hull)return false;
  auto seat=angularCargoMember(e,hull.s.number);
  vec3_t offset,rotated;
  VectorSubtract(e.currentOrigin,hull.currentOrigin,offset);
  jar::angular::RebaseVector(offset,vec3_origin,yaw,rotated);
  VectorAdd(body.destination,rotated,seat.destination);
  seat.yaw=body.yaw;seat.angles[YAW]=AngleNormalize180(seat.angles[YAW]+seat.yaw);
  if(!angularCargoAdd(cargo,seat))return false;
 }
 return true;
}

bool angularCargoProp(gentity_t& e,const Prop& definition,const jarLiftBody_t& deck,const jarLiftBody_t& next,
 const vec3_t before,const vec3_t after,float contactGap,AngularCargo& cargo){
 auto member=angularCargoMember(e,ENTITYNUM_NONE);
 member.geometry=platformPropGeometry(e);
 if(!VectorCompare(before,after))jar::angular::RebaseAngles(e.currentAngles,before,after,member.angles);
 if(!propHull(definition,member.angles,member.mins,member.maxs,e.s.frame))return false;
 angularCargoPosition(deck,next,before,after,e,member,contactGap);
 if(!angularCargoAdd(cargo,member))return false;
 if(!G_SandboxChargesPlanPose(e,member.destination,member.angles,cargo.charges,JAR_LIFT_QUERY))return false;
 jar::physics::Pose pose;
 if(physicsWorld.Read(e.s.number,pose)){
  pose.position=pv(member.destination);pose.angles=pv(member.angles);
  cargo.rigid.push_back({uint32_t(e.s.number),pose,false});
 }
 return true;
}

bool angularCargoRack(gentity_t& rack,const jarLiftBody_t& deck,const jarLiftBody_t& next,
 const vec3_t before,const vec3_t after,AngularCargo& cargo){
 auto body=angularCargoMember(rack,ENTITYNUM_NONE);
 if(!VectorCompare(before,after))jar::angular::RebaseAngles(rack.currentAngles,before,after,body.angles);
 angularCargoPosition(deck,next,before,after,rack,body);
 if(!angularCargoAdd(cargo,body))return false;
 RackStockPlan stock;
 if(!planRackStock(rack,body.destination,body.angles,stock))return false;
 for(std::size_t i=0;i<stock.count;++i){
  const auto& move=stock.members[i];
  auto item=angularCargoMember(*move.item,ENTITYNUM_NONE);
  item.rackSupport=rack.s.number;
  VectorCopy(move.origin,item.destination);VectorCopy(move.angles,item.angles);
  if(!angularCargoAdd(cargo,item))return false;
 }
 return true;
}

bool angularCargoClear(const AngularCargoMember& member,const float* arcPad=nullptr,const float* deckOrigin=nullptr){
 const auto& e=*member.entity;
 const int mask=MASK_PLAYERSOLID|CONTENTS_CORPSE|e.clipmask;
 if(member.geometry&&gi.JAR_TraceMovingHull){
  const auto after=pv(member.angles);
  const auto before=pv(e.currentAngles);
  if(!arcPad||VectorCompare(e.currentAngles,member.angles)){
   JarSweepHull shape;
   if(jar::physics::BuildPropSweep(*member.geometry,after,shape))
    return platformHullClear(shape,arcPad?e.currentOrigin:member.destination,member.destination,e.s.number,mask);
  }else if(deckOrigin){
   // Measure the cargo's rotation around the deck, then add any deck
   // translation and the height adjustment used to keep the cargo supported.
   const auto pivot=pv(deckOrigin);
   jar::physics::PhysgunGrip anchor;
   if(!anchor.Begin(pv(e.currentOrigin),before,pivot,0))return false;
   const auto end=anchor.Origin(pivot,after);
   const auto& destination=member.destination;
   const jar::physics::Vec translation={destination[0]-end.x,destination[1]-end.y,destination[2]-end.z};
   struct Query {const float* pivot;int entity,mask;} query{deckOrigin,e.s.number,mask};
   return jar::physics::TracePropRotation(*member.geometry,before,after,&query,
    [](void* context,const JarSweepHull& shape){
     const auto& request=*static_cast<Query*>(context);
     return platformHullClear(shape,request.pivot,request.pivot,request.entity,request.mask);
    },anchor.localPoint,translation);
  }
 }
 trace_t hit{};
 if(arcPad){
  vec3_t from,to,mins,maxs;
  for(int axis=0;axis<3;++axis){
   from[axis]=e.currentOrigin[axis]+(e.mins[axis]+e.maxs[axis])*.5f;
   to[axis]=member.destination[axis]+(member.mins[axis]+member.maxs[axis])*.5f;
   maxs[axis]=std::max(e.maxs[axis]-e.mins[axis],member.maxs[axis]-member.mins[axis])*.5f+arcPad[axis];
   mins[axis]=-maxs[axis];
  }
  gi.trace(&hit,from,mins,maxs,to,e.s.number,mask,G2_NOCOLLIDE,0);
 }else gi.trace(&hit,member.destination,member.mins,member.maxs,member.destination,e.s.number,mask,G2_NOCOLLIDE,0);
 return !hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
}

bool angularCargoBoxesDisjoint(const AngularCargoMember& a,const AngularCargoMember& b){
 for(int axis=0;axis<3;++axis){
  const float low=std::max(a.destination[axis]+a.mins[axis],b.destination[axis]+b.mins[axis]);
  const float high=std::min(a.destination[axis]+a.maxs[axis],b.destination[axis]+b.maxs[axis]);
  if(high-low<=.125f)return true;
 }
 return false;
}

bool angularCargoHullsDisjoint(const AngularCargoMember& a,const AngularCargoMember& b){
 if(angularCargoBoxesDisjoint(a,b))return true;
 const auto& fixed=a.geometry?a:b;
 const auto& moving=a.geometry?b:a;
 if(!fixed.geometry)return false;
 jar::physics::NativePropHull hull;
 if(!hull.Build(*fixed.geometry,pv(fixed.angles)))return false;
 vec3_t delta;
 VectorSubtract(moving.destination,fixed.destination,delta);
 const auto position=pv(delta);
 if(!moving.geometry)
  return hull.Trace(position,position,pv(moving.mins),pv(moving.maxs)).depth<=.125f;
 JarSweepHull shape;
 if(!jar::physics::BuildPropSweep(*moving.geometry,pv(moving.angles),shape))return false;
 return hull.TraceHull(position,position,shape).depth<=.125f;
}

bool angularCargoPairClear(const AngularCargoMember& a,const AngularCargoMember& b){
 if((a.vehicle!=ENTITYNUM_NONE&&a.vehicle==b.vehicle)||!a.linked||!b.linked)return true;
 const bool shelfA=a.rackSupport!=ENTITYNUM_NONE,shelfB=b.rackSupport!=ENTITYNUM_NONE;
 if((shelfA&&shelfB)||a.rackSupport==b.entity->s.number||b.rackSupport==a.entity->s.number)return true;
 const auto solid=[](const gentity_t& e){
  const int clip=workshopRack(e)?CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP:0;
  return (e.contents&(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE|clip))!=0;
 };
 if((!shelfA&&!solid(*a.entity))||(!shelfB&&!solid(*b.entity)))return true;
 return angularCargoHullsDisjoint(a,b);
}

bool angularCargoChargesFit(const AngularCargo& cargo){
 for(size_t i=0;i<cargo.charges.size();++i){
  const auto& move=cargo.charges[i];
  auto charge=angularCargoMember(*move.charge,ENTITYNUM_NONE);
  VectorCopy(move.origin,charge.destination);
  for(const auto& member:cargo.members){
   if(member.linked&&member.entity!=move.support&&(member.entity->contents&MASK_SHOT)&&
      !angularCargoHullsDisjoint(charge,member))return false;
  }
  for(size_t j=0;j<i;++j){
   const auto& other=cargo.charges[j];
   if(other.support==move.support||!(other.charge->contents&MASK_SHOT)||!(move.charge->contents&MASK_SHOT))continue;
   auto otherCharge=angularCargoMember(*other.charge,ENTITYNUM_NONE);
   VectorCopy(other.origin,otherCharge.destination);
   if(!angularCargoBoxesDisjoint(charge,otherCharge))return false;
  }
 }
 return true;
}

bool angularCargoPlan(gentity_t& e,const vec3_t destination,const vec3_t angles,AngularCargo& cargo){
 const auto deck=liftBody(e);
 auto next=deck;VectorCopy(destination,next.origin);
 if(G_JAR_PortableDeck(e))G_JAR_ProjectedDeckHull(e,angles,next.mins,next.maxs);
 else G_SandboxAngularHull(angles,next.mins,next.maxs);
 vec3_t low,high;JAR_LiftQuery(&deck,low,high);
 gentity_t* candidates[JAR_LIFT_QUERY];
 const int found=gi.EntitiesInBox(low,high,candidates,JAR_LIFT_QUERY);
 if(found>=JAR_LIFT_QUERY)return false;
 std::set<uint32_t> carried;
 for(int i=0;i<found;++i){
  auto& entity=*candidates[i];
  if(!entity.inuse||!entity.linked||&entity==&e||rackStockAttached(entity))continue;
  const bool rack=liftRackCandidate(entity);
  if(!rack&&!(entity.contents&(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE)))continue;
  const auto body=liftBody(entity);
  const bool standing=(entity.client?entity.client->ps.groundEntityNum:entity.s.groundEntityNum)==deck.entity;
  const auto* ownedProp=owned(entity.s.number);
  const auto* definition=ownedProp&&matches(*ownedProp)?prop(ownedProp->catalog):nullptr;
  if(entity.m_pVehicle){
   if(!liftVehicleSupported(entity,deck,body,standing))continue;
   if(!angularCargoVehicle(entity,deck,next,e.currentAngles,angles,cargo))return false;
  }else if(definition){
   if(!standing&&entity.s.pos.trDelta[2]>64)continue;
   jar::physics::Pose rigid;
   const bool simulated=physicsWorld.Read(entity.s.number,rigid);
   float contactGap;
   if(!platformPropSupported(deck,body,entity,standing?3.f:simulated?1.5f:1.f,&contactGap))continue;
   if(ownedProp->frozen||!liftSafeMember(entity)||
      !angularCargoProp(entity,*definition,deck,next,e.currentAngles,angles,contactGap,cargo))return false;
  }else if(rack&&JAR_LiftSupported(&deck,&body,standing)){
   if(!liftSafeMember(entity)||!angularCargoRack(entity,deck,next,e.currentAngles,angles,cargo))return false;
  }
  // On-foot riders still use the native pusher in the same transaction.
  if(standing&&(!entity.client||!entity.s.m_iVehicleNum))carried.insert(attachmentKey(entity));
 }
 for(const auto& member:cargo.members)carried.insert(attachmentKey(*member.entity));
 if((G_JAR_PortableDeck(e)||!cargo.members.empty())&&!platformLinksFit(e,carried))return false;
 if(cargo.members.empty())return true;
 for(size_t i=0;i<cargo.members.size();++i)for(size_t j=0;j<i;++j)
  if(!angularCargoPairClear(cargo.members[i],cargo.members[j]))return false;
 if(!angularCargoChargesFit(cargo))return false;

 AngularCargoUnlinked unlinked(cargo);
 if(deck.linked)gi.unlinkentity(&e);
 const auto spin=jar::physics::AngularVelocity(pv(e.currentAngles),pv(angles),1);
 vec3_t axis={spin.x,spin.y,spin.z},padding;
 const float sag=1-std::cos(.5f*VectorNormalize(axis));
 for(int i=0;i<3;++i)padding[i]=sag*std::sqrt(std::max(0.f,1-axis[i]*axis[i]));
 const bool clear=std::all_of(cargo.members.begin(),cargo.members.end(),[&](const auto& member){
  vec3_t pad;
  const float radius=Distance(deck.origin,member.entity->currentOrigin)+RadiusFromBounds(member.mins,member.maxs);
  VectorScale(padding,radius,pad);
  return angularCargoClear(member,pad,deck.origin);
 })&&std::all_of(cargo.charges.begin(),cargo.charges.end(),[&](const auto& move){
  const auto& charge=*move.charge;
  vec3_t pad;
  const float radius=Distance(deck.origin,charge.currentOrigin)+RadiusFromBounds(charge.mins,charge.maxs);
  VectorScale(padding,radius,pad);
  return G_SandboxChargeFits(move,charge.currentOrigin,pad);
 });
 if(deck.linked)gi.linkentity(&e);
 return clear;
}

bool angularCargoCommit(gentity_t& deck,const AngularCargo& cargo){
 for(const auto& member:cargo.members)if(!angularCargoClear(member))return false;
 for(const auto& move:cargo.charges)if(!G_SandboxChargeFits(move))return false;
 if(!physicsWorld.CorrectPoses(cargo.rigid))return false;
 for(const auto& member:cargo.members){
  auto& entity=*member.entity;
  platformPlace(entity,member.destination,deck.s.number);
  G_SetAngles(&entity,member.angles);
  VectorCopy(member.mins,entity.mins);VectorCopy(member.maxs,entity.maxs);
  if(entity.client&&member.yaw){
   vec3_t view;VectorCopy(entity.client->ps.viewangles,view);view[YAW]=AngleNormalize180(view[YAW]+member.yaw);
   SetClientViewAngle(&entity,view);
  }
  if(entity.m_pVehicle&&member.yaw){
   auto& vehicle=*entity.m_pVehicle;
   vehicle.m_vOrientation[YAW]=AngleNormalize180(vehicle.m_vOrientation[YAW]+member.yaw);
   vehicle.m_vPrevOrientation[YAW]=AngleNormalize180(vehicle.m_vPrevOrientation[YAW]+member.yaw);
   const vec3_t yaw={0,member.yaw,0};
   jar::angular::RebaseVector(entity.client->ps.moveDir,vec3_origin,yaw,entity.client->ps.moveDir);
  }
 }
 G_SandboxChargesPlace(cargo.charges);
 return true;
}
