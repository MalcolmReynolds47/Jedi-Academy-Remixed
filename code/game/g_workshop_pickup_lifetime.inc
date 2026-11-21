// Use the native dropped-item deadline, including its save/load behavior.
#include "qcommon/jar_pickup_lifetime.h"

bool pickupLifetimeEditable(const gentity_t& item){
 const auto* record=owned(item.s.number);
 return record&&record->catalog!=consoleCatalog&&!record->parent&&matches(*record)&&
  item.s.eType==ET_ITEM&&item.item&&(item.flags&FL_DROPPED_ITEM)&&
  !(item.s.eFlags&EF_NODRAW)&&!item.freeAfterEvent&&item.e_ThinkFunc==thinkF_G_FreeEntity&&
  (item.nextthink>0||item.nextthink==jar::pickupLifetime::keepDeadline);
}
int pickupLifetimeClock(const gentity_t& item){
 const auto control=nativeControls.find(item.s.number);
 return control==nativeControls.end()?level.time:control->second.started;
}
int pickupLifetimeRemaining(const gentity_t& item){
 if(item.nextthink==jar::pickupLifetime::keepDeadline)return 0;
 return std::max(1,std::min(jar::pickupLifetime::maxMs,item.nextthink-pickupLifetimeClock(item)));
}
void pickupLifetimeSet(gentity_t& item,int remaining){
 item.nextthink=remaining?pickupLifetimeClock(item)+remaining:jar::pickupLifetime::keepDeadline;
}
