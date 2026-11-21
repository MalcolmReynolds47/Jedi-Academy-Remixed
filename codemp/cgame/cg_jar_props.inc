// Decorative skeletal props have private Ghoul2 instances. Native actors keep
// their own animation/controllers. GPL-2.0-only.
typedef struct {void*ghoul;int model,identity,frame,failed;char name[MAX_QPATH];float radius;} jarPropView_t;
static jarPropView_t propViews[MAX_GENTITIES];
static void PropViewClear(jarPropView_t*v){if(v->ghoul)trap->G2API_CleanGhoul2Models(&v->ghoul);memset(v,0,sizeof(*v));}
static void PropViewsClear(void){for(int i=0;i<MAX_GENTITIES;++i)PropViewClear(&propViews[i]);}
void CG_JAR_PropFrame(void){
 for(int n=0;n<MAX_GENTITIES;++n){jarPropView_t*v=&propViews[n];if(!v->model)continue;const centity_t*c=&cg_entities[n];const entityState_t*s=&c->currentState;
  if(!c->currentValid||s->eType!=ET_GENERAL||s->generic1!=250||!s->modelGhoul2||s->time2!=v->identity||s->modelindex!=v->model)PropViewClear(v);
 }
}
static qboolean SkeletalProp(centity_t*c){
 const entityState_t*s=&c->currentState;if(s->eType!=ET_GENERAL||s->generic1!=250||!s->modelGhoul2)return qfalse;
 if(s->number<0||s->number>=MAX_GENTITIES||s->modelindex<1||s->modelindex>=MAX_MODELS||s->frame<0||s->frame>65535||s->eFlags&EF_NODRAW)return qtrue;
 const char*name=CG_ConfigString(CS_MODELS+s->modelindex);jarPropView_t*v=&propViews[s->number];
 if(v->model!=s->modelindex||v->identity!=s->time2||Q_stricmp(v->name,name)){
  PropViewClear(v);v->model=s->modelindex;v->identity=s->time2;v->frame=-1;Q_strncpyz(v->name,name,sizeof(v->name));
  const char*ext=strrchr(name,'.');
  if(!ext||Q_stricmp(ext,".glm")||trap->G2API_InitGhoul2Model(&v->ghoul,name,0,0,0,0,0)<0||!v->ghoul){v->failed=1;return qtrue;}
  if(trap->G2API_SkinlessModel(v->ghoul,0)){
   char skin[MAX_QPATH];Q_strncpyz(skin,name,sizeof(skin));char*slash=strrchr(skin,'/');
   if(slash){slash[1]=0;Q_strcat(skin,sizeof(skin),"model_default.skin");qhandle_t handle=trap->R_RegisterSkin(skin);if(handle)trap->G2API_SetSkin(v->ghoul,0,handle,handle);}
  }
 }
 if(v->frame!=s->frame&&v->ghoul){
  int frames=0;vec3_t low,high;v->frame=s->frame;v->failed=!trap->JAR_GhoulPoseBounds||!trap->JAR_GhoulPoseBounds(v->ghoul,s->frame,&frames,low,high);v->radius=0;
  if(!v->failed){for(int a=0;a<3;++a){float extent=fmaxf(fabsf(low[a]),fabsf(high[a]));if(!isfinite(extent)||extent>8192||low[a]>high[a])v->failed=1;v->radius+=extent*extent;}v->radius=sqrtf(v->radius);}
 }
 if(v->failed||!v->ghoul)return qtrue;
 refEntity_t ent;memset(&ent,0,sizeof(ent));ent.reType=RT_MODEL;ent.ghoul2=v->ghoul;ent.radius=v->radius;
 VectorCopy(c->lerpOrigin,ent.origin);VectorCopy(c->lerpOrigin,ent.oldorigin);VectorCopy(c->lerpOrigin,ent.lightingOrigin);
 VectorCopy(c->lerpAngles,ent.angles);AnglesToAxis(c->lerpAngles,ent.axis);VectorSet(ent.modelScale,1,1,1);ent.renderfx=RF_LIGHTING_ORIGIN;
 ent.frame=ent.oldframe=s->frame;ent.shaderRGBA[0]=ent.shaderRGBA[1]=ent.shaderRGBA[2]=ent.shaderRGBA[3]=255;
 trap->R_AddRefEntityToScene(&ent);return qtrue;
}
