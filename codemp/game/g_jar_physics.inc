// Authoritative MP rigid props. Native actors/AI retain their own controllers.
static qboolean rigidFailed;
static unsigned rigidGeneration[MAX_GENTITIES];
static qboolean rigidRopes[MAX_GENTITIES];
static unsigned PhysicsIdentity(int n){return (rigidGeneration[n]<<GENTITYNUM_BITS)|(unsigned)(n+1);}
static qboolean PhysicsRope(int n){return JAR_RigidActive()&&rigidRopes[n];}
static qboolean PhysicsConstrained(int n){for(int r=MAX_CLIENTS;r<level.num_entities;++r)if(g_entities[r].inuse&&objects[r].owner&&g_entities[r].s.generic1==251&&(objects[r].first==n+1||objects[r].second==n+1))return qtrue;return qfalse;}
static void PhysicsForget(int n){rigidRopes[n]=qfalse;JAR_RigidForget(n);if(!++rigidGeneration[n])++rigidGeneration[n];}
// Model/frame cache is map-local and bounded independently of asset frame counts.
typedef struct {int catalog,frame,read;vec3_t mins,maxs;} jarRigidBounds_t;
static jarRigidBounds_t rigidBounds[4096];static int rigidFrames[JAR_MP_CATALOG_COUNT];static unsigned rigidEvict;
typedef struct {int read;vec3_t mins,maxs;} jarAnimatedBounds_t;
static jarAnimatedBounds_t animatedBounds[3];
static qboolean PhysicsProp(int n){return n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&g_entities[n].inuse&&objects[n].owner&&ForceProp(n);}
static qboolean PhysicsSolid(int n){
 const int kind=ControllerKind(n);
 return PhysicsProp(n)&&!BodyDormant(n)&&!JAR_ControllerPointGizmo(kind)&&!JAR_ControllerPortal(kind)&&!JAR_MP_DoorCameraRootState(&g_entities[n].s)&&
  !JAR_MP_WelderRootState(&g_entities[n].s)&&PhysicsMoveMask(&g_entities[n])==MASK_PLAYERSOLID;
}
static int PhysicsMoveMask(const gentity_t*e){
 return JAR_MP_CrystalCrateRootState(&e->s)&&e->r.contents==CONTENTS_CORPSE?MASK_SOLID:MASK_PLAYERSOLID;
}
static void PhysicsReset(void){JointReset();JAR_RigidReset();memset(rigidRopes,0,sizeof(rigidRopes));rigidFailed=qfalse;memset(rigidBounds,0,sizeof(rigidBounds));memset(rigidFrames,0,sizeof(rigidFrames));memset(rigidGeneration,0,sizeof(rigidGeneration));memset(animatedBounds,0,sizeof(animatedBounds));rigidEvict=0;}
static qboolean PhysicsModelBounds(int catalog,int frame,vec3_t mins,vec3_t maxs){
 if(catalog<1||catalog>JAR_MP_CATALOG_COUNT||frame<0||frame>65535||!CatalogBody(&jarMPCatalog[catalog-1]))return qfalse;
 unsigned slot=((unsigned)catalog*65599u+(unsigned)frame)%4096;jarRigidBounds_t*cache=NULL;
 for(int i=0;i<4096;++i){jarRigidBounds_t*c=&rigidBounds[(slot+i)%4096];if(!c->catalog||(c->catalog==catalog&&c->frame==frame)){cache=c;break;}}
 if(!cache)cache=&rigidBounds[rigidEvict++%4096];
 if(cache->catalog!=catalog||cache->frame!=frame){memset(cache,0,sizeof(*cache));cache->catalog=catalog;cache->frame=frame;}
 if(!cache->read){
  cache->read=-1;
  if(SkeletalModel(jarMPCatalog[catalog-1].model)){
   int frames=0;
   if(trap->JAR_ModelPoseBounds&&trap->JAR_ModelPoseBounds(jarMPCatalog[catalog-1].model,frame,&frames,cache->mins,cache->maxs)){
    cache->read=1;for(int a=0;a<3;++a)if(!isfinite(cache->mins[a])||!isfinite(cache->maxs[a])||cache->mins[a]>cache->maxs[a]||fabsf(cache->mins[a])>=1024||fabsf(cache->maxs[a])>=1024)cache->read=-1;
   }
   if(frames>0&&frames<=65536)rigidFrames[catalog-1]=frames;
  }else{fileHandle_t f=0;int size=trap->FS_Open(jarMPCatalog[catalog-1].model,&f,FS_READ);md3Header_t h;md3Frame_t pose;
  if(f&&size>=(int)(sizeof(h)+sizeof(pose))&&size<=64*1024*1024&&trap->FS_Read&&trap->FS_Read(&h,sizeof(h),f)==sizeof(h)&&LittleLong(h.ident)==MD3_IDENT&&LittleLong(h.version)==MD3_VERSION){
   int offset=LittleLong(h.ofsFrames),count=LittleLong(h.numFrames),end=LittleLong(h.ofsEnd);
   if(offset>=sizeof(h)&&count>0&&count<=65536&&offset<=size-(int)sizeof(pose)&&end<=size&&end>=offset&&(end-offset)/(int)sizeof(pose)>=count){
    rigidFrames[catalog-1]=count;
    if(frame<count){byte skip[4096];int left=offset+frame*(int)sizeof(pose)-(int)sizeof(h);
     while(left>0){int step=left>sizeof(skip)?sizeof(skip):left;if(trap->FS_Read(skip,step,f)!=step)break;left-=step;}
     if(!left&&trap->FS_Read(&pose,sizeof(pose),f)==sizeof(pose)){
      cache->read=1;for(int a=0;a<3;++a){cache->mins[a]=LittleFloat(pose.bounds[0][a]);cache->maxs[a]=LittleFloat(pose.bounds[1][a]);if(!isfinite(cache->mins[a])||!isfinite(cache->maxs[a])||cache->mins[a]>cache->maxs[a]||fabsf(cache->mins[a])>=1024||fabsf(cache->maxs[a])>=1024)cache->read=-1;}
     }
    }
   }
  }
  if(f)trap->FS_Close(f);
  }
 }
 if(cache->read<0)return qfalse;VectorCopy(cache->mins,mins);VectorCopy(cache->maxs,maxs);return qtrue;
}
static qboolean PhysicsBodyBounds(int catalog,int frame,vec3_t mins,vec3_t maxs){
 if(catalog<1||catalog>JAR_MP_CATALOG_COUNT)return qfalse;
 if(jarMPCatalog[catalog-1].id==JAR_MP_PORTAL_SURFACE_ID){
  if(frame)return qfalse;
  JAR_MP_PortalHull(JAR_PORTAL_WIDTH,JAR_PORTAL_HEIGHT,vec3_origin,mins,maxs);return qtrue;
 }
 if(jarMPCatalog[catalog-1].id==JAR_MP_CRYSTAL_CRATE_ID){
  if(frame||!PhysicsModelBounds(catalog,0,mins,maxs))return qfalse;
  PhysicsCrystalCrateBounds(mins,maxs);return qtrue;
 }
 const int tag=JAR_MP_AnimatedCatalogTag(jarMPCatalog[catalog-1].id);
 if(!tag)return PhysicsModelBounds(catalog,frame,mins,maxs);
 const int last=tag==JAR_MP_BOMB_TAG?JAR_MP_BOMB_LAST_FRAME:tag==JAR_MP_BUTTON_TAG?JAR_BUTTON_LAST:JAR_MP_BEACON_LOOP_LAST;
 if(frame<0||frame>last)return qfalse;
 jarAnimatedBounds_t*cache=&animatedBounds[tag==JAR_MP_BUTTON_TAG?2:tag-JAR_MP_BOMB_TAG];
 if(!cache->read){
  cache->read=-1;
  if(!PhysicsModelBounds(catalog,0,cache->mins,cache->maxs)||rigidFrames[catalog-1]!=last+1)return qfalse;
  for(int f=1;f<=last;++f){
   vec3_t lo,hi;
   if(!PhysicsModelBounds(catalog,f,lo,hi))return qfalse;
   AddPointToBounds(lo,cache->mins,cache->maxs);
   AddPointToBounds(hi,cache->mins,cache->maxs);
  }
  cache->read=1;
 }
 if(cache->read<0)return qfalse;
 VectorCopy(cache->mins,mins);VectorCopy(cache->maxs,maxs);
 return qtrue;
}
static qboolean PhysicsLocalBounds(int n,vec3_t mins,vec3_t maxs){
 const gentity_t* e=&g_entities[n];
 if(JAR_MP_PortalSurface(&e->s)){
  JAR_MP_PortalHull(e->genericValue1,e->genericValue2,vec3_origin,mins,maxs);return qtrue;
 }
 jarObject_t*o=&objects[n];if(PhysicsBodyBounds(o->catalog,g_entities[n].s.frame,mins,maxs))return qtrue;
 const jarMPEntry_t*entry=&jarMPCatalog[o->catalog-1];VectorSet(mins,-entry->radius,-entry->radius,entry->bottom);VectorSet(maxs,entry->radius,entry->radius,entry->bottom+entry->height);return qfalse;
}
static int PhysicsFrameCount(int catalog){if(catalog<1||catalog>JAR_MP_CATALOG_COUNT)return 0;vec3_t lo,hi;if(!rigidFrames[catalog-1])PhysicsModelBounds(catalog,0,lo,hi);return rigidFrames[catalog-1];}
static int PhysicsPoseCount(int n){return PhysicsProp(n)&&!JAR_MP_AnimatedTag(&g_entities[n].s)&&!JAR_MP_DoorCameraRootState(&g_entities[n].s)&&!JAR_MP_WelderRootState(&g_entities[n].s)&&!JAR_MP_CrystalCrateRootState(&g_entities[n].s)&&!JAR_ControllerPortal(ControllerKind(n))?PhysicsFrameCount(objects[n].catalog):0;}
static void PhysicsRotateBounds(const vec3_t low,const vec3_t high,const vec3_t angle,vec3_t mins,vec3_t maxs){
 vec3_t center,half,axis[3];AnglesToAxis(angle,axis);
 for(int a=0;a<3;++a){center[a]=(high[a]+low[a])*.5f;half[a]=fmaxf(.25f,(high[a]-low[a])*.5f);}
 for(int a=0;a<3;++a){float c=0,h=0;for(int k=0;k<3;++k){c+=axis[k][a]*center[k];h+=fabsf(axis[k][a])*half[k];}mins[a]=c-h;maxs[a]=c+h;}
}
static int PhysicsVisitModelHull(const char* model,int frame,void* context,void (*emit)(void*,const float*)){
 return trap->JAR_VisitModelHull&&trap->JAR_VisitModelHull(model,frame,context,emit);
}
static void PhysicsWelderHull(vec3_t mins,vec3_t maxs){
 VectorSet(mins,-JAR_MP_WELDER_HULL,-JAR_MP_WELDER_HULL,-JAR_MP_WELDER_HULL);
 VectorSet(maxs,JAR_MP_WELDER_HULL,JAR_MP_WELDER_HULL,JAR_MP_WELDER_HULL);
}
static void PhysicsCrystalCrateBounds(vec3_t mins,vec3_t maxs){
 VectorSet(mins,-17,-11,0);
 VectorSet(maxs,17,11,38);
}
static qboolean PhysicsPoseHull(int catalog,int frame,const vec3_t angle,vec3_t mins,vec3_t maxs){
 vec3_t low,high;
 if(!PhysicsBodyBounds(catalog,frame,low,high))return qfalse;
 if(jarMPCatalog[catalog-1].id==JAR_MP_DOOR_CAMERA_ID)DoorCameraBounds(angle,0,mins,maxs);
 else if(jarMPCatalog[catalog-1].id==JAR_MP_WELDER_ID){if(frame)return qfalse;PhysicsWelderHull(mins,maxs);}
 else {
  const jarMPEntry_t* entry=&jarMPCatalog[catalog-1];
  const qboolean modelHull=entry->kind==1||
   (entry->kind==5&&(!strcmp(entry->name,"misc_model")||!strcmp(entry->name,"misc_model_static")));
  if(!modelHull||!JAR_RigidModelHull(entry->model,frame,PhysicsVisitModelHull,angle,mins,maxs))
   PhysicsRotateBounds(low,high,angle,mins,maxs);
 }
 return qtrue;
}
static void PhysicsHull(gentity_t*e,const vec3_t angle,vec3_t mins,vec3_t maxs){
 if(DoorCameraHull(e->s.number,angle,mins,maxs))return;
 if(JAR_MP_PortalSurface(&e->s)){JAR_MP_PortalHull(e->genericValue1,e->genericValue2,angle,mins,maxs);return;}
 if(e->s.bolt2==JAR_MP_WALL_OFF_TAG){WallMarkerHull(mins,maxs);return;}
 if(JAR_MP_WelderRootState(&e->s)){PhysicsWelderHull(mins,maxs);return;}
 if(JAR_MP_ModelHullState(&e->s)&&JAR_RigidModelHull(e->model,e->s.frame,PhysicsVisitModelHull,angle,mins,maxs))return;
 vec3_t low,high;PhysicsLocalBounds(e->s.number,low,high);PhysicsRotateBounds(low,high,angle,mins,maxs);
}
static void PhysicsPublish(gentity_t*e){
 e->s.generic1=250;
 if(JAR_MP_PhysgunBox(&e->s,e->classname)||
    (e->classname&&(!strcmp(e->classname,"misc_model")||!strcmp(e->classname,"misc_model_static")))){
  if(JAR_RigidModelHull(e->model,e->s.frame,PhysicsVisitModelHull,NULL,NULL,NULL))
   e->s.bolt2=JAR_MP_MODEL_HULL_TAG;
  else if(e->s.bolt2==JAR_MP_MODEL_HULL_TAG)e->s.bolt2=0;
 }
 e->s.time2=(int)PhysicsIdentity(e->s.number);
 DoorCameraSync(e->s.number);
 VectorCopy(e->r.mins,e->s.origin2);
 VectorCopy(e->r.maxs,e->s.angles2);
 EmplacedSyncPose(e);
 RackSync(e->s.number);
 CrystalCrateSync(e->s.number);
}
static qboolean PhysicsPanelTurnClear(gentity_t* e,const vec3_t origin,const vec3_t angle,const vec3_t contact){
 jarMPGrip_t grip;
 if(!JAR_GripBegin(&grip,e->r.currentOrigin,e->r.currentAngles,contact,0))return qfalse;
 vec3_t spin,axis,low,high,pad,start;
 JAR_AngularVelocity(e->r.currentAngles,angle,1,spin);
 VectorCopy(spin,axis);
 const float radians=VectorNormalize(axis);
 const int slices=JAR_RotationSlices(radians);
 PhysicsHull(e,vec3_origin,low,high);
 const float radius=RadiusFromBounds(low,high)+Distance(contact,e->r.currentOrigin);
 JAR_RotationPadding(axis,radians/slices,radius,pad);
 VectorCopy(e->r.currentOrigin,start);
 PhysicsHull(e,e->r.currentAngles,low,high);
 for(int i=1;i<=slices;++i){
  vec3_t end,rotation,move,nextLow,nextHigh,sweepLow,sweepHigh;
  if(i==slices){VectorCopy(angle,rotation);VectorCopy(origin,end);}
  else {
   JAR_AdvanceAngles(e->r.currentAngles,spin,i/(float)slices,rotation);
   JAR_GripOrigin(&grip,contact,rotation,end);
  }
  PhysicsHull(e,rotation,nextLow,nextHigh);
  VectorSubtract(end,start,move);
  JAR_RotationEnvelope(move,low,high,nextLow,nextHigh,pad,sweepLow,sweepHigh);
  trace_t tr;
  trap->Trace(&tr,start,sweepLow,sweepHigh,start,e->s.number,PhysicsMoveMask(e),qfalse,0,0);
  if(tr.startsolid||tr.allsolid||tr.fraction<1)return qfalse;
  VectorCopy(end,start);VectorCopy(nextLow,low);VectorCopy(nextHigh,high);
 }
 return qtrue;
}
static qboolean PhysicsRotateAt(gentity_t*e,const vec3_t origin,const vec3_t angle,const vec3_t contact){
 vec3_t low,high;PhysicsHull(e,angle,low,high);trace_t tr;
 const qboolean translate=!VectorCompare(origin,e->r.currentOrigin);
 const unsigned id=jarMPCatalog[objects[e->s.number].catalog-1].id;
 const qboolean panel=id==JAR_SLIDING_DOOR_ID||id==JAR_BUTTON_ID||
  (id==JAR_WALL_CATALOG_ID&&e->s.bolt2!=JAR_MP_WALL_OFF_TAG);
 if(contact&&panel){
  if(!PhysicsPanelTurnClear(e,origin,angle,contact))return qfalse;
 }else {
  if(translate){
   trap->Trace(&tr,e->r.currentOrigin,e->r.mins,e->r.maxs,origin,e->s.number,PhysicsMoveMask(e),qfalse,0,0);
   if(tr.startsolid||tr.allsolid||tr.fraction<1)return qfalse;
  }
  trap->Trace(&tr,origin,low,high,origin,e->s.number,PhysicsMoveMask(e),qfalse,0,0);
  if(tr.startsolid||tr.allsolid)return qfalse;
 }
 VectorCopy(low,e->r.mins);VectorCopy(high,e->r.maxs);VectorCopy(angle,e->r.currentAngles);VectorCopy(angle,e->s.angles);VectorCopy(angle,e->s.apos.trBase);e->s.apos.trType=TR_INTERPOLATE;
 if(translate){G_SetOrigin(e,(float*)origin);e->s.pos.trType=TR_INTERPOLATE;}
 JAR_RigidForget(e->s.number);PhysicsPublish(e);trap->LinkEntity((sharedEntity_t*)e);return qtrue;
}
static qboolean PhysicsSetAngles(gentity_t*e,const vec3_t angle){
 return PhysicsRotateAt(e,e->r.currentOrigin,angle,NULL);
}
static qboolean PhysicsSetPose(gentity_t*e,int frame){
 if(!PhysicsPoseCount(e->s.number))return qfalse;vec3_t low,high;
 if(!PhysicsPoseHull(objects[e->s.number].catalog,frame,e->r.currentAngles,low,high))return qfalse;
 trace_t tr;trap->Trace(&tr,e->r.currentOrigin,low,high,e->r.currentOrigin,e->s.number,MASK_PLAYERSOLID,qfalse,0,0);if(tr.startsolid||tr.allsolid)return qfalse;
 if(objects[e->s.number].heldBy)Drop(objects[e->s.number].heldBy-1,qfalse);
 e->s.frame=frame;VectorCopy(low,e->r.mins);VectorCopy(high,e->r.maxs);++objects[e->s.number].poseRevision;JAR_RigidForget(e->s.number);PhysicsPublish(e);trap->LinkEntity((sharedEntity_t*)e);SelectionChanged(e->s.number);return qtrue;
}
static jarMPPose_t PhysicsPose(int n){jarMPPose_t p;VectorCopy(g_entities[n].r.currentOrigin,p.position);VectorCopy(g_entities[n].r.currentAngles,p.angles);VectorCopy(objects[n].velocity,p.linear);VectorCopy(objects[n].angular,p.angular);return p;}
static void PhysicsVelocity(int n,const vec3_t velocity){jarMPPose_t p;vec3_t low,high;if(JAR_RigidRead(n,&p,low,high)){VectorCopy(velocity,p.linear);JAR_RigidCorrect(n,&p);}}
static void PhysicsStop(qboolean freeze){
 if(freeze)for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(PhysicsProp(n)){if(objects[n].heldBy)Drop(objects[n].heldBy-1,qtrue);objects[n].frozen=1;VectorClear(objects[n].velocity);VectorClear(objects[n].angular);VectorClear(g_entities[n].s.pos.trDelta);VectorClear(g_entities[n].s.apos.trDelta);}
 JAR_RigidReset();
}
static void PhysicsFailure(const char*why){PhysicsStop(qtrue);rigidFailed=qtrue;trap->Cvar_Set("jar_mpBox3D","0");Tell(NULL,why);}
static qboolean PhysicsPolygon(void*context,int count,const float*points,const float*normal){return JAR_RigidPolygon(count,points,normal)?qtrue:qfalse;}
static qboolean PhysicsPrepare(void){
 trap->Cvar_Update(&rigidEnabled);if(!sandbox.integer||!rigidEnabled.integer){if(JAR_RigidActive())PhysicsStop(qtrue);if(!rigidEnabled.integer)rigidFailed=qfalse;return qfalse;}
 if(rigidFailed)return qfalse;int count=0;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(PhysicsSolid(n))++count;if(!count)return JAR_RigidActive()?qtrue:qfalse;
 if(!JAR_RigidActive()){
  if(!trap->JAR_CollisionPolygons){rigidFailed=qtrue;return qfalse;}
  JAR_RigidReset();int polygons=trap->JAR_CollisionPolygons(0,CONTENTS_SOLID,NULL,PhysicsPolygon);
  if(polygons<=0||!JAR_RigidStart()){PhysicsFailure("Rigid map geometry unavailable; props frozen in native mode.");return qfalse;}
  Tell(NULL,va("MP Box3D active: %u map triangles, fixed 120 Hz steps.",JAR_RigidTriangles()));
 }
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(PhysicsSolid(n)){
  const gentity_t* e=&g_entities[n];
  jarObject_t*o=&objects[n];vec3_t low,high,half,center;jarMPPose_t p=PhysicsPose(n);
  if(!PhysicsLocalBounds(n,low,high)){PhysicsFailure("Rigid model bounds unavailable; props frozen in native mode.");return qfalse;}
  for(int a=0;a<3;++a){half[a]=fmaxf(.25f,(high[a]-low[a])*.5f);center[a]=(high[a]+low[a])*.5f;}
  const char* model=JAR_MP_ModelHullState(&e->s)?e->model:NULL;
  if(model)JAR_RigidModelHull(model,e->s.frame,PhysicsVisitModelHull,NULL,NULL,NULL);
  if(!JAR_RigidModelBody(n,PhysicsIdentity(n),half,center,&p,o->frozen,o->mass,o->heldBy||o->noGravity?0:1,model,e->s.frame)){PhysicsFailure("Rigid body could not be restored; props frozen in native mode.");return qfalse;}
 }
 RopePrepare();memset(rigidRopes,0,sizeof(rigidRopes));JointClassify(qtrue);JAR_RigidBeginRopes();
 for(int r=MAX_CLIENTS;r<level.num_entities;++r)if(g_entities[r].inuse&&objects[r].owner&&g_entities[r].s.generic1==251){
  jarObject_t*o=&objects[r];int a=o->first-1,b=o->second-1;
  if((a!=ENTITYNUM_WORLD&&!PhysicsSolid(a))||(b!=ENTITYNUM_WORLD&&!PhysicsSolid(b))||!ForcePermission(o->owner,a)||!ForcePermission(o->owner,b)||(!o->jointKind&&(o->length<.01f||o->length>8192)))continue;
  if(o->jointKind){if(JointNative(r))continue;jarMPJoint_t joint;JointRecord(r,&joint);if(!JAR_RigidJoint(&joint,a==ENTITYNUM_WORLD?-1:a,b==ENTITYNUM_WORLD?-1:b)){PhysicsFailure("Rigid joint could not be restored; props frozen.");return qfalse;}rigidRopes[r]=qtrue;continue;}
  if(!RopeRigid(r)||JointNative(r))continue;
  if(!JAR_RigidRope(r+1,a==ENTITYNUM_WORLD?-1:a,a==ENTITYNUM_WORLD?0:PhysicsIdentity(a),b==ENTITYNUM_WORLD?-1:b,b==ENTITYNUM_WORLD?0:PhysicsIdentity(b),o->anchorA,o->anchorB,o->length)){PhysicsFailure("Rigid rope could not be restored; props frozen in native mode.");return qfalse;}
  rigidRopes[r]=qtrue;
 }
 JAR_RigidEndRopes();return qtrue;
}
static void PhysicsActors(gentity_t*prop,const vec3_t oldOrigin,const vec3_t oldAngles,float dt){
 
 jarObject_t*o=&objects[prop->s.number];if(o->frozen||dt<=0)return;vec3_t velocity;VectorCopy(o->velocity,velocity);qboolean corrected=qfalse;
 // Preflight every rider before moving any actor. A ceiling/wall obstruction
 // rolls the prop back as one transform, including its rotation.
 for(int n=0;n<level.num_entities;++n){gentity_t*a=&g_entities[n];
  if(!a->inuse||!a->r.linked||!a->client||a->health<=0||JAR_MP_ActorControlled(a)||a->client->ps.m_iVehicleNum||!(a->r.contents&CONTENTS_BODY)||a->client->ps.groundEntityNum!=prop->s.number)continue;
  vec3_t before[3],after[3],offset,dest;AnglesToAxis(oldAngles,before);AnglesToAxis(prop->r.currentAngles,after);VectorSubtract(a->r.currentOrigin,oldOrigin,offset);VectorCopy(prop->r.currentOrigin,dest);
  for(int k=0;k<3;++k)VectorMA(dest,DotProduct(offset,before[k]),after[k],dest);
  trace_t tr;trap->Trace(&tr,a->r.currentOrigin,a->r.mins,a->r.maxs,dest,prop->s.number,MASK_SOLID,qfalse,0,0);
  if(tr.startsolid||tr.allsolid||tr.fraction<1){
   VectorCopy(oldOrigin,prop->r.currentOrigin);VectorCopy(oldAngles,prop->r.currentAngles);VectorCopy(oldAngles,prop->s.apos.trBase);VectorClear(prop->s.apos.trDelta);VectorClear(o->velocity);VectorClear(o->angular);
   jarMPPose_t p=PhysicsPose(prop->s.number);JAR_RigidCorrect(prop->s.number,&p);PhysicsHull(prop,oldAngles,prop->r.mins,prop->r.maxs);VectorCopy(oldOrigin,prop->s.pos.trBase);VectorClear(prop->s.pos.trDelta);return;
  }
 }

 for(int n=0;n<level.num_entities;++n){gentity_t*a=&g_entities[n];if(!a->inuse||!a->r.linked||!a->client||a->health<=0||JAR_MP_ActorControlled(a)||a->client->ps.m_iVehicleNum||!(a->r.contents&CONTENTS_BODY))continue;
  vec3_t dest,normal;float depth=0;VectorCopy(a->r.currentOrigin,dest);qboolean standing=a->client->ps.groundEntityNum==prop->s.number;float mass=a->m_pVehicle?fmaxf(200,objects[n].mass):80;
  if(standing){vec3_t before[3],after[3],offset;AnglesToAxis(oldAngles,before);AnglesToAxis(prop->r.currentAngles,after);VectorSubtract(a->r.currentOrigin,oldOrigin,offset);VectorCopy(prop->r.currentOrigin,dest);for(int k=0;k<3;++k)VectorMA(dest,DotProduct(offset,before[k]),after[k],dest);}
  else {
   const int modelContact=JAR_MP_ModelHullState(&prop->s)?JAR_RigidModelActor(prop->model,prop->s.frame,
    prop->r.currentAngles,oldOrigin,prop->r.currentOrigin,a->r.currentOrigin,a->r.mins,a->r.maxs,normal,&depth):-1;
   if(modelContact==0||(modelContact<0&&!JAR_RigidSweepActor(oldOrigin,prop->r.currentOrigin,prop->r.mins,prop->r.maxs,
      a->r.absmin,a->r.absmax,normal,&depth)))continue;
   VectorMA(dest,(depth+.05f)*o->mass/(o->mass+mass),normal,dest);
  }
  trace_t tr;trap->Trace(&tr,a->r.currentOrigin,a->r.mins,a->r.maxs,dest,prop->s.number,MASK_SOLID,qfalse,0,0);vec3_t actual={0,0,0};
  if(!tr.startsolid&&!tr.allsolid){VectorSubtract(tr.endpos,a->r.currentOrigin,actual);VectorCopy(tr.endpos,a->client->ps.origin);VectorCopy(tr.endpos,a->r.currentOrigin);VectorCopy(tr.endpos,a->s.pos.trBase);trap->LinkEntity((sharedEntity_t*)a);}
  if(standing){
   for(int k=0;k<3;++k){float remaining=dest[k]-a->r.currentOrigin[k];if(fabsf(remaining)>.01f){prop->r.currentOrigin[k]-=remaining;velocity[k]=0;corrected=qtrue;}}
   vec3_t impulse={0,0,-mass*g_gravity.value*dt};JAR_RigidImpulse(prop->s.number,impulse,a->r.currentOrigin);
  }else{float remainder=fmaxf(0,depth+.05f-DotProduct(actual,normal));VectorMA(prop->r.currentOrigin,-remainder,normal,prop->r.currentOrigin);corrected=qtrue;
   float approach=DotProduct(velocity,normal)-DotProduct(a->client->ps.velocity,normal),impulse=approach>0?approach/(1/o->mass+1/mass):0;
   VectorMA(velocity,-impulse/o->mass,normal,velocity);VectorMA(a->client->ps.velocity,impulse/mass,normal,a->client->ps.velocity);VectorCopy(a->client->ps.velocity,a->s.pos.trDelta);
   if(tr.startsolid||tr.allsolid||tr.fraction<1){float blocked=DotProduct(velocity,normal);if(blocked>0)VectorMA(velocity,-blocked,normal,velocity);}
  }
 }
 if(corrected){jarMPPose_t p=PhysicsPose(prop->s.number);VectorCopy(velocity,p.linear);JAR_RigidCorrect(prop->s.number,&p);VectorCopy(p.position,prop->s.pos.trBase);VectorCopy(velocity,o->velocity);VectorCopy(velocity,prop->s.pos.trDelta);}
}
enum { PHYSICS_PROXY_NEAR = 256 };

