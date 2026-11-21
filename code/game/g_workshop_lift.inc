// Single-player adapters for the shared platform transaction.
#include "qcommon/jar_lift_motion.h"
#include "g_sandbox_charges.h"
jarLiftBody_t liftBody(const gentity_t& e){
 jarLiftBody_t body{};body.entity=e.s.number;body.linked=e.linked;
 VectorCopy(e.currentOrigin,body.origin);VectorCopy(e.mins,body.mins);VectorCopy(e.maxs,body.maxs);
 return body;
}
std::shared_ptr<const jar::physics::PropGeometry> platformPropGeometry(const gentity_t& e){
 const auto* record=owned(e.s.number);
 const auto* model=record&&matches(*record)?prop(record->catalog):nullptr;
 if(!model)return {};
 auto geometry=propTransformGeometry(*model,e.s.frame);
 return geometry&&geometry->RigidHull()?geometry:nullptr;
}

bool platformPropGap(const jarLiftBody_t& deck,const vec3_t origin,
 const jar::physics::NativePropHull& hull,float& gap){
 constexpr float edgeTolerance=.125f;
 vec3_t relative;
 VectorSubtract(deck.origin,origin,relative);
 auto low=pv(deck.mins),high=pv(deck.maxs);
 low.x+=edgeTolerance;low.y+=edgeTolerance;
 high.x-=edgeTolerance;high.y-=edgeTolerance;
 return hull.BoxContactFromBelow(pv(relative),low,high,gap);
}

bool platformPropSupported(const jarLiftBody_t& deck,const jarLiftBody_t& body,
 const gentity_t& entity,float reach,float* contactGap=nullptr){
 if(!JAR_LiftOverlaps(&deck,&body))return false;
 float gap=JAR_LiftGap(&deck,&body);
 if(const auto* hull=propTraceHull(entity,entity.currentAngles))
  if(!platformPropGap(deck,body.origin,*hull,gap))return false;
 if(!(gap>=-.501f&&gap<=reach))return false;
 if(contactGap)*contactGap=gap;
 return true;
}

bool platformHullClear(const JarSweepHull& shape,const vec3_t start,const vec3_t end,int pass,int mask){
 trace_t hit{};
 vec3_t low,high;
 JAR_SweepBounds(shape,low,high);
 gi.JAR_TraceMovingHull(&hit,start,end,low,high,pass,mask,shape);
 return !hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
}

