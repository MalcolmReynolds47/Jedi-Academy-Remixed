// Shared with the headless callback fixture. Native map-authored use is separate.
#include "g_level_change.h"
extern void G_ChangeMap(const char*,const char*,qboolean);
void G_LevelChangeUse(gentity_t*self){
 if(!self||!G_LevelChangePortable(*self))return;
 const std::string identity=self->targetname?self->targetname:"";
 G_ActivateBehavior(self,BSET_USE);
 jar::levelChange::Settings s;
 if(!G_LevelChangeRead(*self,s)||identity!=(self->targetname?self->targetname:""))return;
 if(!G_LevelChangeAvailable(s)){gi.cvar_set("jar_workshopStatus","Destination map is unavailable or invalid. Set an installed map in Context > Travel.");return;}
 G_ChangeMap(s.map.c_str(),s.spawn.c_str(),s.hub?qtrue:qfalse);
}
