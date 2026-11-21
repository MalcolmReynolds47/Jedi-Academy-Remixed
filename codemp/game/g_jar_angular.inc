// Portable angular movers rotate the model and supported bodies together.
static void AngularUse(gentity_t* deck,gentity_t* other,gentity_t* activator){
 const int n=deck->s.number;
 if(!sandbox.integer||(deck->flags&FL_INACTIVE)||objects[n].copyPending||objects[n].heldBy)return;
 if(activator&&other==activator&&activator->client&&activator->s.number<MAX_CLIENTS&&!Edit(activator,n))return;
 deck->genericValue1=!deck->genericValue1;
 deck->timestamp=level.time;
 ++objects[n].propertyRevision;
 SelectionChanged(n);
}

static void AngularInit(gentity_t* deck,int kind,const jarControllerState_t* saved){
 const qboolean pendulum=kind==JAR_PENDULUM_CONTROLLER;
 deck->speed=saved?saved->speed:pendulum?JAR_PENDULUM_SWING:JAR_SPINNER_RATE;
 deck->delay=saved?saved->waitMs:JAR_ANGULAR_PERIOD;
 deck->genericValue1=saved?saved->active:pendulum;
 deck->genericValue2=saved?saved->flags:pendulum?JAR_ANGULAR_FORWARD:JAR_ANGULAR_UP;
 deck->genericValue4=saved?saved->pushSpeed:0;
 deck->timestamp=level.time;
 deck->s.bolt2=JAR_MP_ANGULAR_TAG;
 deck->r.svFlags|=SVF_PLAYER_USABLE;
 deck->use=AngularUse;
}

static void AngularState(const gentity_t* deck,jarControllerState_t* state){
 state->speed=(int)deck->speed;
 state->waitMs=deck->delay;
 state->active=deck->genericValue1;
 state->flags=deck->genericValue2;
 state->repeats=deck->genericValue3;
 state->pushSpeed=deck->genericValue4;
 state->inactive=!!(deck->flags&FL_INACTIVE);
}

static qboolean AngularApply(gentity_t* deck,int kind,const jarControllerState_t* state){
 if(!JAR_ControllerStateValid(kind,state))return qfalse;
 if(deck->delay!=state->waitMs)deck->genericValue4=(int)((long long)deck->genericValue4*state->waitMs/deck->delay);
 deck->speed=(float)state->speed;
 deck->delay=state->waitMs;
 deck->genericValue1=state->active;
 deck->genericValue2=state->flags;
 if(state->inactive)deck->flags|=FL_INACTIVE;else deck->flags&=~FL_INACTIVE;
 return qtrue;
}

static void AngularStatus(gentity_t* deck,int status){
 if(deck->genericValue3==status)return;
 deck->genericValue3=status;
 SelectionChanged(deck->s.number);
}

static void AngularTransform(const vec3_t value,const vec3_t before[3],const vec3_t after[3],vec3_t result){
 vec3_t rotated={0,0,0};
 for(int i=0;i<3;++i)VectorMA(rotated,DotProduct(value,before[i]),after[i],rotated);
 VectorCopy(rotated,result);
}

static qboolean AngularSweep(const jarLiftBody_t* from,const jarLiftBody_t* to,const vec3_t padding){
 vec3_t start,end,low,high;
 for(int axis=0;axis<3;++axis){
  start[axis]=from->origin[axis]+(from->mins[axis]+from->maxs[axis])*.5f;
  end[axis]=to->origin[axis]+(to->mins[axis]+to->maxs[axis])*.5f;
  high[axis]=fmaxf(from->maxs[axis]-from->mins[axis],to->maxs[axis]-to->mins[axis])*.5f+padding[axis];
  low[axis]=-high[axis];
 }
 trace_t trace;
 trap->Trace(&trace,start,low,high,end,from->entity,MASK_PLAYERSOLID|CONTENTS_CORPSE,qfalse,0,0);
 return !trace.startsolid&&!trace.allsolid&&trace.fraction>=1;
}

static qboolean AngularSeparated(const jarLiftBody_t* a,const jarLiftBody_t* b){
 for(int axis=0;axis<3;++axis){
  const float low=fmaxf(a->origin[axis]+a->mins[axis],b->origin[axis]+b->mins[axis]);
  const float high=fminf(a->origin[axis]+a->maxs[axis],b->origin[axis]+b->maxs[axis]);
  if(high-low<=.125f)return qtrue;
 }
 return qfalse;
}

