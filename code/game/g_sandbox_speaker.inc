// Runtime playback for owned speakers. Included by g_target.cpp. GPL-2.0-only.
#include "g_sandbox_speaker.h"

namespace {
void speakerUse(gentity_t& e,gentity_t* activator){
 using namespace jar::speaker;
 Settings settings;
 if(!G_SandboxSpeakerRead(e,settings)||!settings.enabled)return;
 if(settings.mode==Loop||settings.mode==Repeat){
  settings.active=!settings.active;
  settings.remaining=Timed(settings)?settings.period:0;
  if(!G_SandboxSpeakerSet(e,settings)||!Timed(settings))return;
  G_AddEvent(&e,EV_GENERAL_SOUND,e.noise_index);
  return;
 }
 if(e.painDebounceTime>level.time)return;
 gentity_t* receiver=&e;
 if(settings.mode==Activator){
  receiver=activator&&activator->inuse&&activator->client&&!activator->freeAfterEvent?activator:&g_entities[0];
  if(!receiver->inuse||!receiver->client)return;
 }
 G_AddEvent(receiver,settings.mode==Global?EV_GLOBAL_SOUND:EV_GENERAL_SOUND,e.noise_index);
 e.painDebounceTime=level.time+oneShotCooldown;
}
}

void G_SandboxSpeakerTick(gentity_t& e){
 using namespace jar::speaker;
 if(e.nextthink<=0||e.nextthink>level.time)return;
 Settings settings;
 if(!G_SandboxSpeakerRead(e,settings)||!settings.enabled||!Timed(settings)){
  e.nextthink=0;
  return;
 }
 G_AddEvent(&e,EV_GENERAL_SOUND,e.noise_index);
 const int variation=settings.variation?Q_irand(-settings.variation,settings.variation):0;
 e.nextthink=level.time+settings.period+variation;
}
