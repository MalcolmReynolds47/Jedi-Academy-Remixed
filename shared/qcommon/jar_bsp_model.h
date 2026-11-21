// Shared SP/MP BSP model spelling. GPL-2.0-only.
// '#map/path' is the root; '#map/path:N' names one of its inline models.
#ifndef JAR_BSP_MODEL_H
#define JAR_BSP_MODEL_H
static qboolean JAR_BSPModelName(const char*name,char*path,int*child){
 if(!name||name[0]!='#'||!path||!child)return qfalse;
 const char*end=strchr(name+1,':');const size_t length=end?(size_t)(end-name-1):strlen(name+1);
 if(!length||length+10>MAX_QPATH||name[1]=='/'||name[length]=='/')return qfalse;
 for(size_t i=1;i<=length;++i){const unsigned char c=(unsigned char)name[i];
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='/'||c=='.'))return qfalse;
  if(c=='.'&&(i==1||name[i-1]=='/'||name[i-1]=='.'))return qfalse;
  if(c=='/'&&name[i-1]=='/')return qfalse;
 }
 int model=0;
 if(end){if(!*++end)return qfalse;for(;*end;++end){if(*end<'0'||*end>'9'||model>511)return qfalse;model=model*10+(*end-'0');}if(model>511)return qfalse;}
 memcpy(path,"maps/",5);memcpy(path+5,name+1,length);memcpy(path+5+length,".bsp",5);*child=model;return qtrue;
}
#endif
