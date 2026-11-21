// Actual native weather selection, command sync, callbacks and saved fields.
#include "cgame/cg_headers.h"
#include "game/g_functions.h"
#include "memory_save.h"
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
int G_ModelIndex(const char*s){CHECK(!strcmp(s,"models/map_objects/imperial/crate1.md3"));return 7;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t a){VectorCopy(a,e->currentAngles);VectorCopy(a,e->s.apos.trBase);}
#include "game/g_weather.inc"
static std::vector<std::string> commands;
void cgi_R_WorldEffectCommand(const char*s){commands.push_back(s);}
#include "cgame/cg_weather.inc"
int main(){return RunTests("weather controllers, priority, stock materials, transport and native save fields",[]{
 using namespace jar::weather;static_assert(useF_cloudlayer_use==83&&useF_weather_use==84,"Saved callbacks append only");
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid saved weather");};globals.num_entities=30;
 static cvar_t scale{};scale.value=1;gi.cvar=[](const char*n,const char*,int)->cvar_t*{CHECK(!strcmp(n,"r_weatherScale"));return &scale;};
 auto make=[](int slot,const char*name)->gentity_t&{auto&e=g_entities[slot];e.inuse=qtrue;e.s.number=slot;e.classname=const_cast<char*>(name);e.model=const_cast<char*>("@jar_weather");CHECK(G_WeatherSpawnPortable(&e));return e;};
 auto&rain=make(10,"fx_rain");CHECK(rain.e_UseFunc==useF_weather_use&&rain.delay==1&&rain.contents==CONTENTS_ITEM&&rain.s.modelindex==7&&rain.linked&&G_WeatherHasControllers());
 CHECK(G_WeatherFrame(1).layers[0].count==1000);auto&rain2=make(11,"fx_rain");Settings s;CHECK(G_WeatherRead(rain2,s));s.mode=2;s.density=2;CHECK(G_WeatherSet(rain2,s));CHECK(G_WeatherFrame(1).layers[0].count==4000);
 weather_use(&rain2,nullptr,nullptr);CHECK(G_WeatherFrame(1).layers[0].count==1000);weather_use(&rain2,nullptr,nullptr);CHECK(G_WeatherFrame(1).layers[0].count==4000);rain2.inuse=qfalse;CHECK(G_WeatherFrame(1).layers[0].count==1000);
 auto&snow=make(12,"fx_snow");auto&puff=make(13,"fx_puff");auto&wind=make(14,"fx_wind");CHECK(G_WeatherRead(wind,s));s.speed=2000;s.mode=0;CHECK(G_WeatherSet(wind,s));wind.currentAngles[YAW]=90;
 auto f=G_WeatherFrame(1);CHECK(f.layers[1].count==1000&&f.layers[2].count==200&&std::fabs(f.wind[0])<.001f&&std::fabs(f.wind[1]-2000)<.001f);
 CHECK(!Active(G_WeatherFrame(0))&&!Active(G_WeatherFrame(std::numeric_limits<float>::quiet_NaN())));CHECK(G_WeatherFrame(100).layers[0].count==4000&&G_WeatherFrame(.1f).layers[1].count==100);
 s.mode=1;G_WeatherSet(wind,s);level.time=0;CHECK(std::fabs(G_WeatherFrame(1).wind[1]-1000)<.01f);level.time=1000;CHECK(G_WeatherFrame(1).wind[1]>1900);s.mode=2;G_WeatherSet(wind,s);CHECK(std::fabs(G_WeatherFrame(1).wind[0])>1000);
 snow.svFlags|=SVF_INACTIVE;weather_use(&snow,nullptr,nullptr);CHECK(!(snow.spawnflags&offFlag)&&G_WeatherFrame(1).layers[1].count==0);snow.svFlags&=~SVF_INACTIVE;
 Settings before;CHECK(G_WeatherRead(rain,before));auto bad=before;for(float n:{0.f,4.1f,std::numeric_limits<float>::infinity()}){bad.density=n;CHECK(!G_WeatherSet(rain,bad)&&rain.random==1);}bad=before;bad.kind=Snow;CHECK(!G_WeatherSet(rain,bad));
 rain.delay=INT_MAX-1;G_WeatherSet(puff,Settings{Puff});CHECK(rain.delay<puff.delay&&puff.delay<100); // bounded priority compaction
 f=G_WeatherFrame(1);Frame parsed;CHECK(Decode(Encode(f).c_str(),parsed)&&Same(parsed.layers[0],f.layers[0]));const auto encoded=Encode(parsed);
 for(const char*cmd:{"jar_weather", "jar_weather 4001 0 1 1 0 0 1 1 0 0 1 1 0 0 0","jar_weather 1 0 nan 1 0 0 1 1 0 0 1 1 0 0 0","jar_weather 1 0 1 1 0 0 1 1 0 0 1 1 5001 0 0"})CHECK(!Decode(cmd,parsed)&&Encode(parsed)==encoded);
 CHECK(!Decode((encoded+" extra").c_str(),parsed));
 CG_JAR_WeatherClear();CG_JAR_WeatherSync();const auto count=commands.size();CG_JAR_WeatherSync();CHECK(commands.size()==count);scale.value=0;CG_JAR_WeatherSync();CHECK(commands.size()>=count+1&&Decode(commands.back().c_str(),parsed)&&!Active(parsed));
 MemorySave save;ojk::SavedGameHelper h(&save);gentity_t stored{},restored{};stored.delay=puff.delay;stored.random=puff.random;stored.radius=puff.radius;stored.wait=puff.wait;stored.speed=puff.speed;stored.spawnflags=puff.spawnflags;stored.e_UseFunc=puff.e_UseFunc;stored.sg_export(h);save.reset_buffer_offset();restored.sg_import(h);h.ensure_all_data_read();CHECK(restored.delay==puff.delay&&restored.random==puff.random&&restored.radius==puff.radius&&restored.wait==puff.wait&&restored.speed==puff.speed&&restored.e_UseFunc==useF_weather_use);
 G_WeatherRestore();rain.e_UseFunc=useF_NULL;bool rejected=false;try{G_WeatherRestore();}catch(...){rejected=true;}CHECK(rejected);rain.e_UseFunc=useF_weather_use;
 for(int kind=0;kind<3;++kind)for(int mode=0;mode<(kind==Snow?3:4);++mode){Layer l;l.mode=mode;l.size=2;auto d=Describe(kind,l);CHECK(d.width>0&&d.height>0&&d.color[3]==.5f);if(kind==Rain)CHECK(d.oriented&&d.water);}
 for(int i=1;i<globals.num_entities;++i)g_entities[i].inuse=qfalse;CHECK(!G_WeatherHasControllers()&&!Active(G_WeatherFrame(1)));CG_JAR_WeatherClear();CHECK(Decode(commands.back().c_str(),parsed)&&parsed.wind[0]==0);
 });}
