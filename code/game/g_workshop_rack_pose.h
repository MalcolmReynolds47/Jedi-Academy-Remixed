// Transport stock pickups with their portable rack. GPL-2.0-only.
#ifndef G_WORKSHOP_RACK_POSE_H
#define G_WORKSHOP_RACK_POSE_H

#include <array>
#include <cstring>

inline bool workshopRack(const gentity_t& rack){
 return rack.classname&&(!std::strcmp(rack.classname,"misc_model_gun_rack")||
                        !std::strcmp(rack.classname,"misc_model_ammo_rack"));
}

struct RackStockMove {
 gentity_t* item;
 vec3_t origin,angles;
};
struct RackStockPlan {
 std::array<RackStockMove,5> members;
 std::size_t count=0;
};

inline void commitRackStock(const RackStockPlan& plan){
 for(std::size_t i=0;i<plan.count;++i){
  const auto& move=plan.members[i];
  G_SetOrigin(move.item,move.origin);
  G_SetAngles(move.item,move.angles);
  VectorCopy(move.item->currentOrigin,move.item->s.origin);
  VectorCopy(move.item->currentAngles,move.item->s.angles);
  gi.linkentity(move.item);
 }
}

#endif
