// Only one speculative warp is applied per replay. Server snapshots own
// events, visibility and reconciliation; no temporary entities are predicted.
// GPL-2.0-only.
#include "../game/jar_mp_teleport_volume.h"
static qboolean jarTeleportPredicted;
static qboolean jarVehicleTeleportPredicted;
static qboolean CG_JAR_TeleportArrivalClear(const vec3_t origin, const vec3_t mins, const vec3_t maxs,
                                           int vehicle, int pilot);
static void CG_JAR_PredictTeleportVolume(const entityState_t*s,const vec3_t origin,const vec3_t final){
 playerState_t*ps=&cg.predictedPlayerState;trace_t tr;
 if(jarTeleportPredicted||!cg.snap||!cg_pmove.trace||!trap->R_InPVS||
    !trap->R_InPVS(cg.snap->ps.origin,final,cg.snap->areamask))return;
 cg_pmove.trace(&tr,final,cg_pmove.mins,cg_pmove.maxs,final,ps->clientNum,MASK_PLAYERSOLID);
 if(tr.startsolid||tr.allsolid)return;
 JAR_TeleportVolumeApply(s,ps,&cg_pmove.cmd,origin);jarTeleportPredicted=qtrue;
}
static qboolean CG_JAR_PortalDiscontinuity(const playerState_t*old,qboolean oldSpeculative){
 return (jarTeleportPredicted||oldSpeculative)&&((old->eFlags^cg.predictedPlayerState.eFlags)&EF_TELEPORT_BIT);
}

static qboolean CG_JAR_VehiclePortalDiscontinuity(const playerState_t *old, qboolean oldSpeculative) {
 const playerState_t *current = &cg.predictedVehicleState;
 return (jarVehicleTeleportPredicted || oldSpeculative) &&
        (old->clientNum != current->clientNum || ((old->eFlags ^ current->eFlags) & EF_TELEPORT_BIT));
}

// The server checks the rider before stepping the vehicle. Keep that earlier
// position, since the nested vehicle Pmove can attach the rider elsewhere.
static qboolean CG_JAR_VehicleTeleportRiderBlocked(const playerState_t *pilot, const vec3_t mins, const vec3_t maxs) {
 if (cgs.jarMPVersion < JAR_MP_VEHICLE_TELEPORT_FEATURE || jarTeleportPredicted) return qfalse;

 qboolean grant = qfalse;
 for (int i = 0; i < cg_numTriggerEntities; ++i) {
  const centity_t *trigger = cg_triggerEntities[i];
  const entityState_t *s = &trigger->currentState;
  if (trigger->currentValid && JAR_TeleportVolumeValid(s) && s->otherEntityNum2 == pilot->m_iVehicleNum &&
      JAR_PushVolumePilotAllowed(s, pilot->clientNum)) {
   grant = qtrue;
   break;
  }
 }
 if (!grant) return qfalse;

 for (int i = 0; i < cg_numTriggerEntities; ++i) {
  const centity_t *trigger = cg_triggerEntities[i];
  const entityState_t *s = &trigger->currentState;
  if (!trigger->currentValid || (s->eType != ET_TELEPORT_TRIGGER && s->eType != ET_PUSH_TRIGGER)) continue;
  if (s->eType == ET_TELEPORT_TRIGGER && s->generic1 == JAR_MP_TELEPORT_VOLUME) {
   if (JAR_TeleportVolumeValid(s) && JAR_TeleportVolumeNativeContact(s, pilot->origin, mins, maxs)) return qtrue;
  } else if (s->eType == ET_PUSH_TRIGGER && s->generic1 == JAR_MP_PUSH_VOLUME) {
   if (JAR_PushVolumeValid(s)) {
    vec3_t origin, angles;
    JAR_PushVolumePose(s, pilot->commandTime, origin, angles);
    if (JAR_VolumeContact(origin, angles, (float)s->modelindex, pilot->origin, mins, maxs)) return qtrue;
   }
  } else if (s->solid == SOLID_BMODEL && s->modelindex > 0 && s->modelindex < cgs.numInlineModels) {
   const clipHandle_t model = trap->CM_InlineModel(s->modelindex);
   if (model) {
    vec3_t origin, angles;
    trace_t trace;
    BG_EvaluateTrajectory(&s->pos, cg.physicsTime, origin);
    BG_EvaluateTrajectory(&s->apos, cg.physicsTime, angles);
    trap->CM_TransformedTrace(&trace, pilot->origin, pilot->origin, mins, maxs, model, -1, origin, angles, 0);
    if (trace.startsolid || trace.allsolid) return qtrue;
   }
  }
 }
 return qfalse;
}

static void CG_JAR_PredictVehicleTeleportVolumes(pmove_t *move, const centity_t *vehicle) {
 playerState_t *hull = move->ps;
 playerState_t *pilot = &cg.predictedPlayerState;
 if (cgs.jarMPVersion < JAR_MP_VEHICLE_TELEPORT_FEATURE || jarTeleportPredicted || cg.hyperspace ||
     !cg.snap || !trap->R_InPVS || !vehicle->currentValid || vehicle->currentState.eType != ET_NPC ||
     vehicle->currentState.NPC_class != CLASS_VEHICLE || vehicle->currentState.number != hull->clientNum ||
     vehicle->currentState.owner != pilot->clientNum || vehicle->currentState.m_iVehicleNum != pilot->clientNum + 1 ||
     cg.snap->ps.clientNum != pilot->clientNum || cg.snap->ps.m_iVehicleNum != hull->clientNum ||
     cg.snap->vps.clientNum != hull->clientNum || cg.snap->vps.m_iVehicleNum != pilot->clientNum + 1 ||
     !vehicle->m_pVehicle || !vehicle->m_pVehicle->m_pVehicleInfo ||
     vehicle->m_pVehicle->m_iTurboTime > hull->commandTime ||
     (cg_pmove.cmd.buttons & (BUTTON_USE | BUTTON_USE_HOLDABLE))) return;

 const entityState_t *portal = NULL;
 jarVehicleTeleportArrival_t arrival;
 for (int i = 0; i < cg_numTriggerEntities; ++i) {
  const centity_t *trigger = cg_triggerEntities[i];
  jarVehicleTeleportArrival_t proposed;
  if (!trigger->currentValid || !JAR_TeleportVolumeVehicleCandidate(&trigger->currentState, hull, pilot, &move->cmd,
                                                                    move->mins, move->maxs, &proposed)) continue;
  if (portal) return;
  portal = &trigger->currentState;
  arrival = proposed;
 }
 if (!portal || !trap->R_InPVS(cg.snap->ps.origin, arrival.vehicleOrigin, cg.snap->areamask) ||
     !trap->R_InPVS(cg.snap->ps.origin, arrival.pilotOrigin, cg.snap->areamask) ||
     !CG_JAR_TeleportArrivalClear(arrival.vehicleOrigin, arrival.vehicleMins, arrival.vehicleMaxs, hull->clientNum, pilot->clientNum) ||
     !CG_JAR_TeleportArrivalClear(arrival.pilotOrigin, arrival.pilotMins, arrival.pilotMaxs, hull->clientNum, pilot->clientNum)) return;

 JAR_TeleportVolumeApplyVehicle(portal, hull, pilot, &move->cmd, &arrival);
 jarTeleportPredicted = jarVehicleTeleportPredicted = qtrue;
}