int liftClear(const jarLiftBody_t* body,const vec3_t destination){
 const int mask=MASK_PLAYERSOLID|CONTENTS_CORPSE;
 if(gi.JAR_TraceMovingHull){
  const auto& entity=g_entities[body->entity];
  const auto geometry=platformPropGeometry(entity);
  JarSweepHull shape;
  if(geometry&&jar::physics::BuildPropSweep(*geometry,pv(entity.currentAngles),shape))
   return platformHullClear(shape,body->origin,destination,body->entity,mask);
 }
 trace_t tr{};
 gi.trace(&tr,body->origin,body->mins,body->maxs,destination,body->entity,mask,(EG2_Collision)0,0);
 return !tr.startsolid&&!tr.allsolid&&tr.fraction>=1;
}
void platformPlace(gentity_t& e,const vec3_t destination,int deck){
 vec3_t offset;
 VectorSubtract(destination,e.currentOrigin,offset);
 VectorCopy(destination,e.currentOrigin);
 VectorCopy(destination,e.s.origin);VectorAdd(e.s.pos.trBase,offset,e.s.pos.trBase);
 if(G_LiftPortable(e))e.s.pos.trType=TR_INTERPOLATE;
 if(e.client)VectorAdd(e.client->ps.origin,offset,e.client->ps.origin);
 if(e.m_pVehicle&&e.m_pVehicle->m_LandTrace.entityNum==deck&&e.m_pVehicle->m_LandTrace.fraction<1)
  VectorAdd(e.m_pVehicle->m_LandTrace.endpos,offset,e.m_pVehicle->m_LandTrace.endpos);
}
bool liftVehicleSupported(const gentity_t& e,const jarLiftBody_t& deck,const jarLiftBody_t& body,bool standing){
 if(standing&&JAR_LiftSupported(&deck,&body,true))return true;
 const auto& vehicle=*e.m_pVehicle;
 if(!e.client||!vehicle.m_pVehicleInfo||!JAR_LiftOverlaps(&deck,&body))return false;
 const auto& info=*vehicle.m_pVehicleInfo;
 const bool fighter=info.type==VH_FIGHTER;
 const bool landed=fighter&&vehicle.m_LandTrace.entityNum==deck.entity&&
  vehicle.m_LandTrace.fraction<.15f&&vehicle.m_LandTrace.plane.normal[2]>=.7f;
 if(fighter&&!landed)return false;
 const float reach=JAR_LiftVehicleReach(info.hoverHeight,info.landingHeight,landed,e.client->ps.velocity,vehicle.m_ucmd.upmove>0);
 const float gap=JAR_LiftGap(&deck,&body);
 if(reach<=0||gap<-.5f||gap>reach)return false;
 vec3_t end;VectorCopy(body.origin,end);end[2]-=reach;
 trace_t trace{};
 gi.trace(&trace,body.origin,body.mins,body.maxs,end,e.s.number,MASK_PLAYERSOLID,(EG2_Collision)0,0);
 return !trace.startsolid&&!trace.allsolid&&trace.fraction<1&&trace.entityNum==deck.entity&&trace.plane.normal[2]>=.7f;
}
bool liftSafeMember(const gentity_t& e){
 return held!=unsigned(e.s.number)&&!nativeOwnedControl(e.s.number)&&
  (!e.client||!(e.client->ps.eFlags&(EF_FORCE_GRIPPED|EF_LOCKED_TO_WEAPON|EF_HELD_BY_RANCOR|EF_HELD_BY_WAMPA|EF_HELD_BY_SAND_CREATURE|EF_FORCE_DRAINED)));
}
bool liftRackCandidate(const gentity_t& e){
 if(!workshopRack(e))return false;
 const auto* ownedRack=owned(e.s.number);
 // A stationary rack can rest on the deck without a native ground slot.
 // A held or frozen rack can obstruct the deck but cannot move with it.
 return ownedRack&&matches(*ownedRack)&&(e.s.pos.trType==TR_STATIONARY||!liftSafeMember(e));
}
const gentity_t* liftSeatEntity(const gentity_t* seat){
 if(!seat)return nullptr;
 const auto address=reinterpret_cast<uintptr_t>(seat),base=reinterpret_cast<uintptr_t>(g_entities);
 if(address<base||address>=base+sizeof(gentity_t)*ENTITYNUM_WORLD||(address-base)%sizeof(gentity_t))return nullptr;
 return seat->inuse&&seat->client&&seat->s.number==(int)((address-base)/sizeof(gentity_t))?seat:nullptr;
}
bool liftVehicleMembers(const gentity_t& root,jarLiftBody_t* bodies,int& count){
 const auto& vehicle=*root.m_pVehicle;
 if(!root.client||root.health<=0||!vehicle.m_pVehicleInfo||vehicle.m_pParentEntity!=&root||
    vehicle.m_iBoarding||vehicle.m_iDieTime||(vehicle.m_pOldPilot&&vehicle.m_pOldPilot!=vehicle.m_pPilot)||
    !liftSafeMember(root))return false;
 for(const auto* pointer:{vehicle.m_pPilot,vehicle.m_pDroidUnit}){
  if(!pointer)continue;
  const auto* seat=liftSeatEntity(pointer);
  if(!seat||seat==&root||seat->health<=0||seat->m_pVehicle||
     seat->s.m_iVehicleNum!=root.s.number||
     !liftSafeMember(*seat))return false;
  const auto body=liftBody(*seat);
  if(!JAR_LiftAddBody(bodies,&count,&body))return false;
 }
 return true;
}
bool platformLinksFit(const gentity_t& deck,const std::set<uint32_t>& carried){
 const auto deckKey=attachmentKey(deck);
 for(unsigned i=0;i<state.jointCount;++i){
  const auto& joint=state.joints[i];
  if(joint.kind==jar::constraint::Thruster||joint.first==deckKey||joint.second==deckKey)continue;
  if(carried.count(joint.first)!=carried.count(joint.second))return false;
 }
 return true;
}

struct LiftBodiesUnlinked {
 const jarLiftBody_t* bodies;
 int count;
 LiftBodiesUnlinked(const jarLiftBody_t* group,int size):bodies(group),count(size){
  for(int i=0;i<count;++i)if(bodies[i].linked)gi.unlinkentity(&g_entities[bodies[i].entity]);
 }
 ~LiftBodiesUnlinked(){for(int i=0;i<count;++i)if(bodies[i].linked)gi.linkentity(&g_entities[bodies[i].entity]);}
};

