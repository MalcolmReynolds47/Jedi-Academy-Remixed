// Relocate a released component of props, actors and seated vehicle groups.
// GPL-2.0-only.
struct AssemblyTeleportMember {
 uint32_t id;
 gentity_t* entity;
 gentity_t* vehicle;
 jar::physics::Pose pose;
 jar::physics::Vec beforeAngles;
 vec3_t before,after;
 bool linked,rigid,frozen,reseated=false;
 TeleportMoverType mover=TeleportMoverType::None;
 TeleportMoverMotion moverMotion{};
 vec3_t mins,maxs;
 gentity_t* sectionRoot=nullptr;
};

struct AssemblyTeleportPlan {
 std::vector<AssemblyTeleportMember> members;
 std::vector<jar::constraint::Joint*> links;
 struct WorldAnchor {jar::constraint::Joint* joint;jar::constraint::Frame frame;};
 std::vector<WorldAnchor> anchors;
 std::vector<std::pair<jar::constraint::Joint*,jarRopePath_t>> ropes;
 std::vector<jar::physics::PoseUpdate> rigid;
 std::vector<SandboxChargeMove> charges;
 std::vector<VehicleTeleportPlan> vehicles;
 struct Trigger {gentity_t* entity;gentity_t* mover;vec3_t origin,base,trajectoryBase;};
 std::vector<Trigger> triggers;
};

struct PendingAssemblyCrossing {
 uint32_t owner;
 vec3_t previous;
 bool rigid,clipped;
 std::vector<TeleportCrossing> crossings;
 bool mover=false;
 vec3_t previousAngles{};
};
std::vector<PendingAssemblyCrossing> pendingAssemblyCrossings;
int assemblyTeleportFrame=-1,assemblyTeleportWork=0;
const uint32_t assemblyWorldKey=jar::constraint::NativeId(ENTITYNUM_WORLD);

int& assemblyTeleportBudget(){
 if(assemblyTeleportFrame!=level.time){
  assemblyTeleportFrame=level.time;
  assemblyTeleportWork=4096;
 }
 return assemblyTeleportWork;
}

gentity_t* assemblyVehicle(gentity_t& member){
 if(auto* hull=G_TransportVehicleHull(&member))return hull;
 auto* hull=member.owner;
 if(G_VehicleEntityPointer(hull)&&hull->m_pVehicle&&hull->m_pVehicle->m_pDroidUnit==&member&&
    member.s.m_iVehicleNum==hull->s.number)return hull;
 return nullptr;
}

bool assemblyVehicleMemberAllowed(const gentity_t& member){
 const auto& ps=member.client->ps;
 return member.s.number!=held&&!nativeControls.count(member.s.number)&&!(member.svFlags&SVF_NO_TELEPORT)&&
  member.next_roff_time<=level.time&&ps.saberLockTime<=level.time&&
  !(ps.eFlags&(EF_FORCE_GRIPPED|EF_FORCE_DRAINED|EF_LOCKED_TO_WEAPON|
   EF_HELD_BY_RANCOR|EF_HELD_BY_WAMPA|EF_HELD_BY_SAND_CREATURE));
}