static qboolean PhysicsBoxesOverlap(const vec3_t low, const vec3_t high,
                                   const vec3_t otherLow, const vec3_t otherHigh) {
 for (int a = 0; a < 3; ++a)
  if (high[a] < otherLow[a] || low[a] > otherHigh[a]) return qfalse;
 return qtrue;
}

static qboolean PhysicsSectionBrushNear(const gentity_t *brush, const vec3_t low, const vec3_t high) {
 vec3_t mins, maxs;
 // Rotated BSP abs bounds use a sphere around the shared section origin.
 // A small, offset block needs its own transformed bounds for proxy selection.
 SectionHull(brush->r.mins, brush->r.maxs, brush->r.currentAngles, mins, maxs);
 VectorAdd(mins, brush->r.currentOrigin, mins);
 VectorAdd(maxs, brush->r.currentOrigin, maxs);
 if (!PhysicsBoxesOverlap(mins, maxs, low, high)) return qfalse;
 for (int a = 0; a < 3; ++a) {
  mins[a] -= PHYSICS_PROXY_NEAR;
  maxs[a] += PHYSICS_PROXY_NEAR;
 }
 if (trap->EntitiesInBox) {
  static int hits[MAX_GENTITIES];
  const int count = trap->EntitiesInBox(mins, maxs, hits, MAX_GENTITIES);
  for (int i = 0; i < count; ++i) if (PhysicsSolid(hits[i])) return qtrue;
  return qfalse;
 }
 for (int n = MAX_CLIENTS; n < level.num_entities; ++n) {
  const gentity_t *prop = &g_entities[n];
  if (PhysicsSolid(n) && prop->r.linked && PhysicsBoxesOverlap(mins, maxs, prop->r.absmin, prop->r.absmax))
   return qtrue;
 }
 return qfalse;
}

