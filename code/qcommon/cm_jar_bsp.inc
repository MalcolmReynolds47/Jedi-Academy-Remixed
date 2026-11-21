// Collision handles are authoritative; renderer load order is independent.
#include "../../shared/qcommon/jar_bsp_model.h"
qboolean CM_BSPModelInfo(const char*name,int*first,int*count){
 char path[MAX_QPATH];int child=0;if(!first||!count||!JAR_BSPModelName(name,path,&child)||child||strchr(name,':'))return qfalse;
 int offset=cmg.numSubModels;
 for(int i=0;i<NumSubBSP;++i){
  if(!Q_stricmp(SubBSP[i].name,path)){*first=offset;*count=SubBSP[i].numSubModels;return qtrue;}
  offset+=SubBSP[i].numSubModels;
 }
 return qfalse;
}
