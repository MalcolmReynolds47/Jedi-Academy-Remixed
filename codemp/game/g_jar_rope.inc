// Server-authoritative scenery routes; clients receive the chosen bends.
// GPL-2.0-only. Included after force helpers and before the rigid adapter.
typedef struct {
 uint64_t scene;
 unsigned identity;int first,second,valid,history,hasSafePose;
 float safeLength;
 vec3_t a,b;jarMPPose_t poseA,poseB,safeA,safeB;jarRopePathResult_t result;
} jarRopeCache_t;
static jarRopeCache_t ropeCache[MAX_GENTITIES];
static int ropeBudgetTime=-1,ropeBudget=4096,ropeCursor,ropeNoticeAt,ropeNoticeStatus=-1;
static void RopeReset(void){memset(ropeCache,0,sizeof(ropeCache));ropeBudgetTime=-1;ropeBudget=4096;ropeCursor=ropeNoticeAt=0;ropeNoticeStatus=-1;}
static void RopeForget(int n){memset(&ropeCache[n],0,sizeof(ropeCache[n]));}
static qboolean RopeLink(int n){return n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&g_entities[n].inuse&&objects[n].owner&&g_entities[n].s.generic1==251&&!objects[n].jointKind;}
static void RopeTrace(const float*a,const float*b,float radius,jarRopePathHit_t*out,void*data){
 trace_t tr;vec3_t lo={-radius,-radius,-radius},hi={radius,radius,radius};
 trap->Trace(&tr,a,lo,hi,b,ENTITYNUM_NONE,data?JAR_ROPE_PATH_CONTENTS:MASK_SOLID,qfalse,0,0);
 out->fraction=tr.fraction;out->solid=tr.startsolid||tr.allsolid;VectorCopy(tr.endpos,out->point);VectorCopy(tr.plane.normal,out->normal);
 const int n=tr.entityNum;
 if(data&&(out->solid||tr.fraction<1)&&n>=0&&n<ENTITYNUM_WORLD){
  const int contents=g_entities[n].r.contents;
  if((contents&CONTENTS_BODY)&&!(contents&MASK_SOLID))*(qboolean*)data=qtrue;
  if(ForceProp(n)&&jarMPCatalog[objects[n].catalog-1].kind==1){out->prop=n+1;out->generation=controllerEntityEpoch[n];}
 }
}
static uint64_t RopeScene(const jarObject_t* o,const vec3_t a,const vec3_t b){
 vec3_t low,high;JAR_RopePathBounds(&o->ropePath,a,b,low,high);
 int nearby[JAR_ROPE_PATH_NEARBY];
 const int count=trap->EntitiesInBox(low,high,nearby,JAR_ROPE_PATH_NEARBY);
 if(count>=JAR_ROPE_PATH_NEARBY)return 0;
 uint64_t hash=1;
 for(int i=0;i<count;++i){
  const int n=nearby[i];
  if(n<0||n>=ENTITYNUM_WORLD||n+1==o->first||n+1==o->second)continue;
  const gentity_t* e=&g_entities[n];
  if(!e->inuse||!e->r.linked||!(e->r.contents&JAR_ROPE_PATH_CONTENTS))continue;
  hash^=JAR_RopeSceneEntity(n,controllerEntityEpoch[n],e->r.contents,e->s.modelindex,0,e->r.currentOrigin,
                          e->r.bmodel?e->r.currentAngles:vec3_origin,e->r.mins,e->r.maxs);
 }
 return hash;
}
static void RopeWorldNormal(const vec3_t point,const vec3_t other,vec4_t normal){
 if(normal[3])return;
 jarRopePathHit_t h;RopeTrace(point,point,JAR_ROPE_PATH_CLEARANCE,&h,NULL);if(!h.solid)return;
 // Older journals did not retain clicked surface normals. Recover only from
 // a short trace that actually reaches this surface, never a remote obstacle.
 float best=-1e30f;vec3_t toward;VectorSubtract(other,point,toward);
 for(int axis=0;axis<3;++axis)for(int sign=-1;sign<=1;sign+=2){vec3_t from;VectorCopy(point,from);from[axis]+=8*sign;trace_t tr;
  trap->Trace(&tr,from,NULL,NULL,point,ENTITYNUM_NONE,MASK_SOLID,qfalse,0,0);
  if(tr.startsolid||tr.allsolid||tr.fraction>=1||Distance(tr.endpos,point)>1||VectorLengthSquared(tr.plane.normal)<.99f)continue;
  const float score=DotProduct(toward,tr.plane.normal);if(score>best){best=score;VectorCopy(tr.plane.normal,normal);normal[3]=1;}
 }
}
static qboolean RopeEndpoints(jarObject_t*o,vec3_t a,vec3_t b,float*caps){
 int x=o->first-1,y=o->second-1;*caps=0;
 if(x<0||y<0||x>ENTITYNUM_WORLD||y>ENTITYNUM_WORLD||x==y||(x<ENTITYNUM_WORLD&&!g_entities[x].inuse)||(y<ENTITYNUM_WORLD&&!g_entities[y].inuse))return qfalse;
 Endpoint(x,o->anchorA,a);Endpoint(y,o->anchorB,b);
 if(x==ENTITYNUM_WORLD)RopeWorldNormal(a,b,o->jointA);if(y==ENTITYNUM_WORLD)RopeWorldNormal(b,a,o->jointB);
 for(int i=0;i<2;++i){if((i?y:x)!=ENTITYNUM_WORLD)continue;float*p=i?b:a;const float*normal=i?o->jointB:o->jointA;if(!normal[3])continue;
  const float offset=JAR_ROPE_PATH_CLEARANCE*(fabsf(normal[0])+fabsf(normal[1])+fabsf(normal[2]))+.25f;VectorMA(p,offset,normal,p);*caps+=offset;
 }return JAR_RopePathPoint(a)&&JAR_RopePathPoint(b);
}
static jarRopePathResult_t RopeSolve(const jarObject_t*o,jarRopePath_t*path,const vec3_t a,const vec3_t b,int budget,qboolean*bodyContact){
 const int first=o->first-1,second=o->second-1;const qboolean linkA=first<ENTITYNUM_WORLD&&g_entities[first].r.linked,linkB=second<ENTITYNUM_WORLD&&g_entities[second].r.linked;
 if(linkA)trap->UnlinkEntity((sharedEntity_t*)&g_entities[first]);if(linkB)trap->UnlinkEntity((sharedEntity_t*)&g_entities[second]);
 jarRopePathResult_t result=JAR_RopePathSolve(path,a,b,fmaxf(.01f,o->length),RopeTrace,bodyContact,budget);
 if(linkA)trap->LinkEntity((sharedEntity_t*)&g_entities[first]);if(linkB)trap->LinkEntity((sharedEntity_t*)&g_entities[second]);return result;
}
static qboolean RopeRoute(jarObject_t*o,jarRopePath_t*path,float*length){
 if(o->jointKind)return qtrue;vec3_t a,b;float caps;if(!RopeEndpoints(o,a,b,&caps))return qfalse;
 qboolean bodyContact=qfalse;
 jarRopePathResult_t result=RopeSolve(o,path,a,b,JAR_ROPE_PATH_TRACE_LIMIT,&bodyContact);
 // A player standing over a rope must not prevent a world save or restore.
 // Check that the map route exists. Runtime tension still waits for clearance.
 if(bodyContact&&result.status!=JAR_ROPE_PATH_OK)result=RopeSolve(o,path,a,b,JAR_ROPE_PATH_TRACE_LIMIT,NULL);
 if(result.status!=JAR_ROPE_PATH_OK)return qfalse;if(length)*length=result.length+caps;return qtrue;
}
static qboolean RopeAdmission(int n,float*length){return RopeRoute(&objects[n],&objects[n].ropePath,length);}
static jarMPPose_t RopePose(int n){jarMPPose_t pose={0};if(n<ENTITYNUM_WORLD){VectorCopy(g_entities[n].r.currentOrigin,pose.position);VectorCopy(g_entities[n].r.currentAngles,pose.angles);}return pose;}
static void RopeStop(int n){
 if(n<0||n>=ENTITYNUM_WORLD||(!RopeMass(n)&&!objects[n].heldBy))return;
 gentity_t*e=&g_entities[n];VectorClear(objects[n].velocity);VectorClear(objects[n].angular);VectorClear(e->s.pos.trDelta);VectorClear(e->s.apos.trDelta);
 if(e->client){VectorClear(e->client->ps.velocity);if(e->m_pVehicle){VectorClear(e->client->ps.jarVehicleDrift);e->client->ps.speed=0;}}
 else if(PhysicsSolid(n)&&JAR_RigidActive()){jarMPPose_t pose=RopePose(n);JAR_RigidCorrect(n,&pose);}
}
static float RopeRadius(const float*lo,const float*hi){float sum=0;for(int a=0;a<3;++a){float v=fmaxf(fabsf(lo[a]),fabsf(hi[a]));sum+=v*v;}return sqrtf(sum);}
static void RopeRecover(int n,const jarMPPose_t*pose){
 if(n<0||n>=ENTITYNUM_WORLD||(!RopeMass(n)&&!objects[n].heldBy))return;
 gentity_t*e=&g_entities[n];
 // Do not relocate occupied rigs independently of their native rider movement.
 if(e->m_pVehicle&&VehicleOccupied(e->m_pVehicle))return;
 vec3_t lo,hi;if(PhysicsProp(n))PhysicsHull(e,pose->angles,lo,hi);else if(!VehicleBounds(e,pose->angles,lo,hi))return;
 vec3_t sweepLo,sweepHi;const qboolean rotated=!VectorCompare(pose->angles,e->r.currentAngles);float radius=fmaxf(RopeRadius(lo,hi),RopeRadius(e->r.mins,e->r.maxs));
 for(int a=0;a<3;++a){sweepLo[a]=rotated?-radius:fminf(lo[a],e->r.mins[a]);sweepHi[a]=rotated?radius:fmaxf(hi[a],e->r.maxs[a]);}
 trace_t tr;trap->Trace(&tr,e->r.currentOrigin,sweepLo,sweepHi,pose->position,n,MASK_PLAYERSOLID,qfalse,0,0);if(tr.startsolid||tr.allsolid||tr.fraction<1)return;
 if(PhysicsSolid(n)&&JAR_RigidActive()&&!JAR_RigidCorrect(n,pose))return;
 Move(e,pose->position);VectorCopy(pose->angles,e->r.currentAngles);VectorCopy(pose->angles,e->s.angles);VectorCopy(pose->angles,e->s.apos.trBase);VectorCopy(lo,e->r.mins);VectorCopy(hi,e->r.maxs);
 if(e->client){SetClientViewAngle(e,pose->angles);VehicleOrientation(e);}else PhysicsPublish(e);trap->LinkEntity((sharedEntity_t*)e);
}
static qboolean RopeLengthExceeded(float measured,float limit){
 return measured>limit+fmaxf(.005f,limit*2*FLT_EPSILON);
}
static qboolean RopeOverLength(int n){
 return RopeLengthExceeded(ropeCache[n].result.length,objects[n].length);
}
static qboolean RopeHasSafePose(int n){
 return ropeCache[n].hasSafePose&&!RopeLengthExceeded(ropeCache[n].safeLength,objects[n].length);
}
static void RopeRememberLength(int n){
 jarRopeCache_t* cache=&ropeCache[n];
 if(RopeOverLength(n))return;
 cache->safeA=cache->poseA;cache->safeB=cache->poseB;cache->hasSafePose=1;
 cache->safeLength=cache->result.length;
}
static void RopeUpdate(int n){
 jarObject_t*o=&objects[n];jarRopeCache_t*c=&ropeCache[n];const int first=o->first-1,second=o->second-1;
 if(c->identity!=controllerEntityEpoch[n]||c->first!=o->first||c->second!=o->second){memset(c,0,sizeof(*c));c->identity=controllerEntityEpoch[n];c->first=o->first;c->second=o->second;}
 c->valid=0;
 if(!ForcePermission(o->owner,first)||!ForcePermission(o->owner,second)||objects[first].copyPending||objects[second].copyPending||BodyDormant(first)||BodyDormant(second)||ForceTransition(first)||ForceTransition(second)){JAR_RigidRemoveRope(n+1);return;}
 vec3_t a,b;float caps;if(!RopeEndpoints(o,a,b,&caps)){JAR_RigidRemoveRope(n+1);return;}
 if(c->history&&c->scene&&VectorCompare(a,c->a)&&VectorCompare(b,c->b)&&c->scene==RopeScene(o,a,b)){
  c->valid=1;c->poseA=RopePose(first);c->poseB=RopePose(second);RopeRememberLength(n);return;
 }
 if(ropeBudgetTime!=level.time){ropeBudgetTime=level.time;ropeBudget=4096;}
 jarRopePathResult_t result={0};result.status=JAR_ROPE_PATH_BUDGET;
 qboolean bodyContact=qfalse;
 if(ropeBudget>0){result=RopeSolve(o,&o->ropePath,a,b,ropeBudget<JAR_ROPE_PATH_TRACE_LIMIT?ropeBudget:JAR_ROPE_PATH_TRACE_LIMIT,&bodyContact);ropeBudget-=result.traces;}
 if(result.status==JAR_ROPE_PATH_OK){c->valid=c->history=1;c->scene=RopeScene(o,a,b);VectorCopy(a,c->a);VectorCopy(b,c->b);c->poseA=RopePose(first);c->poseB=RopePose(second);c->result=result;c->result.length+=caps;RopeRememberLength(n);}
 else if(bodyContact){JAR_RigidRemoveRope(n+1);return;}
 else{if(RopeHasSafePose(n)){RopeRecover(first,&c->safeA);RopeRecover(second,&c->safeB);}
  else if(c->history){RopeRecover(first,&c->poseA);RopeRecover(second,&c->poseB);}RopeStop(first);RopeStop(second);
  if(level.time>=ropeNoticeAt||ropeNoticeStatus!=(int)result.status){Tell(o->owner<=MAX_CLIENTS?&g_entities[o->owner-1]:NULL,result.status==JAR_ROPE_PATH_BUDGET?"Rope collision work is busy; movement held while links are checked.":"Rope route is obstructed. Move the obstacle or remove the rope to release it.");ropeNoticeAt=level.time+2000;ropeNoticeStatus=result.status;}
 }
 if(!c->valid||o->ropePath.count)JAR_RigidRemoveRope(n+1);
}
static void RopePrepare(void){
 int list[JAR_MP_ATTACHMENT_LIMIT],count=0;for(int n=MAX_CLIENTS;n<level.num_entities&&count<JAR_MP_ATTACHMENT_LIMIT;++n)if(RopeLink(n))list[count++]=n;
 if(!count)return;int start=(unsigned)ropeCursor++%count;
 for(int i=0;i<count;++i)RopeUpdate(list[(start+i)%count]);
}
static qboolean RopeRigid(int n){return RopeLink(n)&&ropeCache[n].valid&&!objects[n].ropePath.count;}
static qboolean RopeNeedsProjection(void){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  if(RopeLink(n)&&ropeCache[n].valid&&objects[n].ropePath.count&&RopeOverLength(n))return qtrue;
 }
 return qfalse;
}
static void RopeRestoreLength(void){
 qboolean retry=qfalse;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  const jarRopeCache_t* cache=&ropeCache[n];const jarObject_t* rope=&objects[n];
  if(!RopeLink(n)||!cache->valid||!rope->ropePath.count||!RopeOverLength(n))continue;
  const int first=rope->first-1,second=rope->second-1;
  if(RopeHasSafePose(n)){RopeRecover(first,&cache->safeA);RopeRecover(second,&cache->safeB);retry=qtrue;}
  RopeStop(first);RopeStop(second);
 }
 if(retry)RopePrepare();
}
static float RopeContactMass(int owner,const jarRopeContact_t* contact){
 const int n=contact->prop-1;
 if(n<MAX_CLIENTS||n>=ENTITYNUM_WORLD||controllerEntityEpoch[n]!=contact->generation||
    !g_entities[n].r.linked||g_entities[n].client||!PhysicsSolid(n)||
    jarMPCatalog[objects[n].catalog-1].kind!=1||!ForcePermission(owner,n))return 0;
 return ForceMass(n);
}
static float RopeContactLoads(const jarRopePathResult_t* path,int owner,vec3_t* points,float* masses,float* outward){
 jarRopeLoad_t loads[JAR_ROPE_PATH_BENDS];int count=0;
 for(int i=0;i<path->contactCount;++i){
  const jarRopeContact_t* contact=&path->contacts[i];const int n=contact->prop-1;
  masses[i]=RopeContactMass(owner,contact);
  if(!masses[i])continue;
  const entityShared_t* body=&g_entities[n].r;
  JAR_RopeContactPoint(contact,body->currentOrigin,body->mins,body->maxs,points[i]);
  vec3_t velocity;ForcePointVelocity(n,points[i],velocity);*outward-=DotProduct(velocity,contact->pull);
  JAR_RopeAddLoad(loads,&count,contact,points[i],body->currentOrigin,masses[i]);
 }
 float inverseMass=0;
 for(int i=0;i<count;++i){
  const jarRopeLoad_t* load=&loads[i];const int n=load->prop-1;
  inverseMass+=JAR_RigidActive()?JAR_RigidImpulseResponse(n,load->linear,load->torque,g_entities[n].r.currentOrigin):
   VectorLengthSquared(load->linear)/load->mass;
 }
 return inverseMass;
}
static void RopePublish(int n){
 gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];jarRopeCache_t*c=&ropeCache[n];vec3_t a,b;float caps;
 e->s.jarRopeState=1;memset(e->s.jarRopeBends,0,sizeof(e->s.jarRopeBends));e->s.jarRopeLength=0;
 if(!c->valid||(o->ropePath.count&&RopeOverLength(n))||!RopeEndpoints(o,a,b,&caps)||!VectorCompare(a,c->a)||!VectorCompare(b,c->b))return;
 Move(e,a);VectorCopy(b,e->s.origin2);e->s.otherEntityNum=o->first-1;e->s.otherEntityNum2=o->second-1;
 e->s.jarRopeState=o->ropePath.count+2;e->s.jarRopeLength=fmaxf(0,o->length-caps);memcpy(e->s.jarRopeBends,o->ropePath.bend,sizeof(e->s.jarRopeBends));
}
