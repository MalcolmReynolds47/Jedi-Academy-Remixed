// Native portable volumes, ordering, transforms and actual save field layout.
#include "cgame/cg_headers.h"
#include "game/g_functions.h"
#include "memory_save.h"
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
int G_ModelIndex(const char*){return 7;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t a){VectorCopy(a,e->currentAngles);VectorCopy(a,e->s.apos.trBase);}
#include "game/g_weather.inc"
int main(){return RunTests("oriented wind/exposure volumes, priority, transport and saved geometry",[]{
 using namespace jar::weather;globals.num_entities=64;gi.linkentity=[](gentity_t*e){e->linked=qtrue;};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid weather volume");};
 auto make=[](int n,const char*name)->gentity_t&{auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=const_cast<char*>(name);e.model=const_cast<char*>("@jar_box");CHECK(G_WeatherSpawnPortable(&e));return e;};
 auto&wind=make(10,"fx_wind_zone");CHECK(wind.e_UseFunc==useF_weather_use&&wind.e_TouchFunc==touchF_NULL&&wind.contents==CONTENTS_TRIGGER&&wind.pos1[0]==-256&&wind.pos2[2]==256&&G_WeatherZoneCount()==1&&G_WeatherZoneCount(&wind)==0);
 vec3_t point={0,0,128},force;auto z=G_WeatherZones(1);CHECK(z.count==1&&Valid(z.values[0])&&Contains(z.values[0],point)&&!Active(G_WeatherFrame(1)));LocalWind(z,point,force);CHECK(force[0]==500&&force[1]==0);point[0]=257;LocalWind(z,point,force);CHECK(force[0]==0);
 wind.currentAngles[YAW]=90;vec3_t size={32,128,64};CHECK(jar::volumeEntity::Resize(wind,size));z=G_WeatherZones(1);VectorSet(point,50,0,32);CHECK(Contains(z.values[0],point));LocalWind(z,point,force);CHECK(std::fabs(force[0])<.001f&&std::fabs(force[1]-500)<.001f);VectorSet(point,0,50,32);CHECK(!Contains(z.values[0],point));
 auto&wind2=make(11,"fx_wind_zone");VectorSet(point,0,0,32);z=G_WeatherZones(1);LocalWind(z,point,force);CHECK(std::fabs(force[0]-500)<.001f&&std::fabs(force[1]-500)<.001f);
 auto&shelter=make(12,"misc_weather_zone");auto&exposed=make(13,"misc_weather_zone");Settings s;CHECK(G_WeatherRead(exposed,s));s.mode=1;CHECK(G_WeatherSet(exposed,s));CHECK(ExposureAt(G_WeatherZones(1),point)==2);weather_use(&exposed,nullptr,nullptr);CHECK(ExposureAt(G_WeatherZones(1),point)==1);weather_use(&exposed,nullptr,nullptr);CHECK(ExposureAt(G_WeatherZones(1),point)==2);exposed.inuse=qfalse;CHECK(ExposureAt(G_WeatherZones(1),point)==1);shelter.svFlags|=SVF_INACTIVE;CHECK(ExposureAt(G_WeatherZones(1),point)==0);shelter.svFlags&=~SVF_INACTIVE;
 auto bad=s;bad.mode=2;CHECK(!G_WeatherSet(shelter,bad));CHECK(!G_WeatherZones(0).count);wind.svFlags|=SVF_INACTIVE;CHECK(G_WeatherZones(1).count==2);wind.svFlags&=~SVF_INACTIVE;
 z=G_WeatherZones(1);Zones decoded;CHECK(Decode(Encode(z).c_str(),decoded)&&decoded.count==3&&Contains(decoded.values[0],point));auto encoded=Encode(decoded);
 CHECK(!Decode((encoded+" extra").c_str(),decoded)&&Encode(decoded)==encoded);for(const char*cmd:{"jar_weather_zones -1","jar_weather_zones 33","jar_weather_zones 1 0","jar_weather_zones nan"})CHECK(!Decode(cmd,decoded)&&Encode(decoded)==encoded);
 auto badZone=z.values[0];badZone.v[4]=5;CHECK(!Valid(badZone));badZone=z.values[0];badZone.v[7]=3;CHECK(!Valid(badZone));badZone=z.values[0];badZone.v[16]=5001;CHECK(!Valid(badZone));
 MemorySave save;ojk::SavedGameHelper h(&save);gentity_t stored{},restored{};VectorCopy(wind.pos1,stored.pos1);VectorCopy(wind.pos2,stored.pos2);VectorCopy(wind.currentAngles,stored.currentAngles);stored.random=wind.random;stored.radius=wind.radius;stored.wait=wind.wait;stored.speed=wind.speed;stored.delay=wind.delay;stored.spawnflags=wind.spawnflags;stored.e_UseFunc=wind.e_UseFunc;stored.sg_export(h);save.reset_buffer_offset();restored.sg_import(h);h.ensure_all_data_read();CHECK(restored.pos1[0]==-16&&restored.pos2[1]==64&&restored.pos2[2]==64&&restored.currentAngles[YAW]==90&&restored.e_UseFunc==useF_weather_use&&restored.delay==wind.delay&&restored.speed==500);
 G_WeatherRestore();wind.pos2[0]=wind.pos1[0];bool rejected=false;try{G_WeatherRestore();}catch(...){rejected=true;}CHECK(rejected);wind.pos2[0]=16;
 for(int i=1;i<globals.num_entities;++i)g_entities[i].inuse=qfalse;for(int i=1;i<=zoneLimit;++i)make(i,"misc_weather_zone");CHECK(G_WeatherZoneCount()==zoneLimit&&G_WeatherZones(1).count==zoneLimit);z=G_WeatherZones(1);CHECK(Decode(Encode(z).c_str(),decoded)&&decoded.count==zoneLimit&&Encode(z).size()<32768);
 make(zoneLimit+1,"misc_weather_zone");rejected=false;try{G_WeatherRestore();}catch(...){rejected=true;}CHECK(rejected);for(int i=1;i<globals.num_entities;++i)g_entities[i].inuse=qfalse;CHECK(!G_WeatherZones(1).count&&!G_WeatherZoneCount());
 });}
