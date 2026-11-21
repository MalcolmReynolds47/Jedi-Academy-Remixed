// Native parser, transformed keys, nested loop and misc_bsp root setup.
// Entity-class callbacks, collision registration and ICARUS are fixture boundaries.
#include "game/g_local.h"
#include "test_support.h"
#include <cmath>
#include <climits>
#include <map>
#include <vector>
#include <string>
qboolean spawning=qfalse,NPCsPrecached=qtrue;
int numSpawnVars=0,numSpawnVarChars=0;
char *spawnVars[MAX_SPAWN_VARS][2];char spawnVarChars[MAX_SPAWN_VARS_CHARS];
level_locals_t level;
game_import_t gi{};game_export_t globals{};gentity_t g_entities[MAX_GENTITIES];
static bool optionalSpawn=false;
bool G_OptionalSpawnActive(){return optionalSpawn;}
bool G_JAR_SuspendActorRecovery(bool value){return false;}
#include "../../code/game/g_subbsp_ownership.inc"
IGhoul2InfoArray &_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
void QDECL G_Error(const char *format,...){char text[1024];va_list args;va_start(args,format);vsnprintf(text,sizeof(text),format,args);va_end(args);throw std::runtime_error(text);}
#include "../../code/game/g_spawn_parse.inc"
#include "../../code/game/g_spawn_context.h"
#include "../../code/game/g_subbsp_transform.inc"
static const char *field(const char *key){for(int i=0;i<numSpawnVars;++i)if(!Q_stricmp(spawnVars[i][0],key))return spawnVars[i][1];return nullptr;}
qboolean G_SpawnString(const char *key,const char *fallback,char **out){const char *v=field(key);*out=const_cast<char*>(v?v:fallback);return v?qtrue:qfalse;}
qboolean G_SpawnInt(const char *key,const char *fallback,int *out){char*v;auto present=G_SpawnString(key,fallback,&v);*out=atoi(v);return present;}
static void parse(const char *text){CHECK(G_ParseSpawnVars(&text));}
static bool worldAllowed=true;
qboolean SP_bsp_worldspawn(){return worldAllowed?qtrue:qfalse;}
void G_SpawnSubBSPGEntityFromSpawnVars(vec3_t,vec3_t);
#include "../../code/game/g_subbsp_parse.inc"
static int active=-1,freed=0;static std::vector<int> switches;
static std::vector<std::string> registrations;
static const char firstMap[]="{ \"classname\" \"worldspawn\" }\n{ \"classname\" \"misc_bsp\" \"bspmodel\" \"second\" \"origin\" \"10 0 0\" \"angle\" \"90\" }\n{ \"classname\" \"target_position\" \"origin\" \"0 10 0\" \"target\" \"door\" }";
static const char secondMap[]="{ \"classname\" \"worldspawn\" }\n{ \"classname\" \"target_position\" \"origin\" \"4 0 0\" \"angles\" \"0 20 0\" }";
static void QDECL print(const char*,...){}
static void brush(gentity_t *e,const char *name){registrations.emplace_back(name);e->s.modelindex=!strcmp(name,"#first")?1:!strcmp(name,"#candidate")?3:2;}
static void link(gentity_t*){}
static const char *activate(int model){active=model;switches.push_back(model);return model==1?firstMap:model==2?secondMap:model==3?"{ classname worldspawn }\n":nullptr;}
int G_BSPIndex(char*){return 1;}
void G_FreeEntity(gentity_t*e){if(G_SubBSPFreeInProgress(e))return;G_SubBSPFreeScope scope(e);e->inuse=qfalse;++freed;}
static gitem_t* (*findSectionItem)(const char*)=nullptr;
gitem_t* FindItem(const char* name){return findSectionItem?findSectionItem(name):nullptr;}
#include "../../code/game/g_subbsp_root.inc"
static int fileReads=0,fileCloses=0,registryCalls=0,mainModels=3;
static bool shortRead=false,cachedResource=false,changedChecksum=false,missingModel=false;
static std::vector<byte> candidateBsp;
static int openBsp(const char*name,fileHandle_t*h,fsMode_t mode){CHECK(mode==FS_READ&&!strcmp(name,"maps/candidate.bsp"));*h=1;return int(candidateBsp.size());}
static int readBsp(void*out,int size,fileHandle_t h){CHECK(h==1&&size==candidateBsp.size());memcpy(out,candidateBsp.data(),size);++fileReads;return size-int(shortRead);}
static void closeBsp(fileHandle_t h){CHECK(h==1);++fileCloses;}
static long modelFile(const char*,void**out){CHECK(!out);return missingModel?-1:12;}
static int registry(int slot,char*name,int size,int*first,int*count,unsigned*checksum){
 ++registryCalls;*checksum=1;
 if(slot==-1){*name=0;*first=0;*count=mainModels;return 1;}
 if(slot==0&&cachedResource){Q_strncpyz(name,"#candidate",size);*first=mainModels;*count=2;*checksum=LittleLong(Com_BlockChecksum(candidateBsp.data(),int(candidateBsp.size())))^unsigned(changedChecksum);return 1;}return 0;
}
static void candidate(const char*text){
 dheader_t h{};h.ident=BSP_IDENT;h.version=BSP_VERSION;h.lumps[LUMP_MODELS]={sizeof(h),2*int(sizeof(dmodel_t))};
 h.lumps[LUMP_ENTITIES]={int(sizeof(h)+2*sizeof(dmodel_t)),int(strlen(text)+1)};candidateBsp.assign(h.lumps[0].fileofs+h.lumps[0].filelen,0);
 memcpy(candidateBsp.data(),&h,sizeof(h));memcpy(candidateBsp.data()+h.lumps[0].fileofs,text,strlen(text)+1);
}
struct Record{std::string classname,target,prefix;vec3_t origin{},angles{};int bsp,depth;};
static std::vector<Record> records;
static void(*sectionSpawnCallback)()=nullptr;
void G_SpawnSubBSPGEntityFromSpawnVars(vec3_t offset,vec3_t angles)
{
    G_AdjustSubBSPSpawnVars(offset,angles);
    if(sectionSpawnCallback){sectionSpawnCallback();return;}
    Record record;record.classname=field("classname");record.target=field("target")?field("target"):"";
    record.prefix=level.mTargetAdjust?level.mTargetAdjust:"";record.bsp=active;record.depth=level.mBSPInstanceDepth;
    CHECK(sscanf(field("origin"),"%f %f %f",&record.origin[0],&record.origin[1],&record.origin[2])==3);
    CHECK(sscanf(field("angles"),"%f %f %f",&record.angles[0],&record.angles[1],&record.angles[2])==3);
    CHECK(spawning&&!NPCsPrecached);records.push_back(record);
    if(record.classname=="misc_bsp")
    {auto&child=g_entities[11];child.inuse=qtrue;child.s.number=11;child.classname="misc_bsp";G_SubBSPAllocated(&child);VectorCopy(record.origin,child.s.origin);VectorCopy(record.angles,child.s.angles);SP_misc_bsp(&child);CHECK(active==record.bsp);CHECK(!strcmp(level.mTargetAdjust,record.prefix.c_str()));}
}
static bool closeEnough(float a,float b){return fabs(a-b)<0.001f;}
template<class F>static void rejects(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}CHECK(caught);}
static void point(const char *key,vec3_t out){CHECK(sscanf(field(key),"%f %f %f",&out[0],&out[1],&out[2])==3);}
