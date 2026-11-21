// Property/ordering service shared by production and fixtures. GPL-2.0-only.
#include "g_weather.h"
#include <climits>
#include <vector>
bool G_WeatherPortable(const gentity_t&e){return e.inuse&&jar::weather::Class(e.classname)!=jar::weather::Invalid&&e.model&&!strcmp(e.model,jar::weather::ZoneClass(e.classname)?"@jar_box":"@jar_weather");}
int G_WeatherZoneCount(const gentity_t*exclude){int count=0;for(int i=1;i<globals.num_entities;++i)if(&g_entities[i]!=exclude&&G_WeatherPortable(g_entities[i])&&jar::weather::ZoneClass(g_entities[i].classname))++count;return count;}
bool G_WeatherRead(const gentity_t&e,jar::weather::Settings&s){
 if(!G_WeatherPortable(e))return false;s.kind=jar::weather::Class(e.classname);s.mode=e.spawnflags&7;s.enabled=!(e.spawnflags&jar::weather::offFlag);s.density=e.random;s.size=e.radius;s.alpha=e.wait;s.speed=e.speed;return jar::weather::Valid(s);
}
static void weatherRaise(gentity_t&e){
 int greatest=0;for(int i=1;i<globals.num_entities;++i)if(G_WeatherPortable(g_entities[i]))greatest=std::max(greatest,g_entities[i].delay);
 if(greatest>=INT_MAX-1){std::vector<gentity_t*> order;for(int i=1;i<globals.num_entities;++i)if(G_WeatherPortable(g_entities[i]))order.push_back(&g_entities[i]);std::stable_sort(order.begin(),order.end(),[](const gentity_t*a,const gentity_t*b){return a->delay<b->delay;});greatest=0;for(auto*p:order)p->delay=++greatest;}
 e.delay=greatest+1;
}
bool G_WeatherSet(gentity_t&e,const jar::weather::Settings&s,bool prioritize){
 if(!G_WeatherPortable(e)||jar::weather::Class(e.classname)!=s.kind||!jar::weather::Valid(s))return false;
 e.spawnflags=(e.spawnflags&~(7|jar::weather::offFlag))|s.mode|(s.enabled?0:jar::weather::offFlag);e.random=s.density;e.radius=s.size;e.wait=s.alpha;e.speed=s.speed;if(prioritize&&s.enabled)weatherRaise(e);return true;
}