static qboolean PhysicsPrepareProxies(void) {
 vec3_t low = {65536, 65536, 65536}, high = {-65536, -65536, -65536};
 qboolean hasProps = qfalse;
 for (int n = MAX_CLIENTS; n < level.num_entities; ++n) if (PhysicsSolid(n)) {
  const gentity_t *prop = &g_entities[n];
  hasProps = qtrue;
  for (int a = 0; a < 3; ++a) {
   low[a] = fminf(low[a], prop->r.currentOrigin[a] + prop->r.mins[a] - PHYSICS_PROXY_NEAR);
   high[a] = fmaxf(high[a], prop->r.currentOrigin[a] + prop->r.maxs[a] + PHYSICS_PROXY_NEAR);
  }
 }

 byte wanted[MAX_GENTITIES] = {0};
 JAR_RigidBeginProxies();
 if (hasProps) for (int n = 0; n < level.num_entities; ++n) {
  gentity_t *e = &g_entities[n];
  if (!e->inuse || !e->r.linked || PhysicsProp(n) || (e->client && !JAR_MP_ActorControlled(e)) ||
      !(e->r.contents & (CONTENTS_SOLID | CONTENTS_BODY))) continue;
  if (!e->r.bmodel && (e->r.absmax[0] - e->r.absmin[0] < .5f ||
                      e->r.absmax[1] - e->r.absmin[1] < .5f || e->r.absmax[2] - e->r.absmin[2] < .5f)) continue;
  const qboolean sectionBrush = SectionCityLive(e) || SectionLiftRoot(n) != ENTITYNUM_NONE ||
      (SectionRoot(n) && objects[n].catalog > 0 && objects[n].catalog <= JAR_MP_CATALOG_COUNT &&
       SectionKind(&jarMPCatalog[objects[n].catalog - 1]) == JAR_MP_SECTION_CITY);
  if (sectionBrush ? !PhysicsSectionBrushNear(e, low, high) : !PhysicsBoxesOverlap(e->r.absmin, e->r.absmax, low, high)) continue;
  wanted[n] = 1;
  JAR_RigidKeepProxy(n, rigidGeneration[n], e->r.bmodel ? (unsigned)e->s.modelindex : 0);
 }
 // Prune old bodies and meshes before allocating replacements. This keeps a
 // complete turnover of two valid sets from exceeding the simultaneous cap.
 JAR_RigidEndProxies();
 for (int n = 0; n < level.num_entities; ++n) if (wanted[n]) {
  const gentity_t *e = &g_entities[n];
  if(e->r.bmodel){
   if(e->s.modelindex<=0 || !trap->JAR_CollisionPolygons){PhysicsFailure("Rigid brush geometry unavailable; props frozen.");return qfalse;}
   if(!JAR_RigidHasProxyMesh(e->s.modelindex)){
    JAR_RigidBeginGeometry();
    if(trap->JAR_CollisionPolygons(e->s.modelindex,CONTENTS_SOLID|CONTENTS_BODY,NULL,PhysicsPolygon)<=0 || !JAR_RigidAddProxyMesh(e->s.modelindex)){
     PhysicsFailure("Rigid brush geometry unavailable or over budget; props frozen.");return qfalse;
    }
   }
   jarMPPose_t p={0};VectorCopy(e->r.currentOrigin,p.position);VectorCopy(e->r.currentAngles,p.angles);
   if(!JAR_RigidMeshProxy(n,rigidGeneration[n],e->s.modelindex,&p)){PhysicsFailure("Rigid brush proxy budget exceeded; props frozen.");return qfalse;}
   continue;
  }
  jarMPPose_t p={0};vec3_t half,center={0,0,0};for(int a=0;a<3;++a){p.position[a]=(e->r.absmax[a]+e->r.absmin[a])*.5f;half[a]=(e->r.absmax[a]-e->r.absmin[a])*.5f;}
  if(!JAR_RigidProxy(n,rigidGeneration[n],half,center,&p)){PhysicsFailure("Rigid native collision proxy budget exceeded; props frozen.");return qfalse;}
 }
 return qtrue;
}