bool assemblyTeleportMembers(uint32_t root,const jar::teleport::Settings& settings,
 AssemblyTeleportPlan& plan,int& workBudget){
 std::set<uint32_t> known{root};
 std::vector<uint32_t> pending{root};
 std::set<uint32_t> links;
 std::set<gentity_t*> vehicles;
 std::set<gentity_t*> binaryTeams,binaryParts;
 std::set<gentity_t*> sections;
 std::map<uint32_t,std::pair<gentity_t*,gentity_t*>> sectionParts;
 const auto enqueue=[&](uint32_t key){if(known.insert(key).second)pending.push_back(key);};
 const auto includeSection=[&](gentity_t& section){
  if(!sections.insert(&section).second)return true;
  const auto key=attachmentKey(section);
  std::vector<gentity_t*> children;
  if(!OwnedId(state,key)||section.next_roff_time||Q3_TaskIDPending(&section,TID_MOVE_NAV)||
     Q3_TaskIDPending(&section,TID_ANGLE_FACE)||!G_BSPSectionTeleportMembers(section,children))return false;
  enqueue(key);
  for(auto* child:children){
   const auto childKey=attachmentKey(*child);
   if(child==&section||!sectionParts.emplace(childKey,std::make_pair(child,&section)).second)return false;
   enqueue(childKey);
  }
  return true;
 };
 for(size_t i=0;i<pending.size();++i){
  if(workBudget<=0)return false;
  --workBudget;
  const auto id=pending[i];
  auto sectionPart=sectionParts.find(id);
  auto* entity=sectionPart==sectionParts.end()?attachmentEntity(id):sectionPart->second.first;
  if(!entity||entity==&g_entities[ENTITYNUM_WORLD]||entity->s.number==held||
     nativeControls.count(entity->s.number)||!JAR_RopePathPoint(entity->currentOrigin))return false;
  auto& member=*entity;
  if(sectionPart==sectionParts.end()){
   auto* parent=G_SubBSPOwner(&member);
   if(parent&&parent!=&member&&jar::bspSection::Portable(*parent)){
    if(!settings.movers||!includeSection(*parent))return false;
    sectionPart=sectionParts.find(id);
    if(sectionPart==sectionParts.end())return false;
   }
  }
  const bool sectionChild=sectionPart!=sectionParts.end();
  const auto* record=OwnedId(state,id);
  const auto* model=record?prop(record->catalog):nullptr;
  const bool frozen=model&&record->frozen;
  const auto mover=sectionChild?TeleportMoverType::None:G_TeleportMoverKind(member);
  auto* hull=member.client?assemblyVehicle(member):nullptr;
  if(sectionChild){
   if(Q3_TaskIDPending(&member,TID_MOVE_NAV)||Q3_TaskIDPending(&member,TID_ANGLE_FACE))return false;
  }else if(member.client){
   if(hull){
    if(!settings.vehicles)return false;
    if(vehicles.insert(hull).second){
     if(!G_TransportVehicleReady(*hull,assemblyVehicleMemberAllowed))return false;
     for(const auto* seat:{hull,hull->m_pVehicle->m_pPilot,hull->m_pVehicle->m_pDroidUnit})
      if(seat)enqueue(attachmentKey(*seat));
    }
   }else if(!G_TeleportActorMatches(&member,settings)||!attachmentMovable(member))return false;
   for(float angle:member.client->ps.viewangles)if(!std::isfinite(angle))return false;
  }else if(mover!=TeleportMoverType::None){
   if(!settings.movers||(!record&&!G_TeleportBrushMover(mover))||
      G_TeleportObjectKind(member)!=jar::teleport::Mover||!G_TeleportMoverReady(member,mover))return false;
   if(mover==TeleportMoverType::Section&&!includeSection(member))return false;
   if(mover==TeleportMoverType::Binary){
    binaryParts.insert(&member);
    if(binaryTeams.insert(member.teammaster?member.teammaster:&member).second){
     std::vector<gentity_t*> parts;
     if(!G_TeleportBinaryTeam(member,parts))return false;
     for(const auto* part:parts)enqueue(attachmentKey(*part));
    }
   }
  }else if(!model||(frozen&&(!settings.frozenProps||id==root))||
     G_TeleportObjectKind(member)!=jar::teleport::Prop)return false;
  G_TeleportRefreshArrival(member,member.currentOrigin);
  if(G_TeleportArrivalPending(member))return false;
  const bool rigid=model&&physicsWorld.Active()&&physicsEligible(*model,member.s.frame);
  auto pose=physicsPose(member);
  if(member.client){pose.linear=pv(member.client->ps.velocity);pose.angular={};}
  if(rigid&&!physicsWorld.Read(member.s.number,pose))return false;
  if(frozen)pose.linear=pose.angular={};
  for(float component:{pose.angles.x,pose.angles.y,pose.angles.z,
     pose.linear.x,pose.linear.y,pose.linear.z,pose.angular.x,pose.angular.y,pose.angular.z})
   if(!std::isfinite(component))return false;
  AssemblyTeleportMember entry{id,&member,hull,pose,pose.angles,{}, {},member.linked!=qfalse,rigid,frozen};
  entry.mover=mover;
  if(sectionChild)entry.sectionRoot=sectionPart->second.second;
  G_TeleportHull(member,entry.mins,entry.maxs);
  for(int axis=0;axis<3;++axis)if(!std::isfinite(entry.mins[axis])||!std::isfinite(entry.maxs[axis])||
     entry.mins[axis]>entry.maxs[axis])return false;
  VectorCopy(member.currentOrigin,entry.before);plan.members.push_back(entry);
  for(unsigned link=0;link<state.jointCount;++link){
   auto& joint=state.joints[link];
   if(joint.kind==jar::constraint::Thruster||(joint.first!=id&&joint.second!=id))continue;
   const auto peer=joint.first==id?joint.second:joint.first;
   if(!jar::constraint::Valid(joint))return false;
   // World anchors terminate traversal. Sharing the world does not connect assemblies.
   if(peer==assemblyWorldKey){if(!settings.worldAnchors)return false;}
   else enqueue(peer);
   if(links.insert(joint.id).second)plan.links.push_back(&joint);
  }
 }
 if(!binaryParts.empty()){
  const int scanCost=(globals.num_entities+15)/16;
  if(workBudget<scanCost)return false;
  workBudget-=scanCost;
  for(int i=0;i<globals.num_entities;++i){
   auto& trigger=g_entities[i];
   if(!trigger.inuse||(trigger.e_TouchFunc!=touchF_Touch_DoorTrigger&&
      trigger.e_TouchFunc!=touchF_Touch_PlatCenterTrigger)||!binaryParts.count(trigger.owner))continue;
   if(trigger.bmodel||trigger.client||trigger.freeAfterEvent||nativeControls.count(trigger.s.number)||
      held==unsigned(trigger.s.number)||Constrained(state,attachmentKey(trigger)))return false;
   plan.triggers.push_back({&trigger,trigger.owner,{},{},{}});
  }
 }
 // A teleported support cannot leave riders behind.
 std::set<int> sectionSupports;
 for(const auto& member:plan.members)
  if(member.sectionRoot||member.mover==TeleportMoverType::Section)sectionSupports.insert(member.entity->s.number);
 for(const auto& member:plan.members)if(member.mover!=TeleportMoverType::None){
  const auto& deck=*member.entity;
  vec3_t low,high;VectorAdd(deck.currentOrigin,member.mins,low);VectorAdd(deck.currentOrigin,member.maxs,high);
  for(int axis=0;axis<3;++axis){low[axis]-=2;high[axis]+=2;}
  high[2]+=JAR_LIFT_MAX_SUPPORT;
  gentity_t* nearby[256];
  const int count=gi.EntitiesInBox(low,high,nearby,256);
  if(count>=256||count>workBudget)return false;
  workBudget-=count;
  for(int i=0;i<count;++i){
   const auto& rider=*nearby[i];
   if(!rider.inuse||&rider==&deck||known.count(attachmentKey(rider)))continue;
   const int ground=rider.client?rider.client->ps.groundEntityNum:rider.s.groundEntityNum;
   const bool landed=rider.m_pVehicle&&rider.m_pVehicle->m_LandTrace.fraction<1&&
    rider.m_pVehicle->m_LandTrace.entityNum==deck.s.number;
   if(ground==deck.s.number||landed)return false;
   if(member.mover==TeleportMoverType::Section){
    const auto* record=owned(rider.s.number);
    if(sectionSupports.count(ground)||(rider.m_pVehicle&&rider.m_pVehicle->m_LandTrace.fraction<1&&
       sectionSupports.count(rider.m_pVehicle->m_LandTrace.entityNum))||
       ((rider.client||(record&&prop(record->catalog)))&&(!jar::volume::Valid(deck.mins,deck.maxs)||
        jar::volume::Contact(deck.mins,deck.maxs,deck.currentOrigin,deck.currentAngles,
         rider.currentOrigin,rider.mins,rider.maxs))))return false;
   }
  }
 }
 return !plan.links.empty();
}

