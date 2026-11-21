// Native actor response and player support. GPL-2.0-only.
struct PropActorResponse {
 gentity_t* actor;
 vec3_t destination,velocity;
 float riderMass=0,impact=0;
};

bool propActorChargeClear(gentity_t& prop,const vec3_t origin,
 const std::vector<PropActorResponse>& actors,std::vector<SandboxChargeMove>& charges){
 if(!G_SandboxChargesPlanPose(prop,prop.currentOrigin,prop.currentAngles,charges,ENTITYNUM_WORLD))return false;
 if(charges.empty())return true;
 vec3_t offset;
 VectorSubtract(origin,prop.currentOrigin,offset);
 const bool moved=VectorLengthSquared(offset)>.000001f;
 SandboxChargesUnlinked own(charges);
 std::vector<gentity_t*> hidden;
 if(moved){
  if(prop.linked){gi.unlinkentity(&prop);hidden.push_back(&prop);}
  for(const auto& response:actors)if(response.actor->linked){
   gi.unlinkentity(response.actor);hidden.push_back(response.actor);
  }
 }
 const bool clear=std::all_of(charges.begin(),charges.end(),[&](SandboxChargeMove& move){
  vec3_t start;
  VectorCopy(move.origin,start);
  VectorAdd(move.origin,offset,move.origin);VectorAdd(move.beamStart,offset,move.beamStart);
  if(moved&&!G_SandboxChargeFits(move,start))return false;
  for(const auto& response:actors){
   const auto& actor=*response.actor;
   vec3_t actorTravel,relativeEnd;
   VectorSubtract(response.destination,actor.currentOrigin,actorTravel);
   VectorSubtract(move.origin,actorTravel,relativeEnd);
   jar::physics::ActorContact contact;
   if(jar::physics::SweepActor(start,relativeEnd,move.charge->mins,move.charge->maxs,
      actor.absmin,actor.absmax,contact))return false;
  }
  return true;
 });
 for(auto* entity:hidden)gi.linkentity(entity);
 return clear;
}

