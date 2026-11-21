// Pending touches exist only within this replay of unacknowledged commands.
// Rebuild from the snapshot before each replay, including rewinds/map restarts.
// GPL-2.0-only.
#include "../game/jar_mp_push_volume.h"
static jarPushPrediction_t jarPushPrediction[MAX_ENTITIES_IN_SNAPSHOT];
static void CG_JAR_BeginPushPrediction(void){
 for(int i=0;i<cg_numTriggerEntities;++i){jarPushPrediction[i].last=cg_triggerEntities[i]->currentState.time;jarPushPrediction[i].spent=qfalse;}
}
static qboolean CG_JAR_PredictPushVolume(entityState_t*s,int index){
 if(s->eType!=ET_PUSH_TRIGGER||s->generic1!=JAR_MP_PUSH_VOLUME)return qfalse;
 JAR_PushVolumePredict(s,&jarPushPrediction[index],&cg.predictedPlayerState,cg_pmove.mins,cg_pmove.maxs);return qtrue;
}
// Run immediately after native vehicle Pmove, using its final oriented hull.
// Passengers never predict the vehicle. Boarding and mismatched seat snapshots
// stay authoritative; no local input or attachment state is modified here.
static void CG_JAR_PredictVehiclePushVolumes(pmove_t*move,const centity_t*vehicle){
 playerState_t*ps=move->ps;const playerState_t*pilot=&cg.predictedPlayerState;
 if(!vehicle->currentValid||vehicle->currentState.eType!=ET_NPC||vehicle->currentState.NPC_class!=CLASS_VEHICLE||vehicle->currentState.number!=ps->clientNum||vehicle->currentState.owner!=pilot->clientNum||!vehicle->m_pVehicle||!vehicle->m_pVehicle->m_pVehicleInfo)return;
 for(int i=0;i<cg_numTriggerEntities;++i)JAR_PushVolumePredictVehicle(&cg_triggerEntities[i]->currentState,&jarPushPrediction[i],ps,pilot,move->mins,move->maxs);
 if(ps->jumppad_frame!=ps->pmove_framecount){ps->jumppad_frame=0;ps->jumppad_ent=0;}
}
