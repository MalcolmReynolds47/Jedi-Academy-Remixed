// A portable model cannot use the inline BSP mover hull. Move its deck and
// supported cargo through the same swept transaction as the portable lift.
static void BobbingUse(gentity_t* deck,gentity_t* other,gentity_t* activator){
 const int n=deck->s.number;
 if(!sandbox.integer||(deck->flags&FL_INACTIVE)||objects[n].copyPending||objects[n].heldBy)return;
 if(activator&&other==activator&&activator->client&&activator->s.number<MAX_CLIENTS&&!Edit(activator,n))return;
 deck->genericValue1=!deck->genericValue1;
 deck->timestamp=level.time;
 ++objects[n].propertyRevision;
 SelectionChanged(n);
}

static void BobbingInit(gentity_t* deck,const jarControllerState_t* saved){
 deck->speed=saved?saved->speed:JAR_BOB_AMPLITUDE;
 deck->delay=saved?saved->waitMs:JAR_BOB_PERIOD;
 deck->genericValue1=saved?saved->active:1;
 // Floor placement is the low point so the default cycle can run on a floor.
 deck->genericValue2=saved?saved->pushSpeed:JAR_BOB_BOTTOM;
 deck->timestamp=level.time;
 deck->s.bolt2=JAR_MP_BOBBING_TAG;
 deck->r.svFlags|=SVF_PLAYER_USABLE;
 deck->use=BobbingUse;
}

static void BobbingState(const gentity_t* deck,jarControllerState_t* state){
 state->speed=(int)deck->speed;
 state->waitMs=deck->delay;
 state->active=deck->genericValue1;
 state->pushSpeed=deck->genericValue2;
 state->repeats=deck->genericValue3;
 state->inactive=!!(deck->flags&FL_INACTIVE);
}

static qboolean BobbingApply(gentity_t* deck,const jarControllerState_t* state){
 if(!JAR_ControllerStateValid(JAR_BOBBING_CONTROLLER,state))return qfalse;
 if(deck->speed!=state->speed)deck->genericValue5=deck->genericValue6=0;
 if(deck->delay!=state->waitMs)deck->genericValue7=0;
 deck->speed=(float)state->speed;
 deck->delay=state->waitMs;
 deck->genericValue1=state->active;
 if(state->inactive)deck->flags|=FL_INACTIVE;else deck->flags&=~FL_INACTIVE;
 return qtrue;
}

static void BobbingStatus(gentity_t* deck,int status){
 if(deck->genericValue3==status)return;
 deck->genericValue3=status;
 SelectionChanged(deck->s.number);
}

static void BobbingWorldTurn(gentity_t* deck,float travel){
 if(deck->genericValue6)return; // Stay blocked if the reverse leg also fails.
 vec3_t destination;
 VectorCopy(deck->r.currentOrigin,destination);
 destination[2]+=travel;
 trace_t hit;
 trap->Trace(&hit,deck->r.currentOrigin,deck->r.mins,deck->r.maxs,destination,deck->s.number,
  MASK_PLAYERSOLID|CONTENTS_CORPSE,qfalse,0,0);
 if(hit.startsolid||hit.allsolid||hit.fraction>=1||hit.entityNum!=ENTITYNUM_WORLD||
    (travel<0?hit.plane.normal[2]<.7f:hit.plane.normal[2]>-.7f))return;

 // A floor or ceiling clips the cycle. Rebase at the turn and let the
 // same group sweep check the reverse leg on the next tick.
 deck->genericValue2=travel<0?JAR_BOB_BOTTOM:JAR_BOB_TOP;
 deck->genericValue5=0;
 deck->genericValue6=1;
}

static qboolean BobbingStep(gentity_t* deck,int phase){
 const float height=JAR_BobHeight((int)deck->speed,phase,deck->genericValue4,deck->pos1[2]);
 const float travel=height-deck->r.currentOrigin[2];
 if(travel&&!LiftTranslate(deck,travel)){
  BobbingWorldTurn(deck,travel);
  return qfalse;
 }
 if(travel)deck->genericValue6=0;
 deck->pos2[2]=deck->r.currentOrigin[2];
 deck->genericValue2=phase;
 return qtrue;
}

static void BobbingFrame(gentity_t* deck){
 const jarObject_t* object=&objects[deck->s.number];
 const int elapsed=Com_Clampi(0,100,level.time-deck->timestamp);
 deck->timestamp=level.time;
 if(!sandbox.integer||(deck->flags&FL_INACTIVE)||object->copyPending||object->heldBy||
    !object->frozen||!deck->genericValue1){
  BobbingStatus(deck,JAR_BOB_STOPPED);
  return;
 }
 if(!elapsed)return;

 const int previous=deck->genericValue2;
 const long long accumulated=(long long)elapsed*JAR_BOB_CYCLE+deck->genericValue7;
 int advance=(int)(accumulated/deck->delay);
 deck->genericValue7=(int)(accumulated%deck->delay);
 // One complete sweep covers the same path as repeated sub-frame cycles.
 if(advance>JAR_BOB_CYCLE)advance=JAR_BOB_CYCLE+advance%JAR_BOB_CYCLE;
 // Use a fixed reference to avoid accumulating position error. Rebase on
 // explicit motion or amplitude edits without jumping the deck or riders.
 const qboolean moved=deck->pos2[2]!=deck->r.currentOrigin[2];
 if(moved)deck->genericValue6=0;
 if(!deck->genericValue5||moved){
  deck->pos1[2]=deck->pos2[2]=deck->r.currentOrigin[2];
  deck->genericValue4=previous;
  deck->genericValue5=1;
 }
 while(advance>0){
  const int phase=deck->genericValue2;
  const int step=Com_Clampi(0,advance,JAR_BobNextTurn(phase)-phase);
  if(!BobbingStep(deck,(phase+step)%JAR_BOB_CYCLE)){
   BobbingStatus(deck,JAR_BOB_BLOCKED);
   return;
  }
  advance-=step;
 }
 BobbingStatus(deck,JAR_BOB_MOVING);
}
