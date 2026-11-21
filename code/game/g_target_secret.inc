// Raven native Secret callback with independent Sandbox discovery state.
#include "g_sandbox_secret.h"

void target_secret_use(gentity_t* self,gentity_t* other,gentity_t* activator)
{
 if(G_SandboxSecret(*self)){
  if((self->svFlags&SVF_INACTIVE)||(self->spawnflags&JAR_SECRET_FOUND)||self->painDebounceTime>level.time)return;
  auto* player=&g_entities[0];
  if(!player->inuse||!player->client||player->health<=0)return;
  if(self->spawnflags&JAR_SECRET_REPEAT)self->painDebounceTime=level.time+1;
  else self->spawnflags|=JAR_SECRET_FOUND;
  G_Sound(player,self->noise_index);
  gi.SendServerCommand(0,"cp @SP_INGAME_SECRET_AREA");
  G_UseTargets(self,player);
  return;
 }
 // Authored secrets retain the campaign tally and native notification.
 auto& stats=level.clients[0].sess.missionStats;
 ++stats.secretsFound;
 G_Sound(activator?activator:self,self->noise_index);
 gi.SendServerCommand(0,"cp @SP_INGAME_SECRET_AREA");
 if(stats.secretsFound>stats.totalSecrets)++stats.totalSecrets;
}
