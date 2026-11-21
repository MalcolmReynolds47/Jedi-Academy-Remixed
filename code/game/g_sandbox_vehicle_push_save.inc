// Save the horizontal component separately from native playerState velocity.
// GPL-2.0-only.
#include "g_sandbox_vehicle_push.h"

static Vehicle_t* VehiclePushSaveHull(gentity_t& entity){
 if(!entity.inuse||!entity.client||entity.client->NPC_class!=CLASS_VEHICLE||!entity.m_pVehicle||
    entity.m_pVehicle->m_pParentEntity!=&entity||!entity.m_pVehicle->m_pVehicleInfo)return nullptr;
 return entity.m_pVehicle;
}

static bool VehiclePushHasDrift(const Vehicle_t* vehicle){
 return vehicle&&G_VehiclePushDriftValid(vehicle->jarPushDrift)&&VectorLengthSquared(vehicle->jarPushDrift)>0;
}

static bool G_VehiclePushPresent(){
 const int limit=std::min(globals.num_entities,int(ENTITYNUM_WORLD));
 for(int slot=MAX_CLIENTS;slot<limit;++slot)if(VehiclePushHasDrift(VehiclePushSaveHull(g_entities[slot])))return true;
 return false;
}

static void G_VehiclePushExport(bool autosave,ojk::SavedGameHelper& save){
 const int limit=std::min(globals.num_entities,int(ENTITYNUM_WORLD));
 std::vector<uint16_t> live;
 if(!autosave)for(int slot=MAX_CLIENTS;slot<limit;++slot){
  if(VehiclePushHasDrift(VehiclePushSaveHull(g_entities[slot])))live.push_back(uint16_t(slot));
 }
 save.write<uint32_t>(uint32_t(live.size()));
 for(const auto slot:live){
  const auto& drift=g_entities[slot].m_pVehicle->jarPushDrift;
  save.write<uint16_t>(slot);save.write<float>(drift[0]);save.write<float>(drift[1]);
 }
}

static bool G_VehiclePushImport(bool autosave,ojk::SavedGameHelper& save){
 uint32_t count;save.read<uint32_t>(count);
 if(count>=ENTITYNUM_WORLD||(autosave&&count))return false;
 const int limit=std::min(globals.num_entities,int(ENTITYNUM_WORLD));
 struct SavedDrift {Vehicle_t* vehicle;vec3_t drift;};
 std::vector<SavedDrift> rows;
 rows.reserve(count);
 uint16_t previous=MAX_CLIENTS-1;
 for(uint32_t i=0;i<count;++i){
  uint16_t slot;save.read<uint16_t>(slot);
  vec3_t drift{};save.read<float>(drift[0]);save.read<float>(drift[1]);
  if(slot<=previous||slot>=limit||g_entities[slot].s.number!=slot||!G_VehiclePushDriftValid(drift))return false;
  auto* vehicle=VehiclePushSaveHull(g_entities[slot]);
  if(!vehicle)return false;
  rows.push_back({vehicle,{drift[0],drift[1],0}});previous=slot;
 }
 save.ensure_all_data_read();
 for(int slot=MAX_CLIENTS;slot<limit;++slot)if(auto* vehicle=VehiclePushSaveHull(g_entities[slot]))
  VectorClear(vehicle->jarPushDrift);
 for(const auto& row:rows)VectorCopy(row.drift,row.vehicle->jarPushDrift);
 return true;
}
