// Native trigger/item prediction with an owned-volume branch. GPL-2.0-only.
#include "cg_jar_teleport_prediction.inc"
static void CG_TouchTriggerPrediction( void ) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;
	qboolean	spectator;
	const entityState_t *portal=NULL;
	vec3_t arrival,final;
	int portals=0;

	// dead clients don't activate triggers
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	spectator = ( cg.predictedPlayerState.pm_type == PM_SPECTATOR );

	if ( cg.predictedPlayerState.pm_type != PM_NORMAL && cg.predictedPlayerState.pm_type != PM_JETPACK && cg.predictedPlayerState.pm_type != PM_FLOAT && !spectator ) {
		return;
	}

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM && !spectator ) {
			CG_TouchItem( cent );
			continue;
		}

		if ( CG_JAR_PredictPushVolume(ent, i) ) continue;
		if ( ent->eType == ET_TELEPORT_TRIGGER && ent->generic1 == JAR_MP_TELEPORT_VOLUME ) {
			vec3_t proposed,clear;
			if ( cent->currentValid && !jarTeleportPredicted && JAR_TeleportVolumeCandidate(ent,&cg.predictedPlayerState,&cg_pmove.cmd,cg_pmove.mins,cg_pmove.maxs,proposed,clear) ) {
				VectorCopy(proposed,arrival);VectorCopy(clear,final);
				portal=ent;
				++portals;
			}
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = trap->CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		trap->CM_Trace( &trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin, cg_pmove.mins, cg_pmove.maxs, cmodel, -1, 0 );

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			cg.hyperspace = qtrue;
		} else if ( ent->eType == ET_PUSH_TRIGGER ) {
			BG_TouchJumpPad( &cg.predictedPlayerState, ent );
		}
	}

	if ( portals==1 && !cg.hyperspace ) CG_JAR_PredictTeleportVolume(portal,arrival,final);

	// if we didn't touch a jump pad this pmove frame
	if ( cg.predictedPlayerState.jumppad_frame != cg.predictedPlayerState.pmove_framecount ) {
		cg.predictedPlayerState.jumppad_frame = 0;
		cg.predictedPlayerState.jumppad_ent = 0;
	}
}
