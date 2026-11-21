// Included by both client games with JAR_BSP_CALL mapped to their engine API.
#include "jar_bsp_model.h"
static char jarBspRegistered[MAX_SUB_BSP][MAX_QPATH];
static void CG_ResetBSPModels(void){memset(jarBspRegistered,0,sizeof(jarBspRegistered));}
void CG_RegisterBSPModels(void){
 for(int slot=1;slot<MAX_SUB_BSP;++slot){
  const char*name=CG_ConfigString(CS_BSP_MODELS+slot);if(!*name)continue;
  if(!Q_stricmp(jarBspRegistered[slot],name))continue;
  char path[MAX_QPATH];int child,first=0,count=0;
  if(!JAR_BSPModelName(name,path,&child)||child||strchr(name,':')){CG_Printf("JA:R: invalid BSP model name in slot %d\n",slot);continue;}
  if(!JAR_BSP_CALL(CM_BSPModelInfo)(name,&first,&count)){
   JAR_BSP_CALL(CM_LoadMap)(name,qtrue);
   if(!JAR_BSP_CALL(CM_BSPModelInfo)(name,&first,&count)){CG_Printf("JA:R: BSP collision registration failed: %s\n",name);continue;}
  }
  if(first<1||count<1||first>=(int)ARRAY_LEN(cgs.inlineDrawModel)||count>(int)ARRAY_LEN(cgs.inlineDrawModel)-first){
   CG_Printf("JA:R: BSP model range exceeds client capacity: %s\n",name);continue;
  }
  qboolean complete=qtrue;
  for(int model=0;model<count;++model){
   char key[MAX_QPATH];vec3_t low,high;Com_sprintf(key,sizeof(key),"%s:%d",name,model);
   const qhandle_t handle=JAR_BSP_CALL(R_RegisterModel)(key);
   if(!handle){complete=qfalse;continue;}
   cgs.inlineDrawModel[first+model]=handle;JAR_BSP_CALL(R_ModelBounds)(handle,low,high);
   for(int a=0;a<3;++a)cgs.inlineModelMidpoints[first+model][a]=(low[a]+high[a])*.5f;
  }
  if(complete){Q_strncpyz(jarBspRegistered[slot],name,MAX_QPATH);if(first+count>cgs.numInlineModels)cgs.numInlineModels=first+count;}
  else CG_Printf("JA:R: BSP draw models incomplete: %s\n",name);
 }
}
