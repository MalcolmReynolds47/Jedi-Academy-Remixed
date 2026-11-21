#pragma once
#include "qcommon/jar_rail.h"
inline bool G_RailPortable(const gentity_t&e){return e.inuse&&jar::rail::Class(e.classname)!=jar::rail::None&&e.model&&!strcmp(e.model,"@jar_rail");}
inline bool G_RailRead(const gentity_t&e,jar::rail::Settings&s){
 if(!G_RailPortable(e))return false;s.kind=jar::rail::Class(e.classname);s.delay=e.wait;s.enabled=!(e.spawnflags&2);s.center=(e.spawnflags&1)!=0;
 if(s.kind==jar::rail::Track){s.length=e.radius;s.width=e.random;s.cell=e.mass;s.speed=e.speed;s.count=e.health;}
 if(s.kind==jar::rail::Lane)s.width=e.radius;
 return jar::rail::Valid(s);
}
inline bool G_RailSet(gentity_t&e,const jar::rail::Settings&s){
 if(!G_RailPortable(e)||s.kind!=jar::rail::Class(e.classname)||!jar::rail::Valid(s))return false;
 e.delay=level.time+int(s.delay*1000);e.wait=s.delay;e.spawnflags=(e.spawnflags&~3)|(s.center?1:0)|(s.enabled?0:2);
 if(s.kind==jar::rail::Track){e.radius=s.length;e.random=s.width;e.mass=s.cell;e.speed=s.speed;e.health=s.count;}
 if(s.kind==jar::rail::Lane)e.radius=s.width;
 return true;
}
bool G_RailSpawn(gentity_t&);
bool G_RailMoved(gentity_t&);
bool G_RailCanMove(const gentity_t&,const vec3_t destination);
void G_RailPortableFrame();
bool G_RailRestore();
bool G_RailPreview(const gentity_t&,vec3_t corners[4]);
void portable_rail_use(gentity_t*,gentity_t*,gentity_t*);
