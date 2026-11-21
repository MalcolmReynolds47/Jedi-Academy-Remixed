// Shared by the SP/MP classic renderers. Never infer a BSP slot from CS order.
#include "../../shared/qcommon/jar_bsp_model.h"
static qhandle_t JAR_RegisterBSPModel(const char*name){
#ifdef DEDICATED
 return 0;
#else
 char path[MAX_QPATH],key[MAX_QPATH];int child=0;if(!JAR_BSPModelName(name,path,&child))return 0;
 int index=0;for(;index<tr.numBSPModels;++index)if(!Q_stricmp(tr.bspModels[index].name,path))break;
 if(index==tr.numBSPModels){
  if(index>=MAX_SUB_BSP)return 0;
  // Check availability before reserving a slot. The native loader diagnoses
  // malformed BSP contents; an absent file must not consume the registry.
  if(ri.FS_ReadFile(path,nullptr)<=0)return 0;
  R_IssuePendingRenderCommands();
  RE_LoadWorldMap_Actual(path,tr.bspModels[index],index+1);
  ++tr.numBSPModels;
 }
 Com_sprintf(key,sizeof(key),"*%d-%d",index+1,child);
 for(modelHash_t*entry=mhHashTable[generateHashValue(key,FILE_HASH_SIZE)];entry;entry=entry->next){
  if(!Q_stricmp(entry->name,key)){
   const qhandle_t handle=entry->handle;if(!handle||tr.models[handle]->type==MOD_BAD)return 0;
   RE_InsertModelIntoHash(name,tr.models[handle]);return handle;
  }
 }
 return 0;
#endif
}
