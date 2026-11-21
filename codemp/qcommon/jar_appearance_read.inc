// MP uses SP's namespaced visual/audio overlay policy.
// Both attempts go through the native reader: pure-pak restrictions, search
// order, handles and checksums remain authoritative. No recursive overlay.
#include "qcommon/jar_appearance.h"
static long FS_FOpenFileReadOriginal(const char *filename,fileHandle_t *file,qboolean uniqueFILE);
long FS_FOpenFileRead(const char *filename,fileHandle_t *file,qboolean uniqueFILE) {
 if(filename&&file&&strlen(filename)<MAX_QPATH) {
  char normalized[MAX_QPATH],candidate[MAX_OSPATH];
  Q_strncpyz(normalized,filename,sizeof(normalized));Q_strlwr(normalized);
  for(char *p=normalized;*p;++p)if(*p=='\\')*p='/';
  const char *category=jar::AssetCategory(normalized);
  if(category) {
   const char*cvar=!strcmp(category,"font")?"jar_fontPack":!strcmp(category,"ui")?"jar_uiPack":!strcmp(category,"saber")?"jar_saberPack":!strcmp(category,"sound")?"jar_soundPack":"jar_fxPack";
   const char*initial=!strcmp(category,"font")?"sans":!strcmp(category,"ui")?"up2":"stock";
   const char *pack=Cvar_Get(cvar,initial,CVAR_ARCHIVE,"")->string;
   if(jar::AssetOverlay(candidate,sizeof(candidate),normalized,category,pack)) {
    const long size=FS_FOpenFileReadOriginal(candidate,file,uniqueFILE);
    if(size>=0)return size;
   }
  }
 }
 return FS_FOpenFileReadOriginal(filename,file,uniqueFILE);
}
