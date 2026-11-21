// Exercise production client pose and muzzle code against controlled Ghoul2.
#include "ghoul2/G2.h"
#include "game/jar_mp_emplaced.h"
#include "cgame/cg_jar_emplaced.inc"

centity_t cg_entities[MAX_GENTITIES];
static int chairHandle, chairModels, chairBolts, chairPoses, chairRecoils, chairMatrices, chairEffects, chairCleans;
static qboolean chairMissingBolt, chairBlockedMatrix;
static vec3_t chairPose, chairBase, chairOrigin;
static int chairMuzzle;
static int ewebHandle, ewebModels, ewebBoneUpdates, ewebAnimations, ewebSurfaces, ewebBolts, ewebMatrices, ewebCleans;
static qboolean ewebMatrixBlocked;

const char *CG_ConfigString(int index){
 assert(index==CS_MODELS+4||index==CS_MODELS+5);
 return index==CS_MODELS+4?"models/map_objects/mp/turret_chair.glm":"models/map_objects/hoth/eweb_model.glm";
}
int BG_EmplacedView(vec3_t aim,vec3_t base,float *yaw,float constraint){
 assert(constraint==60);
 const float relative=AngleSubtract(aim[YAW],base[YAW]);
 if(fabsf(relative)<=constraint)return 0;
 *yaw=base[YAW]+Com_Clamp(-constraint,constraint,relative);
 return 2;
}
void BG_GiveMeVectorFromMatrix(mdxaBone_t *matrix,int axis,vec3_t out){
 assert(axis==ORIGIN||axis==NEGATIVE_X||axis==NEGATIVE_Y);
 for(int i=0;i<3;++i)out[i]=axis==ORIGIN?matrix->matrix[i][3]:-matrix->matrix[i][axis==NEGATIVE_Y?1:0];
}
void CG_G2ServerBoneAngles(centity_t *cent){
 assert(cent->ghoul2==&ewebHandle&&cent->currentState.boneIndex1==12&&cent->currentState.boneIndex2==13);
 assert(cent->currentState.boneAngles1[0]==15&&cent->currentState.boneAngles2[2]==20);
 ++ewebBoneUpdates;
}

static int ChairInit(void **model,const char *path,int index,qhandle_t skin,qhandle_t shader,int flags,int lod){
 assert(!*model&&!strcmp(path,"models/map_objects/mp/turret_chair.glm")&&!index&&!skin&&!shader&&!flags&&!lod);
 *model=&chairHandle;++chairModels;return 0;
}
static void ChairClean(void **model){
 assert(*model==&chairHandle);
 *model=NULL;++chairCleans;
}
static int ChairBolt(void *model,int index,const char *tag){
 assert(model==&chairHandle&&!index);
 ++chairBolts;
 if(chairMissingBolt)return -1;
 assert(!strcmp(tag,"*flash01")||!strcmp(tag,"*flash02"));
 return !strcmp(tag,"*flash02");
}
static qboolean ChairAim(void *model,int index,const char *bone,const vec3_t angles,int flags,int up,int right,int forward,
                         qhandle_t *models,int blend,int time){
 assert(model==&chairHandle&&!index&&!strcmp(bone,"Bone02")&&flags==BONE_ANGLES_REPLACE);
 assert(up==NEGATIVE_Y&&right==NEGATIVE_X&&forward==POSITIVE_Z&&!models&&!blend&&time==cg.time);
 ++chairPoses;VectorCopy(angles,chairPose);return qtrue;
}
static qboolean ChairRecoil(void *model,int index,const char *bone,int start,int end,int flags,float speed,int time,float frame,int blend){
 assert(model==&chairHandle&&!index&&!strcmp(bone,"Bone02")&&start==1&&end==4);
 assert(flags==(BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND)&&speed==1&&time==cg.time&&frame==-1&&blend==300);
 ++chairRecoils;return qtrue;
}
static qboolean ChairMatrix(void *model,int index,int bolt,mdxaBone_t *matrix,const vec3_t angles,const vec3_t origin,
                            int time,qhandle_t *models,vec3_t scale){
 assert(model==&chairHandle&&!index&&time==cg.time&&models==cgs.gameModels&&fabsf(scale[0]-1.2f)<.001f);
 assert(VectorCompare(angles,chairBase)&&VectorCompare(origin,chairOrigin));
 ++chairMatrices;chairMuzzle=bolt;
 if(chairBlockedMatrix)return qfalse;
 memset(matrix,0,sizeof(*matrix));matrix->matrix[0][0]=1;
 for(int i=0;i<3;++i)matrix->matrix[i][3]=origin[i];
 matrix->matrix[0][3]+=bolt+8;
 return qtrue;
}
static void ChairEffect(int id,vec3_t point,vec3_t direction,int volume,int radius,qboolean portal){
 assert(id==17&&point[0]==chairOrigin[0]+chairMuzzle+8&&point[1]==chairOrigin[1]&&point[2]==chairOrigin[2]);
 assert(direction[0]==-1&&direction[1]==0&&direction[2]==0&&volume==-1&&radius==-1&&!portal);
 ++chairEffects;
}

