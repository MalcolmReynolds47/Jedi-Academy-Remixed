// Shared-world rope/thruster forces on native actors and simulated props.
// Native movement still owns actor contacts, animation and collision. GPL-2.0-only.
static qboolean ForceProp(int n){return n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&objects[n].catalog>0&&objects[n].catalog<=JAR_MP_CATALOG_COUNT&&CatalogBody(&jarMPCatalog[objects[n].catalog-1]);}
static qboolean ForceTransition(int n){return n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&g_entities[n].m_pVehicle&&!VehicleAvailable(&g_entities[n]);}
static qboolean ForcePermission(int owner,int n){
 if(n==ENTITYNUM_WORLD)return qtrue;
 if(n<0||n>=ENTITYNUM_WORLD||!g_entities[n].inuse||owner<1||owner>MAX_CLIENTS+1)return qfalse;
 gentity_t*e=&g_entities[n];
 // A restored assembly can contain native members still inside NPC_Begin.
 // Keep its attachments inert until the entire native endpoint is ready.
 if(owner==MAX_CLIENTS+1){
  if(n<MAX_CLIENTS)return qfalse;
  if(objects[n].owner!=owner&&!(e->client&&!objects[n].owner&&mapEditing.integer))return qfalse;
  return objects[n].copyPending||!e->client||(ActorAliveMode(e,qtrue)&&VehicleSeatsPermit(NULL,e));
 }
 gentity_t*p=&g_entities[owner-1];
 if(!p->inuse||!p->client||p->client->pers.connected!=CON_CONNECTED)return qfalse;
 if(objects[n].copyPending)return Edit(p,n);
 if(e->m_pVehicle)return ActorAliveMode(e,qtrue)&&VehicleSeatsPermit(p,e)&&(objects[n].owner?Edit(p,n):mapEditing.integer!=0);
 return e->client?ActorEdit(p,n):Edit(p,n);
}
static void ForceRemoveThruster(int n){
 jarObject_t*o=&objects[n];
 o->thruster=o->thrustBy=o->thrustMounted=0;
 VectorClear(o->thrustAnchor);VectorSet(o->thrustAxis,0,0,1);
 thrustCreation[n].order=0;
}
static void ForceDetach(int n){
 if(n<0||n>=ENTITYNUM_WORLD)return;
 objects[n].output[0]=objects[n].output[1]=0;ControllerForget(n);ForceRemoveThruster(n);
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(g_entities[i].inuse&&g_entities[i].s.generic1==251&&objects[i].owner&&(objects[i].first==n+1||objects[i].second==n+1))G_FreeEntity(&g_entities[i]);
}
static void ForceClientReset(int c){
 ForceDetach(c);
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse){
  if(g_entities[n].client&&objects[n].thrustBy==c+1&&objects[n].owner!=c+1)ForceRemoveThruster(n);
 }
}
static float ForceMass(int n){
 if(n<0||n>=ENTITYNUM_WORLD||!g_entities[n].inuse||objects[n].copyPending||objects[n].frozen||objects[n].heldBy||JAR_MP_ActorControlled(&g_entities[n]))return 0;
 if(BodyDormant(n))return 0;
 if(!ForceProp(n)&&(!g_entities[n].client||!ActorAlive(&g_entities[n])))return 0;
 const float mass=objects[n].mass;return JAR_ContextMassValid(mass)?mass:80;
}
qboolean JAR_MP_VehicleForces(int n){
 if(!sandbox.integer||level.intermissiontime||n<MAX_CLIENTS||n>=ENTITYNUM_WORLD||!ActorVehicle(&g_entities[n])||VehicleOccupied(g_entities[n].m_pVehicle)||!ForceMass(n))return qfalse;
 jarObject_t*o=&objects[n];if(o->thruster&&ForcePermission(o->thrustBy?o->thrustBy:o->owner,n))return qtrue;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(g_entities[i].inuse&&g_entities[i].s.generic1==251&&objects[i].owner){
  jarObject_t*r=&objects[i];if(r->first!=n+1&&r->second!=n+1)continue;
  int a=r->first-1,b=r->second-1;
  if(ForcePermission(r->owner,a)&&ForcePermission(r->owner,b)&&!(a<ENTITYNUM_WORLD&&objects[a].copyPending)&&!(b<ENTITYNUM_WORLD&&objects[b].copyPending))return qtrue;
 }
 return qfalse;
}
static void ForceVelocity(int n,vec3_t velocity){
 VectorClear(velocity);if(n<0||n>=ENTITYNUM_WORLD)return;
 if(objects[n].heldBy){VectorCopy(objects[n].velocity,velocity);return;}
 if(objects[n].frozen||JAR_MP_ActorControlled(&g_entities[n]))return;
 if(g_entities[n].client)VectorCopy(g_entities[n].client->ps.velocity,velocity);
 else if(ForceProp(n))VectorCopy(objects[n].velocity,velocity);
}
static float RopeMass(int n){if(n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&PhysicsSolid(n)&&JAR_RigidActive()&&objects[n].heldBy&&!objects[n].frozen)return objects[n].mass;return ForceMass(n);}
static void ForcePointVelocity(int n,const vec3_t point,vec3_t velocity){if(n<ENTITYNUM_WORLD&&PhysicsProp(n)&&JAR_RigidPointVelocity(n,point,velocity))return;ForceVelocity(n,velocity);}
static float RopeInverseMass(int n,const vec3_t point,const vec3_t pull,float mass){
 if(!mass)return 0;
 if(PhysicsSolid(n)&&JAR_RigidActive()){
  const float response=JAR_RigidImpulseResponse(n,pull,vec3_origin,point);
  if(response>0)return response;
 }
 return DotProduct(pull,pull)/mass;
}
static void ForceImpulseAt(int n,const vec3_t direction,float impulse,float mass,const vec3_t point){
 if(!mass||!isfinite(impulse)||impulse==0)return;
 if(point&&PhysicsSolid(n)&&JAR_RigidActive()){
  vec3_t push,low,high;jarMPPose_t pose;VectorScale(direction,impulse,push);
  if(JAR_RigidForce(n,push,point)&&JAR_RigidRead(n,&pose,low,high)){VectorCopy(pose.linear,objects[n].velocity);VectorCopy(pose.angular,objects[n].angular);}return;
 }
 vec3_t velocity;ForceVelocity(n,velocity);VectorMA(velocity,impulse/mass,direction,velocity);
 for(int a=0;a<3;++a)if(!isfinite(velocity[a]))return;
 gentity_t*e=&g_entities[n];
 qboolean driven=e->client&&e->m_pVehicle&&VehicleOccupied(e->m_pVehicle);
 if(driven){JAR_VehicleDriftVelocity(&e->client->ps,velocity);VectorCopy(e->client->ps.velocity,velocity);}
 else{float speed=VectorLength(velocity);if(speed>1500)VectorScale(velocity,1500/speed,velocity);}
 if(e->client){
  VectorCopy(velocity,e->client->ps.velocity);
  if(velocity[2]>1)e->client->ps.groundEntityNum=ENTITYNUM_NONE;
  if(e->m_pVehicle&&!driven){vec3_t forward;AngleVectors(e->r.currentAngles,forward,NULL,NULL);e->client->ps.speed=DotProduct(velocity,forward);}
  // The normal client/NPC end-frame path already converted this snapshot.
  // Re-running it here could consume a second queued weapon/animation event.
  // Only velocity and the lifted ground flag changed; position stays native.
  VectorCopy(velocity,e->s.pos.trDelta);e->s.groundEntityNum=e->client->ps.groundEntityNum;
 }else{VectorCopy(velocity,objects[n].velocity);PhysicsVelocity(n,velocity);}
}
static void ThrusterFrame(int n,vec3_t point,vec3_t direction){
 jarObject_t*o=&objects[n];if(!o->thrustMounted){VectorCopy(g_entities[n].r.currentOrigin,point);AngleVectors(g_entities[n].r.currentAngles,NULL,NULL,direction);return;}
 Endpoint(n,o->thrustAnchor,point);vec3_t axis[3];AnglesToAxis(g_entities[n].r.currentAngles,axis);VectorClear(direction);for(int a=0;a<3;++a)VectorMA(direction,o->thrustAxis[a],axis[a],direction);
}
#include "g_jar_rope.inc"
static void ForceRopeVisuals(void){
 RopePrepare();
 for(int n=0;n<level.num_entities;++n){jarObject_t*o=&objects[n];int id=thrusterVisual[n]-1;
  qboolean show=g_entities[n].inuse&&!BodyDormant(n)&&(o->thrustMounted||o->thruster)&&!o->copyPending&&ForcePermission(o->thrustBy?o->thrustBy:o->owner,n);
  if(!show){if(id>=MAX_CLIENTS&&id<ENTITYNUM_WORLD&&g_entities[id].inuse)G_FreeEntity(&g_entities[id]);thrusterVisual[n]=0;continue;}
  if(id<0){if(!Room())continue;gentity_t*v=G_TrySpawn();if(!v)continue;id=v->s.number;thrusterVisual[n]=id+1;v->classname="jar_thruster_visual";v->s.eType=ET_GENERAL;v->s.generic1=254;v->s.otherEntityNum=n;}
  vec3_t point,axis;ThrusterFrame(n,point,axis);gentity_t*v=&g_entities[id];Move(v,point);VectorCopy(axis,v->s.origin2);v->s.time2=o->thruster;
 }

 for(int n=MAX_CLIENTS;n<level.num_entities;++n){gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];if(!e->inuse||!o->owner||e->s.generic1!=251)continue;
  if(!o->jointKind)RopePublish(n);
 }
}
static void ForceFrame(float dt){
 RopePrepare();
 for(int n=0;n<level.num_entities;++n){gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];if(!e->inuse||!o->thruster)continue;
  const int owner=o->thrustBy?o->thrustBy:o->owner;
  if(!ForcePermission(owner,n)){o->thruster=o->thrustBy=0;continue;}
  if(!sandbox.integer||level.intermissiontime||dt<=0)continue;
  vec3_t point,up;ThrusterFrame(n,point,up);const double impulse=JAR_THRUSTER_SIGN*(double)o->thrustForce*32*dt;
  ForceImpulseAt(n,up,(float)fmin(1000000,fmax(-1000000,impulse)),ForceMass(n),o->thrustMounted?point:NULL);
 }
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];if(!e->inuse||!o->owner||e->s.generic1!=251)continue;
  const int first=o->first-1,second=o->second-1;
  if(!ForcePermission(o->owner,first)||!ForcePermission(o->owner,second)){G_FreeEntity(e);continue;}
  vec3_t a,b;Endpoint(first,o->anchorA,a);Endpoint(second,o->anchorB,b);
  if(!sandbox.integer||level.intermissiontime||dt<=0||(first<ENTITYNUM_WORLD&&objects[first].copyPending)||(second<ENTITYNUM_WORLD&&objects[second].copyPending)||ForceTransition(first)||ForceTransition(second)||BodyDormant(first)||BodyDormant(second))continue;
  if(o->jointKind||(PhysicsRope(n)&&RopeRigid(n))||!ropeCache[n].valid)continue;
  const jarRopePathResult_t*path=&ropeCache[n].result;float distance=path->length;
  if(!isfinite(distance)||!isfinite(o->length)||distance<=0)continue;
  const float ma=RopeMass(first),mb=RopeMass(second);
  vec3_t va,vb;ForcePointVelocity(first,a,va);ForcePointVelocity(second,b,vb);
  float outward=-DotProduct(va,path->pullA)-DotProduct(vb,path->pullB);
  float masses[JAR_ROPE_PATH_BENDS];
  vec3_t points[JAR_ROPE_PATH_BENDS];
  const float contactResponse=RopeContactLoads(path,o->owner,points,masses,&outward);
  const float inverse=RopeInverseMass(first,a,path->pullA,ma)+RopeInverseMass(second,b,path->pullB,mb)+contactResponse;
  if(inverse<=0)continue;
  const float allowed=fmaxf(0,o->length-distance)/dt;
  const float impulse=fmaxf(0,(outward-allowed)/inverse);
  ForceImpulseAt(first,path->pullA,impulse,ma,a);ForceImpulseAt(second,path->pullB,impulse,mb,b);
  for(int i=0;i<path->contactCount;++i){const jarRopeContact_t* contact=&path->contacts[i];
   if(masses[i])ForceImpulseAt(contact->prop-1,contact->pull,impulse,masses[i],points[i]);
  }
 }
}
