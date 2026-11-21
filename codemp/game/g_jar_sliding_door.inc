// Model-backed doors and lifts preserve their path relative to the current pose.
#include "g_jar_lift.inc"
static void SlidingDoorUse(gentity_t*door,gentity_t*other,gentity_t*activator);

static void SlidingDoorInit(gentity_t*door,const jarControllerState_t*saved,qboolean lift){
 door->speed=saved?saved->speed:JAR_SLIDE_SPEED;
 door->wait=saved?saved->waitMs:lift?JAR_LIFT_WAIT:0;
 door->genericValue1=saved?saved->active:0;
 door->genericValue2=saved?saved->flags:JAR_SLIDE_UP;
 door->genericValue3=saved?saved->damage:lift?JAR_LIFT_HEIGHT:JAR_SLIDE_TRAVEL;
 door->genericValue6=saved?saved->pushSpeed:0;
 door->timestamp=level.time;
 door->s.bolt2=lift?JAR_MP_LIFT_TAG:JAR_MP_SLIDING_DOOR_TAG;
 door->sound1to2=G_SoundIndex(lift?JAR_LIFT_START_SOUND:JAR_DOOR_START_SOUND);
 door->soundLoop=G_SoundIndex(lift?JAR_LIFT_MOVE_SOUND:JAR_DOOR_MOVE_SOUND);
 door->soundPos2=G_SoundIndex(lift?JAR_LIFT_STOP_SOUND:JAR_DOOR_STOP_SOUND);
 door->r.svFlags|=SVF_PLAYER_USABLE;
 door->use=SlidingDoorUse;
}

static void SlidingDoorState(const gentity_t*door,jarControllerState_t*s){
 s->speed=(int)door->speed;s->waitMs=(int)door->wait;
 s->active=door->genericValue1;s->flags=door->genericValue2;s->damage=door->genericValue3;
 s->pushSpeed=door->genericValue6;s->repeats=door->genericValue5;s->inactive=!!(door->flags&FL_INACTIVE);
}

static qboolean SlidingDoorApply(gentity_t*door,const jarControllerState_t*s){
 jarControllerState_t settings=*s;
 settings.pushSpeed=door->genericValue6;
 const int kind=door->s.bolt2==JAR_MP_LIFT_TAG?JAR_LIFT_CONTROLLER:JAR_SLIDING_DOOR_CONTROLLER;
 if(!JAR_ControllerStateValid(kind,&settings)||
    (settings.pushSpeed&&door->genericValue2!=settings.flags))return qfalse;

 if(!settings.active||settings.pushSpeed!=settings.damage*JAR_SLIDE_MILLI)door->genericValue4=0;
 else if(door->wait!=settings.waitMs)door->genericValue4=settings.waitMs;
 door->speed=(float)settings.speed;door->wait=(float)settings.waitMs;
 door->genericValue1=settings.active;door->genericValue2=settings.flags;door->genericValue3=settings.damage;
 if(settings.inactive)door->flags|=FL_INACTIVE;else door->flags&=~FL_INACTIVE;
 return qtrue;
}

static void SlidingDoorAxis(const gentity_t*door,vec3_t direction){
 vec3_t right,up;
 AngleVectors(door->r.currentAngles,NULL,right,up);
 const int axis=door->genericValue2;
 if(axis==JAR_SLIDE_LEFT||axis==JAR_SLIDE_RIGHT)VectorScale(right,axis==JAR_SLIDE_LEFT?-1:1,direction);
 else VectorScale(up,axis==JAR_SLIDE_DOWN?-1:1,direction);
}

