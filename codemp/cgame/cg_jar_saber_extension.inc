// Use SP ignition for the local player's held blades; native MP owns all
// activation, retraction, thrown blades, NPCs and server collision lengths.
#include "qcommon/jar_combat.h"
static void CG_JAR_SetSaberLengths(centity_t*cent,clientInfo_t*ci) {
    const playerState_t*ps=&cg.predictedPlayerState;
    const qboolean quick=jar_saberExtension.integer==1 && cg.snap &&
        cent->currentState.eType==ET_PLAYER && cent->currentState.number==ps->clientNum &&
        ps->clientNum==cg.snap->ps.clientNum && ps->clientNum<MAX_CLIENTS &&
        ps->pm_type==PM_NORMAL && !(ps->pm_flags&PMF_FOLLOW) &&
        ps->stats[STAT_HEALTH]>0 && !(cent->currentState.eFlags&EF_DEAD) &&
        !cent->currentState.saberInFlight && !ps->saberInFlight &&
        !ps->m_iVehicleNum && !cg.snap->ps.m_iVehicleNum &&
        !ps->emplacedIndex && !cg.snap->ps.emplacedIndex &&
        !(cent->currentState.eFlags2&EF2_HELD_BY_MONSTER) &&
        !com_cameraMode.integer;
    int elapsed=cg.frametime;
    if (quick && cent->jarSaberClockValid) {
        if (cg.time==cent->jarSaberTime) return;
        if (cg.time<cent->jarSaberTime) elapsed=0;
    }
    cent->jarSaberClockValid=quick;cent->jarSaberTime=cg.time;
    for (int n=0;n<MAX_SABERS;++n) {
        saberInfo_t*saber=&ci->saber[n];float before[MAX_BLADES];
        for (int b=0;b<saber->numBlades && b<MAX_BLADES;++b) before[b]=saber->blade[b].length;
        BG_SI_SetLengthGradual(saber,cg.time);
        if (!quick || saber->type==SABER_SITH_SWORD) continue;
        for (int b=0;b<saber->numBlades && b<MAX_BLADES;++b) {
            float target=saber->blade[b].desiredLength;
            if (target==-1) target=saber->blade[b].lengthMax;
            if (!(target>before[b])) continue;
            float length=JAR_ExtendSaber(before[b],saber->blade[b].lengthMax,elapsed,1,0);
            saber->blade[b].length=length<target ? length : target;
        }
    }
}
