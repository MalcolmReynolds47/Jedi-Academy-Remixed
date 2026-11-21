// Original static droid geometry through MP's native Ghoul2 NPC path.
// GPL-2.0-only. Preserve loaded definitions, classes, teams and custom bodies.
#include "jar_mp_droid_models.h"
typedef struct { char head[MAX_QPATH], torso[MAX_QPATH], legs[MAX_QPATH]; } jarDroidParts_t;
static qboolean JAR_DroidFileExists(const char *path) {
 fileHandle_t f=0;int size=trap->FS_Open(path,&f,FS_READ);if(f)trap->FS_Close(f);return size>0;
}
static qboolean JAR_DroidFileMatches(const char *path,int bytes,unsigned hash) {
 fileHandle_t f=0;byte buffer[1024];unsigned value=2166136261u;
 int size=trap->FS_Open(path,&f,FS_READ);
 if(size!=bytes||!f){if(f)trap->FS_Close(f);return qfalse;}
 while(size>0){int count=size>sizeof(buffer)?sizeof(buffer):size;
  if(trap->FS_Read(buffer,count,f)!=count){trap->FS_Close(f);return qfalse;}
  for(int i=0;i<count;++i)value=(value^buffer[i])*16777619u;size-=count;}
 trap->FS_Close(f);return value==hash;
}
static qboolean JAR_MPAdaptDroid(const char *name,char *model,const char *skin,const jarDroidParts_t *parts,qboolean *md3) {
 const jarMPDroidModel_t *body=NULL;
 if(Q_stricmp(skin,"default"))return qtrue;
 if(*md3&&!Q_stricmp(parts->head,"none")&&!Q_stricmp(parts->torso,"none")){
  if(!Q_stricmp(name,"mouse")&&!Q_stricmp(parts->legs,"mouse"))body=&jarMPDroidModels[0];
  else if(!Q_stricmp(name,"remote_sp")&&!Q_stricmp(parts->legs,"remote_sp"))body=&jarMPDroidModels[1];
  else if(!Q_stricmp(name,"seeker")&&!Q_stricmp(parts->legs,"remote")&&
          !JAR_DroidFileExists("models/players/remote/lower.md3")&&!JAR_DroidFileExists("models/players/remote/lower.mdr"))body=&jarMPDroidModels[1];
 }else if(!*md3&&!Q_stricmp(name,"remote")&&!Q_stricmp(model,"remote")&&
          !JAR_DroidFileExists("models/players/remote/model.glm"))body=&jarMPDroidModels[1];
 if(!body)return qtrue;
 if(!JAR_DroidFileMatches(body->source,body->bytes,body->hash)||
    !JAR_DroidFileMatches(va("models/players/%s/model.glm",body->target),body->modelBytes,body->modelHash)||
    !JAR_DroidFileExists(va("models/players/%s/model.gla",body->target))||
    !JAR_DroidFileExists(va("models/players/%s/model_default.skin",body->target))||
    !JAR_DroidFileExists(va("models/players/%s/animation.cfg",body->target))){
  Com_Printf("JA:R: %s needs its qualified original droid model and MP adapter assets.\n",name);return qfalse;
 }
 Q_strncpyz(model,body->target,MAX_QPATH);*md3=qfalse;return qtrue;
}
