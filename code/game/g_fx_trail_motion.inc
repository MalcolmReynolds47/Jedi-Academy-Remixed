// Workshop Explosion Trail flights use point sweeps and the native FX cadence.
// Included in g_fx.cpp after the ambient sound declarations. GPL-2.0-only.

namespace {
constexpr int workshopTrailLifetime=30000;
constexpr int workshopTrailPulseInterval=50;

bool trailFlightPosition(const vec3_t origin){
 for(int axis=0;axis<3;++axis)if(!std::isfinite(origin[axis])||
     std::fabs(origin[axis])>2*MAX_WORLD_COORD)return false;
 return true;
}

void workshopTrailImpact(gentity_t& flight,trace_t& hit){
 if(!(hit.surfaceFlags&SURF_NOIMPACT)){
  const auto lifetime=G_EntityLifetime(&flight);
  if(flight.splashDamage&&flight.splashRadius){
   G_RadiusDamage(hit.endpos,&flight,flight.splashDamage,flight.splashRadius,&flight,MOD_EXPLOSIVE_SPLASH);
   if(!flight.inuse||G_EntityLifetime(&flight)!=lifetime)return;
  }
  if(flight.cameraGroup){
   if(VectorLengthSquared(hit.plane.normal)==0)VectorScale(flight.movedir,-1,hit.plane.normal);
   G_PlayEffect(flight.cameraGroup,hit.endpos,hit.plane.normal);
  }
  if(VALIDSTRING(flight.soundSet)){
   const int cue=CAS_GetBModelSound(flight.soundSet,BMS_END);
   if(cue>=0)if(auto* event=G_TempEntity(hit.endpos,EV_BMODEL_SOUND))event->s.eventParm=cue;
  }
 }
 G_FreeEntity(&flight);
}
}

bool G_WorkshopTrailFrame(gentity_t& flight){
 if(flight.e_ThinkFunc!=thinkF_fx_explosion_trail_think||!(VectorLengthSquared(flight.movedir)>0))return false;
 // Earlier Workshop flights save their native launch clock but no deadline.
 if(!flight.setTime)flight.setTime=flight.s.pos.trTime+workshopTrailLifetime;
 if(level.time>=flight.setTime){G_FreeEntity(&flight);return true;}

 vec3_t origin;
 EvaluateTrajectory(&flight.s.pos,level.time,origin);
 if(!trailFlightPosition(flight.currentOrigin)||!trailFlightPosition(origin)){
  G_FreeEntity(&flight);return true;
 }
 if(!VectorCompare(flight.currentOrigin,origin)){
  trace_t hit{};
  gi.trace(&hit,flight.currentOrigin,vec3_origin,vec3_origin,origin,
   flight.owner&&flight.owner->inuse?flight.owner->s.number:ENTITYNUM_NONE,flight.clipmask,G2_RETURNONHIT,10);
  if(hit.startsolid||hit.allsolid||hit.fraction<1){
   if(hit.startsolid||hit.allsolid)VectorCopy(flight.currentOrigin,hit.endpos);
   workshopTrailImpact(flight,hit);return true;
  }
 }
 if(G_JAR_WorkshopOutsideMapPoint(origin)){G_FreeEntity(&flight);return true;}

 flight.s.eFlags&=~EF_BLOCKED_MOVER;
 VectorCopy(origin,flight.currentOrigin);
 gi.linkentity(&flight);
 if(level.time>=flight.nextthink){
  const auto lifetime=G_EntityLifetime(&flight);
  G_RadiusDamage(origin,&flight,flight.damage,flight.radius,&flight,MOD_EXPLOSIVE_SPLASH);
  if(!flight.inuse||G_EntityLifetime(&flight)!=lifetime)return true;
  G_PlayEffect(flight.fxID,origin,flight.movedir);
  flight.nextthink=level.time+workshopTrailPulseInterval;
 }
 return true;
}