static qboolean AngularRotate(gentity_t* deck,const vec3_t target){
 jarLiftBody_t before[JAR_LIFT_MAX_RIDERS+1],after[JAR_LIFT_MAX_RIDERS+1];
 vec3_t angles[JAR_LIFT_MAX_RIDERS+1];
 float yaw[JAR_LIFT_MAX_RIDERS+1]={0};
 int vehicleGroup[JAR_LIFT_MAX_RIDERS+1]={0};
 const int count=LiftCollect(deck,before,vehicleGroup);
 if(!count)return qfalse;

 vec3_t oldAxes[3],newAxes[3],rotation,axis,arc;
 AnglesToAxis(deck->r.currentAngles,oldAxes);
 AnglesToAxis(target,newAxes);
 JAR_AngularVelocity(deck->r.currentAngles,target,1,rotation);
 VectorCopy(rotation,axis);
 const float sagitta=1-cosf(VectorNormalize(axis)*.5f);
 for(int i=0;i<3;++i)arc[i]=sagitta*sqrtf(fmaxf(0,1-axis[i]*axis[i]));

 after[0]=before[0];
 VectorCopy(target,angles[0]);
 PhysicsHull(deck,target,after[0].mins,after[0].maxs);
 for(int i=1;i<count;++i){
  gentity_t* member=&g_entities[before[i].entity];
  const int vehicle=vehicleGroup[i];
  after[i]=before[i];
  VectorCopy(member->r.currentAngles,angles[i]);
  if(vehicle&&vehicle!=i){
   vec3_t offset,turned,turn={0,yaw[vehicle],0},basis[3];
   AnglesToAxis(turn,basis);
   VectorSubtract(before[i].origin,before[vehicle].origin,offset);
   VectorScale(basis[0],offset[0],turned);
   VectorMA(turned,offset[1],basis[1],turned);
   VectorMA(turned,offset[2],basis[2],turned);
   VectorAdd(after[vehicle].origin,turned,after[i].origin);
   yaw[i]=yaw[vehicle];
  }else{
   vec3_t offset;
   VectorSubtract(before[i].origin,before[0].origin,offset);
   AngularTransform(offset,oldAxes,newAxes,offset);
   VectorAdd(after[0].origin,offset,after[i].origin);
   if(PhysicsSolid(member->s.number)){
    JAR_AdvanceAngles(member->r.currentAngles,rotation,1,angles[i]);
    PhysicsHull(member,angles[i],after[i].mins,after[i].maxs);
   }else if(member->client){
    vec3_t view={0,member->client->ps.viewangles[YAW],0},heading,turned;
    AngleVectors(view,heading,NULL,NULL);
    AngularTransform(heading,oldAxes,newAxes,turned);
    if(hypotf(turned[0],turned[1])>1e-4f)yaw[i]=AngleSubtract(atan2f(turned[1],turned[0])*(180.f/M_PI),view[YAW]);
    angles[i][YAW]=AngleNormalize180(angles[i][YAW]+yaw[i]);
    if(member->m_pVehicle&&!VehicleBounds(member,angles[i],after[i].mins,after[i].maxs))return qfalse;
   }
   after[i].origin[2]=after[0].origin[2]+after[0].maxs[2]-after[i].mins[2]+fmaxf(.125f,JAR_LiftGap(&before[0],&before[i]));
  }
  for(int a=0;a<3;++a)if(!(fabsf(after[i].origin[a])<=65535))return qfalse;
  if(member->r.contents&(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE))for(int j=1;j<i;++j){
   if(vehicle&&vehicle==vehicleGroup[j])continue;
   if((g_entities[before[j].entity].r.contents&(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE))&&
      !AngularSeparated(&after[i],&after[j]))return qfalse;
  }
 }

 jarMPRigidPoseUpdate_t updates[JAR_LIFT_MAX_RIDERS+1];
 int updateCount=0;
 for(int i=0;i<count;++i)if(before[i].linked)LiftLinked(before[i].entity,0);
 qboolean clear=qtrue;
 for(int i=0;i<count;++i){
  vec3_t pad;
  const float radius=Distance(before[i].origin,before[0].origin)+RadiusFromBounds(before[i].mins,before[i].maxs);
  VectorScale(arc,radius,pad);
  if(!AngularSweep(&before[i],&after[i],pad)){clear=qfalse;break;}
  const int n=before[i].entity;
  vec3_t unusedMin,unusedMax;
  jarMPPose_t pose;
  if(PhysicsSolid(n)&&JAR_RigidRead(n,&pose,unusedMin,unusedMax)){
   VectorCopy(after[i].origin,pose.position);
   VectorCopy(angles[i],pose.angles);
   updates[updateCount++]=(jarMPRigidPoseUpdate_t){(unsigned)n,pose,objects[n].frozen};
  }
 }
 if(clear)clear=JAR_RigidCorrectPoses(updates,updateCount)?qtrue:qfalse;
 if(clear)for(int i=0;i<count;++i){
  gentity_t* member=&g_entities[before[i].entity];
  LiftPlaceNative(member->s.number,after[i].origin,deck->s.number);
  VectorCopy(after[i].mins,member->r.mins);
  VectorCopy(after[i].maxs,member->r.maxs);
  if(member->client){
   member->client->ps.delta_angles[YAW]+=ANGLE2SHORT(yaw[i]);
   member->client->ps.viewangles[YAW]=AngleNormalize180(member->client->ps.viewangles[YAW]+yaw[i]);
  }
  if(!member->client||member->m_pVehicle){
   VectorCopy(angles[i],member->r.currentAngles);
   VectorCopy(angles[i],member->s.apos.trBase);
   VectorCopy(angles[i],member->s.angles);
   member->s.apos.trType=TR_INTERPOLATE;
  }
  if(member->m_pVehicle)VehicleOrientation(member);
  if(PhysicsProp(member->s.number))PhysicsPublish(member);
 }
 for(int i=0;i<count;++i)if(before[i].linked)LiftLinked(before[i].entity,1);
 return clear;
}

