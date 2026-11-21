// JA:R local-player primary punch admission. GPL-2.0-only.
// Included by PM_Weapon and the production-path regression fixture.
#include "qcommon/jar_combat.h"
extern cvar_t *jar_continuousFists;
static bool PM_JARContinuousPunch() {
    return jar_continuousFists && jar_continuousFists->integer==1 &&
        g_debugMelee && g_debugMelee->integer && pm && pm->ps && pm->gent &&
        pm->gent->client && !pm->gent->NPC && pm->gent->s.number==0 &&
        // NPC_InitGame registers _humanoid at animation-set zero before maps.
        // Playable creature models must retain their native cadence/animations.
        pm->gent->client->clientInfo.animFileIndex==0 &&
        pm->ps->clientNum==0 && pm->ps->weapon==WP_MELEE &&
        !PM_DroidMelee(pm->gent->client->NPC_class) &&
        (pm->cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK))==BUTTON_ATTACK;
}

// A positive result is the accepted animation's cooldown and permission for
// the caller's normal fire event. No queue or save-facing state is introduced.
static int PM_JARStartPunch() {
    if (!PM_JARContinuousPunch() || !JAR_PunchReady(pm->ps->weaponTime,pm->ps->torsoAnimTimer,PM_KickMove(pm->ps->saberMove),pm->ps->legsAnimTimer))
        return 0;
    if (PM_KickMove(pm->ps->saberMove)) {
        // A held primary can follow a completed kick without a synthetic idle
        // command. Negative timers are indefinite holds, not completed actions.
        if (pm->ps->legsAnimTimer!=0) return 0;
        pm->ps->saberMove=LS_NONE;
    }
    const int anim=PM_PickAnim(pm->gent,BOTH_MELEE1,BOTH_MELEE2);
    if (anim!=BOTH_MELEE1 && anim!=BOTH_MELEE2) return 0;
    const bool fullBody=VectorCompare(pm->ps->velocity,vec3_origin) && pm->cmd.upmove>=0;
    PM_SetAnim(pm,fullBody?SETANIM_BOTH:SETANIM_TORSO,anim,
        SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART);
    if (pm->ps->torsoAnim!=anim || pm->ps->torsoAnimTimer<=0) return 0;
    if (gi.JAR_Log) gi.JAR_Log(2,"melee","punch","anim=%d duration_ms=%d full_body=%d input=%d",
        anim,pm->ps->torsoAnimTimer,fullBody,pm->cmd.buttons);
    return pm->ps->torsoAnimTimer;
}
