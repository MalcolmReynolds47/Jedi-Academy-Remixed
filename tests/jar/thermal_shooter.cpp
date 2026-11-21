#include "game/g_local.h"
#include "game/b_local.h"
#include "game/g_functions.h"
#include "game/g_workshop.h"
#include "game/w_local.h"
#include "test_support.h"

level_locals_t level{};weaponData_t weaponData[WP_NUM_WEAPONS]{};
vec3_t forwardVec={1,0,0},muzzle={0,0,64};
static gentity_t bolt;static bool exhausted;static int lobCalls,enemyNumber;static vec3_t lobTarget;
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
gentity_t*G_TrySpawn(qboolean){bolt=gentity_t{};return exhausted?nullptr:&bolt;}
void WP_TraceSetStart(const gentity_t*,vec3_t,const vec3_t,const vec3_t){}
int G_SoundIndex(const char*){return 1;}
qboolean WP_LobFire(gentity_t*e,vec3_t,vec3_t target,vec3_t,vec3_t,int,vec3_t velocity,qboolean tracePath,int ignore,int enemy,float,float,float,qboolean){
 CHECK(ignore==e->s.number && tracePath);++lobCalls;enemyNumber=enemy;VectorCopy(target,lobTarget);VectorSet(velocity,100,0,100);return qtrue;
}
#include "game/wp_thermal_fire.inc"

static gentity_t* linkedTarget;
static bool workshopOwned;
bool G_JAR_WorkshopOwns(const gentity_t*){return workshopOwned;}
gentity_t* G_Find(gentity_t*,int field,const char* name){
 CHECK(field==FOFS(targetname));
 return linkedTarget&&linkedTarget->inuse&&name&&!strcmp(name,linkedTarget->targetname)?linkedTarget:nullptr;
}
void FireWeapon(gentity_t* shooter,qboolean alt){
 VectorCopy(shooter->client->renderInfo.muzzlePoint,muzzle);
 VectorCopy(shooter->client->renderInfo.muzzleDir,forwardVec);
 CHECK(WP_FireThermalDetonator(shooter,alt));
}
#include "game/g_workshop_shooter.inc"

int main(){return RunTests("native grenade shooter without NPC or enemy records",[]{
 gentity_t shooter{},target{};gclient_t client{};gNPC_t npc{};
 shooter.client=&client;shooter.s.number=12;shooter.health=100;target.s.number=13;
 VectorSet(target.currentOrigin,256,0,128);shooter.enemy=&target;level.time=5000;
 weaponData[WP_THERMAL].damage=100;weaponData[WP_THERMAL].splashDamage=100;weaponData[WP_THERMAL].splashRadius=128;
 for(const char*name:{"misc_weapon_shooter","shooter_grenade"}){
  shooter.classname=const_cast<char*>(name);CHECK(!shooter.NPC);
  for(qboolean alt:{qfalse,qtrue}){
   CHECK(WP_FireThermalDetonator(&shooter,alt)==&bolt);CHECK(enemyNumber==13);
   CHECK(std::fabs(lobTarget[0]-256)<=5 && std::fabs(lobTarget[1])<=5 && std::fabs(lobTarget[2]-128)<=5);
   CHECK(bolt.owner==&shooter && bolt.s.weapon==WP_THERMAL && bolt.s.pos.trType==TR_GRAVITY);
   CHECK(bolt.damage==int(100*TD_NPC_DAMAGE_CUT) && bolt.splashRadius==128);
  }
 }
 // The target can also be a saved position with no live enemy pointer.
 shooter.enemy=nullptr;shooter.target=const_cast<char*>("destination");VectorSet(shooter.pos1,320,24,144);
 CHECK(WP_FireThermalDetonator(&shooter,qfalse));CHECK(enemyNumber==ENTITYNUM_NONE && VectorCompare(lobTarget,shooter.pos1));
 // NPCs still use their aiming error. Ordinary player throws bypass lob targeting.
 shooter.NPC=&npc;shooter.enemy=&target;npc.currentAim=0;Rand_Init(73);WP_FireThermalDetonator(&shooter,qfalse);CHECK(enemyNumber==13);
 shooter.NPC=nullptr;shooter.enemy=nullptr;shooter.target=nullptr;shooter.s.number=0;shooter.classname=const_cast<char*>("player");
 const int before=lobCalls;CHECK(WP_FireThermalDetonator(&shooter,qfalse));CHECK(lobCalls==before && bolt.damage==100 && bolt.e_ThinkFunc==thinkF_WP_ThermalThink);
 // A pasted shooter has a copied target name and no cached enemy. Its first
 // shot, a moving target and a deleted target all use the current link.
 workshopOwned=true;linkedTarget=&target;target.inuse=qtrue;target.targetname=const_cast<char*>("copied_target");
 shooter.classname=const_cast<char*>("shooter_grenade");shooter.s.number=12;shooter.target=target.targetname;
 shooter.spawnflags=2;shooter.wait=500;VectorSet(shooter.currentOrigin,0,0,64);
 misc_weapon_shooter_fire(&shooter);CHECK(shooter.enemy==&target&&enemyNumber==13&&lobCalls==before+1);
 CHECK(shooter.nextthink==5500&&shooter.e_ThinkFunc==thinkF_misc_weapon_shooter_fire);
 VectorSet(target.currentOrigin,0,384,160);level.time=5500;
 misc_weapon_shooter_fire(&shooter);CHECK(lobCalls==before+2&&std::fabs(lobTarget[1]-384)<=5&&forwardVec[1]>.9f);
 target.targetname=const_cast<char*>("reused_slot");VectorSet(shooter.pos1,320,24,144);
 misc_weapon_shooter_fire(&shooter);CHECK(!shooter.enemy&&VectorCompare(shooter.pos1,vec3_origin)&&lobCalls==before+2&&forwardVec[0]==1);
 shooter.svFlags|=SVF_INACTIVE;bolt.damage=-1;
 misc_weapon_shooter_fire(&shooter);
 CHECK(bolt.damage==-1&&shooter.e_ThinkFunc==thinkF_NULL&&!shooter.nextthink);
 shooter.s.number=0;shooter.classname=const_cast<char*>("player");shooter.target=nullptr;
 exhausted=true;CHECK(!WP_FireThermalDetonator(&shooter,qfalse));CHECK(lobCalls==before+2);
});}