static int EWebInit(void **model,const char *path,int index,qhandle_t skin,qhandle_t shader,int flags,int lod){
 assert(!*model&&!strcmp(path,"models/map_objects/hoth/eweb_model.glm")&&!index&&!skin&&!shader&&!flags&&!lod);
 *model=&ewebHandle;++ewebModels;return 0;
}
static void EWebClean(void **model){assert(*model==&ewebHandle);*model=NULL;++ewebCleans;}
static qboolean EWebAnim(void *model,int index,const char *bone,int start,int end,int flags,float speed,int time,float frame,int blend){
 assert(model==&ewebHandle&&!index&&!strcmp(bone,"model_root")&&start==2&&end==4);
 assert(flags==(BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND)&&speed==1&&time==cg.time&&frame==-1&&blend==100);
 ++ewebAnimations;return qtrue;
}
static qboolean EWebSurface(void *model,const char *name,int flags){
 assert(model==&ewebHandle&&!strcmp(name,"eweb_damage")&&flags==(ewebSurfaces?0:G2SURFACEFLAG_NODESCENDANTS));
 ++ewebSurfaces;return qtrue;
}
static int EWebBolt(void *model,int index,const char *tag){
 assert(model==&ewebHandle&&!index&&!strcmp(tag,"*cannonflash"));
 ++ewebBolts;return 0;
}
static qboolean EWebMatrix(void *model,int index,int bolt,mdxaBone_t *matrix,const vec3_t angles,const vec3_t origin,
                          int time,qhandle_t *models,vec3_t scale){
 assert(model==&ewebHandle&&!index&&!bolt&&time==cg.time&&models==cgs.gameModels);
 assert(angles[YAW]==75&&origin[0]==100&&origin[1]==200&&origin[2]==300&&scale[0]==1);
 ++ewebMatrices;
 if(ewebMatrixBlocked)return qfalse;
 memset(matrix,0,sizeof(*matrix));matrix->matrix[0][0]=1;matrix->matrix[1][1]=2;
 for(int i=0;i<3;++i)matrix->matrix[i][3]=origin[i];
 return qtrue;
}
static void EWebPresentationCases(void){
 imports.G2API_InitGhoul2Model=EWebInit;imports.G2API_CleanGhoul2Models=EWebClean;
 imports.G2API_SetBoneAnim=EWebAnim;imports.G2API_SetSurfaceOnOff=EWebSurface;
 imports.G2API_AddBolt=EWebBolt;imports.G2API_GetBoltMatrix=EWebMatrix;
 centity_t gun={0};entityState_t *s=&gun.currentState;
 gun.currentValid=qtrue;s->number=90;s->eType=ET_GENERAL;s->modelGhoul2=1;s->modelindex=5;
 s->generic1=250;s->bolt2=JAR_MP_EWEB_TAG;s->time2=80;s->weapon=WP_NONE;s->speed=180;
 s->activeForcePass=NUM_FORCE_POWERS+1;s->otherEntityNum2=700;
 s->boneOrient=13;s->boneIndex1=12;s->boneIndex2=13;s->boneAngles1[0]=15;s->boneAngles2[2]=20;
 s->eFlags=EF_G2ANIMATING;s->torsoAnim=gun.pe.torso.animationNumber=2;s->legsAnim=gun.pe.legs.animationNumber=4;
 VectorSet(gun.lerpAngles,0,75,0);VectorSet(gun.lerpOrigin,100,200,300);VectorSet(gun.modelScale,1,1,1);
 playerState_t operatorState={0};operatorState.weapon=WP_EMPLACED_GUN;operatorState.emplacedIndex=s->number;
 assert(JAR_MP_EWebState(s)&&!JAR_MP_ChairState(s)&&JAR_MP_EmplacedConstraint(s)==180);
 assert(CG_JAR_EWebOperator(&gun,&operatorState)&&JAR_MP_EWebAmmo(s)==700);
 s->otherEntityNum2=1500;assert(JAR_MP_EWebAmmo(s)==1500);
 s->otherEntityNum2=JAR_MP_EWEB_WIRE_AMMO_MAX+1;assert(JAR_MP_EWebAmmo(s)==JAR_MP_EWEB_WIRE_AMMO_MAX);
 s->otherEntityNum2=0;assert(!JAR_MP_EWebAmmo(s));
 s->emplacedOwner=1;assert(!CG_JAR_EWebOperator(&gun,&operatorState));s->emplacedOwner=0;
 operatorState.emplacedIndex=91;assert(!CG_JAR_EWebOperator(&gun,&operatorState));operatorState.emplacedIndex=90;
 gun.currentValid=qfalse;assert(!CG_JAR_EWebOperator(&gun,&operatorState));gun.currentValid=qtrue;

 // The first render loads the stock model before its server pose and recoil.
 CG_GeneralGhoul2Pose(&gun);
 assert(ewebModels==1&&ewebBoneUpdates==1&&ewebAnimations==1&&!ewebSurfaces);
 CG_GeneralGhoul2Pose(&gun);assert(ewebModels==1&&ewebBoneUpdates==2&&ewebAnimations==1);
 s->torsoFlip=qtrue;CG_GeneralGhoul2Pose(&gun);assert(ewebAnimations==2);
 s->time=-1;CG_GeneralGhoul2Pose(&gun);assert(ewebSurfaces==1&&gun.jarEWebWrecked);
 CG_GeneralGhoul2Pose(&gun);assert(ewebSurfaces==1);
 s->time=0;CG_GeneralGhoul2Pose(&gun);assert(ewebSurfaces==2&&!gun.jarEWebWrecked);

 // Both matrix columns are present; the real server muzzle uses negative Y.
 vec3_t point,direction;
 assert(CG_JAR_EWebMuzzle(&gun,point,direction));
 assert(ewebBolts==1&&ewebMatrices==1&&point[0]==100&&point[1]==216&&point[2]==300);
 assert(direction[0]==0&&direction[1]==-1&&direction[2]==0);
 ewebMatrixBlocked=qtrue;VectorSet(point,1,2,3);VectorSet(direction,4,5,6);
 assert(!CG_JAR_EWebMuzzle(&gun,point,direction)&&ewebBolts==1&&ewebMatrices==2&&point[0]==1&&direction[0]==4);

 entityState_t chair=*s;chair.bolt2=JAR_MP_CHAIR_TAG;
 assert(CG_JAR_EmplacedSlotChanged(s,&chair));
 gun.jarEWebWrecked=qtrue;
 CG_JAR_EmplacedTransition(&gun,&chair);
 assert(ewebCleans==1&&!gun.ghoul2&&!gun.bolt1&&!gun.jarEWebWrecked);
 puts("MP portable E-Web presentation: first-frame stock model and native pose, recoil, wreck, ammo, reciprocal seat, muzzle axis and chair-slot reuse passed.");
}