static void PhysicsFrame(float dt){
 if(!JAR_RigidActive()||!PhysicsPrepareProxies())return;
 JAR_RigidStep(dt,g_gravity.value);
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(PhysicsSolid(n)){gentity_t*e=&g_entities[n];jarMPPose_t p;vec3_t before,angles;VectorCopy(e->r.currentOrigin,before);VectorCopy(e->r.currentAngles,angles);
  if(!JAR_RigidRead(n,&p,e->r.mins,e->r.maxs)){
   if(JAR_RigidOutOfBounds(n)&&!CopyOccupied(e)){G_FreeEntity(e);continue;}
   PhysicsFailure("Rigid state invalid; props frozen.");return;
  }
  if(CullFallenObject(e,p.position,e->r.maxs[2]))continue;
  Move(e,p.position);VectorCopy(p.angles,e->r.currentAngles);VectorCopy(p.angles,e->s.angles);VectorCopy(p.angles,e->s.apos.trBase);e->s.apos.trType=TR_INTERPOLATE;
  if(!objects[n].frozen){VectorCopy(p.linear,objects[n].velocity);VectorCopy(p.angular,objects[n].angular);}VectorCopy(p.linear,e->s.pos.trDelta);e->s.pos.trTime=e->s.apos.trTime=level.time;
  for(int a=0;a<3;++a)e->s.apos.trDelta[a]=dt>0?AngleSubtract(p.angles[a],angles[a])/dt:0;
  PhysicsActors(e,before,angles,dt);PhysicsPublish(e);trap->LinkEntity((sharedEntity_t*)e);
 }
 unsigned n;float speed;for(int i=0;JAR_RigidImpact(i,&n,&speed);++i)if(n<ENTITYNUM_WORLD&&g_entities[n].inuse&&speed>100&&level.time>=objects[n].nextImpact){G_Sound(&g_entities[n],CHAN_BODY,G_SoundIndex("sound/movers/objects/objectHit.wav"));objects[n].nextImpact=level.time+180;}
}
static void PhysicsStatus(void){Tell(NULL,va("MP physics: Box3D %s; %u triangles; %u fixed steps.",JAR_RigidActive()?"active":"inactive",JAR_RigidTriangles(),JAR_RigidSteps()));}
