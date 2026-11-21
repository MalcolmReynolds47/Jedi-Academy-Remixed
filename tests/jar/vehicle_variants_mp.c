// Execute the complete MP vehicle loader; no server or renderer is launched.
#include "../../codemp/game/bg_vehicleLoad.c"
#include <assert.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL: %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
gameImport_t imports;gameImport_t *trap=&imports;
stringID_table_t animTable[MAX_ANIMATIONS+1]={{"BOTH_VS_IDLE",0},{"",-1}};
int G_ModelIndex(const char*s){return 1;}int G_SoundIndex(const char*s){return 1;}int G_EffectIndex(const char*s){return 1;}
void G_SetSharedVehicleFunctions(vehicleInfo_t*v){}void G_SetAnimalVehicleFunctions(vehicleInfo_t*v){}void G_SetSpeederVehicleFunctions(vehicleInfo_t*v){}void G_SetWalkerVehicleFunctions(vehicleInfo_t*v){}void G_SetFighterVehicleFunctions(vehicleInfo_t*v){}
void *BG_Alloc(int n){return calloc(1,n);}void *BG_TempAlloc(int n){return calloc(1,n);}void BG_TempFree(int n){}
static void QDECL Print(const char*s,...){ }static void QDECL Error(int code,const char*s,...){abort();}
void (QDECL *Com_Printf)(const char*,...)=Print;void (QDECL *Com_Error)(int,const char*,...)=Error;
void BG_GiveMeVectorFromMatrix(mdxaBone_t*m,int flags,vec3_t out){abort();}
int BG_GetTime(void){abort();return 0;}
#include "vehicle_variants_cases.inc"
#define MAX_NPC_DATA_SIZE 0x40000
char NPCParms[MAX_NPC_DATA_SIZE],npcParseBuffer[MAX_NPC_DATA_SIZE];
#include "../../codemp/game/g_npc_load.inc"
static int npcReads,npcCloses;static char npcSource[128];
static int ListNPC(const char*path,const char*ext,char*out,int size){
 int total=0,count=0;for(int i=0;i<201;++i){char name[64];Com_sprintf(name,sizeof(name),i==200?"z-95.npc":"fixture_npc_%03d.npc",i);int n=(int)strlen(name)+1;if(total+n+1>=size)break;memcpy(out+total,name,n);total+=n;++count;}out[total]=0;return count;
}
static int OpenNPC(const char*path,fileHandle_t*f,fsMode_t mode){
 CHECK(mode==FS_READ);const char*name=strrchr(path,'/')+1;char actor[64];Q_strncpyz(actor,name,sizeof(actor));*strrchr(actor,'.')=0;
 Com_sprintf(npcSource,sizeof(npcSource),"%s\n{\nclass CLASS_VEHICLE\n}\n",actor);*f=1;return (int)strlen(npcSource);
}
static void ReadNPC(void*dest,int n,fileHandle_t f){CHECK(f==1&&n==(int)strlen(npcSource));memcpy(dest,npcSource,n);++npcReads;}
static void CloseNPC(fileHandle_t f){CHECK(f==1);++npcCloses;}
static void NPCCases(void){imports.FS_GetFileList=ListNPC;imports.FS_Open=OpenNPC;imports.FS_Read=ReadNPC;imports.FS_Close=CloseNPC;NPC_LoadParms();CHECK(npcReads==201&&npcCloses==201);CHECK(JAR_VehicleBlock(NPCParms,"z-95"));}

int main(int argc,char**argv){VehicleCases(argc,argv);NPCCases();puts("PASS: native MP vehicle definitions, color skins, precedence and capacity");return 0;}
