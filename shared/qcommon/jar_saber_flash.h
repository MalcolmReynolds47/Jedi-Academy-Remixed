// Saber contact flashes are disabled for every appearance preset in SP and MP.
#ifndef JAR_SABER_FLASH_H
#define JAR_SABER_FLASH_H
#include <string.h>
static inline int JAR_SaberContactEffect(const char*file){
 char name[256];unsigned i;if(!file)return 0;
 for(i=0;file[i]&&i<sizeof(name)-1;++i){char c=file[i];name[i]=c=='\\'?'/':c>='A'&&c<='Z'?c+32:c;}name[i]=0;
 const char*base=strrchr(name,'/');base=base?base+1:name;
 return !strcmp(base,"saber_cut")||!strcmp(base,"saber_block")||!strcmp(base,"saber_cut.efx")||!strcmp(base,"saber_block.efx");
}
#endif
