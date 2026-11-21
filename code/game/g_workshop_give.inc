// Sandbox Give links retain their native pickups until collection or unlink.
#include "qcommon/jar_pickup_lifetime.h"
namespace givePickup {
constexpr int reservedDeadline=jar::pickupLifetime::giveDeadline;
constexpr int expiryMs=jar::pickupLifetime::defaultMs;

void updateExpiry(gentity_t& pickup,bool linked){
 if(pickup.e_ThinkFunc!=thinkF_G_FreeEntity)return;
 if(linked&&G_LogicPickup(pickup)&&(pickup.nextthink>=0||pickup.nextthink==reservedDeadline)){
  // Legacy Give could leave a rejected item unlinked with a zero deadline.
  if(!pickup.linked)gi.linkentity(&pickup);
  if(pickup.nextthink!=reservedDeadline){
   const int clock=pickupLifetimeClock(pickup);
   pickup.setTime=pickup.nextthink>0?std::max(1,std::min(jar::pickupLifetime::maxMs,pickup.nextthink-clock)):expiryMs;
  }
  pickup.nextthink=reservedDeadline;
 }else if(!linked&&pickup.nextthink==reservedDeadline){
  const int remaining=pickup.setTime>0&&pickup.setTime<=jar::pickupLifetime::maxMs?pickup.setTime:expiryMs;
  pickup.nextthink=pickupLifetimeClock(pickup)+remaining;
  pickup.setTime=0;
 }
}

bool hasLink(const gentity_t& source,const char* name){
 if(G_LogicKind(source)!=jar::logic::Give||source.e_UseFunc!=useF_Use_Target_Give)return false;
 for(int slot=0;slot<jar::logic::outputCount;++slot){
  const char* output=G_LogicOutput(source,slot);
  if(output&&!strcmp(output,name))return true;
 }
 return false;
}

void sync(const char* name){
 auto* pickup=name&&*name?G_Find(nullptr,FOFS(targetname),name):nullptr;
 const auto* owner=pickup?owned(pickup->s.number):nullptr;
 if(!owner||!matches(*owner))return;
 for(unsigned i=0;i<state.ownedCount;++i){
  const auto& candidate=state.owned[i];
  const auto& source=g_entities[candidate.entity];
  if(hasLink(source,name)&&matches(candidate)){updateExpiry(*pickup,true);return;}
 }
 updateExpiry(*pickup,false);
}

void restore(){
 std::set<std::string> links;
 for(unsigned i=0;i<state.ownedCount;++i){
  const auto& record=state.owned[i];
  const auto& source=g_entities[record.entity];
  if(G_LogicKind(source)!=jar::logic::Give||source.e_UseFunc!=useF_Use_Target_Give||!matches(record))continue;
  for(int slot=0;slot<jar::logic::outputCount;++slot){
   if(const char* name=G_LogicOutput(source,slot))links.emplace(name);
  }
 }
 for(unsigned i=0;i<state.ownedCount;++i){
  const auto& record=state.owned[i];
  auto& pickup=g_entities[record.entity];
  if(pickup.item&&pickup.targetname&&matches(record))updateExpiry(pickup,links.count(pickup.targetname)!=0);
 }
}
}

void setSlottedLink(gentity_t& source,int slot,const char* name){
 if(G_LogicKind(source)!=jar::logic::Give){G_LogicSetOutput(source,slot,name);return;}
 const char* before=G_LogicOutput(source,slot);
 const std::string oldName=before?before:"";
 G_LogicSetOutput(source,slot,name);
 givePickup::sync(oldName.c_str());
 givePickup::sync(G_LogicOutput(source,slot));
}