bool assemblyTeleportDestinations(AssemblyTeleportPlan& plan,const gentity_t& target,bool keepVelocity){
 vec3_t offset;
 VectorSubtract(target.currentOrigin,plan.members.front().before,offset);
 for(auto& member:plan.members){
  if(member.entity==&target)return false;
  VectorAdd(member.before,offset,member.after);
 }
 for(auto& member:plan.members){
  if(member.vehicle!=member.entity)continue;
  plan.vehicles.emplace_back();
  auto& vehicle=plan.vehicles.back();
  if(!G_PlanVehicleTeleportAt(*member.entity,member.after,member.entity->m_pVehicle->m_vOrientation,
      true,keepVelocity,cg.time?cg.time:level.time,vehicle))return false;
  for(int i=0;i<vehicle.count;++i){
   const auto& seat=vehicle.members[i];
   auto entry=std::find_if(plan.members.begin(),plan.members.end(),[&](const auto& item){return item.entity==seat.entity;});
   if(entry==plan.members.end())return false;
   vec3_t correction;VectorSubtract(seat.position,entry->after,correction);
   entry->reseated=i>0&&VectorLengthSquared(correction)>.015625f;
   VectorCopy(seat.position,entry->after);
   entry->pose.angles=pv(seat.angles);
  }
 }
 float lowest=std::numeric_limits<float>::infinity();
 for(const auto& member:plan.members)
  if(!member.sectionRoot)lowest=std::min(lowest,member.after[2]+member.mins[2]);
 const float surface=(target.contents&(CONTENTS_SOLID|CONTENTS_BODY))?std::max(0.f,target.maxs[2]):0.f;
 const float floor=target.currentOrigin[2]+surface+1;
 const float lift=floor-lowest;
 for(auto& member:plan.members){
  member.after[2]+=lift;
  if(!member.entity->client&&!G_SandboxChargesPlan(*member.entity,member.after,plan.charges))return false;
 }
 const float height=G_SandboxChargesFloorOffset(plan.charges,floor);
 G_SandboxChargesRaise(plan.charges,height);
 for(auto& vehicle:plan.vehicles)for(int i=0;i<vehicle.count;++i)vehicle.members[i].position[2]+=lift+height;
 for(auto& member:plan.members){
  member.after[2]+=height;
  if(!JAR_RopePathPoint(member.after))return false;
  if(member.sectionRoot){
   vec3_t offset,base;
   VectorSubtract(member.after,member.before,offset);
   VectorAdd(member.entity->s.origin,offset,base);
   if(!JAR_RopePathPoint(base))return false;
   VectorAdd(member.entity->s.pos.trBase,offset,base);
   if(!JAR_RopePathPoint(base))return false;
  }
  member.pose.position=pv(member.after);
  if(!keepVelocity)member.pose.linear=member.pose.angular={};
  if(member.rigid)plan.rigid.push_back({uint32_t(member.entity->s.number),member.pose,member.frozen});
  if(member.mover!=TeleportMoverType::None&&
     !G_PlanTeleportMover(*member.entity,member.mover,member.after,keepVelocity,member.moverMotion))return false;
 }
 for(auto& trigger:plan.triggers){
  if(trigger.entity==&target)return false;
  const auto mover=std::find_if(plan.members.begin(),plan.members.end(),[&](const auto& item){return item.entity==trigger.mover;});
  if(mover==plan.members.end())return false;
  vec3_t offset;
  VectorSubtract(mover->after,mover->before,offset);
  VectorAdd(trigger.entity->currentOrigin,offset,trigger.origin);
  VectorAdd(trigger.entity->s.origin,offset,trigger.base);
  VectorAdd(trigger.entity->s.pos.trBase,offset,trigger.trajectoryBase);
  vec3_t mins,maxs;
  VectorAdd(trigger.origin,trigger.entity->mins,mins);VectorAdd(trigger.origin,trigger.entity->maxs,maxs);
  if(!JAR_RopePathPoint(mins)||!JAR_RopePathPoint(maxs))return false;
  for(int axis=0;axis<3;++axis)if(mins[axis]>maxs[axis])return false;
 }
 return true;
}

