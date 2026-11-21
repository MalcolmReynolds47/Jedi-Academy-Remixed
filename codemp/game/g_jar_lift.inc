// Multiplayer adapters for the shared platform transaction.
#include "qcommon/jar_lift_motion.h"

static jarLiftBody_t LiftBody(const gentity_t* e){
 jarLiftBody_t body;body.entity=e->s.number;body.linked=e->r.linked;
 VectorCopy(e->r.currentOrigin,body.origin);VectorCopy(e->r.mins,body.mins);VectorCopy(e->r.maxs,body.maxs);
 return body;
}
static int LiftClear(const jarLiftBody_t* body,const vec3_t destination){
 trace_t tr;
 trap->Trace(&tr,body->origin,body->mins,body->maxs,destination,body->entity,MASK_PLAYERSOLID|CONTENTS_CORPSE,qfalse,0,0);
 return !tr.startsolid&&!tr.allsolid&&tr.fraction>=1;
}
static void LiftLinked(int number,int link){
 sharedEntity_t* e=(sharedEntity_t*)&g_entities[number];
 if(link)trap->LinkEntity(e);else trap->UnlinkEntity(e);
}
static void LiftPlaceNative(int number,const vec3_t destination,int deck){
 gentity_t* e=&g_entities[number];vec3_t offset;
 VectorSubtract(destination,e->r.currentOrigin,offset);
 VectorCopy(destination,e->r.currentOrigin);
 VectorCopy(destination,e->s.origin);VectorAdd(e->s.pos.trBase,offset,e->s.pos.trBase);
 if(e->client)VectorAdd(e->client->ps.origin,offset,e->client->ps.origin);
 if(e->m_pVehicle&&e->m_pVehicle->m_LandTrace.entityNum==deck&&e->m_pVehicle->m_LandTrace.fraction<1)
  VectorAdd(e->m_pVehicle->m_LandTrace.endpos,offset,e->m_pVehicle->m_LandTrace.endpos);
}
static void LiftPlace(int number,const vec3_t destination,int deck){
 LiftPlaceNative(number,destination,deck);
 if(PhysicsSolid(number)&&JAR_RigidActive()){
  const jarMPPose_t pose=PhysicsPose(number);JAR_RigidCorrect(number,&pose);
 }
}
static qboolean LiftVehicleSupported(const gentity_t* e,const jarLiftBody_t* deck,const jarLiftBody_t* body,qboolean standing){
 if(standing&&JAR_LiftSupported(deck,body,1))return qtrue;
 const Vehicle_t* vehicle=e->m_pVehicle;
 if(!e->client||!vehicle->m_pVehicleInfo||!JAR_LiftOverlaps(deck,body))return qfalse;
 const vehicleInfo_t* info=vehicle->m_pVehicleInfo;
 const qboolean fighter=info->type==VH_FIGHTER;
 const qboolean landed=fighter&&vehicle->m_LandTrace.entityNum==deck->entity&&
  vehicle->m_LandTrace.fraction<.15f&&vehicle->m_LandTrace.plane.normal[2]>=.7f;
 if(fighter&&!landed)return qfalse;
 const float reach=JAR_LiftVehicleReach(info->hoverHeight,info->landingHeight,landed,e->client->ps.velocity,vehicle->m_ucmd.upmove>0);
 const float gap=JAR_LiftGap(deck,body);
 if(reach<=0||gap<-.5f||gap>reach)return qfalse;
 vec3_t end;VectorCopy(body->origin,end);end[2]-=reach;
 trace_t trace;
 trap->Trace(&trace,body->origin,body->mins,body->maxs,end,e->s.number,MASK_PLAYERSOLID,qfalse,0,0);
 return !trace.startsolid&&!trace.allsolid&&trace.fraction<1&&trace.entityNum==deck->entity&&trace.plane.normal[2]>=.7f;
}
static qboolean LiftSafeMember(gentity_t* e){
 const int n=e->s.number;
 return !objects[n].heldBy&&!objects[n].copyPending&&!JAR_MP_ActorControlled(e)&&!PhysicsConstrained(n)&&
  (n>=MAX_CLIENTS||(playerTools.integer&&!players[n].noTools));
}
static qboolean LiftVehicleMembers(gentity_t* root,int owner,jarLiftBody_t* bodies,int* count){
 const int n=root->s.number;
 if(!root->client||!ActorAlive(root)||!LiftSafeMember(root))return qfalse;
 if(owner>0&&owner<=MAX_CLIENTS){
  if(!ForcePermission(owner,n))return qfalse;
 }else if(owner!=MAX_CLIENTS+1||
          (objects[n].owner?(objects[n].owner!=owner&&!sharedEditing.integer):!mapEditing.integer)||
          !VehicleSeatsPermit(NULL,root))return qfalse;
 const Vehicle_t* vehicle=root->m_pVehicle;
 for(int i=-1;i<=vehicle->m_iNumPassengers;++i){
  const bgEntity_t* pointer=i<0?vehicle->m_pPilot:i==vehicle->m_iNumPassengers?vehicle->m_pDroidUnit:vehicle->m_ppPassengers[i];
  if(!pointer)continue;
  gentity_t* seat=VehicleSeatEntity(pointer);
  if(!seat||!LiftSafeMember(seat))return qfalse;
  const jarLiftBody_t body=LiftBody(seat);
  if(!JAR_LiftAddBody(bodies,count,&body))return qfalse;
 }
 return qtrue;
}
static int LiftCollect(gentity_t* deck,jarLiftBody_t* bodies,int* vehicleGroup){
 bodies[0]=LiftBody(deck);int count=1;
 int candidates[JAR_LIFT_QUERY];vec3_t low,high;JAR_LiftQuery(&bodies[0],low,high);
 const int found=trap->EntitiesInBox(low,high,candidates,JAR_LIFT_QUERY);
 if(found>=JAR_LIFT_QUERY)return qfalse;
 const int owner=objects[deck->s.number].owner;
 for(int i=0;i<found;++i){
  const int n=candidates[i];if(n<0||n>=ENTITYNUM_WORLD)continue;
  gentity_t* e=&g_entities[n];
  if(!e->inuse||!e->r.linked||e==deck||!(e->r.contents&(CONTENTS_BODY|CONTENTS_SOLID|CONTENTS_ITEM|CONTENTS_CORPSE)))continue;
  const qboolean standing=(e->client?e->client->ps.groundEntityNum:e->s.groundEntityNum)==deck->s.number;
  const qboolean looseProp=PhysicsSolid(n)&&!objects[n].frozen;
  if(!e->m_pVehicle&&(!standing&&(!looseProp||objects[n].velocity[2]>64)))continue;
  if(!e->m_pVehicle&&e->client&&e->client->ps.m_iVehicleNum)continue;
  const jarLiftBody_t body=LiftBody(e);
  if(e->m_pVehicle){
   if(!LiftVehicleSupported(e,&bodies[0],&body,standing))continue;
   const int first=count;
   if(!JAR_LiftAddBody(bodies,&count,&body)||!LiftVehicleMembers(e,owner,bodies,&count))return qfalse;
   if(vehicleGroup)for(int member=first;member<count;++member)vehicleGroup[member]=first;
   continue;
  }
  if(!JAR_LiftSupported(&bodies[0],&body,standing))continue;
  if(!LiftSafeMember(e)||(vehicleGroup&&PhysicsSolid(n)&&objects[n].frozen)||
     (looseProp&&!(owner==MAX_CLIENTS+1&&sharedEditing.integer)&&!ForcePermission(owner,n))||
     !JAR_LiftAddBody(bodies,&count,&body))return qfalse;
 }
 return count;
}

static qboolean LiftTranslate(gentity_t* deck,float dz){
 jarLiftBody_t bodies[JAR_LIFT_MAX_RIDERS+1];
 const int count=LiftCollect(deck,bodies,NULL);
 return count&&JAR_LiftMove(bodies,count,dz,LiftClear,LiftLinked,LiftPlace)?qtrue:qfalse;
}