void physicsActors(gentity_t& propEntity,const vec3_t oldOrigin,const vec3_t oldAngles,
 const jar::physics::Pose& p,float dt,bool frozen){
 if(frozen||dt<=0)return;
 vec3_t origin,velocity,low,high;
 VectorCopy(propEntity.currentOrigin,origin);nativeVector(p.linear,velocity);
 for(int axis=0;axis<3;++axis){
  low[axis]=std::min(oldOrigin[axis],origin[axis])+propEntity.mins[axis]-4;
  high[axis]=std::max(oldOrigin[axis],origin[axis])+propEntity.maxs[axis]+4;
 }
 gentity_t* nearby[256];
 const int count=gi.EntitiesInBox(low,high,nearby,256);
 float propMass=0;
 const jar::physics::NativePropHull* propShape=nullptr;
 bool checkedShape=false;
 std::vector<PropActorResponse> responses;
 bool corrected=false;
 for(int i=0;i<count;++i){
  auto& actor=*nearby[i];
  if(!actor.client||actor.health<=0||nativeControls.count(actor.s.number))continue;
  if(!propMass)propMass=std::max(1.f,physicsWorld.Mass(propEntity.s.number));
  const float mass=std::max(40.f,std::min(2000.f,actor.mass>0?float(actor.mass):80.f));
  const bool standing=actor.client->ps.groundEntityNum==propEntity.s.number;
  vec3_t delta,destination;
  VectorSubtract(origin,oldOrigin,delta);
  jar::physics::ActorContact contact;
  if(standing)VectorAdd(actor.currentOrigin,delta,destination);
  else {
   if(!checkedShape){propShape=propTraceHull(propEntity,propEntity.currentAngles);checkedShape=true;}
   if(propShape){
    // Sweep the actor backward relative to the moving prop. This uses the
    // same hull and contact plane as ordinary player/NPC traces.
    vec3_t first,last;
    VectorSubtract(actor.currentOrigin,oldOrigin,first);
    VectorSubtract(actor.currentOrigin,origin,last);
    const auto hit=propShape->Trace(pv(first),pv(last),pv(actor.mins),pv(actor.maxs));
    if(hit.depth<=.001f)continue;
    nativeVector(hit.normal,contact.normal);contact.depth=hit.depth;
   }else if(!jar::physics::SweepActor(oldOrigin,origin,propEntity.mins,propEntity.maxs,
      actor.absmin,actor.absmax,contact))continue;
   VectorMA(actor.currentOrigin,(contact.depth+.05f)*propMass/(propMass+mass),contact.normal,destination);
  }
  PropActorResponse response{&actor,{},{}};
  VectorCopy(actor.currentOrigin,response.destination);VectorCopy(actor.client->ps.velocity,response.velocity);
  trace_t hit{};
  gi.trace(&hit,actor.currentOrigin,actor.mins,actor.maxs,destination,propEntity.s.number,MASK_SOLID,G2_NOCOLLIDE,0);
  if(!hit.startsolid&&!hit.allsolid)VectorCopy(hit.endpos,response.destination);
  vec3_t actual;
  VectorSubtract(response.destination,actor.currentOrigin,actual);
  if(standing){
   response.riderMass=mass;
   for(int axis=0;axis<3;++axis)if(std::fabs(delta[axis]-actual[axis])>.01f){
    origin[axis]-=delta[axis]-actual[axis];velocity[axis]=0;corrected=true;
   }
  }else{
   const float remainder=std::max(0.f,contact.depth+.05f-DotProduct(actual,contact.normal));
   VectorMA(origin,-remainder,contact.normal,origin);corrected=true;
   response.impact=DotProduct(velocity,contact.normal)-DotProduct(response.velocity,contact.normal);
   const float impulse=jar::physics::ContactImpulse(propMass,mass,response.impact);
   VectorMA(velocity,-impulse/propMass,contact.normal,velocity);
   VectorMA(response.velocity,impulse/mass,contact.normal,response.velocity);
   if(hit.fraction<1||hit.startsolid||hit.allsolid){
    const float inward=DotProduct(velocity,contact.normal);
    if(inward>0)VectorMA(velocity,-inward,contact.normal,velocity);
   }
  }
  responses.push_back(response);
 }
 if(responses.empty())return;

 // A rider may have been staged before another actor limited the support.
 // Reconcile each rider with the final displacement before any actor moves.
 bool clear=true;
 vec3_t travel;
 VectorSubtract(origin,oldOrigin,travel);
 for(auto& response:responses){
  if(!response.riderMass)continue;
  auto& actor=*response.actor;
  vec3_t end;
  VectorAdd(actor.currentOrigin,travel,end);
  if(DistanceSquared(end,response.destination)<.0001f)continue;
  if(VectorCompare(end,actor.currentOrigin)){VectorCopy(end,response.destination);continue;}
  trace_t hit{};
  gi.trace(&hit,actor.currentOrigin,actor.mins,actor.maxs,end,propEntity.s.number,MASK_SOLID,G2_NOCOLLIDE,0);
  if(hit.startsolid||hit.allsolid||hit.fraction<1){clear=false;break;}
  VectorCopy(end,response.destination);
 }
 std::vector<SandboxChargeMove> charges;
 if(!clear||!propActorChargeClear(propEntity,origin,responses,charges)){
  auto previous=p;
  previous.position=pv(oldOrigin);previous.angles=pv(oldAngles);previous.linear={};previous.angular={};
  if(!physicsWorld.CorrectPose(propEntity.s.number,previous))return;
  G_SetOrigin(&propEntity,oldOrigin);G_SetAngles(&propEntity,oldAngles);
  propEntity.s.pos.trType=propEntity.s.apos.trType=TR_INTERPOLATE;
  VectorClear(propEntity.s.apos.trDelta);
  jar::physics::Vec lo,hi;
  if(physicsWorld.Bounds(propEntity.s.number,lo,hi)){
   nativeVector(lo,propEntity.mins);nativeVector(hi,propEntity.maxs);
  }
  return;
 }
 if(corrected){
  if(!physicsWorld.Correct(propEntity.s.number,pv(origin),pv(velocity)))return;
  VectorCopy(origin,propEntity.currentOrigin);VectorCopy(origin,propEntity.s.pos.trBase);
  VectorCopy(velocity,propEntity.s.pos.trDelta);
 }
 for(const auto& response:responses){
  auto& actor=*response.actor;
  if(!VectorCompare(response.destination,actor.currentOrigin)){
   G_SetOrigin(&actor,response.destination);gi.linkentity(&actor);
  }
  VectorCopy(response.velocity,actor.client->ps.velocity);
  if(response.riderMass)
   physicsWorld.Impulse(propEntity.s.number,{0,0,-response.riderMass*(g_gravity?g_gravity->value:800)*dt},pv(actor.currentOrigin));
  else impactSound(propEntity.s.number,response.impact);
 }
 charges.erase(std::remove_if(charges.begin(),charges.end(),[](const SandboxChargeMove& move){
  return VectorCompare(move.origin,move.charge->currentOrigin)&&VectorCompare(move.angles,move.charge->currentAngles);
 }),charges.end());
 if(!charges.empty()){
  gi.linkentity(&propEntity);
  G_SandboxChargesPlace(charges);G_SandboxChargesRefresh(charges);
 }
}
