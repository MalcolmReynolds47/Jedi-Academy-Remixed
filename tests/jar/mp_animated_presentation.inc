// Test the production adjustment to a native MD3 render entity.
#include "cgame/cg_jar_animated.inc"

static centity_t AnimatedPresentationFixture(int tag,int phase,int frame,int nextFrame){
 centity_t cent={0};
 entityState_t *current=&cent.currentState;
 cent.interpolate=qtrue;current->eType=ET_GENERAL;current->generic1=250;current->bolt2=tag;
 current->number=100;current->modelindex=10;current->time2=701;current->time=phase;current->frame=frame;
 cent.nextState=*current;cent.nextState.frame=nextFrame;
 return cent;
}

static void AnimatedPresentationFrame(const centity_t *cent,int frame,int oldframe,float blend,qboolean frozenShader){
 refEntity_t ent={0};
 ent.frame=ent.oldframe=cent->currentState.frame;
 CG_JAR_AnimatedProp(cent,&ent);
 assert(ent.frame==frame&&ent.oldframe==oldframe&&ent.backlerp==blend);
 assert(ent.shaderTime==(frozenShader?cg.time*.001f:0.0f));
}

static void AnimatedPresentationCases(void){
 cg.time=12345;cg.frameInterpolation=.25f;
 centity_t bomb=AnimatedPresentationFixture(JAR_MP_BOMB_TAG,JAR_MP_BOMB_ARMED,0,0);
 AnimatedPresentationFrame(&bomb,0,0,0,qfalse);
 bomb.nextState.time=JAR_MP_BOMB_DISARMING;bomb.nextState.frame=1;
 AnimatedPresentationFrame(&bomb,0,0,0,qfalse);
 bomb=AnimatedPresentationFixture(JAR_MP_BOMB_TAG,JAR_MP_BOMB_DISARMING,2,3);
 AnimatedPresentationFrame(&bomb,3,2,.75f,qtrue);
 bomb=AnimatedPresentationFixture(JAR_MP_BOMB_TAG,JAR_MP_BOMB_DISARMED,JAR_MP_BOMB_LAST_FRAME,JAR_MP_BOMB_LAST_FRAME);
 AnimatedPresentationFrame(&bomb,JAR_MP_BOMB_LAST_FRAME,JAR_MP_BOMB_LAST_FRAME,0,qtrue);
 cg.time+=800;
 AnimatedPresentationFrame(&bomb,JAR_MP_BOMB_LAST_FRAME,JAR_MP_BOMB_LAST_FRAME,0,qtrue);

 centity_t beacon=AnimatedPresentationFixture(JAR_MP_BEACON_TAG,JAR_MP_BEACON_DEPLOYING,29,JAR_MP_BEACON_DEPLOY_LAST);
 AnimatedPresentationFrame(&beacon,JAR_MP_BEACON_DEPLOY_LAST,29,.75f,qfalse);
 beacon.currentState.frame=JAR_MP_BEACON_DEPLOY_LAST;
 beacon.nextState.time=JAR_MP_BEACON_SIGNALLING;beacon.nextState.frame=JAR_MP_BEACON_LOOP_FIRST;
 AnimatedPresentationFrame(&beacon,JAR_MP_BEACON_DEPLOY_LAST,JAR_MP_BEACON_DEPLOY_LAST,0,qfalse);
 beacon=AnimatedPresentationFixture(JAR_MP_BEACON_TAG,JAR_MP_BEACON_SIGNALLING,JAR_MP_BEACON_LOOP_LAST,JAR_MP_BEACON_LOOP_FIRST);
 AnimatedPresentationFrame(&beacon,JAR_MP_BEACON_LOOP_FIRST,JAR_MP_BEACON_LOOP_LAST,.75f,qfalse);

 // Never blend across absent, reused, hidden or malformed snapshots.
 for(int refusal=0;refusal<10;++refusal){
  beacon=AnimatedPresentationFixture(JAR_MP_BEACON_TAG,JAR_MP_BEACON_SIGNALLING,40,41);
  cg.frameInterpolation=.25f;
  switch(refusal){
   case 0:beacon.interpolate=qfalse;break;
   case 1:++beacon.nextState.time2;break;
   case 2:++beacon.nextState.modelindex;break;
   case 3:++beacon.nextState.number;break;
   case 4:beacon.nextState.bolt2=JAR_MP_BOMB_TAG;break;
   case 5:beacon.nextState.eFlags=EF_NODRAW;break;
   case 6:beacon.nextState.eFlags=EF_SHADER_ANIM;break;
   case 7:beacon.nextState.frame=42;break;
   case 8:beacon.currentState.frame=JAR_MP_BEACON_LOOP_LAST+1;beacon.nextState.frame=beacon.currentState.frame+1;break;
   case 9:cg.frameInterpolation=NAN;break;
  }
  AnimatedPresentationFrame(&beacon,beacon.currentState.frame,beacon.currentState.frame,0,qfalse);
 }
 cg.frameInterpolation=.25f;
 bomb=AnimatedPresentationFixture(JAR_MP_BOMB_TAG,JAR_MP_BOMB_DISARMING,2,3);
 bomb.currentState.generic1=bomb.nextState.generic1=0;
 AnimatedPresentationFrame(&bomb,2,2,0,qfalse);
 centity_t button=AnimatedPresentationFixture(JAR_MP_BUTTON_TAG,JAR_BUTTON_PRESSING,3,4);
 button.nextState.time=JAR_BUTTON_HELD;
 AnimatedPresentationFrame(&button,4,3,.75f,qfalse);
 button=AnimatedPresentationFixture(JAR_MP_BUTTON_TAG,JAR_BUTTON_RETURNING,4,3);
 AnimatedPresentationFrame(&button,3,4,.75f,qfalse);
 button=AnimatedPresentationFixture(JAR_MP_BUTTON_TAG,JAR_BUTTON_RETURNING,1,0);
 button.nextState.time=JAR_BUTTON_IDLE;
 AnimatedPresentationFrame(&button,0,1,.75f,qfalse);
 button.nextState.time2+=1;
 AnimatedPresentationFrame(&button,1,1,0,qfalse);
 bomb.currentState.generic1=bomb.nextState.generic1=250;
 bomb.currentState.bolt2=bomb.nextState.bolt2=0;
 AnimatedPresentationFrame(&bomb,2,2,0,qfalse);
 puts("MP animated MD3 presentation: bomb shader freeze, adjacent frame blend, beacon loop wrap and guarded snapshot boundaries passed.");
}
