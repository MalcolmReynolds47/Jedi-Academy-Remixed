// Actual allocator, allocation transaction, lifetimes and menu wrapper.
// Native free cleanup, collision linking and ICARUS are service boundaries.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_entity_pool.h"
#include "game/g_workshop.h"
#include "game/Q3_Interface.h"
#include "game/g_nav.h"
#include "memory_save.h"
#include <functional>
#include <vector>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
struct EmptyGhoulPool:IGhoul2InfoArray {
 int New()override{throw std::runtime_error("Unexpected Ghoul2 allocation");}
 void Delete(int)override{throw std::runtime_error("Unexpected Ghoul2 deletion");}
 bool IsValid(int)const override{return false;}
 std::vector<CGhoul2Info>&Get(int)override{throw std::runtime_error("Unexpected Ghoul2 read");}
 const std::vector<CGhoul2Info>&Get(int)const override{throw std::runtime_error("Unexpected Ghoul2 read");}
};
IGhoul2InfoArray&_TheGhoul2InfoArray(){static EmptyGhoulPool pool;return pool;}
void G_Error(const char*,...){throw std::runtime_error("Native error");}
static bool recovery=false;bool G_JAR_SuspendActorRecovery(bool value){const bool previous=recovery;recovery=value;return previous;}
void SetInUse(gentity_t*e){e->inuse=qtrue;}qboolean PInUse(int n){return g_entities[n].inuse;}
#include "game/g_subbsp_ownership.inc"
#include "game/g_spawn_transaction.inc"
#include "game/g_entity_alloc.inc"
static std::vector<int> freed;static std::function<void(gentity_t*)> onFree;
void G_FreeEntity(gentity_t*e){if(G_SubBSPFreeInProgress(e))return;CHECK(e->inuse);G_SubBSPFreeScope scope(e);if(onFree)onFree(e);freed.push_back(e->s.number);e->inuse=qfalse;e->freetime=level.time;}

