// Single-player hull and native seat relocation. Included by the Sandbox
// teleporter runtime to share its clearance, cooldown and arrival tracking.
// GPL-2.0-only.
bool PlanVehicleTeleport(gentity_t& hull,const gentity_t& target,const jar::teleport::Settings& settings,
 VehicleTeleportPlan& plan){
 const auto& vehicle=*hull.m_pVehicle;
 if(&target==vehicle.m_pPilot||&target==vehicle.m_pDroidUnit)return false;
 vec3_t origin,angles;
 if(!DestinationPosition(target,hull,origin))return false;
 origin[2]+=1;
 VectorCopy(settings.keepView?vehicle.m_vOrientation:target.currentAngles,angles);
 if(!settings.keepView&&vehicle.m_pVehicleInfo->type!=VH_FIGHTER)angles[PITCH]=angles[ROLL]=0;
 if(!G_PlanVehicleTeleportAt(hull,origin,angles,settings.keepView,settings.keepVelocity,
    cg.time?cg.time:level.time,plan))return false;
 for(int i=0;i<plan.count;++i)if(TeleportedThisFrame(*plan.members[i].entity))return false;
 return true;
}

bool VehicleTeleportClear(const VehicleTeleportPlan& plan){
 VehicleTeleportCollisionScope scope(plan);
 for(int i=0;i<plan.count;++i){
  const auto& member=plan.members[i];
  if(!TeleportClear(*member.entity,member.position,MASK_PLAYERSOLID|member.entity->clipmask))return false;
 }
 return true;
}

bool TeleportVehicle(gentity_t& source,gentity_t& hull,gentity_t* activator,bool use){
 jar::teleport::Settings settings;
 if(!G_TeleportRead(source,settings)||!settings.enabled||!settings.vehicles)return false;
 if(!G_TransportVehicleReady(hull))
  return !use&&settings.props&&settings.assemblies&&G_JAR_WorkshopTeleportAssembly(source,hull);
 VehicleTeleportPlan plan;
 const auto prepare=[&]{
  const auto* target=G_TeleportDestination(source);
  return target&&PlanVehicleTeleport(hull,*target,settings,plan)&&VehicleTeleportClear(plan);
 };
 if(!prepare()||!G_ContactClockReady(source,hull,settings.interval))return false;
 if(use&&source.behaviorSet[BSET_USE]){
  const auto sourceLife=G_EntityLifetime(&source),hullLife=G_EntityLifetime(&hull);
  const auto activatorLife=G_EntityLifetime(activator);
  const auto* pilot=hull.m_pVehicle->m_pPilot;
  const auto* droid=hull.m_pVehicle->m_pDroidUnit;
  const auto pilotLife=pilot?G_EntityLifetime(pilot):0;
  const auto droidLife=droid?G_EntityLifetime(droid):0;
  G_ActivateBehavior(&source,BSET_USE);
  if(G_EntityLifetime(&source)!=sourceLife||G_EntityLifetime(&hull)!=hullLife||
     G_EntityLifetime(activator)!=activatorLife||G_TransportVehicleHull(activator)!=&hull||
     hull.m_pVehicle->m_pPilot!=pilot||hull.m_pVehicle->m_pDroidUnit!=droid||
     (pilot&&G_EntityLifetime(pilot)!=pilotLife)||(droid&&G_EntityLifetime(droid)!=droidLife)||
     !G_TeleportRead(source,settings)||!settings.enabled||!settings.vehicles||!G_TransportVehicleReady(hull)||
     !prepare())return false;
 }
 G_CommitVehicleTeleport(plan,settings.keepVelocity);
 return true;
}
