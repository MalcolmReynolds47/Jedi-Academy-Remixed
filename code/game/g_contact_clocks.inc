// Sparse native contact clocks, owned by the active SP level. GPL-2.0-only.
#include "g_contact_clocks.h"
#include "g_contact_trigger.h"
#include "g_sandbox_push.h"
#include "g_sandbox_teleport.h"
#include "g_logic_settings.h"
#include "g_subbsp_ownership.h"
#include "qcommon/ojk_saved_game_helper.h"
#include <map>

namespace {
constexpr uint32_t contactClockLimit=65536;
using ContactPair=std::pair<uint16_t,uint16_t>;
struct ContactClock {
 uint64_t triggerLife,actorLife;
 int64_t deadline;
};
std::map<ContactPair,ContactClock> contactClocks;
int64_t nextContactSweep;

bool ContactEndpoints(ContactPair pair){
 if(!pair.first||pair.first==pair.second||pair.first>=globals.num_entities||pair.second>=globals.num_entities||
    pair.first>=ENTITYNUM_WORLD||pair.second>=ENTITYNUM_WORLD)return false;
 const auto& trigger=g_entities[pair.first];
 const auto& subject=g_entities[pair.second];
 if(trigger.s.number!=pair.first||subject.s.number!=pair.second)return false;
 if(G_ContactKind(trigger)==jar::contactTrigger::Hurt)return G_LogicLiveActor(&subject)&&subject.takedamage;
 if(const auto teleport=G_TeleportKind(trigger))return G_LogicLiveActor(&subject)||
  (teleport==jar::teleport::Volume&&G_TeleportObjectKind(subject));
 const auto push=G_PushKind(trigger);
 return push&&(G_LogicLiveActor(&subject)||(push==jar::push::Volume&&subject.inuse&&
  subject.classname&&!strcmp(subject.classname,"jar_sandbox_prop")));
}

bool ContactClockLive(ContactPair pair,const ContactClock& clock){
 return clock.deadline>level.time&&ContactEndpoints(pair)&&
  clock.triggerLife==G_EntityLifetime(&g_entities[pair.first])&&
  clock.actorLife==G_EntityLifetime(&g_entities[pair.second]);
}

ContactClock ContactDeadline(ContactPair pair,int remaining){
 return {G_EntityLifetime(&g_entities[pair.first]),G_EntityLifetime(&g_entities[pair.second]),
  int64_t(level.time)+remaining};
}
}

bool G_ContactClockReady(const gentity_t& trigger,const gentity_t& actor,int interval){
 if(trigger.s.number<=0||trigger.s.number>=ENTITYNUM_WORLD||actor.s.number<0||actor.s.number>=ENTITYNUM_WORLD||
    &trigger!=&g_entities[trigger.s.number]||&actor!=&g_entities[actor.s.number]||
    interval<jar::contactTrigger::minInterval||interval>jar::contactTrigger::maxTime)return false;
 const ContactPair pair{uint16_t(trigger.s.number),uint16_t(actor.s.number)};
 if(!ContactEndpoints(pair))return false;
 auto entry=contactClocks.find(pair);
 if(entry!=contactClocks.end()){
  if(ContactClockLive(pair,entry->second))return false;
  entry->second=ContactDeadline(pair,interval);
  return true;
 }
 if(level.time>=nextContactSweep){
  for(auto it=contactClocks.begin();it!=contactClocks.end();){
   if(!ContactClockLive(it->first,it->second))it=contactClocks.erase(it);
   else ++it;
  }
  nextContactSweep=int64_t(level.time)+1000;
 }
 if(contactClocks.size()>=contactClockLimit)return false;
 contactClocks.emplace(pair,ContactDeadline(pair,interval));
 return true;
}

void G_ContactClocksReset(){contactClocks.clear();nextContactSweep=0;}

void G_ContactClocksExport(bool autosave,ojk::SavedGameHelper& save){
 uint32_t count=0;
 if(!autosave){
  for(const auto& entry:contactClocks)if(ContactClockLive(entry.first,entry.second))++count;
 }
 save.write<uint32_t>(count);
 if(!autosave)for(const auto& entry:contactClocks)if(ContactClockLive(entry.first,entry.second)){
  save.write<uint16_t>(entry.first.first);save.write<uint16_t>(entry.first.second);
  save.write<uint32_t>(uint32_t(entry.second.deadline-level.time));
 }
}

bool G_ContactClocksImport(bool autosave,ojk::SavedGameHelper& save){
 uint32_t count;save.read<uint32_t>(count);
 if(count>contactClockLimit||(autosave&&count))return false;
 decltype(contactClocks) candidate;
 for(uint32_t i=0;i<count;++i){
  ContactPair pair;uint32_t remaining;
  save.read<uint16_t>(pair.first);save.read<uint16_t>(pair.second);save.read<uint32_t>(remaining);
  if(!remaining||remaining>jar::contactTrigger::maxTime||!ContactEndpoints(pair)||
     !candidate.emplace(pair,ContactDeadline(pair,int(remaining))).second)return false;
 }
 contactClocks.swap(candidate);nextContactSweep=0;
 return true;
}
