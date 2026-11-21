// Native held poses turn around their local grab point. GPL-2.0-only.
void physgunNativeTurn(gentity_t& e,float dt){
 const auto oldAngles=pv(e.currentAngles);
 const auto wanted=physgunGrip.Facing(physgunViewYaw);
 const auto spin=jar::physics::AngularVelocity(oldAngles,wanted,1);
 vec3_t axis;nativeVector(spin,axis);
 const float angle=VectorLength(axis);
 if(angle<.0001f)return;
 const auto pivot=physgunGrip.Point(pv(e.currentOrigin),oldAngles);
 vec3_t worldPivot;nativeVector(pivot,worldPivot);
 // Limit the angular speed and the origin speed of a very long prop.
 const float rate=std::min(8.f,1024.f/std::max(1.f,Distance(e.currentOrigin,worldPivot)));
 const auto next=angle<=rate*dt?wanted:jar::physics::AdvanceAngles(oldAngles,spin,rate*dt/angle);
 vec3_t angles,origin;
 nativeVector(next,angles);
 nativeVector(physgunGrip.Origin(pivot,next),origin);
 for(float value:origin)if(!std::isfinite(value)||std::fabs(value)>=65535)return;
 if(e.m_pVehicle){
  nativeMoveVehicle(e,origin,angles,level.time,worldPivot);
  return;
 }
 RackStockPlan stock;
 if(!rackStockPose(e,origin,angles,true,stock))return;
 std::vector<SandboxChargeMove> charges;
 if(const auto* model=editedProp(e)){
  if(!propTransformClear(e,*model,e.s.frame,origin,angles,charges,true,worldPivot,physgunGrip.localPoint))return;
  if(!propHull(*model,angles,e.mins,e.maxs,e.s.frame))return;
  G_SetOrigin(&e,origin);
  G_SetAngles(&e,angles);
 }else if(G_JAR_PushableMover(e)){
  vec3_t pathStart,pathEnd;
  VectorCopy(e.pos1,pathStart);
  VectorCopy(e.pos2,pathEnd);
  if(!G_JAR_MoverRotateAt(e,angles,origin,worldPivot))return;
  // Native release rebases the paused path once using the full held motion.
  VectorCopy(pathStart,e.pos1);
  VectorCopy(pathEnd,e.pos2);
 }else{
  if(!nativeObjectTurnClear(e,origin,angles,worldPivot,physgunGrip,!jar::volumeEntity::Portable(e)))return;
  G_SetOrigin(&e,origin);
  G_SetAngles(&e,angles);
 }
 if(e.client)SetClientViewAngle(&e,angles);
 setupMoved(e);
 gi.linkentity(&e);
 placeEditedCharges(charges);
 rackStockLoose(e);
 commitRackStock(stock);
}
