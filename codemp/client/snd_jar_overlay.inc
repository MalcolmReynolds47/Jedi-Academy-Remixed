// Keep the source SFX identity while selecting the correct decoder extension.
#include "qcommon/jar_appearance.h"
static void S_JAR_SelectSound(char*name,int capacity,int&length,char*&extension){
 char overlay[MAX_QPATH];
 if(jar::SoundOverlay(overlay,sizeof(overlay),name,Cvar_Get("jar_soundPack","stock",CVAR_ARCHIVE,"")->string,
  [](const char*path){return FS_ReadFile(path,nullptr)>0;})){
  Q_strncpyz(name,overlay,capacity);length=(int)strlen(name);extension=name+length-4;
 }
}
