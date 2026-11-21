#include "cgame/cg_jar_door_camera.inc"

static centity_t DoorCameraShaftFixture(void){
 centity_t shaft={0};
 entityState_t *state=&shaft.currentState;
 state->number=103;state->eType=ET_GENERAL;state->generic1=JAR_MP_DOOR_CAMERA_PART_TAG;
 state->bolt2=JAR_MP_DOOR_CAMERA_SHAFT;state->modelindex=9;state->otherEntityNum=100;
 state->time2=701;state->speed=2;
 shaft.nextState=*state;shaft.nextState.speed=6;shaft.interpolate=qtrue;
 return shaft;
}

static void DoorCameraScaleCheck(const centity_t *shaft,qboolean valid,float scale){
 refEntity_t ent={0};VectorSet(ent.modelScale,2,3,4);
 assert(CG_JAR_DoorCameraScale(shaft,&ent)==valid);
 assert(ent.modelScale[0]==(valid?1:2)&&ent.modelScale[1]==(valid?1:3)&&ent.modelScale[2]==(valid?scale:4));
}

static void DoorCameraPresentationCases(void){
 centity_t shaft=DoorCameraShaftFixture();
 cg.frameInterpolation=.25f;DoorCameraScaleCheck(&shaft,qtrue,3);
 cg.frameInterpolation=1.25f;DoorCameraScaleCheck(&shaft,qtrue,6);
 cg.frameInterpolation=-.1f;DoorCameraScaleCheck(&shaft,qtrue,2);
 cg.frameInterpolation=NAN;DoorCameraScaleCheck(&shaft,qtrue,2);
 cg.frameInterpolation=.5f;
 for(int mismatch=0;mismatch<9;++mismatch){
  shaft=DoorCameraShaftFixture();
  switch(mismatch){
   case 0:shaft.interpolate=qfalse;break;
   case 1:++shaft.nextState.time2;break;
   case 2:++shaft.nextState.modelindex;break;
   case 3:++shaft.nextState.otherEntityNum;break;
   case 4:++shaft.nextState.number;break;
   case 5:shaft.nextState.bolt2=JAR_MP_DOOR_CAMERA_HEAD;break;
   case 6:shaft.nextState.eFlags=EF_NODRAW;break;
   case 7:shaft.nextState.speed=INFINITY;break;
   case 8:shaft.nextState.speed=JAR_MP_DOOR_CAMERA_MAX_SCALE+1;break;
  }
  DoorCameraScaleCheck(&shaft,qtrue,2);
 }
 shaft=DoorCameraShaftFixture();shaft.interpolate=qfalse;
 shaft.currentState.speed=JAR_MP_DOOR_CAMERA_MIN_SCALE;DoorCameraScaleCheck(&shaft,qtrue,JAR_MP_DOOR_CAMERA_MIN_SCALE);
 shaft.currentState.speed=JAR_MP_DOOR_CAMERA_MAX_SCALE;DoorCameraScaleCheck(&shaft,qtrue,JAR_MP_DOOR_CAMERA_MAX_SCALE);
 shaft.currentState.speed=0;DoorCameraScaleCheck(&shaft,qfalse,0);
 shaft.currentState.speed=NAN;DoorCameraScaleCheck(&shaft,qfalse,0);
 shaft.currentState.speed=2;shaft.currentState.bolt2=JAR_MP_DOOR_CAMERA_HEAD;DoorCameraScaleCheck(&shaft,qfalse,0);
 shaft.currentState.bolt2=JAR_MP_DOOR_CAMERA_SHAFT;shaft.currentState.generic1=250;DoorCameraScaleCheck(&shaft,qfalse,0);
 shaft.currentState.generic1=JAR_MP_DOOR_CAMERA_PART_TAG;shaft.currentState.modelGhoul2=1;DoorCameraScaleCheck(&shaft,qfalse,0);
 puts("MP door camera shaft: bounded nonuniform scale, head isolation and lifetime-safe snapshot blending passed.");
}