struct AssemblyTeleportClearance {
 const std::vector<AssemblyTeleportMember>& members;
 bool staged=false;
 explicit AssemblyTeleportClearance(const std::vector<AssemblyTeleportMember>& group):members(group){
  for(const auto& member:members)if(member.linked)gi.unlinkentity(member.entity);
 }
 void Stage(){
  staged=true;
  for(const auto& member:members){
   VectorCopy(member.after,member.entity->currentOrigin);
   if(member.linked)gi.linkentity(member.entity);
  }
 }
 ~AssemblyTeleportClearance(){
  for(const auto& member:members){
   if(staged)VectorCopy(member.before,member.entity->currentOrigin);
   if(member.linked)gi.linkentity(member.entity);
  }
 }
};

const AssemblyTeleportMember* assemblyTeleportMember(const AssemblyTeleportPlan& plan,uint32_t id){
 const auto member=std::find_if(plan.members.begin(),plan.members.end(),[id](const auto& item){return item.id==id;});
 return member==plan.members.end()?nullptr:&*member;
}

bool assemblyWorldAnchorSurface(const vec3_t point,const vec3_t normal,int& budget){
 if(budget<=0)return false;
 --budget;
 // Leave the pivot at its exact translated position. Allow the trace's surface epsilon.
 vec3_t hitNormal;
 return JAR_WorldAnchorSurface(point,normal,G_JAR_WorkshopAnchorTrace,hitNormal)!=qfalse;
}

