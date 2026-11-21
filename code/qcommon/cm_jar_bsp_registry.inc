// SP save/load collision identities. No renderer registration order is assumed.
// Included by cm_load.cpp. GPL-2.0-only.
int CM_BSPModelAt(int index,char*name,int size,int*first,int*count,unsigned*checksum){
 if(!name||size<MAX_QPATH||!first||!count||!checksum||index<-1)return -1;
 if(index>=NumSubBSP)return 0;
 if(index==-1){
  if(cmg.numSubModels<1||cmg.numSubModels>MAX_SUBMODELS)return -1;
  *name=0;*first=0;*count=cmg.numSubModels;*checksum=cmg.checksum;return 1;
 }
 int offset=cmg.numSubModels;
 for(int n=0;n<=index;++n){if(SubBSP[n].numSubModels<1||SubBSP[n].numSubModels>MAX_SUBMODELS-offset)return -1;if(n<index)offset+=SubBSP[n].numSubModels;}
 const auto&map=SubBSP[index];const size_t length=strlen(map.name);
 if(length<10||Q_strncmp(map.name,"maps/",5)||Q_stricmp(map.name+length-4,".bsp"))return -1;
 char key[MAX_QPATH],path[MAX_QPATH];int child;
 Com_sprintf(key,sizeof(key),"#%.*s",int(length-9),map.name+5);
 if(!JAR_BSPModelName(key,path,&child)||child||Q_stricmp(path,map.name))return -1;
 Q_strncpyz(name,key,size);*first=offset;*count=map.numSubModels;*checksum=map.checksum;return 1;
}
qboolean CM_RegisterBSPModel(const char*name,int*first,int*count,unsigned*checksum){
 char path[MAX_QPATH];int child=0;
 if(!first||!count||!checksum||!JAR_BSPModelName(name,path,&child)||strchr(name,':'))return qfalse;
 int offset=0,models=0;
 if(!CM_BSPModelInfo(name,&offset,&models)){
  if(NumSubBSP>=MAX_SUB_BSP||FS_ReadFile(path,nullptr)<=0)return qfalse;
  CM_LoadSubBSP(path,qfalse);
  if(!CM_BSPModelInfo(name,&offset,&models))return qfalse;
 }
 if(offset<1||offset>=MAX_SUBMODELS||models<1||models>MAX_SUBMODELS-offset)return qfalse;
 const int slot=CM_FindSubBSP(offset);if(slot<0||slot>=NumSubBSP)return qfalse;
 *first=offset;*count=models;*checksum=SubBSP[slot].checksum;return qtrue;
}
