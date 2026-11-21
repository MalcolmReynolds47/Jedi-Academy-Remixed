// Resolve connected prop and mover crossings after movement publishes poses.
// GPL-2.0-only.
struct AssemblyMoverSample {uint32_t owner;uint64_t life;vec3_t origin;};
std::map<int,AssemblyMoverSample> teleportMoverOrigins;

void queueAssemblyMovers(bool enabled){
 std::map<int,AssemblyMoverSample> next;
 const auto sampleMover=[&](gentity_t& mover,uint32_t id){
  const auto slot=mover.s.number;
  const auto result=next.emplace(slot,AssemblyMoverSample{id,G_EntityLifetime(&mover),{}});
  if(!result.second)return;
  auto& sample=result.first->second;
  VectorCopy(mover.currentOrigin,sample.origin);
  const auto previous=teleportMoverOrigins.find(slot);
  if(previous==teleportMoverOrigins.end()||previous->second.owner!=id||previous->second.life!=sample.life||
     VectorCompare(previous->second.origin,mover.currentOrigin))return;
  auto crossings=G_TeleportCrossings(mover,previous->second.origin,mover.currentOrigin,false,true);
  if(!crossings.empty())pendingAssemblyCrossings.push_back({id,{},false,false,std::move(crossings),true});
 };
 std::set<gentity_t*> binaryTeams;
 if(enabled)for(unsigned i=0;i<state.jointCount;++i){
  const auto& joint=state.joints[i];
  if(joint.kind==jar::constraint::Thruster)continue;
  for(const auto id:{joint.first,joint.second}){
   auto* mover=attachmentEntity(id);
   if(!mover||attachmentKey(*mover)!=id)continue;
   auto* section=G_SubBSPOwner(mover);
   if(section&&section!=mover&&jar::bspSection::Portable(*section)){
    sampleMover(*section,attachmentKey(*section));
    continue;
   }
   if(G_TeleportObjectKind(*mover)!=jar::teleport::Mover)continue;
   if(G_TeleportMoverKind(*mover)!=TeleportMoverType::Binary){
    sampleMover(*mover,id);
    continue;
   }
   auto* leader=mover->teammaster?mover->teammaster:mover;
   if(binaryTeams.count(leader))continue;
   std::vector<gentity_t*> parts;
   if(!G_TeleportBinaryTeam(*mover,parts))continue;
   binaryTeams.insert(leader);
   for(auto* part:parts)sampleMover(*part,attachmentKey(*part));
  }
 }
 teleportMoverOrigins.swap(next);
}

void finishAssemblyCrossings(float dt){
 struct Candidate {uint32_t owner;const TeleportCrossing* crossing;};
 std::vector<Candidate> candidates;
 for(const auto& motion:pendingAssemblyCrossings)for(const auto& crossing:motion.crossings)
  candidates.push_back({motion.owner,&crossing});
 std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){
  if(a.crossing->fraction!=b.crossing->fraction)return a.crossing->fraction<b.crossing->fraction;
  if(a.crossing->source!=b.crossing->source)return a.crossing->source->s.number<b.crossing->source->s.number;
  return a.owner<b.owner;
 });
 int& traceBudget=assemblyTeleportBudget();
 for(const auto& candidate:candidates){
  if(traceBudget<=0)break;
  teleportAssembly(candidate.owner,*candidate.crossing->source,traceBudget);
 }
 for(const auto& motion:pendingAssemblyCrossings){
  if(motion.mover)continue;
  const auto* record=OwnedId(state,motion.owner);
  if(!record||!matches(*record))continue;
  auto& entity=g_entities[record->entity];
  if(G_TeleportArrivalPending(entity))continue;
  if(motion.rigid){
   jar::physics::Pose pose;
   if(!physicsWorld.Read(entity.s.number,pose))continue;
   physicsActors(entity,motion.previous,motion.previousAngles,pose,dt,record->frozen!=0);
   entity.contents=physicsFrozenContents(entity);gi.linkentity(&entity);
  }else if(motion.clipped){
   impactSound(entity.s.number,VectorLength(entity.s.pos.trDelta));VectorClear(entity.s.pos.trDelta);
  }
  pushPropContacts(entity,motion.previous,motion.rigid);
 }
 pendingAssemblyCrossings.clear();
}