bool assemblyTeleportWorldAnchors(AssemblyTeleportPlan& plan,int& budget){
 for(auto* joint:plan.links){
  const bool first=joint->first==assemblyWorldKey;
  if(!first&&joint->second!=assemblyWorldKey)continue;
  const auto* member=assemblyTeleportMember(plan,first?joint->second:joint->first);
  if(!member)return false;
  auto frame=first?joint->a:joint->b;
  jar::physics::Vec origin,axis;
  if(!jar::physics::FrameTransform({},frame,origin,axis))return false;
  vec3_t point,normal,offset;
  nativeVector(origin,point);nativeVector(axis,normal);
  VectorSubtract(member->after,member->before,offset);
  VectorAdd(frame.position,offset,frame.position);
  if(!jar::constraint::Valid(frame))return false;
  if(!assemblyWorldAnchorSurface(point,normal,budget)){
   if(joint->kind==jar::constraint::Rope)return false;
   // A rigid joint axis can face into the surface. Retain its stored orientation.
   VectorNegate(normal,normal);
   if(!assemblyWorldAnchorSurface(point,normal,budget))return false;
  }
  if(!assemblyWorldAnchorSurface(frame.position,normal,budget))return false;
  plan.anchors.push_back({joint,frame});
 }
 return true;
}

bool assemblyRopeEndpoint(const AssemblyTeleportMember* member,const jar::constraint::Frame& frame,
 bool before,vec3_t point,float& caps){
 auto pose=member?member->pose:jar::physics::Pose{};
 if(member&&before){pose.position=pv(member->before);pose.angles=member->beforeAngles;}
 jar::physics::Vec position,direction;
 if(!jar::physics::FrameTransform(pose,frame,position,direction))return false;
 nativeVector(position,point);
 if(!member){vec3_t axis;nativeVector(direction,axis);caps+=ropeWorldEndClearance(point,axis);}
 return true;
}