static void AngularFrame(gentity_t* deck){
 const jarObject_t* object=&objects[deck->s.number];
 int remaining=Com_Clampi(0,100,level.time-deck->timestamp);
 deck->timestamp=level.time;
 if(!object->heldBy&&object->frozen){
  deck->s.pos.trTime=deck->s.apos.trTime=level.time;
  VectorClear(deck->s.apos.trDelta);
 }
 if(!sandbox.integer||(deck->flags&FL_INACTIVE)||object->copyPending||object->heldBy||!object->frozen||!deck->genericValue1){
  AngularStatus(deck,JAR_ANGULAR_STOPPED);
  return;
 }
 const qboolean pendulum=ControllerKind(deck->s.number)==JAR_PENDULUM_CONTROLLER;
 const double maximum=pendulum?deck->speed*6283.185307179586/deck->delay:fabs(deck->speed);
 const int stepLimit=maximum>80?Com_Clampi(1,100,(int)(8000/maximum)):100;
 vec3_t initialAngles;
 VectorCopy(deck->r.currentAngles,initialAngles);
 const int elapsed=remaining;
 while(remaining>0){
  const int step=remaining<stepLimit?remaining:stepLimit;
  int phase=deck->genericValue4;
  const float turn=JAR_AngularStep(pendulum,(int)deck->speed,deck->delay,step,&phase);
  if(turn){
   vec3_t axes[3],spin,target;
   AnglesToAxis(deck->r.currentAngles,axes);
   const int axis=deck->genericValue2==JAR_ANGULAR_UP?2:deck->genericValue2==JAR_ANGULAR_FORWARD?0:1;
   VectorScale(axes[axis],turn*(M_PI/180.f),spin);
   JAR_AdvanceAngles(deck->r.currentAngles,spin,1,target);
   if(!AngularRotate(deck,target)){AngularStatus(deck,JAR_ANGULAR_BLOCKED);return;}
  }
  deck->genericValue4=phase;
  remaining-=step;
 }
 if(elapsed)for(int i=0;i<3;++i)deck->s.apos.trDelta[i]=AngleSubtract(deck->r.currentAngles[i],initialAngles[i])*1000.f/elapsed;
 AngularStatus(deck,JAR_ANGULAR_MOVING);
}
