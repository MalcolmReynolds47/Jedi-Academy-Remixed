// Actual MP saber adapter/native fallback and punch adapter, with native
// animation timing and controlled failure injection. No window, network client or gameplay acceptance.
#include "cgame/cg_local.h"
#include "game/bg_local.h"
#include <assert.h>
#include <stdio.h>
cg_t cg;vmCvar_t jar_saberExtension,com_cameraMode;
pmove_t*pm;bgEntity_t*pm_entSelf;
static snapshot_t snapshot;static int animCalls,lastParts,reject,indefinite;
static animation_t combatAnimations[MAX_TOTALANIMATIONS];
cgameImport_t*trap;
saberMoveData_t saberMoveData[LS_MOVE_MAX];
static void Error(int code,const char*fmt,...){abort();}
static void Print(const char*fmt,...){}
void(*Com_Error)(int,const char*,...)=Error;
void(*Com_Printf)(const char*,...)=Print;
void*BG_Alloc(int size){assert(0);return NULL;}
saberInfo_t*BG_MySaber(int client,int saber){assert(0);return NULL;}
qboolean PM_RunningAnim(int anim){assert(anim==BOTH_MELEE1||anim==BOTH_MELEE2);return qfalse;}
qboolean PM_WalkingAnim(int anim){assert(anim==BOTH_MELEE1||anim==BOTH_MELEE2);return qfalse;}
#define PM_SetAnim JAR_NativeSetAnim
#include "game/bg_panimate.c"
#undef PM_SetAnim
void PM_SetAnim(int parts,int anim,int flags){
 ++animCalls;lastParts=parts;assert(flags==(SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART));
 if(reject)return;JAR_NativeSetAnim(parts,anim,flags);if(indefinite)pm->ps->torsoTimer=-1;
}
#include "game/bg_jar_native_saber_length.inc"
#include "cgame/cg_jar_saber_extension.inc"
#include "game/bg_jar_fists.inc"
static void SaberReset(centity_t*c,clientInfo_t*ci){
 memset(&cg,0,sizeof(cg));memset(&snapshot,0,sizeof(snapshot));memset(c,0,sizeof(*c));memset(ci,0,sizeof(*ci));
 cg.snap=&snapshot;cg.time=1000;cg.frametime=25;cg.predictedPlayerState.clientNum=snapshot.ps.clientNum=3;cg.predictedPlayerState.stats[STAT_HEALTH]=100;
 c->currentState.number=3;c->currentState.eType=ET_PLAYER;jar_saberExtension.integer=1;com_cameraMode.integer=0;
 for(int n=0;n<MAX_SABERS;++n){ci->saber[n].numBlades=2;ci->saber[n].type=SABER_SINGLE;for(int b=0;b<2;++b){ci->saber[n].blade[b].lengthMax=40+n*20;ci->saber[n].blade[b].desiredLength=-1;}}
}
static void Protect(centity_t*c,int reason){
 switch(reason){case 0:jar_saberExtension.integer=0;break;case 1:c->currentState.eType=ET_NPC;break;case 2:c->currentState.number=4;break;case 3:cg.predictedPlayerState.pm_type=PM_SPECTATOR;break;case 4:cg.predictedPlayerState.pm_flags=PMF_FOLLOW;break;case 5:cg.predictedPlayerState.stats[STAT_HEALTH]=0;break;case 6:c->currentState.eFlags=EF_DEAD;break;case 7:c->currentState.saberInFlight=1;break;case 8:cg.predictedPlayerState.saberInFlight=1;break;case 9:cg.predictedPlayerState.m_iVehicleNum=77;break;case 10:snapshot.ps.m_iVehicleNum=77;break;case 11:cg.predictedPlayerState.emplacedIndex=99;break;case 12:snapshot.ps.emplacedIndex=99;break;case 13:c->currentState.eFlags2=EF2_HELD_BY_MONSTER;break;case 14:com_cameraMode.integer=1;break;case 15:cg.snap=NULL;break;}
}
static void FistReset(pmove_t*m,playerState_t*ps,bgEntity_t*e){
 memset(m,0,sizeof(*m));memset(ps,0,sizeof(*ps));memset(e,0,sizeof(*e));pm=m;pm_entSelf=e;m->ps=ps;m->animations=combatAnimations;combatAnimations[BOTH_MELEE1].numFrames=7;combatAnimations[BOTH_MELEE2].numFrames=12;combatAnimations[BOTH_MELEE1].frameLerp=combatAnimations[BOTH_MELEE2].frameLerp=50;m->jarContinuousFists=qtrue;m->cmd.buttons=BUTTON_ATTACK;
 ps->clientNum=3;ps->stats[STAT_HEALTH]=100;ps->weapon=WP_MELEE;ps->torsoAnim=BOTH_STAND1;e->s.eType=ET_PLAYER;animCalls=lastParts=reject=indefinite=0;
}
int main(void){
 centity_t c;clientInfo_t ci;SaberReset(&c,&ci);
 for(int f=0;f<10;++f){cg.time=1000+f*25;CG_JAR_SetSaberLengths(&c,&ci);for(int n=0;n<2;++n)for(int b=0;b<2;++b)assert(fabsf(ci.saber[n].blade[b].length-(f+1)*(4+n*2))<.001f);clientInfo_t same=ci;CG_JAR_SetSaberLengths(&c,&ci);assert(!memcmp(&same,&ci,sizeof(ci)));}
 for(int reason=0;reason<16;++reason){SaberReset(&c,&ci);Protect(&c,reason);clientInfo_t native=ci;for(int n=0;n<2;++n)BG_SI_SetLengthGradual(&native.saber[n],cg.time);CG_JAR_SetSaberLengths(&c,&ci);assert(!memcmp(&native,&ci,sizeof(ci)));}
 SaberReset(&c,&ci);ci.saber[0].type=SABER_SITH_SWORD;saberInfo_t sword=ci.saber[0];BG_SI_SetLengthGradual(&sword,cg.time);CG_JAR_SetSaberLengths(&c,&ci);assert(!memcmp(&sword,&ci.saber[0],sizeof(sword))&&ci.saber[1].blade[0].length==6);
 SaberReset(&c,&ci);for(int n=0;n<2;++n)for(int b=0;b<2;++b){ci.saber[n].blade[b].length=20;ci.saber[n].blade[b].desiredLength=0;}clientInfo_t retract=ci;for(int n=0;n<2;++n)BG_SI_SetLengthGradual(&retract.saber[n],cg.time);CG_JAR_SetSaberLengths(&c,&ci);assert(!memcmp(&retract,&ci,sizeof(ci)));
 SaberReset(&c,&ci);ci.saber[0].blade[0].desiredLength=2;CG_JAR_SetSaberLengths(&c,&ci);assert(ci.saber[0].blade[0].length==2);cg.time=900;CG_JAR_SetSaberLengths(&c,&ci);assert(ci.saber[1].blade[0].length==6);
 SaberReset(&c,&ci);cg.frametime=0;CG_JAR_SetSaberLengths(&c,&ci);assert(!ci.saber[0].blade[0].length);c.jarSaberClockValid=qfalse;cg.frametime=250;CG_JAR_SetSaberLengths(&c,&ci);assert(ci.saber[1].blade[1].length==60);
 pmove_t m;playerState_t ps;bgEntity_t e;FistReset(&m,&ps,&e);assert(PM_JARContinuousPunch()&&PM_JARStartPunch()==350&&lastParts==SETANIM_BOTH);assert(!PM_JARStartPunch()&&animCalls==1);
 for(int reason=0;reason<15;++reason){FistReset(&m,&ps,&e);switch(reason){case 0:m.jarContinuousFists=qfalse;break;case 1:pm_entSelf=NULL;break;case 2:ps.clientNum=MAX_CLIENTS;break;case 3:e.s.eType=ET_NPC;break;case 4:e.localAnimIndex=2;break;case 5:m.nonHumanoid=qtrue;break;case 6:ps.pm_type=PM_DEAD;break;case 7:ps.stats[STAT_HEALTH]=0;break;case 8:ps.m_iVehicleNum=88;break;case 9:ps.emplacedIndex=88;break;case 10:ps.forceHandExtend=HANDEXTEND_CHOKE;break;case 11:ps.eFlags2=EF2_HELD_BY_MONSTER;break;case 12:ps.weapon=WP_SABER;break;case 13:m.cmd.buttons=BUTTON_ALT_ATTACK;break;case 14:m.cmd.buttons=BUTTON_ATTACK|BUTTON_ALT_ATTACK;break;}assert(!PM_JARContinuousPunch()&&!PM_JARStartPunch()&&!animCalls);}
 FistReset(&m,&ps,&e);m.cmd.buttons=0;assert(!PM_JARStartPunch());
 for(int t=-1;t<=1;++t)if(t){FistReset(&m,&ps,&e);ps.torsoTimer=t;assert(!PM_JARStartPunch()&&!animCalls);FistReset(&m,&ps,&e);ps.saberMove=LS_KICK_F;ps.legsTimer=t;assert(!PM_JARStartPunch()&&ps.saberMove==LS_KICK_F);}
 FistReset(&m,&ps,&e);ps.saberMove=LS_KICK_F;assert(PM_JARStartPunch()==350&&ps.saberMove==LS_NONE);
 FistReset(&m,&ps,&e);ps.velocity[0]=20;assert(PM_JARStartPunch()==350&&lastParts==SETANIM_TORSO);FistReset(&m,&ps,&e);m.cmd.upmove=-127;assert(PM_JARStartPunch()==350&&lastParts==SETANIM_TORSO);
 FistReset(&m,&ps,&e);reject=1;assert(!PM_JARStartPunch());FistReset(&m,&ps,&e);indefinite=1;assert(!PM_JARStartPunch());
 FistReset(&m,&ps,&e);int times[5],count=0;for(int t=0;t<=2000;t+=50){if(t){if(ps.weaponTime>0)ps.weaponTime-=50;if(ps.torsoTimer>0)ps.torsoTimer-=50;if(ps.legsTimer>0)ps.legsTimer-=50;}int duration=PM_JARStartPunch();if(duration){assert(count<5);times[count++]=t;ps.weaponTime=duration;}}
 assert(count==5&&times[0]==0&&times[1]==350&&times[2]==950&&times[3]==1300&&times[4]==1900);m.cmd.buttons=0;ps.weaponTime=ps.torsoTimer=ps.legsTimer=0;assert(!PM_JARStartPunch()&&animCalls==5);
 FistReset(&m,&ps,&e);ps.fd.forcePowersActive=1<<FP_RAGE;assert(PM_JARStartPunch()==(int)(350/1.7));
 assert(JAR_FistsSelected("1")&&!JAR_FistsSelected("")&&!JAR_FistsSelected("1.5")&&!JAR_FistsSelected("01")&&!JAR_FistsSelected("0")&&!JAR_FistsSelected(NULL));
 puts("MP combat: exact 250ms dual/staff ignition, native protected/retraction paths, same-frame stability, native animation hold timers and completed-animation punch cadence, release, kicks/grabs and strict preference admission passed.");
}