bool assemblyTeleportClear(AssemblyTeleportPlan& plan,int& traceBudget){
 const auto hulls=std::count_if(plan.members.begin(),plan.members.end(),[](const auto& member){return !member.sectionRoot;});
 if(traceBudget<hulls)return false;
 AssemblyTeleportClearance scope(plan.members);
 for(const auto& member:plan.members){
  if(member.sectionRoot)continue; // Qualified solids fit inside their traced section hull.
  const auto& entity=*member.entity;
  trace_t hit{};--traceBudget;
  const int mask=entity.client?(MASK_PLAYERSOLID|entity.clipmask):
   member.mover!=TeleportMoverType::None?(MASK_PLAYERSOLID|CONTENTS_CORPSE):(MASK_SOLID|CONTENTS_BODY);
  gi.trace(&hit,member.after,member.mins,member.maxs,member.after,entity.s.number,
   mask,(EG2_Collision)0,0);
  if(hit.startsolid||hit.allsolid||hit.fraction<1)return false;
 }
 if(!G_SandboxChargesClear(plan.charges,traceBudget)||!assemblyTeleportWorldAnchors(plan,traceBudget))return false;
 const bool ropes=std::any_of(plan.links.begin(),plan.links.end(),[](const auto* joint){return joint->kind==jar::constraint::Rope;});
 const bool reseated=std::any_of(plan.members.begin(),plan.members.end(),[](const auto& member){return member.reseated;});
 if(!ropes&&!reseated)return true;
 scope.Stage();
 // An updated seat can shift relative to the rest of the assembly.
 // Ignore its own hull and seats, then check the staged neighboring bodies.
 if(reseated)for(const auto& vehicle:plan.vehicles){
  VehicleTeleportCollisionScope ownVehicle(vehicle);
  for(const auto& member:plan.members){
   if(member.vehicle!=vehicle.members[0].entity||!member.reseated)continue;
   if(traceBudget<=0)return false;
   --traceBudget;
   auto& seat=*member.entity;trace_t hit{};
   gi.trace(&hit,member.after,seat.mins,seat.maxs,member.after,seat.s.number,
     MASK_PLAYERSOLID|seat.clipmask,(EG2_Collision)0,0);
   if(hit.startsolid||hit.allsolid||hit.fraction<1)return false;
  }
 }
 for(auto* joint:plan.links){
  if(joint->kind!=jar::constraint::Rope)continue;
  const auto* a=assemblyTeleportMember(plan,joint->first);
  const auto* b=assemblyTeleportMember(plan,joint->second);
  const auto* frameA=&joint->a;
  const auto* frameB=&joint->b;
  if(!a||!b){
   const auto anchor=std::find_if(plan.anchors.begin(),plan.anchors.end(),[joint](const auto& item){return item.joint==joint;});
   if(anchor==plan.anchors.end()||(a?joint->second:joint->first)!=assemblyWorldKey)return false;
   (a?frameB:frameA)=&anchor->frame;
  }
  vec3_t from,to;
  float caps=0;
  if(!assemblyRopeEndpoint(a,*frameA,false,from,caps)||!assemblyRopeEndpoint(b,*frameB,false,to,caps)||traceBudget<=0)return false;
  jarRopePath_t path{};
  auto& world=g_entities[ENTITYNUM_WORLD];
  RopeTraceScope endpoints(a?*a->entity:world,b?*b->entity:world);
  const auto route=JAR_RopePathSolve(&path,from,to,joint->length,ropeTrace,nullptr,std::min(traceBudget,JAR_ROPE_PATH_TRACE_LIMIT));
  traceBudget-=route.traces;
  if(route.status!=JAR_ROPE_PATH_OK)return false;
  // Retain the rest length and disallow added stretch at the destination.
  vec3_t oldFrom,oldTo;
  float oldCaps=0;
  if(!assemblyRopeEndpoint(a,joint->a,true,oldFrom,oldCaps)||!assemblyRopeEndpoint(b,joint->b,true,oldTo,oldCaps))return false;
  jarRopePathResult_t old{};JAR_RopePathMeasure(&joint->path,oldFrom,oldTo,&old);
  if(route.length+caps>std::max(joint->length,old.length+oldCaps)+.25f)return false;
  plan.ropes.emplace_back(joint,path);
 }
 return true;
}