void MatchTeam(gentity_t*,moverState_t,int){}
#include "game/g_door_trigger_spawn.inc"
static gentity_t*Make(const char*name="fixture"){auto*e=G_Spawn();e->classname=const_cast<char*>(name);return e;}
static int liveCount(){int count=0;for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD;++n)if(g_entities[n].inuse)++count;return count;}
static int messages=0,lists=0;static std::string lastMessage;
static std::function<gentity_t*()> implementation;
void message(const char*s){++messages;lastMessage=s;}
void list(){++lists;}
gentity_t*spawnImpl(uint32_t,const float*,const float*,int,bool,bool,WorkshopSpawnFeedback){return implementation();}
gentity_t *NPC=nullptr,*_saved_NPC=nullptr;gNPC_t *NPCInfo=nullptr,*_saved_NPCInfo=nullptr;
gclient_t *client=nullptr,*_saved_client=nullptr;usercmd_t ucmd{},_saved_ucmd{};
#include "game/g_npc_context.h"
#include "game/g_workshop_spawn.inc"
static gentity_t*Menu(WorkshopSpawnFeedback feedback=WorkshopSpawnFeedback::Individual){return spawn(1,nullptr,nullptr,0,false,false,feedback);}
static void Reset(){G_ResetSpawnTransactions();G_SubBSPOwnershipReset();for(auto&e:g_entities){e.inuse=qfalse;e.freetime=0;}globals.num_entities=MAX_CLIENTS;level.time=100;freed.clear();onFree={};messages=lists=0;lastMessage.clear();implementation={};NPC=_saved_NPC=nullptr;NPCInfo=_saved_NPCInfo=nullptr;client=_saved_client=nullptr;ucmd={};_saved_ucmd={};CHECK(!recovery);}
static void FillExcept(int remaining){for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD-JAR_ENTITY_RESERVE-remaining;++n)g_entities[n].inuse=qtrue;globals.num_entities=ENTITYNUM_WORLD-JAR_ENTITY_RESERVE-remaining;}
int main(){return RunTests("optional native spawn transactions: required children, rollback, reuse and menu recovery",[]{
 gi.Printf=[](const char*,...){};gi.linkentity=[](gentity_t*){};
 // A real native callback creates the child that cannot fit. The menu wrapper
 // catches only allocation exhaustion and reports it after cleanup completes.
 Reset();auto*existing=Make("existing");FillExcept(1);const int before=liveCount();
 implementation=[](){auto*door=Make("func_door");VectorSet(door->absmin,0,0,0);VectorSet(door->absmax,32,8,64);Think_SpawnNewDoorTrigger(door);return door;};
 CHECK(!Menu()&&liveCount()==before&&existing->inuse&&freed.size()==1&&messages==1&&lists==1);
 CHECK(lastMessage.find("incomplete spawn removed")!=std::string::npos&&!G_OptionalSpawnActive());
 for(int n=ENTITYNUM_WORLD-JAR_ENTITY_RESERVE;n<ENTITYNUM_WORLD;++n)CHECK(!g_entities[n].inuse);
 auto*required=G_Spawn();CHECK(required->s.number==ENTITYNUM_WORLD-JAR_ENTITY_RESERVE-1);
 G_Spawn();CHECK(g_entities[ENTITYNUM_WORLD-JAR_ENTITY_RESERVE].inuse); // ordinary native reserve still usable
 // An ignored optional failure still invalidates Commit, including when the
 // native setup returned an apparently usable root.
 Reset();FillExcept(2);const int filled=liveCount();
 implementation=[](){auto*root=Make();Make();CHECK(!G_TrySpawn(qfalse));return root;};
 CHECK(!Menu()&&liveCount()==filled&&freed.size()==2&&messages==1&&lists==1);
 Reset();FillExcept(2);
 implementation=[](){auto*root=Make();Make();CHECK(!G_TrySpawn(qfalse));return root;};
 CHECK(!Menu(WorkshopSpawnFeedback::Assembly)&&liveCount()==filled&&freed.size()==2&&messages==0&&lists==0);
 // Successful menu allocation retains its native children; ordinary failure
 // cleans all new allocations and retains the setup's own error message.
 Reset();implementation=[](){auto*root=Make();Make();return root;};CHECK(Menu()&&liveCount()==2&&freed.empty()&&!G_OptionalSpawnActive());
 Reset();existing=Make();implementation=[](){Make();Make();message("Placement blocked");return (gentity_t*)nullptr;};
 CHECK(!Menu()&&liveCount()==1&&existing->inuse&&lastMessage=="Placement blocked"&&lists==0);
 // Committed nested children still belong to the outer transaction.
 Reset();existing=Make();{G_SpawnTransaction outer;Make();{G_SpawnTransaction inner;Make();CHECK(inner.Commit());}CHECK(liveCount()==3);}
 CHECK(liveCount()==1&&existing->inuse&&freed.size()==2);
 // Nested failure can be followed by a successful alternative; reused slots
 // have new identities and are not confused with prior rollback records.
 Reset();gentity_t*retained;
 {G_SpawnTransaction outer(5);Make();{G_SpawnTransaction inner(1);auto*old=Make();G_FreeEntity(old);CHECK(!G_TrySpawn(qtrue)&&!inner.Commit());}
  retained=Make();CHECK(outer.Commit());}
 CHECK(liveCount()==2&&retained->inuse&&!G_OptionalSpawnActive());
 // A bounded budget counts allocation churn, not just simultaneously live
 // entities. Ancestor budgets also constrain a nested transaction.
 Reset();{G_SpawnTransaction transaction(2);auto*first=Make();G_FreeEntity(first);auto*second=Make();G_FreeEntity(second);CHECK(!G_TrySpawn(qfalse)&&transaction.Exhausted()&&!transaction.Commit());}
 CHECK(liveCount()==0);
 Reset();{G_SpawnTransaction outer(1);Make();{G_SpawnTransaction inner;CHECK(!G_TrySpawn(qtrue)&&!inner.Commit());}CHECK(outer.Exhausted()&&!outer.Commit());}CHECK(liveCount()==0);
 // Cleanup events run under normal policy and cannot be adopted by an outer
 // transaction or mistaken for the dead occupant of a reused entity slot.
 Reset();gentity_t*event=nullptr;
 {G_SpawnTransaction outer;auto*old=Make();G_FreeEntity(old);{G_SpawnTransaction inner;Make();
   onFree=[&](gentity_t*){CHECK(recovery&&!G_OptionalSpawnActive());if(!event)event=Make("cleanup_event");};}
  onFree={};}
 CHECK(event&&event->inuse&&liveCount()==1&&!recovery);
 // Native NPC startup replaces shared AI state. Rollback must leave neither
 // active nor saved globals pointing to its freed actor, and preserve outer AI.
 Reset();gNPC_t info{};gclient_t npcClient{};existing=Make();existing->NPC=&info;existing->client=&npcClient;
 NPC=_saved_NPC=existing;NPCInfo=_saved_NPCInfo=&info;client=_saved_client=&npcClient;ucmd.buttons=7;_saved_ucmd.buttons=9;
 implementation=[]()->gentity_t*{NPC=_saved_NPC=Make();NPCInfo=_saved_NPCInfo=nullptr;client=_saved_client=nullptr;ucmd.buttons=11;_saved_ucmd.buttons=13;throw G_SpawnCapacity{};};
 CHECK(!Menu()&&NPC==existing&&_saved_NPC==existing&&NPCInfo==&info&&client==&npcClient&&ucmd.buttons==7&&_saved_ucmd.buttons==9);
 {G_NPCContext context;G_FreeEntity(existing);auto*reuse=Make();CHECK(reuse==existing);}CHECK(!NPC&&!NPCInfo&&!client&&!_saved_NPC);
 // Unrelated engine errors still propagate; a capacity exception is the only
 // recoverable menu error. Native required allocation outside the scope drops.
 Reset();implementation=[]()->gentity_t*{Make();throw std::runtime_error("other error");};bool other=false;
 try{Menu();}catch(const std::runtime_error&){other=true;}CHECK(other&&liveCount()==0&&messages==0&&!G_OptionalSpawnActive());
 for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD;++n)g_entities[n].inuse=qtrue;globals.num_entities=ENTITYNUM_WORLD;bool fatal=false;
 try{G_Spawn();}catch(const std::runtime_error&){fatal=true;}CHECK(fatal);
 // A reset must not restore abandoned stack pointers, including if a live
 // scope unwinds after the reset. Lifetime identity never aliases a new level.
 Reset();uint64_t oldLife;
 {G_SpawnTransaction transaction;auto*old=Make();oldLife=G_EntityLifetime(old);G_ResetSpawnTransactions();G_SubBSPOwnershipReset();CHECK(!transaction.Commit());}
 CHECK(!G_OptionalSpawnActive());Reset();CHECK(G_EntityLifetime(Make())!=oldLife);
 });}
