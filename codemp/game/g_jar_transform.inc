// Exact context placement. Validate the complete pose before publishing it.
// Native actor movement resumes normally after Release; no damage or new AI.
static qboolean ContextTransformAllowed(int n){
 return !objects[n].heldBy&&!objects[n].copyPending&&!JAR_MP_NativeEmplacedOccupied(&g_entities[n])&&
  (SectionRoot(n)||PhysicsProp(n)||(g_entities[n].client&&objects[n].frozen&&JAR_MP_ActorControlled(&g_entities[n])));
}
static qboolean ContextNear(const vec3_t a,const vec3_t b){for(int i=0;i<3;++i)if(fabsf(a[i]-b[i])>.001f)return qfalse;return qtrue;}
static qboolean ContextTransformCurrent(int n,const jarPropertyUndo_t*u){
 return ContextTransformAllowed(n)&&objects[n].poseRevision==u->poseRevision&&objects[n].frozen==u->frozen&&
  ContextNear(g_entities[n].r.currentOrigin,u->newOrigin)&&ContextNear(g_entities[n].r.currentAngles,u->newAngles);
}
static qboolean ContextTransformParse(const char*s,vec3_t origin,vec3_t angles){
 for(int i=0;i<6;++i){
  const char *end=i<5?strchr(s,'_'):s+strlen(s);
  char number[32];float value;
  if(!end||end-s>=(ptrdiff_t)sizeof(number))return qfalse;
  memcpy(number,s,end-s);number[end-s]='\0';
  if(!JAR_ParseFloat(number,&value))return qfalse;
  if(i<3){
   if(value<=MIN_WORLD_COORD||value>=MAX_WORLD_COORD)return qfalse;
   origin[i]=value;
  }else{value=fmodf(value,360);angles[i-3]=value<0?value+360:value;}
  s=end+(i<5);
 }return qtrue;
}
static qboolean ContextTransformApply(int n,const vec3_t origin,const vec3_t angles){
 if(!ContextTransformAllowed(n))return qfalse;
 gentity_t*e=&g_entities[n];vec3_t lo,hi;
 if(SectionRoot(n))return SectionPose(e,origin,angles,qfalse);
 if(PhysicsProp(n)){
  if(e->s.bolt2==JAR_MP_WALL_OFF_TAG)WallMarkerHull(lo,hi);
  else if(!DoorCameraHull(n,angles,lo,hi)&&!PhysicsPoseHull(objects[n].catalog,e->s.frame,angles,lo,hi))return qfalse;
 }
 else if(!VehicleBounds(e,angles,lo,hi))return qfalse;
 // Occupants are checked as part of the placement. Temporarily unlink only
 // this vehicle and its seats to avoid treating their old hulls as obstacles.
 gentity_t*seats[VEH_MAX_PASSENGERS+2];qboolean linked[VEH_MAX_PASSENGERS+2];vec3_t destinations[VEH_MAX_PASSENGERS+2];int count=0;
 if(e->m_pVehicle){Vehicle_t*v=e->m_pVehicle;
  if(v->m_pPilot)seats[count++]=(gentity_t*)v->m_pPilot;
  for(int i=0;i<v->m_iNumPassengers;++i)seats[count++]=(gentity_t*)v->m_ppPassengers[i];
  if(v->m_pDroidUnit)seats[count++]=(gentity_t*)v->m_pDroidUnit;
 }
 for(int i=0;i<count;++i){VectorCopy(origin,destinations[i]);Vehicle_t*v=e->m_pVehicle;
  // Match native AttachRidersGeneric and VehicleAttachRiders exactly: driver
  // and droid bolt matrices use yaw, passenger cameras use vehicle origin.
  int bolt=-1;const qboolean pilot=(bgEntity_t*)seats[i]==v->m_pPilot;
  if(pilot)bolt=trap->G2API_AddBolt(e->ghoul2,0,"*driver");
  else if((bgEntity_t*)seats[i]==v->m_pDroidUnit){bolt=v->m_iDroidUnitTag;if(bolt<0)VectorCopy(seats[i]->r.currentOrigin,destinations[i]);}
  if(pilot||bolt>=0){mdxaBone_t matrix;vec3_t yaw={0,angles[YAW],0};
   if(bolt<0||!trap->G2API_GetBoltMatrix(e->ghoul2,0,bolt,&matrix,yaw,origin,level.time,NULL,e->modelScale))return qfalse;
   BG_GiveMeVectorFromMatrix(&matrix,ORIGIN,destinations[i]);
  }
  if(!CopyVector(destinations[i],65535))return qfalse;
 }
 for(int i=0;i<count;++i){linked[i]=seats[i]->r.linked;trap->UnlinkEntity((sharedEntity_t*)seats[i]);}
 const qboolean wasLinked=e->r.linked;trap->UnlinkEntity((sharedEntity_t*)e);
 trace_t tr;trap->Trace(&tr,origin,lo,hi,origin,n,PhysicsMoveMask(e),qfalse,0,0);qboolean clear=!tr.startsolid&&!tr.allsolid;
 for(int i=0;i<count&&clear;++i){trap->Trace(&tr,destinations[i],seats[i]->r.mins,seats[i]->r.maxs,destinations[i],seats[i]->s.number,MASK_PLAYERSOLID,qfalse,0,0);clear=!tr.startsolid&&!tr.allsolid;}
 if(wasLinked)trap->LinkEntity((sharedEntity_t*)e);for(int i=0;i<count;++i)if(linked[i])trap->LinkEntity((sharedEntity_t*)seats[i]);
 if(!clear)return qfalse;
 const qboolean moved=!ContextNear(e->r.currentOrigin,origin);
 VectorCopy(lo,e->r.mins);VectorCopy(hi,e->r.maxs);VectorCopy(angles,e->r.currentAngles);VectorCopy(angles,e->s.angles);VectorCopy(angles,e->s.apos.trBase);e->s.apos.trType=TR_INTERPOLATE;
 Move(e,origin);
 if(e->client){SetClientViewAngle(e,e->r.currentAngles);if(moved)e->client->ps.eFlags^=EF_TELEPORT_BIT;ActorSnapshot(e);}
 else {if(moved)e->s.eFlags^=EF_TELEPORT_BIT;JAR_RigidForget(n);PhysicsPublish(e);}
 if(moved)for(int i=0;i<count;++i){seats[i]->client->ps.eFlags^=EF_TELEPORT_BIT;seats[i]->s.eFlags^=EF_TELEPORT_BIT;}
 ++objects[n].propertyRevision;return qtrue;
}
