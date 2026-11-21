// Native MP animation adapter for SP's Continuous punch cadence.
#include "qcommon/jar_combat.h"
static qboolean PM_JARContinuousPunch(void) {
    return pm->jarContinuousFists && pm_entSelf &&
        pm->ps->clientNum>=0 && pm->ps->clientNum<MAX_CLIENTS &&
        pm_entSelf->s.eType==ET_PLAYER && pm_entSelf->localAnimIndex<=1 &&
        !pm->nonHumanoid && pm->ps->pm_type==PM_NORMAL &&
        pm->ps->stats[STAT_HEALTH]>0 && !pm->ps->m_iVehicleNum &&
        !pm->ps->emplacedIndex && pm->ps->forceHandExtend==HANDEXTEND_NONE &&
        !(pm->ps->eFlags2&EF2_HELD_BY_MONSTER) && pm->ps->weapon==WP_MELEE &&
        (pm->cmd.buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK))==BUTTON_ATTACK;
}
static int PM_JARStartPunch(void) {
    const qboolean kick=BG_KickMove(pm->ps->saberMove);
    if (!PM_JARContinuousPunch() || !JAR_PunchReady(pm->ps->weaponTime>0?pm->ps->weaponTime:0,
        pm->ps->torsoTimer,kick,pm->ps->legsTimer)) return 0;
    if (kick) pm->ps->saberMove=LS_NONE;
    const int anim=pm->ps->torsoAnim==BOTH_MELEE1 ? BOTH_MELEE2 : BOTH_MELEE1;
    const qboolean fullBody=VectorCompare(pm->ps->velocity,vec3_origin) && pm->cmd.upmove>=0;
    PM_SetAnim(fullBody?SETANIM_BOTH:SETANIM_TORSO,anim,
        SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART);
    return pm->ps->torsoAnim==anim && pm->ps->torsoTimer>0 ? pm->ps->torsoTimer : 0;
}