static void EmplacedPresentationCases(void){
 memset(&cg,0,sizeof(cg));static snapshot_t snapshot;cg.snap=&snapshot;cg.time=900;snapshot.serverTime=875;
 imports.G2API_InitGhoul2Model=ChairInit;imports.G2API_AddBolt=ChairBolt;imports.G2API_SetBoneAngles=ChairAim;
 imports.G2API_SetBoneAnim=ChairRecoil;imports.G2API_GetBoltMatrix=ChairMatrix;imports.FX_PlayEffectID=ChairEffect;
 imports.G2API_CleanGhoul2Models=ChairClean;
 cgs.effects.mEmplacedMuzzleFlash=17;
 centity_t chair={0};entityState_t *s=&chair.currentState;
 s->number=90;s->eType=ET_GENERAL;s->modelGhoul2=1;s->modelindex=4;s->bolt2=1;s->generic1=250;s->time2=44;s->speed=60;
 s->origin2[0]=-44;s->activeForcePass=NUM_FORCE_POWERS+1;s->iModelScale=120;
 s->apos.trType=TR_STATIONARY;VectorSet(s->apos.trBase,0,35,0);
 VectorSet(chairBase,0,35,0);VectorSet(chairOrigin,420,16,96);
 VectorSet(s->angles,0,190,0);VectorSet(s->origin,1,2,3);
 snapshot.ps.clientNum=0;snapshot.ps.emplacedIndex=s->number;snapshot.ps.weapon=WP_EMPLACED_GUN;
 VectorSet(snapshot.ps.viewangles,50,120,0);
 assert(JAR_MP_EmplacedState(s)&&JAR_MP_EmplacedConstraint(s)==60);
 entityState_t authored=*s;authored.generic1=0;authored.origin2[0]=45;
 assert(!JAR_MP_EmplacedState(&authored)&&JAR_MP_EmplacedConstraint(&authored)==45);

 // A shot with no existing Ghoul2 model resolves the snapshot pose and bolt zero.
 CG_JAR_EmplacedFireEvent(&chair,chairOrigin);
 assert(chairModels==1&&chairBolts==1&&chairMuzzle==0&&chairPoses==1&&chairRecoils==1&&chairMatrices==1&&chairEffects==1);
 assert(chairPose[PITCH]==40&&chairPose[YAW]==60&&chair.jarEmplacedAimed);
 CG_JAR_EmplacedFireEvent(&chair,chairOrigin);
 assert(chairModels==1&&chairBolts==1&&chairMuzzle==0&&chairEffects==2);
 s->eventParm=1;chairMissingBolt=qtrue;
 CG_JAR_EmplacedFireEvent(&chair,chairOrigin);assert(chairBolts==2&&chairMatrices==2&&chairEffects==2);
 chairMissingBolt=qfalse;chairBlockedMatrix=qtrue;
 CG_JAR_EmplacedFireEvent(&chair,chairOrigin);assert(chairBolts==3&&chairMuzzle==1&&chairMatrices==3&&chairEffects==2);
 chairBlockedMatrix=qfalse;
 CG_JAR_EmplacedFireEvent(&chair,chairOrigin);assert(chairBolts==3&&chairMuzzle==1&&chairEffects==3);

 // Remote pose requires a matching operator; the rendered pose uses lerp angles.
 s->emplacedOwner=1;centity_t *gunner=&cg_entities[1];gunner->currentValid=qtrue;
 gunner->currentState.weapon=WP_EMPLACED_GUN;gunner->currentState.otherEntityNum2=91;
 int before=chairPoses;CG_JAR_EmplacedAim(&chair,chairBase,qfalse);assert(chairPoses==before);
 gunner->currentState.otherEntityNum2=90;VectorSet(gunner->lerpAngles,11,10,0);
 CG_JAR_EmplacedAim(&chair,chairBase,qfalse);assert(chairPoses==before+1&&chairPose[PITCH]==11&&chairPose[YAW]==-25);
 s->activeForcePass=0;
 CG_JAR_EmplacedAim(&chair,chairBase,qfalse);assert(chairPoses==before+2&&!VectorLength(chairPose)&&!chair.jarEmplacedAimed);
 CG_JAR_EmplacedAim(&chair,chairBase,qfalse);assert(chairPoses==before+2);

 // Moving the same chair or briefly leaving its PVS keeps its current model.
 entityState_t next=*s;next.pos.trBase[0]=50;next.activeForcePass=NUM_FORCE_POWERS+1;
 chair.previousEvent=EV_FIRE_WEAPON;chair.interpolate=chair.currentValid=qtrue;
 assert(!CG_JAR_EmplacedSlotChanged(s,&next));
 CG_JAR_EmplacedTransition(&chair,&next);
 assert(!chairCleans&&chair.ghoul2&&chair.bolt1&&chair.bolt2&&chair.previousEvent==EV_FIRE_WEAPON&&chair.interpolate);
 chair.currentValid=qfalse;
 CG_JAR_EmplacedTransition(&chair,&next);assert(!chairCleans&&chair.ghoul2);
 // The three replacement paths are handled before the new state can fire.
 for(int change=0;change<3;++change){
  next=*s;
  if(change==0)++next.time2;
  else if(change==1)++next.modelindex;
  else next.bolt2=0;
  chair.ghoul2=&chairHandle;chair.bolt1=1;chair.bolt2=2;chair.jarEmplacedAimed=qtrue;
  chair.previousEvent=EV_FIRE_WEAPON;chair.interpolate=qtrue;
  assert(CG_JAR_EmplacedSlotChanged(s,&next));
  CG_JAR_EmplacedTransition(&chair,&next);
  assert(chairCleans==change+1&&!chair.ghoul2&&!chair.bolt1&&!chair.bolt2&&!chair.jarEmplacedAimed&&!chair.previousEvent&&!chair.interpolate);
  CG_JAR_EmplacedTransition(&chair,&next);assert(chairCleans==change+1);
 }
 next=*s;++next.time2;
 chair.currentState=authored;chair.ghoul2=&chairHandle;
 assert(!CG_JAR_EmplacedSlotChanged(s,&authored));
 assert(CG_JAR_EmplacedSlotChanged(s,&next));
 CG_JAR_EmplacedTransition(&chair,&next);assert(chairCleans==4&&!chair.ghoul2);
 chair.currentState=next;
 CG_JAR_EmplacedFireEvent(&chair,chairOrigin);
 assert(chairModels==2&&chairBolts==4&&chairEffects==4&&chairMuzzle==1);
 puts("MP emplaced presentation: stock model, portable yaw, first-snapshot muzzle, both bolt caches, safe FX refusal, idle pose reset and slot replacement passed.");
 EWebPresentationCases();
}
