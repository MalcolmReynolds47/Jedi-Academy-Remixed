// Actual entity allocator, delayed door trigger, ownership and save codec.
// Native free cleanup, collision linking and ICARUS are service boundaries.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_entity_pool.h"
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
static gentity_t*Make(const char*name){auto*e=G_Spawn();e->classname=const_cast<char*>(name);return e;}
static gentity_t*Root(){auto*e=Make("misc_bsp");e->s.modelindex=1;CHECK(G_SubBSPBegin(e));return e;}
static void Reset(){G_SubBSPOwnershipReset();for(auto&e:g_entities){e.inuse=qfalse;e.freetime=0;}globals.num_entities=MAX_CLIENTS;level.time=100;freed.clear();onFree={};}
int main(){return RunTests("map-section ownership: native allocation, delayed helpers, nested teardown and saves",[]{
 gi.Printf=[](const char*,...){};gi.linkentity=[](gentity_t*){};
 Reset();auto*world=Make("world_helper");auto*root=Root();auto*other=Root();gentity_t *door,*nested,*deep,*foreign,*late,*reused;
 {G_SubBSPCapture scope(root);
  door=Make("func_door");door->targetname="original_door";door->target="original_switch";VectorSet(door->absmin,0,0,0);VectorSet(door->absmax,32,8,64);
  nested=Root();{G_SubBSPCapture inner(nested);deep=Make("target_position");CHECK(G_SubBSPOwner(deep)==nested);}
  auto*sibling=Make("target_relay");CHECK(G_SubBSPOwner(sibling)==root);
  {G_SubBSPCapture unowned(world);foreign=Make("world_event");CHECK(!G_SubBSPOwner(foreign));}
  reused=Make("temporary");const int slot=reused->s.number;G_FreeEntity(reused);
  {G_SubBSPCapture unowned(nullptr);reused=Make("unrelated_reuse");CHECK(reused->s.number==slot&&!G_SubBSPOwner(reused));}
 }
 {G_SubBSPCapture scope(other);late=Make("other_helper");}
 // The real native delayed callback allocates and links its own trigger.
 {G_SubBSPCapture callback(door);Think_SpawnNewDoorTrigger(door);}
 gentity_t*trigger=nullptr;for(int n=MAX_CLIENTS;n<globals.num_entities;++n)if(g_entities[n].inuse&&g_entities[n].owner==door)trigger=&g_entities[n];
 CHECK(trigger&&trigger->e_TouchFunc==touchF_Touch_DoorTrigger&&trigger->mins[1]==-120&&trigger->maxs[1]==128&&G_SubBSPOwner(trigger)==root);
 CHECK(!strcmp(door->targetname,"original_door")&&!strcmp(door->target,"original_switch"));
 CHECK(!G_SubBSPBegin(root)&&!G_SubBSPBegin(world)&&!G_SubBSPBegin(&g_entities[0]));
 // Full saves restore ownership without changing any native entity names.
 MemorySave saved;ojk::SavedGameHelper h(&saved);G_SubBSPOwnershipExport(false,h);G_SubBSPOwnershipReset();CHECK(!G_SubBSPHasOwnership());saved.reset_buffer_offset();CHECK(G_SubBSPOwnershipImport(false,h));h.ensure_all_data_read();
 CHECK(G_SubBSPOwner(door)==root&&G_SubBSPOwner(trigger)==root&&G_SubBSPOwner(deep)==nested&&G_SubBSPOwner(late)==other&&!G_SubBSPOwner(reused));
 {G_SubBSPCapture callback(deep);auto*afterLoad=Make("loaded_child");CHECK(G_SubBSPOwner(afterLoad)==nested);}
 // Auto-save contains no world ownership and leaves the fresh map intact.
 MemorySave automatic;ojk::SavedGameHelper ah(&automatic);G_SubBSPOwnershipExport(true,ah);automatic.reset_buffer_offset();CHECK(G_SubBSPOwnershipImport(true,ah)&&G_SubBSPOwner(deep)==nested);ah.ensure_all_data_read();
 // Bad records are rejected atomically while current ownership remains usable.
 auto invalid=[&](std::initializer_list<int> words){MemorySave bad;ojk::SavedGameHelper bh(&bad);for(int value:words)bh.write<int32_t>(value);bad.reset_buffer_offset();CHECK(!G_SubBSPOwnershipImport(false,bh));CHECK(G_SubBSPOwner(door)==root);};
 invalid({ENTITYNUM_WORLD+1});invalid({1,0,-1,1,0});
 invalid({1,door->s.number,other->s.number,0,int(bspClassHash(door->classname))}); // absent owner
 invalid({1,root->s.number,root->s.number,1,int(bspClassHash(root->classname))});
 invalid({2,root->s.number,other->s.number,1,int(bspClassHash(root->classname)),other->s.number,root->s.number,1,int(bspClassHash(other->classname))});
 invalid({2,root->s.number,-1,1,int(bspClassHash(root->classname)),root->s.number,-1,1,int(bspClassHash(root->classname))});
 char*prior=door->classname;door->classname="reused_role";saved.reset_buffer_offset();CHECK(!G_SubBSPOwnershipImport(false,h));door->classname=prior;
 // A nested section can be removed independently; deleting the outer section
 // later must ignore its former children's reused slots.
 G_FreeEntity(nested);CHECK(!nested->inuse&&!deep->inuse&&door->inuse&&late->inuse);
 auto*replacement=Make("unrelated_nested_slot");CHECK(replacement==nested&&!G_SubBSPOwner(replacement));
 onFree=[&](gentity_t*e){if(e==trigger){CHECK(recovery);G_FreeEntity(root);}}; // reentrant native cleanup
 {G_SubBSPCapture stale(door);G_FreeEntity(root);auto*newEvent=Make("event_after_parent_free");CHECK(!G_SubBSPOwner(newEvent));}
 CHECK(!door->inuse&&!trigger->inuse&&world->inuse&&foreign->inuse&&reused->inuse&&replacement->inuse&&late->inuse&&other->inuse&&!recovery);
 CHECK(std::count(freed.begin(),freed.end(),root->s.number)==1);
 G_FreeEntity(other);CHECK(!late->inuse&&!G_SubBSPHasOwnership());
 // Maximum native section depth remains valid through serialization; one
 // further nested root is refused without attaching a new group.
 Reset();root=Root();gentity_t*last=root;
 for(int depth=1;depth<MAX_SUB_BSP;++depth){G_SubBSPCapture scope(last);last=Root();}
 {G_SubBSPCapture scope(last);auto*tooDeep=Make("misc_bsp");tooDeep->s.modelindex=1;CHECK(!G_SubBSPBegin(tooDeep));G_FreeEntity(tooDeep);auto*leaf=Make("leaf");CHECK(G_SubBSPOwner(leaf)==last);}
 MemorySave deepest;ojk::SavedGameHelper dh(&deepest);G_SubBSPOwnershipExport(false,dh);G_SubBSPOwnershipReset();deepest.reset_buffer_offset();CHECK(G_SubBSPOwnershipImport(false,dh));dh.ensure_all_data_read();G_FreeEntity(root);CHECK(!G_SubBSPHasOwnership());
 // Optional allocation still preserves the critical reserve and never records
 // a rejected allocation. The required native path retains its ERR_DROP rule.
 Reset();root=Root();for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD-JAR_ENTITY_RESERVE;++n)g_entities[n].inuse=qtrue;globals.num_entities=ENTITYNUM_WORLD-JAR_ENTITY_RESERVE;
 {G_SubBSPCapture scope(root);CHECK(!G_TrySpawn(qfalse));auto*essential=G_TrySpawn(qtrue);CHECK(essential&&G_SubBSPOwner(essential)==root);}
 G_SubBSPOwnershipReset();CHECK(!G_SubBSPHasOwnership());
 });}
