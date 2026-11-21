// The existing saved ownership links identify captured rack stock. GPL-2.0-only.
#include "g_workshop_rack_pose.h"
#include "qcommon/jar_angular_trajectory.h"
#include <cmath>

bool rackStockChild(const gentity_t& item){
 if(!item.item||item.s.eType!=ET_ITEM)return false;
 const auto* member=owned(item.s.number);
 const auto* parent=member&&member->parent?OwnedId(state,member->parent):nullptr;
 return parent&&matches(*member)&&matches(*parent)&&workshopRack(g_entities[parent->entity]);
}

void rackStockLoose(gentity_t& item){
 if((item.flags&FL_FORCE_PULLABLE_ONLY)&&rackStockChild(item))
  item.flags&=~FL_FORCE_PULLABLE_ONLY;
}

bool rackStockSuspended(const gentity_t& item){
 // Native rack stock from older saves can retain the allocator's zero ground slot.
 return item.s.eType==ET_ITEM&&item.item&&item.linked&&
        (item.flags&FL_FORCE_PULLABLE_ONLY)&&item.s.pos.trType==TR_STATIONARY&&
        (item.s.groundEntityNum==ENTITYNUM_NONE||item.s.groundEntityNum==0);
}

bool rackStockAttached(const gentity_t& item){
 return rackStockSuspended(item)&&rackStockChild(item);
}

bool planRackStock(const gentity_t& rack,const vec3_t origin,const vec3_t angles,RackStockPlan& plan){
 if(!workshopRack(rack))return true;
 const bool turn=!VectorCompare(angles,rack.currentAngles);
 if(!turn&&VectorCompare(origin,rack.currentOrigin))return true;
 const auto* parent=owned(rack.s.number);
 if(!parent||!matches(*parent))return true;
 for(unsigned i=0;i<state.ownedCount;++i){
  const auto& member=state.owned[i];
  if(member.parent!=parent->id||!matches(member))continue;
  auto& item=g_entities[member.entity];
  if(!rackStockSuspended(item))continue;
  if(plan.count==plan.members.size())return false;
  auto& move=plan.members[plan.count++];
  move.item=&item;
  vec3_t offset;
  VectorSubtract(item.currentOrigin,rack.currentOrigin,offset);
  if(turn)jar::angular::RebaseVector(offset,rack.currentAngles,angles,offset);
  VectorAdd(origin,offset,move.origin);
  if(turn)jar::angular::RebaseAngles(item.currentAngles,rack.currentAngles,angles,move.angles);
  else VectorCopy(item.currentAngles,move.angles);
  for(float axis:move.origin)if(!std::isfinite(axis)||std::fabs(axis)>=65535)return false;
 }
 return true;
}

bool rackStockPose(gentity_t& rack,const vec3_t origin,const vec3_t angles,bool sweep,RackStockPlan& plan){
 if(!planRackStock(rack,origin,angles,plan))return false;
 if(!plan.count)return true;

 // Stock hangs inside the rack's clip bounds. Exclude that specific
 // rack while validating the items against external geometry and actors.
 const bool linked=rack.linked;
 if(linked)gi.unlinkentity(&rack);
 bool clear=true;
 for(std::size_t i=0;i<plan.count&&clear;++i){
  const auto& move=plan.members[i];
  const auto& item=*move.item;
  trace_t hit{};
  gi.trace(&hit,sweep?item.currentOrigin:move.origin,item.mins,item.maxs,move.origin,
           item.s.number,MASK_SOLID|CONTENTS_BODY,G2_NOCOLLIDE,0);
  clear=!hit.startsolid&&!hit.allsolid&&hit.fraction==1;
 }
 if(linked)gi.linkentity(&rack);
 return clear;
}

bool rackStockMove(gentity_t& rack,const vec3_t origin,const vec3_t angles){
 RackStockPlan stock;
 if(!rackStockPose(rack,origin,angles,true,stock))return false;
 commitRackStock(stock);
 return true;
}
