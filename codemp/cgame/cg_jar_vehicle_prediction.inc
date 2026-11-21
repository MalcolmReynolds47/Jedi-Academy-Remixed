// A held/frozen vehicle follows its authoritative pose, including for its pilot.
// Native Pmove changes view angles before its PM_FREEZE return. GPL-2.0-only.
static qboolean JAR_FrozenVehiclePrediction(pmove_t*move,bgEntity_t*vehicle,bgEntity_t*pilot){
 if(move->ps->pm_type!=PM_FREEZE)return qfalse;
 if(move->cmd.serverTime>move->ps->commandTime)move->ps->commandTime=move->cmd.serverTime;
 if(vehicle&&vehicle->ghoul2&&vehicle->m_pVehicle&&vehicle->m_pVehicle->m_pVehicleInfo&&pilot){
  Vehicle_t*v=vehicle->m_pVehicle;v->m_pPilot=pilot;
  if(v->m_pVehicleInfo->AttachRiders)v->m_pVehicleInfo->AttachRiders(v);
 }
 return qtrue;
}