bool teleportAssembly(uint32_t root,gentity_t& source,int& traceBudget){
 jar::teleport::Settings settings;
 if(!G_TeleportRead(source,settings)||settings.kind!=jar::teleport::Volume||
    !settings.enabled||!settings.props||!settings.assemblies)return false;
 const auto* target=G_TeleportDestination(source);
 if(!target||(target->svFlags&SVF_INACTIVE))return false;
 AssemblyTeleportPlan plan;
 if(!assemblyTeleportMembers(root,settings,plan,traceBudget)||!assemblyTeleportDestinations(plan,*target,settings.keepVelocity)||
    !assemblyTeleportClear(plan,traceBudget))return false;
 const auto primary=std::min_element(plan.members.begin(),plan.members.end(),[](const auto& a,const auto& b){return a.id<b.id;});
 if(!G_ContactClockReady(source,*primary->entity,settings.interval)||!physicsWorld.CorrectPoses(plan.rigid))return false;

 for(const auto& vehicle:plan.vehicles)G_CommitVehicleTeleport(vehicle,settings.keepVelocity);
 for(const auto& member:plan.members){
  nativeJointHistory.erase(member.id);
  if(member.vehicle)continue;
  auto& entity=*member.entity;
  if(member.sectionRoot){
   G_BSPSectionTranslateChild(entity,member.after);
   continue;
  }else if(entity.client){
   // A translation preserves both the actor's view and local attachment frame.
   const int ground=entity.client->ps.groundEntityNum;
   vec3_t origin,view;VectorCopy(member.after,origin);--origin[2];
   VectorCopy(entity.client->ps.viewangles,view);
   G_JAR_TeleportActor(&entity,origin,view,settings.keepVelocity);
   if(std::any_of(plan.members.begin(),plan.members.end(),[ground](const auto& support){
       return (support.mover!=TeleportMoverType::None||support.sectionRoot)&&support.entity->s.number==ground;
      }))entity.client->ps.groundEntityNum=entity.s.groundEntityNum=ground;
   nativeVector(member.pose.angles,entity.currentAngles);
   if(!member.linked)gi.unlinkentity(&entity);
  }else if(member.mover!=TeleportMoverType::None){
   G_CommitTeleportMover(entity,member.after,member.moverMotion);
   if(member.linked)gi.linkentity(&entity);
  }else{
   vec3_t linear,angular;nativeVector(member.pose.linear,linear);nativeVector(member.pose.angular,angular);
   G_CommitTeleportPropMotion(entity,member.after,linear,angular);
   entity.contents=physicsFrozenContents(entity);
   if(member.linked)gi.linkentity(&entity);
  }
  G_TeleportMarkArrival(entity);
 }
 for(auto* joint:plan.links)physicsWorld.ForgetJoint(joint->id);
 for(const auto& anchor:plan.anchors)
  (anchor.joint->first==assemblyWorldKey?anchor.joint->a:anchor.joint->b)=anchor.frame;
 for(const auto& rope:plan.ropes){rope.first->path=rope.second;ropeStates.erase(rope.first->id);}
 for(const auto& trigger:plan.triggers){
  auto& entity=*trigger.entity;
  VectorCopy(trigger.origin,entity.currentOrigin);VectorCopy(trigger.base,entity.s.origin);
  VectorCopy(trigger.trajectoryBase,entity.s.pos.trBase);
  if(entity.linked)gi.linkentity(&entity);
 }
 G_SandboxChargesCommit(plan.charges);
 return true;
}
