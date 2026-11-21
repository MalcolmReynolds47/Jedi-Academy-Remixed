// Immutable Weld/Hinge frames; native AI and movement remain authoritative.
// GPL-2.0-only.
static byte jointNativeKeys[MAX_GENTITIES],jointPendingKeys[MAX_GENTITIES];
static struct {unsigned identity;int time;vec3_t origin,angles;} jointHistory[MAX_GENTITIES];
static void JointReset(void){memset(jointNativeKeys,0,sizeof(jointNativeKeys));memset(jointPendingKeys,0,sizeof(jointPendingKeys));memset(jointHistory,0,sizeof(jointHistory));JAR_NativeJointsClear();}
static qboolean JointLink(int n){return n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&g_entities[n].inuse&&objects[n].owner&&g_entities[n].s.generic1==251&&objects[n].jointKind>=1&&objects[n].jointKind<=2;}
static qboolean ConstraintLink(int n){return JointLink(n)||(RopeLink(n)&&ropeCache[n].valid);}
static qboolean JointEndpoint(int n){return n==ENTITYNUM_WORLD||(n>=0&&n<ENTITYNUM_WORLD&&g_entities[n].inuse&&(PhysicsProp(n)||(g_entities[n].client&&ActorAlive(&g_entities[n]))));}
static void JointClassify(qboolean rigid){
 memset(jointNativeKeys,0,sizeof(jointNativeKeys));memset(jointPendingKeys,0,sizeof(jointPendingKeys));
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ConstraintLink(n)){
  int a=objects[n].first-1,b=objects[n].second-1;if(a<0||a>ENTITYNUM_WORLD||b<0||b>ENTITYNUM_WORLD)continue;
  if(!rigid||(a!=ENTITYNUM_WORLD&&!PhysicsSolid(a))||(b!=ENTITYNUM_WORLD&&!PhysicsSolid(b))||
     (!objects[n].jointKind&&(!RopeRigid(n)||objects[n].length>8192))){if(a!=ENTITYNUM_WORLD)jointNativeKeys[a]=1;if(b!=ENTITYNUM_WORLD)jointNativeKeys[b]=1;}
  if(BodyDormant(a)||BodyDormant(b)||objects[a].copyPending||objects[b].copyPending||ForceTransition(a)||ForceTransition(b)){if(a!=ENTITYNUM_WORLD)jointPendingKeys[a]=1;if(b!=ENTITYNUM_WORLD)jointPendingKeys[b]=1;}
 }
 // A mixed component must use one solver, including its prop-to-prop links.
 for(int pass=0;pass<JAR_MP_ATTACHMENT_LIMIT;++pass){qboolean changed=qfalse;
  for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ConstraintLink(n)){
   int a=objects[n].first-1,b=objects[n].second-1;if(a<0||a>ENTITYNUM_WORLD||b<0||b>ENTITYNUM_WORLD)continue;
   if(a!=ENTITYNUM_WORLD&&b!=ENTITYNUM_WORLD&&jointNativeKeys[a]!=jointNativeKeys[b]){jointNativeKeys[a]=jointNativeKeys[b]=1;changed=qtrue;}
   if(a!=ENTITYNUM_WORLD&&b!=ENTITYNUM_WORLD&&jointPendingKeys[a]!=jointPendingKeys[b]){jointPendingKeys[a]=jointPendingKeys[b]=1;changed=qtrue;}
  }if(!changed)break;
 }
}
static qboolean JointNative(int n){int a=objects[n].first-1,b=objects[n].second-1;return ConstraintLink(n)&&a>=0&&a<=ENTITYNUM_WORLD&&b>=0&&b<=ENTITYNUM_WORLD&&(jointNativeKeys[a]||jointNativeKeys[b]);}
static void JointRecord(int n,jarMPJoint_t*j){
 jarObject_t*o=&objects[n];memset(j,0,sizeof(*j));j->id=n+1;j->kind=o->jointKind;j->first=PhysicsIdentity(o->first-1);j->second=PhysicsIdentity(o->second-1);
 VectorCopy(o->anchorA,j->a);VectorCopy(o->anchorB,j->b);memcpy(j->ra,o->jointA,sizeof(j->ra));memcpy(j->rb,o->jointB,sizeof(j->rb));
}
static void RopeRecord(int n,qboolean box3d,jarMPJoint_t *record,byte* seen){
 const jarObject_t* rope=&objects[n];
 JointRecord(n,record);
 record->kind=3;
 record->ra[3]=record->rb[3]=1;
 record->length=rope->length;
 if(box3d)return; // Match the straight joint's physical world anchors.
 float caps=0;
 if(rope->first-1==ENTITYNUM_WORLD){VectorCopy(ropeCache[n].a,record->a);caps+=Distance(rope->anchorA,record->a);}
 if(rope->second-1==ENTITYNUM_WORLD){VectorCopy(ropeCache[n].b,record->b);caps+=Distance(rope->anchorB,record->b);}
 record->length=fmaxf(.01f,rope->length-caps);
 record->path=rope->ropePath;
 const jarRopePathResult_t* result=&ropeCache[n].result;
 for(int i=0;i<result->contactCount;++i){
  const jarRopeContact_t* contact=&result->contacts[i];
  if(!RopeContactMass(rope->owner,contact))continue;
  const int slot=contact->prop-1,bend=contact->bend;
  record->bends[bend].key=PhysicsIdentity(slot);
  Anchor(slot,record->path.bend[bend],record->bends[bend].local);
  seen[slot]=1;
 }
}
static jarMPPose_t JointPose(int n){jarMPPose_t pose={0};if(n==ENTITYNUM_WORLD)return pose;pose=PhysicsPose(n);if(g_entities[n].client)ForceVelocity(n,pose.linear);return pose;}
static void JointTool(gentity_t*p,const trace_t*tr){
 jarPlayer_t*cl=&players[p->s.number];const int n=tr->entityNum,kind=cl->tool-4;
 if(kind<1||kind>2||!JointEndpoint(n)){Tell(p,"Select a prop, NPC, vehicle, consenting player or world surface.");return;}
 jarMPPose_t pose=JointPose(n);
 if(cl->first==ENTITYNUM_NONE){
  if(!JAR_JointFrame(&pose,tr->endpos,tr->plane.normal,cl->firstAnchor,cl->firstRotation)){Tell(p,"Joint anchor is outside supported bounds.");return;}
  cl->first=n;Tell(p,kind==2?"Hinge pivot set. The axis follows this face's normal; select a second surface. Right click cancels.":"First weld anchor set; select a second surface. Right click cancels.");return;
 }
 int first=cl->first;if(first==n)return;cl->first=ENTITYNUM_NONE;
 if(!JointEndpoint(first)||(first!=ENTITYNUM_WORLD&&!Manipulate(p,first))||!Room())return;
 int count=0;for(int r=MAX_CLIENTS;r<level.num_entities;++r)if(g_entities[r].inuse&&objects[r].owner&&g_entities[r].s.generic1==251){
  ++count;jarObject_t*o=&objects[r];if(o->jointKind==kind&&((o->first==first+1&&o->second==n+1)||(o->second==first+1&&o->first==n+1))){Tell(p,"That pair already has this joint.");return;}
 }
 if(count>=JAR_MP_ATTACHMENT_LIMIT){Tell(p,"Attachment capacity reached.");return;}
 jarMPPose_t before=JointPose(first);vec3_t point,axis,a,b;vec4_t ra,rb;
 if(!JAR_JointPoint(&before,cl->firstAnchor,cl->firstRotation,point,axis)||Distance(point,tr->endpos)>8192){
  Tell(p,"Joint anchors must fit within supported local bounds and 8192 units.");return;
 }
 if(n==ENTITYNUM_WORLD)VectorCopy(tr->endpos,point);
 if(!JAR_JointFrame(&before,point,axis,a,ra)||!JAR_JointFrame(&pose,point,axis,b,rb)||
    !CopyVector(a,first==ENTITYNUM_WORLD?65535:8192)||!CopyVector(b,n==ENTITYNUM_WORLD?65535:8192)){Tell(p,"Joint anchors must fit within supported local bounds and 8192 units.");return;}
 gentity_t*e=G_TrySpawn();if(!e)return;Track(e,p->s.number,-1);jarObject_t*o=&objects[e->s.number];o->first=first+1;o->second=n+1;o->jointKind=kind;
 VectorCopy(a,o->anchorA);VectorCopy(b,o->anchorB);memcpy(o->jointA,ra,sizeof(ra));memcpy(o->jointB,rb,sizeof(rb));
 e->classname=kind==1?"jar_weld":"jar_hinge";e->s.eType=ET_GENERAL;e->s.generic1=251;e->s.weapon=kind;e->r.svFlags|=SVF_NOCLIENT;Move(e,point);
 Tell(p,kind==1?"Weld created. Released members move together; frozen members and the world anchor it. Z undoes the link.":"Hinge created. Released members turn around the first face's axis. Z undoes the link.");
}
static qboolean JointProject(float dt,qboolean rigid,byte*blocked,qboolean advance){
 JointClassify(rigid);
 static jarMPJointBody_t bodies[MAX_GENTITIES];static jarMPPose_t before[MAX_GENTITIES];
 static jarMPJoint_t links[JAR_MP_ATTACHMENT_LIMIT];static int slots[MAX_GENTITIES];byte seen[MAX_GENTITIES]={0};int count=0,linkCount=0;
 // Box3D applies continuous impulses during the step. A coupled projection
 // at the end of the frame removes residual geometric drift from both paths.
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ConstraintLink(n)){
  jarObject_t*o=&objects[n];int a=o->first-1,b=o->second-1;
  if(a<0||a>ENTITYNUM_WORLD||b<0||b>ENTITYNUM_WORLD)continue;
  if(jointPendingKeys[a]||jointPendingKeys[b]||!ForcePermission(o->owner,a)||!ForcePermission(o->owner,b)||!JointEndpoint(a)||!JointEndpoint(b))continue;
  if(linkCount==JAR_MP_ATTACHMENT_LIMIT)return qfalse;
  if(o->jointKind)JointRecord(n,&links[linkCount]);else RopeRecord(n,rigid&&PhysicsRope(n)&&RopeRigid(n),&links[linkCount],seen);
  ++linkCount;seen[a]=seen[b]=1;
 }
 if(!linkCount){JAR_NativeJointsClear();memset(jointHistory,0,sizeof(jointHistory));return qfalse;}
 for(int n=0;n<=ENTITYNUM_WORLD;++n)if(seen[n]){
  jarMPJointBody_t*b=&bodies[count];memset(b,0,sizeof(*b));slots[count++]=n;b->key=PhysicsIdentity(n);b->pose=JointPose(n);b->movable=n!=ENTITYNUM_WORLD&&!objects[n].frozen&&!objects[n].copyPending&&!blocked[n];b->mass=JAR_ContextMassValid(objects[n].mass)?objects[n].mass:80;
  if(n==ENTITYNUM_WORLD){b->mass=80;VectorSet(b->half,8,8,8);continue;}
  gentity_t*e=&g_entities[n];vec3_t low,high;
  if(rigid&&PhysicsSolid(n))JAR_RigidRead(n,&b->pose,low,high);
  else if(jointHistory[n].identity==b->key&&jointHistory[n].time<level.time&&level.time-jointHistory[n].time<=100){
   const float elapsed=(level.time-jointHistory[n].time)*.001f;
   if(e->client||!b->movable)JAR_AngularVelocity(jointHistory[n].angles,e->r.currentAngles,elapsed,b->pose.angular);
   if(!b->movable&&!blocked[n]&&!objects[n].heldBy){VectorSubtract(e->r.currentOrigin,jointHistory[n].origin,b->pose.linear);VectorScale(b->pose.linear,1/elapsed,b->pose.linear);}
  }
  for(int a=0;a<3;++a)b->half[a]=fmaxf(.25f,fminf(8192,(e->r.maxs[a]-e->r.mins[a])*.5f));
  before[count-1]=b->pose;
  if(advance&&(!rigid||!PhysicsSolid(n))&&PhysicsProp(n)&&b->movable)JAR_AdvanceAngles(b->pose.angles,b->pose.angular,dt,b->pose.angles);
 }
 if(!JAR_NativeJoints(bodies,count,links,linkCount,dt))return qfalse;
 qboolean progress=qfalse;
 for(int i=0;i<count;++i){const int n=slots[i];jarMPJointBody_t*b=&bodies[i];if(!b->movable||n==ENTITYNUM_WORLD)continue;
  gentity_t*e=&g_entities[n];jarMPPose_t*pose=&b->pose;vec3_t low,high;
  const qboolean turned=!VectorCompare(pose->angles,before[i].angles);
  const qboolean relocated=!VectorCompare(pose->position,before[i].position);
  if(!turned&&!relocated&&VectorCompare(pose->linear,before[i].linear)&&VectorCompare(pose->angular,before[i].angular))continue;
  if(turned||relocated){
   if(PhysicsProp(n))PhysicsHull(e,pose->angles,low,high);
   else if(!VehicleBounds(e,pose->angles,low,high)){blocked[n]=1;progress=qtrue;continue;}
   // Welded and hinged peers can overlap. Roped ends still collide with each
   // other, and all other solids still block the swept correction.
   int unlinked[JAR_MP_ATTACHMENT_LIMIT],unlinkedCount=0;
   for(int r=0;r<linkCount;++r)if(links[r].kind!=3){
    const jarObject_t*link=&objects[links[r].id-1];
    int a=link->first-1,c=link->second-1,peer=a==n?c:c==n?a:-1;
    if(peer>=0&&peer<ENTITYNUM_WORLD&&g_entities[peer].r.linked){unlinked[unlinkedCount++]=peer;trap->UnlinkEntity((sharedEntity_t*)&g_entities[peer]);}
   }
   trace_t tr;trap->Trace(&tr,e->r.currentOrigin,low,high,pose->position,n,MASK_PLAYERSOLID,qfalse,0,0);
   for(int p=0;p<unlinkedCount;++p)trap->LinkEntity((sharedEntity_t*)&g_entities[unlinked[p]]);
   if(tr.startsolid||tr.allsolid){blocked[n]=1;progress=qtrue;continue;}
   VectorCopy(tr.endpos,pose->position);
   if(tr.fraction<1){
    blocked[n]=1;
    float speed=DotProduct(pose->linear,tr.plane.normal);
    if(speed<0)VectorMA(pose->linear,-speed,tr.plane.normal,pose->linear);
   }
   progress=qtrue;
  }
  if(rigid&&PhysicsSolid(n)&&!JAR_RigidCorrect(n,pose)){blocked[n]=1;progress=qtrue;continue;}
  const qboolean driven=e->client&&e->m_pVehicle&&VehicleOccupied(e->m_pVehicle);
  if(driven){JAR_VehicleDriftVelocity(&e->client->ps,pose->linear);VectorCopy(e->client->ps.velocity,pose->linear);}
  if(turned||relocated){
   Move(e,pose->position);VectorCopy(pose->angles,e->r.currentAngles);VectorCopy(pose->angles,e->s.angles);VectorCopy(pose->angles,e->s.apos.trBase);e->s.apos.trType=TR_INTERPOLATE;
   VectorCopy(low,e->r.mins);VectorCopy(high,e->r.maxs);
  }
  VectorCopy(pose->linear,objects[n].velocity);VectorCopy(pose->angular,objects[n].angular);VectorCopy(pose->linear,e->s.pos.trDelta);
  if(e->client){if(turned){SetClientViewAngle(e,pose->angles);VehicleOrientation(e);}VectorCopy(pose->linear,e->client->ps.velocity);if(pose->linear[2]>1)e->client->ps.groundEntityNum=ENTITYNUM_NONE;e->s.groundEntityNum=e->client->ps.groundEntityNum;if(e->m_pVehicle&&!driven){vec3_t forward;AngleVectors(pose->angles,forward,NULL,NULL);e->client->ps.speed=DotProduct(pose->linear,forward);}}
  else{PhysicsPublish(e);VectorCopy(pose->angular,e->s.apos.trDelta);}
  trap->LinkEntity((sharedEntity_t*)e);if(driven)VehicleAttachRiders(e);
 }
 for(int i=0;i<count;++i){int n=slots[i];jointHistory[n].identity=PhysicsIdentity(n);jointHistory[n].time=level.time;VectorCopy(g_entities[n].r.currentOrigin,jointHistory[n].origin);VectorCopy(g_entities[n].r.currentAngles,jointHistory[n].angles);}
 return progress;
}
static void JointFrame(float dt,qboolean rigid){
 if(!sandbox.integer||level.intermissiontime||dt<=0)return;
 byte blocked[MAX_GENTITIES]={0};
 RopePrepare();
 // Each round may expose a different corner. RopePrepare shares a single
 // trace budget across all rounds, even when several routes need correction.
 for(int pass=0;pass<16;++pass){
  const qboolean progress=JointProject(dt,rigid,blocked,pass==0);
  if(!progress)break;
  RopePrepare();
  if(!RopeNeedsProjection())return;
 }
 RopeRestoreLength();
}
