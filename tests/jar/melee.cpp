#include "game/g_local.h"
#include "game/bg_local.h"
#include "game/anims.h"
#include "game/wp_saber.h"
#include "test_support.h"
#include <vector>
game_import_t gi={};
static cvar_t enabled={},debug={};
cvar_t *jar_continuousFists=&enabled,*g_debugMelee=&debug;
pmove_t *pm=nullptr;
static bool reject=false,indefinite=false,droid=false;
static int animCalls=0,lastParts=0,selected=-2;
qboolean PM_DroidMelee(int){return droid?qtrue:qfalse;}
qboolean PM_KickMove(int move){return move==LS_KICK_F?qtrue:qfalse;}
int PM_PickAnim(gentity_t *e,int,int){return selected!=-2?selected:(e->client->ps.torsoAnim==BOTH_MELEE1?BOTH_MELEE2:BOTH_MELEE1);}
void PM_SetAnim(pmove_t *m,int parts,int anim,int flags,int){
    ++animCalls;lastParts=parts;
    CHECK(flags==(SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART));
    if(reject)return;
    const int duration=indefinite?-1:(anim==BOTH_MELEE1?350:600);
    if(parts&SETANIM_TORSO){m->ps->torsoAnim=anim;m->ps->torsoAnimTimer=duration;}
    if(parts&SETANIM_LEGS){m->ps->legsAnim=anim;m->ps->legsAnimTimer=duration;}
}
IGhoul2InfoArray &_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
#include "game/bg_jar_fists.inc"
int main(){return RunTests("production continuous-punch admission and animation cadence",[]{
    gentity_t player;gclient_t client={};pmove_t move={};
    player.client=&client;player.NPC=nullptr;player.s.number=0;move.gent=&player;move.ps=&client.ps;pm=&move;
    auto reset=[&]{client.ps={};client.ps.weapon=WP_MELEE;client.ps.torsoAnim=BOTH_STAND1;
        client.ps.pm_flags=PMF_ATTACK_HELD;move.cmd={};move.cmd.buttons=BUTTON_ATTACK;
        client.clientInfo.animFileIndex=0;client.NPC_class=CLASS_PLAYER;
        enabled.integer=debug.integer=1;droid=reject=indefinite=false;selected=-2;animCalls=lastParts=0;player.NPC=nullptr;player.s.number=0;};
    reset();CHECK(PM_JARContinuousPunch());CHECK(PM_JARStartPunch()==350);CHECK(lastParts==SETANIM_BOTH);
    CHECK(PM_JARStartPunch()==0 && animCalls==1); // holding never restarts an unfinished punch
    reset();move.cmd.buttons=0;CHECK(!PM_JARContinuousPunch() && PM_JARStartPunch()==0 && animCalls==0);
    for(int buttons:{BUTTON_ALT_ATTACK,BUTTON_ATTACK|BUTTON_ALT_ATTACK}){reset();move.cmd.buttons=buttons;CHECK(!PM_JARContinuousPunch());}
    for(int mode:{-1,0,2,999}){reset();enabled.integer=mode;CHECK(!PM_JARContinuousPunch());}
    reset();debug.integer=0;CHECK(!PM_JARContinuousPunch());
    reset();client.ps.weapon=WP_SABER;CHECK(!PM_JARContinuousPunch());
    reset();player.s.number=2;CHECK(!PM_JARContinuousPunch());
    reset();client.ps.clientNum=2;CHECK(!PM_JARContinuousPunch());
    reset();gNPC_t npc={};player.NPC=&npc;CHECK(!PM_JARContinuousPunch());
    reset();droid=true;CHECK(!PM_JARContinuousPunch());
    for(int index:{-1,1,2,42}){reset();client.clientInfo.animFileIndex=index;CHECK(!PM_JARContinuousPunch());}
    for(auto creature:{CLASS_RANCOR,CLASS_WAMPA}){reset();client.NPC_class=creature;client.clientInfo.animFileIndex=1;
        CHECK(!PM_JARContinuousPunch() && PM_JARStartPunch()==0 && animCalls==0);}
    reset();reject=true;CHECK(PM_JARStartPunch()==0 && animCalls==1);
    reset();indefinite=true;CHECK(PM_JARStartPunch()==0);CHECK(PM_JARStartPunch()==0 && animCalls==1);
    reset();selected=-1;CHECK(PM_JARStartPunch()==0 && animCalls==0);
    for(int timer:{1,100,-1}){reset();client.ps.torsoAnimTimer=timer;CHECK(PM_JARStartPunch()==0 && animCalls==0);}
    reset();client.ps.weaponTime=1;CHECK(PM_JARStartPunch()==0 && animCalls==0);
    for(int timer:{1,100,-1}){reset();client.ps.saberMove=LS_KICK_F;client.ps.legsAnimTimer=timer;
        CHECK(PM_JARStartPunch()==0 && client.ps.saberMove==LS_KICK_F && animCalls==0);}
    reset();client.ps.saberMove=LS_KICK_F;CHECK(PM_JARStartPunch()==350 && client.ps.saberMove==LS_NONE);
    reset();client.ps.saberMove=LS_READY;client.ps.torsoAnimTimer=300;CHECK(PM_JARStartPunch()==0 && client.ps.saberMove==LS_READY);
    reset();client.ps.velocity[0]=100;CHECK(PM_JARStartPunch()==350 && lastParts==SETANIM_TORSO);
    reset();move.cmd.upmove=-127;CHECK(PM_JARStartPunch()==350 && lastParts==SETANIM_TORSO);
    reset();std::vector<int> fired;
    for(int t=0;t<=2000;t+=50){
        if(t){for(int *timer:{&client.ps.weaponTime,&client.ps.torsoAnimTimer,&client.ps.legsAnimTimer})*timer=std::max(0,*timer-50);}
        const int duration=PM_JARStartPunch();if(duration){fired.push_back(t);client.ps.weaponTime=duration;}
    }
    CHECK((fired==std::vector<int>{0,350,950,1300,1900}));
    move.cmd.buttons=0;client.ps.weaponTime=client.ps.torsoAnimTimer=client.ps.legsAnimTimer=0;
    CHECK(PM_JARStartPunch()==0);CHECK(animCalls==5); // no queued action after release
});}