static void SlidingDoorUse(gentity_t*door,gentity_t*other,gentity_t*activator){
 const int n=door->s.number;
 jarObject_t*o=&objects[n];
 if(!sandbox.integer||(door->flags&FL_INACTIVE)||o->copyPending||o->heldBy)return;
 if(activator&&other==activator&&activator->client&&activator->s.number<MAX_CLIENTS&&!Edit(activator,n))return;
 if(!o->frozen){
  o->frozen=1;
  VectorClear(o->velocity);VectorClear(o->angular);
  JAR_RigidForget(n);
 }
 ControllerCaptureActivator(door,activator);
 door->genericValue1=!door->genericValue1;
 door->genericValue4=0;
 door->timestamp=level.time;
 ++o->propertyRevision;
 SelectionChanged(n);
}

static void SlidingDoorStatus(gentity_t*door,int status){
 if(door->genericValue5==status)return;
 door->genericValue5=status;
 SelectionChanged(door->s.number);
}

static void SlidingDoorFrame(gentity_t*door){
 const int n=door->s.number;
 const jarObject_t*o=&objects[n];
 const int elapsed=Com_Clampi(0,100,level.time-door->timestamp);
 door->timestamp=level.time;
 if(!sandbox.integer||(door->flags&FL_INACTIVE)||o->copyPending||o->heldBy||!o->frozen){
  door->s.loopSound=0;
  SlidingDoorStatus(door,JAR_SLIDE_PAUSED);
  return;
 }
 if(!elapsed)return;
 if(door->genericValue4>0){
  door->genericValue4=Com_Clampi(0,door->genericValue4,door->genericValue4-elapsed);
  if(door->genericValue4)return;
  door->genericValue1=0;
  ++objects[n].propertyRevision;
  SelectionChanged(n);
 }
 const int target=door->genericValue1?door->genericValue3*JAR_SLIDE_MILLI:0;
 const int remaining=target-door->genericValue6;
 if(!remaining){door->s.loopSound=0;SlidingDoorStatus(door,JAR_SLIDE_REST);return;}
 const int limit=(int)door->speed*elapsed;
 const int step=Com_Clampi(-limit,limit,remaining);
 qboolean blocked=qfalse;int advance=step;
 if(door->s.bolt2==JAR_MP_LIFT_TAG){
  blocked=!LiftTranslate(door,(float)step/JAR_SLIDE_MILLI);
  if(blocked)advance=0;
 }else{
  vec3_t direction,next;
  SlidingDoorAxis(door,direction);
  VectorMA(door->r.currentOrigin,(float)step/JAR_SLIDE_MILLI,direction,next);
  trace_t trace;
  trap->Trace(&trace,door->r.currentOrigin,door->r.mins,door->r.maxs,next,n,MASK_PLAYERSOLID|CONTENTS_CORPSE,qfalse,0,0);
  blocked=trace.startsolid||trace.allsolid||trace.fraction<1;
  advance=trace.startsolid||trace.allsolid?0:blocked?(int)(step*Com_Clamp(0,1,trace.fraction)):step;
  if(advance){
   VectorMA(door->r.currentOrigin,(float)advance/JAR_SLIDE_MILLI,direction,next);
   Move(door,next);
   if(JAR_RigidActive()){const jarMPPose_t pose=PhysicsPose(n);JAR_RigidCorrect(n,&pose);}
  }
 }
 if(advance){
  door->genericValue6+=advance;
  if(!door->s.loopSound&&door->sound1to2)G_Sound(door,CHAN_BODY,door->sound1to2);
 }
 if(door->genericValue6==target){
  door->s.loopSound=0;
  if(door->soundPos2)G_Sound(door,CHAN_BODY,door->soundPos2);
  door->genericValue4=door->genericValue1?(int)door->wait:0;
  door->genericValue5=JAR_SLIDE_REST;
  SelectionChanged(n);
  JAR_MP_UseTargets(door,ControllerActivator(door),door->genericValue1?"@jar_primary":"@jar_secondary");
  return;
 }
 door->s.loopSound=blocked?0:door->soundLoop;
 SlidingDoorStatus(door,blocked?JAR_SLIDE_BLOCKED:JAR_SLIDE_MOVING);
}
