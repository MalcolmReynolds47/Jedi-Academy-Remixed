// Native controller and client lifecycle integration; GL capture tested separately.
#include "cgame/cg_headers.h"
#include "game/g_functions.h"
#include "memory_save.h"
#include <vector>
#include <string>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
int G_ModelIndex(const char*s){CHECK(!strcmp(s,"models/map_objects/imperial/crate1.md3"));return 7;}
qboolean G_SpawnFloat(const char*,const char*d,float*out){*out=float(atof(d));return qfalse;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t a){VectorCopy(a,e->currentAngles);VectorCopy(a,e->s.apos.trBase);}
void G_FreeEntity(gentity_t*e){e->inuse=qfalse;}
#include "game/g_reflection_probe.inc"
static std::vector<jar::reflection::Probe> submitted;static int result=0;static std::string status;
int cgi_R_SetReflectionProbes(const jar::reflection::Probe*p,int count){submitted.clear();if(count)submitted.assign(p,p+count);return result;}
void cgi_Cvar_Set(const char*k,const char*v){CHECK(!strcmp(k,"jar_reflectionStatus"));status=v;}
#include "cgame/cg_reflection_probe.inc"
static gentity_t& Make(int slot){auto&e=g_entities[slot];e.inuse=qtrue;e.s.number=slot;e.classname=const_cast<char*>("misc_cubemap");e.model=const_cast<char*>("@jar_reflection");e.targetname=const_cast<char*>("jar_sb_test");return e;}
int main(){return RunTests("reflection controller, client lifetime, refresh and native saves",[]{
 static_assert(useF_reflection_probe_use==useF_portable_rail_use+1,"Saved callbacks append only");
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;};globals.num_entities=100;
 auto&authored=Make(1);authored.model=nullptr;SP_misc_cubemap(&authored);CHECK(!authored.inuse);
 auto&e=Make(10);VectorSet(e.s.origin,10,20,30);SP_misc_cubemap(&e);CHECK(e.inuse&&e.linked&&e.radius==512&&e.count==1&&e.s.modelindex==7&&e.s.eType==ET_GENERAL&&e.e_UseFunc==useF_reflection_probe_use&&e.contents==CONTENTS_ITEM&&(e.svFlags&SVF_BROADCAST)&&e.maxs[2]==22);
 CG_JAR_ReflectionSync();CHECK(submitted.size()==1&&submitted[0].origin[2]==30&&status=="Reflections ready: 0 / 1");auto owner=submitted[0].owner;
 jar::reflection::Settings s;CHECK(G_ReflectionRead(e,s));s.radius=1024;e.spawnflags=128;CHECK(G_ReflectionSet(e,s));CG_JAR_ReflectionSync();CHECK(submitted[0].owner==owner&&submitted[0].revision==1&&submitted[0].radius==1024&&e.spawnflags==128);
 for(float bad:{0.f,15.f,32769.f,std::numeric_limits<float>::quiet_NaN()}){auto invalid=s;invalid.radius=bad;CHECK(!G_ReflectionSet(e,invalid)&&e.radius==1024);}
 VectorSet(e.currentOrigin,60000,50000,40000);CHECK(G_ReflectionMoved(e));CG_JAR_ReflectionSync();CHECK(submitted.size()==1&&submitted[0].origin[0]==60000&&e.s.origin[0]==60000&&submitted[0].owner==owner);
 CHECK(G_ReflectionRefresh(e));CG_JAR_ReflectionSync();CHECK(submitted[0].revision==2);e.count=INT_MAX;CHECK(G_ReflectionRefresh(e)&&e.count==1);
 reflection_probe_use(&e,nullptr,nullptr);CG_JAR_ReflectionSync();CHECK(submitted.empty()&&status=="No active probes"&&(e.spawnflags&128));reflection_probe_use(&e,nullptr,nullptr);CG_JAR_ReflectionSync();CHECK(submitted.size()==1&&submitted[0].owner!=owner);owner=submitted[0].owner;
 e.svFlags|=SVF_INACTIVE;reflection_probe_use(&e,nullptr,nullptr);CG_JAR_ReflectionSync();CHECK(submitted.empty()&&!(e.spawnflags&1));e.svFlags&=~SVF_INACTIVE;e.s.eFlags|=EF_NODRAW;CG_JAR_ReflectionSync();CHECK(submitted.empty());e.s.eFlags&=~EF_NODRAW;
 for(auto test:std::vector<std::pair<int,const char*>>{{jar::reflection::Unsupported,"Requires Rend2 renderer"},{jar::reflection::Disabled,"Enable Cubemap reflections in Graphics"},{jar::reflection::NoWorld,"Waiting for the map"},{jar::reflection::Invalid,"Reflection submission rejected"},{1,"Reflections ready: 1 / 1"}}){result=test.first;CG_JAR_ReflectionSync();CHECK(status==test.second);}owner=submitted[0].owner;
 e.targetname=const_cast<char*>("jar_sb_reused");CG_JAR_ReflectionSync();CHECK(submitted[0].owner!=owner);owner=submitted[0].owner;
 CG_JAR_ReflectionClear();CHECK(submitted.empty());CG_JAR_ReflectionSync();CHECK(submitted[0].owner!=owner);owner=submitted[0].owner;
 CHECK(G_ReflectionRestore());CG_JAR_ReflectionSync();CHECK(submitted[0].owner!=owner);
 // Actual gentity codec retains settings/callback; restoration rejects corrupt state.
 MemorySave save;ojk::SavedGameHelper h(&save);gentity_t persisted{},restored{};persisted.radius=e.radius;persisted.count=17;persisted.spawnflags=129;persisted.s.eType=e.s.eType;persisted.e_UseFunc=e.e_UseFunc;persisted.sg_export(h);save.reset_buffer_offset();restored.sg_import(h);h.ensure_all_data_read();CHECK(restored.radius==1024&&restored.count==17&&restored.spawnflags==129&&restored.e_UseFunc==useF_reflection_probe_use&&restored.s.eType==ET_GENERAL);
 e.count=0;CHECK(!G_ReflectionRestore());e.count=1;e.radius=15;CHECK(!G_ReflectionRestore());e.radius=512;e.currentOrigin[0]=70000;CHECK(!G_ReflectionRestore());e.currentOrigin[0]=0;e.e_UseFunc=useF_NULL;CHECK(!G_ReflectionRestore());e.e_UseFunc=useF_reflection_probe_use;e.s.eType=ET_ITEM;CHECK(!G_ReflectionRestore());e.s.eType=ET_GENERAL;CHECK(G_ReflectionRestore());
 e.inuse=qfalse;CG_JAR_ReflectionSync();CHECK(submitted.empty());
 for(int i=1;i<=64;++i){auto&p=Make(i);CHECK(G_ReflectionSpawn(p));}CHECK(G_ReflectionCount()==64&&G_ReflectionRestore());auto&extra=Make(65);SP_misc_cubemap(&extra);CHECK(!extra.inuse);CG_JAR_ReflectionSync();CHECK(submitted.size()==64);
 extra.inuse=qtrue;extra.count=1;extra.radius=512;CHECK(!G_ReflectionRestore());CG_JAR_ReflectionSync();CHECK(submitted.empty()&&status=="Reflection capacity exceeded");extra.inuse=qfalse;
 G_FreeEntity(&g_entities[5]);extra.inuse=qtrue;CHECK(G_ReflectionSpawn(extra)&&G_ReflectionCount()==64);CG_JAR_ReflectionSync();CHECK(submitted.size()==64);
 });}
