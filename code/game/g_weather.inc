// Independent Sandbox weather is reconciled into renderer-owned layers.
// Authored CS_WORLD_FX commands and settings are never replaced. GPL-2.0-only.
#include <vector>
#include "g_volume_geometry.h"
#include "g_weather_properties.inc"
void weather_use(gentity_t*self,gentity_t*,gentity_t*){jar::weather::Settings s;if(!self||(self->svFlags&SVF_INACTIVE)||!G_WeatherRead(*self,s))return;s.enabled=!s.enabled;G_WeatherSet(*self,s);}
bool G_WeatherSpawnPortable(gentity_t*e){
 if(!e||!G_WeatherPortable(*e))return false;
 jar::weather::Settings s;s.kind=jar::weather::Class(e->classname);if(s.kind>=jar::weather::Puff)s.mode=0;
 G_WeatherSet(*e,s);e->s.eType=ET_GENERAL;e->e_UseFunc=useF_weather_use;e->e_ThinkFunc=thinkF_NULL;e->e_TouchFunc=touchF_NULL;e->nextthink=0;e->takedamage=qfalse;e->contents=CONTENTS_ITEM;
 e->s.modelindex=G_ModelIndex("models/map_objects/imperial/crate1.md3");VectorSet(e->mins,-21,-21,0);VectorSet(e->maxs,21,21,22);
 G_SetOrigin(e,e->s.origin);G_SetAngles(e,e->s.angles);e->svFlags|=SVF_PLAYER_USABLE;if(jar::weather::ZoneClass(e->classname)){e->contents=CONTENTS_TRIGGER;jar::volumeEntity::Initialize(*e);}gi.linkentity(e);return true;
}
bool G_WeatherHasControllers(){for(int i=1;i<globals.num_entities;++i)if(G_WeatherPortable(g_entities[i]))return true;return false;}
void G_WeatherRestore(){if(G_WeatherZoneCount()>jar::weather::zoneLimit)gi.Error(ERR_DROP,"Too many saved weather volumes");for(int i=1;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_WeatherPortable(e))continue;jar::weather::Settings s;if(!G_WeatherRead(e,s)||e.e_UseFunc!=useF_weather_use||e.s.eType!=ET_GENERAL||e.delay<1)gi.Error(ERR_DROP,"Invalid saved weather controller");if(jar::weather::ZoneClass(e.classname)&&!jar::volumeEntity::Refresh(e))gi.Error(ERR_DROP,"Invalid weather volume geometry");gi.linkentity(&e);}}
jar::weather::Frame G_WeatherFrame(float scale){
 using namespace jar::weather;Frame f;if(!std::isfinite(scale)||scale<=0)return f;scale=std::min(scale,4.f);
 const gentity_t*winner[4]{};Settings settings[4];
 for(int i=1;i<globals.num_entities;++i){const auto&e=g_entities[i];Settings s;if(!G_WeatherRead(e,s)||s.kind>Wind||!s.enabled||(e.svFlags&SVF_INACTIVE)||e.delay<1)continue;auto*&w=winner[s.kind];if(!w||e.delay>w->delay||(e.delay==w->delay&&e.s.number>w->s.number)){w=&e;settings[s.kind]=s;}}
 for(int kind=0;kind<3;++kind)if(winner[kind]){const auto&s=settings[kind];auto&l=f.layers[kind];const float intensity=(kind!=Puff&&s.mode==0)?.5f:(kind!=Puff&&s.mode==2)?2.f:1.f;l.count=std::min(particleLimit,std::max(1,int(std::lround((kind==Puff?200:1000)*s.density*scale*intensity))));l.mode=s.mode;l.size=s.size;l.alpha=s.alpha;}
 if(winner[Wind]){const auto&e=*winner[Wind];const auto&s=settings[Wind];vec3_t angle;VectorCopy(e.currentAngles,angle);const float t=(level.time%1000000)*.001f;float strength=s.speed;if(s.mode==1)strength*=.5f+.5f*std::sin(t*1.7f);if(s.mode==2)angle[YAW]+=std::sin(t*.7f)*90;AngleVectors(angle,f.wind,nullptr,nullptr);VectorScale(f.wind,strength,f.wind);}
 return f;
}

jar::weather::Zones G_WeatherZones(float scale){
 using namespace jar::weather;Zones z;if(!std::isfinite(scale)||scale<=0)return z;
 std::vector<const gentity_t*> sources;for(int i=1;i<globals.num_entities;++i){const auto&e=g_entities[i];Settings s;if(G_WeatherRead(e,s)&&s.kind>=WindZone&&s.enabled&&!(e.svFlags&SVF_INACTIVE)&&e.delay>0&&jar::volume::Valid(e.pos1,e.pos2))sources.push_back(&e);}
 std::stable_sort(sources.begin(),sources.end(),[](const gentity_t*a,const gentity_t*b){return a->delay<b->delay||(a->delay==b->delay&&a->s.number<b->s.number);});
 for(auto*e:sources){if(z.count==zoneLimit)break;Settings s;G_WeatherRead(*e,s);auto&out=z.values[z.count];vec3_t axis[3];AnglesToAxis(e->currentAngles,axis);VectorCopy(e->currentOrigin,out.v);
  for(int a=0;a<3;++a){VectorMA(out.v,(e->pos1[a]+e->pos2[a])*.5f,axis[a],out.v);VectorCopy(axis[a],out.v+4+4*a);out.v[7+4*a]=(e->pos2[a]-e->pos1[a])*.5f;}
  out.v[3]=s.kind==Exposure?float(s.mode+1):0;
  if(s.kind==WindZone){vec3_t angles;VectorCopy(e->currentAngles,angles);float strength=s.speed,t=(level.time%1000000)*.001f;if(s.mode==1)strength*=.5f+.5f*std::sin(t*1.7f);if(s.mode==2)angles[YAW]+=std::sin(t*.7f)*90;AngleVectors(angles,out.v+16,nullptr,nullptr);VectorScale(out.v+16,strength,out.v+16);}
  if(Valid(out))++z.count;
 }return z;
}
