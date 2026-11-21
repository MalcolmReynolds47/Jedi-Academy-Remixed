// SP adapter for the shared additive facial controller. GPL-2.0-only.
#include "qcommon/jar_face_controller.h"
extern vmCvar_t jar_facePreset,jar_faceSensitivity;
extern int iCGResetCount;
static jar::FaceState jarFaceStates[MAX_GENTITIES];
static void CG_JAR_ApplyFace(centity_t*cent){
 auto*gent=cent->gent;
 if(!gent||!gent->client||!gent->ghoul2.IsValid()||gent->playerModel<0||gent->playerModel>=gent->ghoul2.size()||gent->s.number<0||gent->s.number>=MAX_GENTITIES)return;
 auto&model=gent->ghoul2[gent->playerModel];
 jar::ApplyFace(model,jarFaceStates[gent->s.number],gent->s.number,cg.time,gent->ghoul2.DiagnosticHandle(),iCGResetCount,
  jar_facePreset.integer,gent->health>0&&gent->faceBone!=BONE_INDEX_INVALID,(jar_facePreset.integer>=1&&jar_facePreset.integer<=3&&gent->health>0&&gent->faceBone!=BONE_INDEX_INVALID&&!(model.mFlags&0x10)?gi.JAR_VoiceEnergy(gent->s.clientNum):0),jar_faceSensitivity.value,
  [&](const char*name){return gi.G2API_GetBoneIndex(&model,name,qtrue);},
  [&](int index,const mdxaBone_t&matrix,int flags,int time){gi.G2API_SetBoneAnglesMatrixIndex(&model,index,matrix,flags,nullptr,0,time);},
  [&](int index){gi.G2API_StopBoneAnglesIndex(&model,index);});
}
