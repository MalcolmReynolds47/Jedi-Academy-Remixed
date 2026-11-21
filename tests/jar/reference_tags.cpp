// Actual native tag registry with portable additions; no renderer or game launch.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_reference_tags.h"
#include "memory_save.h"
#include <deque>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];int delayedShutDown=0;
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
gentity_t*G_Find(gentity_t*from,int offset,const char*match){for(int i=from?int(from-g_entities)+1:0;i<globals.num_entities;++i){auto&e=g_entities[i];auto*key=*reinterpret_cast<char**>(reinterpret_cast<char*>(&e)+offset);if(e.inuse&&key&&!strcmp(key,match))return &e;}return nullptr;}
void CG_DrawNode(vec3_t,int){throw std::runtime_error("Unexpected debug rendering in reference fixture");}
void G_FreeEntity(gentity_t*e){G_ReferenceForget(e);e->inuse=qfalse;}
static gentity_t&Make(int slot,const char*type,const char*name){auto&e=g_entities[slot];e.inuse=qtrue;e.s.number=slot;e.classname=G_NewString(type);e.targetname=G_NewString(name);CHECK(G_ReferenceSpawn(e));return e;}
int main(){return RunTests("native script reference lookup, scoped goals, conflicts, motion, lifetime and saved restore",[]{
 globals.num_entities=64;gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};gi.Printf=[](const char*,...){};
 vec3_t origin={10,20,30},angles={1,2,3},value;TAG_Init();CHECK(TAG_Add("mission",nullptr,origin,angles,16,0));
 auto&e=Make(10,"ref_tag","jar_sb_1");VectorSet(e.currentOrigin,100,200,300);VectorSet(e.currentAngles,5,10,15);
 CHECK(TAG_GetOrigin(nullptr,"JAR_SB_1",value)&&value[0]==100);CHECK(TAG_GetAngles(nullptr,"jar_sb_1",value)&&value[2]==15);
 jar::reference::Settings s;s.name="mission";CHECK(!G_ReferenceSet(e,s)&&delayedShutDown==0);CHECK(TAG_GetOrigin(nullptr,"mission",value)&&value[0]==10);
 s.name="look_here";s.owner="actorA";s.radius=64;CHECK(G_ReferenceSet(e,s));CHECK(!TAG_GetOrigin(nullptr,"look_here",value));CHECK(TAG_GetOrigin("actorA","LOOK_HERE",value)&&value[1]==200);
 auto&other=Make(11,"ref_tag","jar_sb_2");CHECK(!G_ReferenceSet(other,s));s.owner="actorB";CHECK(G_ReferenceSet(other,s));VectorSet(other.currentOrigin,400,500,600);CHECK(TAG_GetOrigin("actorB","look_here",value)&&value[0]==400);
 jar::reference::Settings globalConflict{"look_here","",16,true};CHECK(!G_ReferenceSet(other,globalConflict));
 s.name="bad;quit";CHECK(!G_ReferenceSet(e,s));s.name=std::string(32,'a');CHECK(!G_ReferenceSet(e,s));s.name="valid";s.owner="__WORLD__";CHECK(!G_ReferenceSet(e,s));s.owner="";s.radius=1025;CHECK(!G_ReferenceSet(e,s));
 auto&goal=Make(12,"waypoint_navgoal","jar_sb_3");s.name="destination";s.radius=48;CHECK(G_ReferenceSet(goal,s));CHECK(TAG_GetFlags(nullptr,"destination")==RTF_NAVGOAL&&TAG_GetRadius(nullptr,"destination")==48);
 s.owner="actorA";CHECK(!G_ReferenceSet(goal,s));s.owner="";s.enabled=false;CHECK(G_ReferenceSet(goal,s)&&!TAG_GetOrigin(nullptr,"destination",value));CHECK(!G_ReferenceSet(other,s));s.enabled=true;CHECK(G_ReferenceSet(goal,s));
 // Query current transforms without per-frame registry rebuilds. A linked point
 // supplies orientation while translation remains the tag's own position.
 e.currentOrigin[0]=140;CHECK(TAG_GetOrigin("actorA","look_here",value)&&value[0]==140);
 goal.target=other.targetname;VectorSet(goal.currentOrigin,400,400,600);CHECK(TAG_GetAngles(nullptr,"destination",value)&&std::fabs(value[1]-90)<.001f);
 jar::reference::Settings captured;CHECK(G_ReferenceRead(goal,captured));auto&copy=Make(13,"waypoint_navgoal","jar_sb_4");CHECK(G_ReferenceCopy(copy,captured));CHECK(strcmp(copy.message,goal.message)&&TAG_GetRadius(nullptr,copy.message)==48&&TAG_GetRadius(nullptr,"destination")==48);
 // Native entity serializers retain flags/radius/pose; native string field
 // loading is provided by g_savegame's existing message/ownername entries.
 MemorySave save;ojk::SavedGameHelper writer(&save);gentity_t stored{},loaded{};stored.radius=goal.radius;stored.svFlags=goal.svFlags;stored.e_UseFunc=goal.e_UseFunc;VectorCopy(goal.currentOrigin,stored.currentOrigin);stored.sg_export(writer);save.reset_buffer_offset();loaded.sg_import(writer);writer.ensure_all_data_read();CHECK(loaded.radius==48&&loaded.e_UseFunc==useF_marker_use&&loaded.currentOrigin[1]==400);
 G_ReferenceReset();CHECK(!TAG_GetOrigin(nullptr,"destination",value));CHECK(TAG_GetOrigin(nullptr,"mission",value));CHECK(G_ReferenceRestore());CHECK(TAG_GetOrigin(nullptr,"destination",value)&&value[1]==400);CHECK(TAG_GetOrigin("actorA","look_here",value)&&value[0]==140);
 // Save restoration rejects namespace conflicts in either entity order, even
 // when a tag is disabled, without removing authored mission records.
 auto&conflict=Make(14,"ref_tag","jar_sb_5");
 conflict.message=G_NewString("look_here");conflict.ownername=nullptr;conflict.svFlags|=SVF_INACTIVE;
 CHECK(!G_ReferenceRestore()&&TAG_GetOrigin(nullptr,"mission",value));
 CHECK(!TAG_GetOrigin("actorA","look_here",value));G_FreeEntity(&conflict);CHECK(G_ReferenceRestore());
 auto&earlyConflict=Make(9,"ref_tag","jar_sb_6");earlyConflict.message=G_NewString("look_here");
 CHECK(!G_ReferenceRestore()&&TAG_GetOrigin(nullptr,"mission",value));G_FreeEntity(&earlyConflict);CHECK(G_ReferenceRestore());
 auto&malformed=Make(14,"ref_tag","jar_sb_7");malformed.radius=1.5f;
 CHECK(!G_ReferenceRestore());G_FreeEntity(&malformed);CHECK(G_ReferenceRestore());
 G_FreeEntity(&goal);CHECK(!TAG_GetOrigin(nullptr,"destination",value));goal.inuse=qtrue;goal.classname=G_NewString("NPC");CHECK(!TAG_GetOrigin(nullptr,"destination",value));
 // An unannounced class change also invalidates a cached entry. Normal free
 // removes its reservation so the name can be used by a new generation.
 e.classname=G_NewString("NPC");CHECK(!TAG_GetOrigin("actorA","look_here",value));G_FreeEntity(&e);e.inuse=qtrue;e.classname=G_NewString("ref_tag");CHECK(G_ReferenceSet(e,jar::reference::Settings{"look_here","actorA",8,true}));
 TAG_Init();CHECK(!TAG_GetOrigin(nullptr,"mission",value)&&!TAG_GetOrigin("actorA","look_here",value)&&delayedShutDown==0);
 });}
