// Production target search/use/team dispatch and ICARUS entity-name index.
// Script interpretation, physics and renderer execution are not mocked as proof.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/Q3_Interface.h"
#include "memory_save.h"
#include <deque>
#include <functional>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
void G_Error(const char*,...){throw std::runtime_error("Native error");}
bool G_JAR_SuspendActorRecovery(bool){return false;}
qboolean PInUse(int n){return g_entities[n].inuse;}
static std::deque<std::string> strings;char*G_NewString(const char*s){strings.emplace_back(s?s:"");return const_cast<char*>(strings.back().c_str());}
#include "game/g_subbsp_ownership.inc"
#include "game/g_script_entities.inc"
static entitylist_t index;
void G_FreeEntity(gentity_t*e){if(G_SubBSPFreeInProgress(e))return;G_SubBSPFreeScope scope(e);JarForgetScriptEntity(index,e);e->inuse=qfalse;}
static std::vector<int> fired;
void GEntity_UseFunc(gentity_t*e,gentity_t*,gentity_t*){G_SubBSPCapture scope(e);fired.push_back(e->s.number);}
#include "game/g_entity_find.inc"
#include "game/g_entity_targets.inc"
#include "game/g_entity_teams.inc"
static int next=1;
static gentity_t*Make(const char*classname,const char*name=""){
 auto*e=&g_entities[next];e->inuse=qtrue;e->s.number=next++;globals.num_entities=next;G_SubBSPAllocated(e);e->classname=G_NewString(classname);e->targetname=G_NewString(name);e->script_targetname=G_NewString(name);e->m_iIcarusID=e->s.number;e->e_UseFunc=useF_Use_Target_Print;return e;
}
static gentity_t*Root(bool isolate){auto*e=Make("misc_bsp");e->s.modelindex=1;CHECK(G_SubBSPBegin(e,isolate));return e;}
static std::vector<int> Find(int field,const char*name){std::vector<int> out;gentity_t*e=nullptr;while((e=G_Find(e,field,name)))out.push_back(e->s.number);return out;}
int main(){return RunTests("isolated map target, team and script-name resolution with native save restoration",[]{
 gi.Printf=[](const char*,...){};
 auto&player=g_entities[0];player.inuse=qtrue;player.s.number=0;player.targetname=player.script_targetname=G_NewString("player");player.m_iIcarusID=1000;
 auto*world=Make("func_door","door");auto*authored=Root(false);gentity_t*native;{G_SubBSPCapture scope(authored);native=Make("func_door","door");}
 auto*a=Root(true);gentity_t *ad,*nested,*nd,*privateRoot,*pd;
 {G_SubBSPCapture scope(a);ad=Make("func_door","door");nested=Root(false);{G_SubBSPCapture inner(nested);nd=Make("func_door","door");}
  privateRoot=Root(true);{G_SubBSPCapture inner(privateRoot);pd=Make("func_door","door");}}
 auto*b=Root(true);gentity_t*bd;{G_SubBSPCapture scope(b);bd=Make("func_door","door");}
 CHECK(!G_SubBSPNamespace(native)&&G_SubBSPNamespace(nd)==G_SubBSPNamespace(ad)&&G_SubBSPNamespace(pd)!=G_SubBSPNamespace(ad));
 CHECK(Find(FOFS(targetname),"door")==std::vector<int>({world->s.number,native->s.number}));
 {G_SubBSPCapture scope(a);CHECK(Find(FOFS(targetname),"DOOR")==std::vector<int>({ad->s.number,nd->s.number}));CHECK(G_Find(nullptr,FOFS(targetname),"player")==&player);CHECK(Find(FOFS(classname),"func_door").size()==6);CHECK(!G_Find(nullptr,FOFS(targetname),"missing"));}
 {G_SubBSPCapture scope(b);CHECK(G_PickTarget(const_cast<char*>("door"))==bd);}
 {G_SubBSPCapture scope(privateRoot);CHECK(G_PickTarget(const_cast<char*>("door"))==pd);}
 // Direct output calls select the source context even outside its think frame.
 G_UseTargets2(a,&player,"door");CHECK(fired==std::vector<int>({ad->s.number,nd->s.number}));fired.clear();
 G_UseTargets2(b,&player,"door");CHECK(fired==std::vector<int>({bd->s.number}));fired.clear();
 G_UseTargets2(ad,&player,"self");CHECK(fired==std::vector<int>({ad->s.number}));
 for(auto*e:{world,native,ad,nd,pd,bd}){e->target=G_NewString("switch");e->cameraGroup=G_NewString("shot");e->team=G_NewString("door_team");}
 {G_SubBSPCapture scope(b);CHECK(G_Find(nullptr,FOFS(target),"switch")==bd&&G_Find(nullptr,FOFS(cameraGroup),"shot")==bd);}
 // Name collisions are retained in ICARUS; only the matching section wins.
 for(auto*e:{&player,world,native,ad,nd,pd,bd})JarAssociateScriptEntity(index,e);
 CHECK(JarScriptEntityByName(index,"door")==native->s.number);
 {G_SubBSPCapture scope(a);CHECK(JarScriptEntityByName(index,"DOOR")==nd->s.number);CHECK(JarScriptEntityByName(index,"player")==0);}
 {G_SubBSPCapture scope(b);CHECK(JarScriptEntityByName(index,"door")==bd->s.number);}
 JarForgetScriptEntity(index,ad);{G_SubBSPCapture scope(a);CHECK(JarScriptEntityByName(index,"door")==nd->s.number);}
 nd->script_targetname=G_NewString("renamed");JarAssociateScriptEntity(index,nd);
 {G_SubBSPCapture scope(a);CHECK(JarScriptEntityByName(index,"door")==-1&&JarScriptEntityByName(index,"renamed")==nd->s.number);}
 // A reused slot without a valid ICARUS binding must never receive an old name.
 bd->m_iIcarusID=IIcarusInterface::ICARUS_INVALID;{G_SubBSPCapture scope(b);CHECK(JarScriptEntityByName(index,"door")==-1);}bd->m_iIcarusID=bd->s.number;
 // Save restoration does not rewrite script names or rebuild registry keys.
 MemorySave saved;ojk::SavedGameHelper h(&saved);G_SubBSPOwnershipExport(false,h);G_SubBSPOwnershipReset();saved.reset_buffer_offset();CHECK(G_SubBSPOwnershipImport(false,h,true));h.ensure_all_data_read();
 {G_SubBSPCapture scope(a);CHECK(JarScriptEntityByName(index,"renamed")==nd->s.number&&G_SubBSPNamespace(nd)==G_SubBSPNamespace(a));}
 saved.reset_buffer_offset();CHECK(!G_SubBSPOwnershipImport(false,h,false));CHECK(G_SubBSPNamespace(ad)); // V39 cannot represent isolated roots.
 // Raw teams link only within a namespace, including authored nested sections.
 G_FindTeams();CHECK(native->teammaster==world&&nd->teammaster==ad&&bd->teammaster==bd&&pd->teammaster==pd);CHECK(world->teamchain==native&&ad->teamchain==nd);
 CHECK(!strcmp(ad->targetname,"door")&&!strcmp(nd->script_targetname,"renamed"));
 {G_SubBSPCapture stale(a);G_FreeEntity(a);CHECK(!G_Find(nullptr,FOFS(targetname),"door")&&!G_Find(nullptr,FOFS(targetname),"player")&&JarScriptEntityByName(index,"door")==-1);}
 CHECK(!ad->inuse&&!nd->inuse&&!pd->inuse&&bd->inuse&&world->inuse);
 {G_SubBSPCapture scope(b);CHECK(JarScriptEntityByName(index,"door")==bd->s.number);}CHECK(JarScriptEntityByName(index,"door")==native->s.number);
 // Legacy ownership bytes retain global authored name behavior.
 G_FreeEntity(b);MemorySave legacy;ojk::SavedGameHelper lh(&legacy);G_SubBSPOwnershipExport(false,lh);G_SubBSPOwnershipReset();legacy.reset_buffer_offset();CHECK(G_SubBSPOwnershipImport(false,lh,false));lh.ensure_all_data_read();CHECK(!G_SubBSPNamespace(native));
 });}
