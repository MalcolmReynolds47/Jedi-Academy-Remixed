// Native prop movement and bounded sliding at mounted-charge contacts.
void nativePropFall(gentity_t& e,const PropPhysics& settings,float dt,vec3_t destination){
 if(!settings.noGravity)e.s.pos.trDelta[2]-=(g_gravity?g_gravity->value:800)*dt;
 VectorMA(e.currentOrigin,dt,e.s.pos.trDelta,destination);
}

void advanceNativeProp(gentity_t& e,const vec3_t desired,float remaining){
 vec3_t target;VectorCopy(desired,target);
 std::vector<SandboxChargeMove> charges;
 for(int step=0;step<2;++step){
  vec3_t previous;VectorCopy(e.currentOrigin,previous);
  trace_t hit{};hit.fraction=1;
  bool chargeContact=false,blocked=false;
  if(!VectorCompare(target,previous)){
   charges.clear();
   blocked=!G_SandboxPropTrace(e,target,hit,charges,chargeContact,propMovingHull(e));
   if(!blocked){
    vec3_t velocity;VectorCopy(e.s.pos.trDelta,velocity);
    G_SetOrigin(&e,hit.endpos);VectorCopy(velocity,e.s.pos.trDelta);e.s.pos.trType=TR_INTERPOLATE;
    gi.linkentity(&e);placeEditedCharges(charges);
   }
  }
  const bool clipped=blocked||hit.fraction<1;
  if(!blocked&&teleportPropContacts(e,previous,false,clipped))return;
  if(clipped){
   if(chargeContact&&!blocked){
    const float inward=-DotProduct(e.s.pos.trDelta,hit.plane.normal);
    impactSound(e.s.number,inward);
    if(inward>0)VectorMA(e.s.pos.trDelta,inward,hit.plane.normal,e.s.pos.trDelta);
   }else {
    impactSound(e.s.number,VectorLength(e.s.pos.trDelta));
    VectorClear(e.s.pos.trDelta);
   }
  }
  pushPropContacts(e,previous,false);
  if(blocked||!clipped||!chargeContact||step==1)break;
  remaining*=1-hit.fraction;
  if(remaining<=0||VectorLengthSquared(e.s.pos.trDelta)<.0001f)break;
  VectorMA(e.currentOrigin,remaining,e.s.pos.trDelta,target);
 }
}
