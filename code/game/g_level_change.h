// Existing saved message/target/spawnflags fields own all portable settings.
// GPL-2.0-only. Include after g_local.h.
#ifndef G_LEVEL_CHANGE_H
#define G_LEVEL_CHANGE_H
#include "g_functions.h"
#include "qcommon/jar_level_change.h"
inline bool G_LevelChangePortable(const gentity_t&e){return e.inuse&&jar::levelChange::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_level_change");}
inline bool G_LevelChangeAvailable(const jar::levelChange::Settings&s){return jar::levelChange::Valid(s)&&gi.FS_ReadFile(("maps/"+s.map+".bsp").c_str(),nullptr)>0;}
inline bool G_LevelChangeRead(const gentity_t&e,jar::levelChange::Settings&out){
 if(!G_LevelChangePortable(e))return false;
 jar::levelChange::Settings s;s.map=e.message?e.message:"";s.spawn=e.target?e.target:"";s.hub=(e.spawnflags&1)!=0;
 if(!jar::levelChange::Valid(s))return false;out=std::move(s);return true;
}
inline bool G_LevelChangeSet(gentity_t&e,const jar::levelChange::Settings&s){
 if(!G_LevelChangePortable(e)||!G_LevelChangeAvailable(s))return false;
 char*map=G_NewString(s.map.c_str());char*spawn=s.spawn.empty()?nullptr:G_NewString(s.spawn.c_str());
 if(e.message&&gi.bIsFromZone(e.message,TAG_G_ALLOC))gi.Free(e.message);
 if(e.target&&gi.bIsFromZone(e.target,TAG_G_ALLOC))gi.Free(e.target);
 e.message=map;e.target=spawn;e.spawnflags=s.hub?e.spawnflags|1:e.spawnflags&~1;return true;
}
inline bool G_LevelChangeSpawn(gentity_t&e){
 jar::levelChange::Settings s;if(!G_LevelChangeRead(e,s)||!G_LevelChangeAvailable(s))return false;
 e.count=-1;e.noise_index=0;G_SetOrigin(&e,e.s.origin);e.e_UseFunc=useF_target_level_change_use;return true;
}
#endif