bool liftMove(const jarLiftBody_t* bodies,int count,float dz){
 std::vector<SandboxChargeMove> charges;
 std::vector<jar::physics::PoseUpdate> rigid;
 for(int i=1;i<count;++i){
  auto& e=g_entities[bodies[i].entity];
  const auto* o=owned(e.s.number);
  if(!o||!prop(o->catalog))continue;
  vec3_t destination;VectorCopy(e.currentOrigin,destination);destination[2]+=dz;
  if(!G_SandboxChargesPlanPose(e,destination,e.currentAngles,charges,JAR_LIFT_QUERY))return false;
  jar::physics::Pose pose;
  if(physicsWorld.Read(e.s.number,pose)){
   pose.position=pv(destination);
   rigid.push_back({uint32_t(e.s.number),pose,o->frozen!=0});
  }
 }
 {
  LiftBodiesUnlinked riders(bodies,count);
  SandboxChargesUnlinked mounted(charges);
  if(!JAR_LiftSweep(bodies,count,dz,liftClear))return false;
  for(const auto& move:charges)if(!G_SandboxChargeFits(move,move.charge->currentOrigin))return false;
  if(!physicsWorld.CorrectPoses(rigid))return false;
  for(int i=0;i<count;++i){
   vec3_t destination;VectorCopy(bodies[i].origin,destination);destination[2]+=dz;
   platformPlace(g_entities[bodies[i].entity],destination,bodies[0].entity);
  }
  G_SandboxChargesPlace(charges);
 }
 G_SandboxChargesRefresh(charges);
 return true;
}

bool liftTranslate(gentity_t& deck,float dz){
 jarLiftBody_t bodies[JAR_LIFT_MAX_RIDERS+1];bodies[0]=liftBody(deck);int count=1;
 gentity_t* candidates[JAR_LIFT_QUERY];
 vec3_t low,high;JAR_LiftQuery(&bodies[0],low,high);
 const int found=gi.EntitiesInBox(low,high,candidates,JAR_LIFT_QUERY);
 if(found>=JAR_LIFT_QUERY)return false;
 for(int i=0;i<found;++i){
  const auto& e=*candidates[i];
  if(!e.inuse||!e.linked||&e==&deck||rackStockAttached(e))continue;
  const bool rack=liftRackCandidate(e);
  if(!rack&&!(e.contents&(CONTENTS_BODY|CONTENTS_SOLID|CONTENTS_ITEM|CONTENTS_CORPSE)))continue;
  const bool standing=e.client?e.client->ps.groundEntityNum==deck.s.number:e.s.groundEntityNum==deck.s.number;
  const auto* o=owned(e.s.number);
  const bool modelProp=o&&matches(*o)&&prop(o->catalog);
  const bool looseProp=modelProp&&!o->frozen;
  if(!e.m_pVehicle&&!standing&&((!rack&&!looseProp)||e.s.pos.trDelta[2]>64))continue;
  if(!e.m_pVehicle&&e.client&&e.s.m_iVehicleNum>0&&e.s.m_iVehicleNum<ENTITYNUM_WORLD)continue;
  const auto body=liftBody(e);
  if(e.m_pVehicle){
   if(!liftVehicleSupported(e,bodies[0],body,standing))continue;
   if(!JAR_LiftAddBody(bodies,&count,&body)||!liftVehicleMembers(e,bodies,count))return false;
   continue;
  }
  if(modelProp){
   if(!platformPropSupported(bodies[0],body,e,standing?3.f:1.f))continue;
  }else if(!JAR_LiftSupported(&bodies[0],&body,standing))continue;
  if(!liftSafeMember(e)||!JAR_LiftAddBody(bodies,&count,&body))return false;
  if(rack){
   RackStockPlan stock;
   vec3_t destination;VectorCopy(body.origin,destination);destination[2]+=dz;
   if(!planRackStock(e,destination,e.currentAngles,stock))return false;
   for(std::size_t item=0;item<stock.count;++item){
    const auto shelfBody=liftBody(*stock.members[item].item);
    if(!JAR_LiftAddBody(bodies,&count,&shelfBody))return false;
   }
  }
 }
 std::set<uint32_t> carried;
 for(int i=1;i<count;++i)carried.insert(attachmentKey(g_entities[bodies[i].entity]));
 return platformLinksFit(deck,carried)&&liftMove(bodies,count,dz);
}
