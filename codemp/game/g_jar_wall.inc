// An owned wall uses stock model collision and never edits area portals.
static void WallMarkerHull(vec3_t mins,vec3_t maxs){
 VectorSet(mins,-16,-16,-16);
 VectorSet(maxs,16,16,16);
}

static void WallFields(gentity_t*wall,qboolean solid){
 wall->genericValue1=solid;
 wall->s.bolt2=solid?0:JAR_MP_WALL_OFF_TAG;
 wall->r.contents=solid?CONTENTS_SOLID:CONTENTS_ITEM;
 wall->clipmask=solid?MASK_PLAYERSOLID:0;
 if(solid)wall->r.svFlags|=SVF_PLAYER_USABLE;
 else {wall->r.svFlags&=~SVF_PLAYER_USABLE;wall->s.solid=0;}
}

static qboolean WallSet(gentity_t*wall,qboolean solid){
 const int n=wall->s.number;
 if(solid==wall->genericValue1)return qtrue;
 vec3_t mins,maxs;
 if(solid){
  if(!PhysicsPoseHull(objects[n].catalog,wall->s.frame,wall->r.currentAngles,mins,maxs))return qfalse;
  trace_t trace;
  trap->Trace(&trace,wall->r.currentOrigin,mins,maxs,wall->r.currentOrigin,n,MASK_PLAYERSOLID,qfalse,0,0);
  if(trace.startsolid||trace.allsolid)return qfalse;
 }else WallMarkerHull(mins,maxs);
 if(objects[n].heldBy)Drop(objects[n].heldBy-1,qfalse);
 PhysicsForget(n);
 VectorClear(objects[n].velocity);VectorClear(objects[n].angular);
 VectorClear(wall->s.pos.trDelta);VectorClear(wall->s.apos.trDelta);
 VectorCopy(mins,wall->r.mins);VectorCopy(maxs,wall->r.maxs);
 WallFields(wall,solid);
 PhysicsPublish(wall);trap->LinkEntity((sharedEntity_t*)wall);
 return qtrue;
}

static void WallUse(gentity_t*wall,gentity_t*other,gentity_t*activator){
 const int n=wall->s.number;
 if(!sandbox.integer||(wall->flags&FL_INACTIVE)||objects[n].copyPending)return;
 if(!WallSet(wall,!wall->genericValue1)){
  if(activator&&activator==other&&activator->s.number<MAX_CLIENTS)
   Tell(activator,"Wall is blocked. Clear the space and try again.");
  return;
 }
 ++objects[n].propertyRevision;
 SelectionChanged(n);
}
