// Server owns the stock switch animation and fires links at its two endpoints.
static jarButtonState_t ButtonRead(const gentity_t* e){
 const jarButtonState_t state={e->delay,e->genericValue11,e->s.frame,e->genericValue8};
 return state;
}
static void ButtonWrite(gentity_t* e,const jarButtonState_t* state){
 e->delay=state->waitMs;e->genericValue11=e->s.time=state->phase;
 e->s.frame=state->frame;e->genericValue8=state->remaining;e->timestamp=level.time;
}
static qboolean ButtonReady(const gentity_t* e){
 const jarObject_t* o=&objects[e->s.number];
 return sandbox.integer&&!(e->flags&FL_INACTIVE)&&!o->heldBy&&!o->copyPending;
}
static void ButtonUse(gentity_t* e,gentity_t* other,gentity_t* activator){
 if(!ButtonReady(e)||e->genericValue11!=JAR_BUTTON_IDLE)return;
 if(other==activator&&activator&&activator->client){
  if(activator->s.number<MAX_CLIENTS){
   if(!Edit(activator,e->s.number)||(!controllerCalling[e->s.number]&&!Alive(activator)))return;
  }else if(!ActionActivator(e,activator))return;
 }
 jarButtonState_t state=ButtonRead(e);
 state.phase=JAR_BUTTON_PRESSING;state.remaining=JAR_BUTTON_TICK;
 ControllerCaptureActivator(e,activator);
 ButtonWrite(e,&state);
 if(e->sound1to2)G_Sound(e,CHAN_BODY,e->sound1to2);
 SelectionChanged(e->s.number);
}
static qboolean ButtonInit(gentity_t* e){
 if(!MediaValidate(9,JAR_BUTTON_PRESS_SOUND)||!MediaValidate(9,JAR_BUTTON_RETURN_SOUND))return qfalse;
 e->sound1to2=G_SoundIndex(JAR_BUTTON_PRESS_SOUND);e->sound2to1=G_SoundIndex(JAR_BUTTON_RETURN_SOUND);
 if(!e->sound1to2||!e->sound2to1)return qfalse;
 e->s.bolt2=JAR_MP_BUTTON_TAG;e->use=ButtonUse;e->r.svFlags|=SVF_PLAYER_USABLE;
 e->delay=JAR_BUTTON_WAIT;e->timestamp=level.time;
 return qtrue;
}
static void ButtonState(const gentity_t* e,jarControllerState_t* s){
 s->waitMs=e->delay;s->active=e->genericValue11;s->inactive=!!(e->flags&FL_INACTIVE);
}
static qboolean ButtonApply(gentity_t* e,const jarControllerState_t* s){
 if(!JAR_ControllerStateValid(JAR_BUTTON_CONTROLLER,s))return qfalse;
 jarButtonState_t state=ButtonRead(e);
 state.waitMs=s->waitMs;
 if(state.phase==JAR_BUTTON_HELD){
  if(!state.waitMs){state.phase=JAR_BUTTON_RETURNING;state.remaining=JAR_BUTTON_TICK;}
  else state.remaining=Com_Clampi(1,state.waitMs,state.remaining);
 }
 ButtonWrite(e,&state);
 if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
 return qtrue;
}
static void ButtonTick(gentity_t* e){
 const int elapsed=Com_Clampi(0,100,level.time-e->timestamp);
 e->timestamp=level.time;
 if(e->genericValue11==JAR_BUTTON_IDLE||!ButtonReady(e))return;
 jarButtonState_t state=ButtonRead(e);
 const int previous=state.phase;
 const int events=JAR_ButtonAdvance(&state,elapsed);
 ButtonWrite(e,&state);
 if(state.phase!=previous)SelectionChanged(e->s.number);
 if((events&JAR_BUTTON_RETURN)&&e->sound2to1)G_Sound(e,CHAN_BODY,e->sound2to1);
 if(events&(JAR_BUTTON_PRESSED|JAR_BUTTON_RELEASED)){
  gentity_t* actor=ControllerActivator(e);
  if(events&JAR_BUTTON_RELEASED)ControllerCaptureActivator(e,NULL);
  JAR_MP_UseTargets(e,actor,events&JAR_BUTTON_PRESSED?"@jar_primary":"@jar_secondary");
 }
}
