// Production parameter storage, native counter semantics and target callback.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_script_parms.h"
#include "memory_save.h"
#include <deque>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<parms_t> allocations;
void*G_Alloc(int size){CHECK(size==sizeof(parms_t));allocations.emplace_back();return &allocations.back();}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);}
void Q3_SetParm(int entity,int slot,const char*value){CHECK(entity>=0&&entity<ENTITYNUM_WORLD);jar::entityParms::Native(g_entities[entity],slot,value);}
#include "game/g_target_parms.inc"
int main(){return RunTests("script parameters: literal edit, counters, native target activation and native save layout",[]{
 using namespace jar::entityParms;auto&source=g_entities[10];source.inuse=qtrue;source.s.number=10;
 auto&player=g_entities[0];player.inuse=qtrue;player.s.number=0;
 SP_target_change_parm(&source);CHECK(source.e_UseFunc==useF_target_change_parm_use);target_change_parm_use(&source,nullptr,&player);CHECK(!player.parms);
 for(unsigned i=0;i<jar::parms::count;++i)CHECK(Literal(source,i,"slot "+std::to_string(i)));
 CHECK(Literal(source,0,"+5")&&Literal(source,1,"")&&Literal(source,2,"-2"));CHECK(Literal(player,0,"10")&&Literal(player,1,"unchanged"));
 target_change_parm_use(&source,nullptr,&player);CHECK(!strcmp(player.parms->parm[0],"15.000000")&&!strcmp(player.parms->parm[1],"unchanged")&&!strcmp(player.parms->parm[2],"-2.000000")&&!strcmp(player.parms->parm[15],"slot 15"));
 CHECK(!strcmp(source.parms->parm[0],"+5"));target_change_parm_use(&source,nullptr,&source);CHECK(!strcmp(source.parms->parm[0],"10.000000"));
 for(int slot:{-1,16,999})CHECK(Native(player,slot,"x")==Invalid);CHECK(Native(player,0,nullptr)==Invalid);
 CHECK(Literal(player,0,"0"));for(const char*bad:{"+inf","-inf","+nan","+1e999"})CHECK(Native(player,0,bad)==Invalid&&!strcmp(player.parms->parm[0],"0"));
 CHECK(Native(player,0,"+0")==Assigned&&!strcmp(player.parms->parm[0],"+0"));CHECK(Native(player,0,"+5suffix")==Assigned&&!strcmp(player.parms->parm[0],"5.000000"));
 CHECK(Native(player,0,"++5")==Assigned&&!strcmp(player.parms->parm[0],"10.000000"));CHECK(Native(player,0,"--2")==Assigned&&!strcmp(player.parms->parm[0],"12.000000"));
 CHECK(Native(player,0,std::string(90,'x').c_str())==Truncated&&strlen(player.parms->parm[0])==63);
 CHECK(Literal(player,0,"a; \"quoted\" \\ text"));CHECK(!Literal(player,16,"x")&&!Literal(player,0,std::string(64,'x'))&&!Literal(player,0,std::string("x\0y",3))&&!Literal(player,0,"x\ny"));
 std::string read;char unterminated[64];memset(unterminated,'x',sizeof(unterminated));CHECK(!jar::parms::Read(unterminated,read));CHECK(jar::parms::Read(nullptr,read)&&read.empty());
 MemorySave save;ojk::SavedGameHelper h(&save);player.parms->sg_export(h);parms_t restored{};save.reset_buffer_offset();restored.sg_import(h);h.ensure_all_data_read();CHECK(!memcmp(&restored,player.parms,sizeof(restored)));
 const auto before=restored;player.inuse=qfalse;target_change_parm_use(&source,nullptr,&player);CHECK(!memcmp(&before,player.parms,sizeof(before)));player.inuse=qtrue;
 source.inuse=qfalse;target_change_parm_use(&source,nullptr,&player);CHECK(!memcmp(&before,player.parms,sizeof(before)));
 });}
