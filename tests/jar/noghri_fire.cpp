#include "game/g_local.h"
#include "game/b_local.h"
#include "game/w_local.h"
#include "game/wp_saber.h"
#include "test_support.h"

static cvar_t skill{};cvar_t*g_spskill=&skill;
vec3_t forwardVec={1,0,0},muzzle={0,0,64};
static gentity_t missile;
static int emitted,traceCalls,hintCalls;
static bool exhausted;
static vec3_t firedDirection;
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
void WP_TraceSetStart(const gentity_t*,vec3_t,const vec3_t,const vec3_t){++traceCalls;}
bool WP_MissileTargetHint(gentity_t*,vec3_t,vec3_t){++hintCalls;return false;}
gentity_t*CreateMissile(vec3_t start,vec3_t direction,float speed,int life,gentity_t*owner,qboolean alt){
 CHECK(VectorCompare(start,muzzle));CHECK(speed==1200 && life==10000 && !alt);
 CHECK(owner->client && owner->client->ps.weapon==WP_NOGHRI_STICK);
 ++emitted;VectorCopy(direction,firedDirection);return exhausted?nullptr:&missile;
}

int main(){return RunTests("native Noghri staff player and NPC firing",[]{
 gentity_t shooter{};gclient_t client{};gNPC_t npc{};
 shooter.client=&client;client.ps.weapon=WP_NOGHRI_STICK;
 // The reported crash: a normal player has a client but no NPC aiming record.
 for(int difficulty=0;difficulty<3;++difficulty){
  skill.integer=difficulty;
  for(int shot=0;shot<64;++shot){
   missile.damage=-1;WP_FireNoghriStick(&shooter);
   CHECK(missile.damage==10 && missile.s.weapon==WP_NOGHRI_STICK);
   CHECK(!strcmp(missile.classname,"noghri_proj"));
   CHECK(firedDirection[0]>.99f && std::fabs(firedDirection[1])<.01f && std::fabs(firedDirection[2])<.01f);
   CHECK(missile.dflags==DAMAGE_NO_KNOCKBACK && missile.methodOfDeath==MOD_BLASTER);
   CHECK(missile.clipmask==(MASK_SHOT|CONTENTS_LIGHTSABER));
   CHECK(missile.splashDamage==0 && missile.splashRadius==100 && missile.splashMethodOfDeath==MOD_GAS);
  }
 }
 // Sight 2+ bypasses spread for both player and NPC shooters.
 for(int kind=0;kind<2;++kind){
  shooter.NPC=kind?&npc:nullptr;shooter.s.number=kind?7:0;
  client.ps.forcePowersActive=1<<FP_SEE;
  for(int sight=FORCE_LEVEL_2;sight<=FORCE_LEVEL_3;++sight){
   client.ps.forcePowerLevel[FP_SEE]=sight;WP_FireNoghriStick(&shooter);CHECK(VectorCompare(firedDirection,forwardVec));
  }
 }
 client.ps.forcePowersActive=0;
 // NPC difficulty damage and currentAim-dependent spread remain native.
 const int expectedDamage[]={1,5,10};
 for(int difficulty=0;difficulty<3;++difficulty){
  skill.integer=difficulty;WP_FireNoghriStick(&shooter);CHECK(missile.damage==expectedDamage[difficulty]);
 }
 npc.currentAim=0;Rand_Init(73);WP_FireNoghriStick(&shooter);const float inaccurate=std::fabs(firedDirection[1]);
 npc.currentAim=6;Rand_Init(73);WP_FireNoghriStick(&shooter);CHECK(std::fabs(firedDirection[1])<inaccurate);
 exhausted=true;missile.damage=-1;WP_FireNoghriStick(&shooter);CHECK(missile.damage==-1);
 CHECK(traceCalls==emitted && hintCalls==emitted);
});}
