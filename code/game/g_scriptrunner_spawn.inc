// Native spawn defaults shared with the portable setup fixture. GPL-2.0-only.
#include <algorithm>
#include <cmath>
#include <climits>
/*QUAKED target_scriptrunner (1 0 0) (-4 -4 -4) (4 4 4) runonactivator x x x x x x INACTIVE
Usescript: native ICARUS script. Count: uses (-1 infinite, default 1).
Wait: cooldown seconds. Delay: seconds before running. Flag 1 runs on activator.
*/
void SP_target_scriptrunner(gentity_t*self){
 if(!self->behaviorSet[BSET_USE])gi.Printf(S_COLOR_RED "SP_target_scriptrunner has no USESCRIPT\n");
 if(self->spawnflags&128)self->svFlags|=SVF_INACTIVE;
 if(!self->count)self->count=1;
 self->radius=0;G_SpawnFloat("delay","0",&self->radius);
 self->delay=std::isfinite(self->radius)?int(std::max(0.,std::min(double(INT_MAX),double(self->radius)*1000))):0;
 self->wait*=1000;
 G_SetOrigin(self,self->s.origin);self->e_UseFunc=useF_target_scriptrunner_use;
}
