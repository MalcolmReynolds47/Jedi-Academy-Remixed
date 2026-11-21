// Exercise the production original-mech lifecycle without launching the game.
#include "game/AI_GalakMech_legacy.cpp"
#include "test_support.h"
#include <map>

game_import_t gi{}; level_locals_t level{}; cg_t cg{};
gentity_t *NPC; gNPC_t *NPCInfo;
struct Pool:IGhoul2InfoArray {
 std::vector<CGhoul2Info> entries;
 int New() override { return 1; }
 void Delete(int) override {}
 bool IsValid(int h)const override{return h==1;}
 std::vector<CGhoul2Info>&Get(int)override{return entries;}
 const std::vector<CGhoul2Info>&Get(int)const override{return entries;}
} pool;
IGhoul2InfoArray&_TheGhoul2InfoArray(){return pool;}
static std::map<std::string,int> timers,surfaces;
static int animations,sounds,effects,freed,linked;
static bool blocked,exhausted;
static gentity_t helper;
gentity_t g_entities[MAX_GENTITIES];usercmd_t ucmd{};
static cvar_t skill{};cvar_t*g_spskill=&skill;
static int damageCalls;
qboolean NPC_CheckPlayerTeamStealth(){return qfalse;}
void NPC_Pain(gentity_t*,gentity_t*,gentity_t*,const vec3_t,int,int,int){}
int G_EffectIndex(const char*){return 1;}
void G_PlayEffect(const char*,const vec3_t){++effects;}
void G_Sound(gentity_t*,int){++sounds;}
void G_Damage(gentity_t*,gentity_t*,gentity_t*,const vec3_t,const vec3_t,int,int,int,int){++damageCalls;}
void G_Throw(gentity_t*,const vec3_t,float){}
qboolean G_ClearLOS(gentity_t*,gentity_t*){return qtrue;}
void NPC_ChangeWeapon(int){}
void WeaponThink(qboolean){}
int NPC_ShotEntity(gentity_t*,vec3_t){return ENTITYNUM_WORLD;}
void NPC_ReachedGoal(){}
gentity_t*UpdateGoal(){return nullptr;}
qboolean NPC_MoveToGoal(qboolean){return qfalse;}
void CalcEntitySpot(const gentity_t*e,spot_t,vec3_t point){VectorCopy(e->currentOrigin,point);}
qboolean NPC_UpdateAngles(qboolean,qboolean){return qtrue;}
qboolean NPC_FreeCombatPoint(int,qboolean){return qtrue;}
qboolean NPC_CheckEnemyExt(qboolean){return NPC->enemy?qtrue:qfalse;}
qboolean G_BoundsOverlap(const vec3_t,const vec3_t,const vec3_t,const vec3_t){return qfalse;}
qboolean NPC_FaceEnemy(qboolean){return qtrue;}
bool STEER::Reached(gentity_t*,gentity_t*,float,bool){return false;}
qboolean G_StandardHumanoid(gentity_t*){return qfalse;}
qboolean Q3_TaskIDPending(gentity_t*,taskID_t){return qfalse;}
void NPC_AimAdjust(int){}
qboolean WP_LobFire(gentity_t*,vec3_t,vec3_t,vec3_t,vec3_t,int,vec3_t,qboolean,int,int,float,float,float,qboolean){return qfalse;}
qboolean InFront(vec3_t,vec3_t,vec3_t,float){return qtrue;}
void G_SoundAtSpot(vec3_t,int,qboolean){++sounds;}
qboolean PM_CrouchAnim(int){return qfalse;}
void TIMER_Set(gentity_t*,const char*n,int t){timers[n]=level.time+t;}
qboolean TIMER_Done(gentity_t*,const char*n){return timers[n]<=level.time?qtrue:qfalse;}
void NPC_SetAnim(gentity_t*e,int,int anim,int,int){e->client->ps.torsoAnim=anim;e->client->ps.torsoAnimTimer=1500;++animations;}
int G_SoundIndex(const char*){return 1;}
void G_SoundOnEnt(gentity_t*,soundChannel_t,const char*){++sounds;}
void G_PlayEffect(const char*,int){++effects;}
void G_PlayEffect(const char*,const vec3_t,const vec3_t){++effects;}
void G_AddEvent(gentity_t*,int,int){}
void G_AddVoiceEvent(gentity_t*,int,int){}
void NPC_SetPainEvent(gentity_t*){}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);}
gentity_t*G_TrySpawn(qboolean){if(exhausted)return nullptr;helper.inuse=qtrue;return &helper;}
void G_FreeEntity(gentity_t*e){NPC_GalakMech_ClearBeam(e);e->inuse=qfalse;++freed;}

int main(){return RunTests("SP Galak native shield, weakpoint, charge, fire and cleanup",[]{
 gentity_t self{};gclient_t client{};gNPC_t ai{};
 self.client=&client;self.NPC=&ai;self.inuse=qtrue;self.playerModel=0;
 self.ghoul2.push_back(CGhoul2Info());NPC=&self;NPCInfo=&ai;
 client.NPC_class=CLASS_GALAKMECH;self.health=800;level.time=1000;
 gi.G2API_SetSurfaceOnOff=[](CGhoul2Info*,const char*n,int f){surfaces[n]=f;return qtrue;};
 gi.G2API_AddBolt=[](CGhoul2Info*,const char*){return -1;};
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t lo,const vec3_t hi,const vec3_t,int,int,EG2_Collision,int){CHECK(lo[0]==-60&&hi[2]==80);*t={};t->startsolid=blocked?qtrue:qfalse;};
 gi.linkentity=[](gentity_t*){++linked;};
 NPC_GalakMech_Init(&self);
 CHECK(client.ps.stats[STAT_ARMOR]==500&&self.mins[0]==-60&&self.maxs[2]==80&&client.standheight==80);
 CHECK((self.flags&FL_SHIELDED)&&surfaces["torso_shield_off"]==TURN_ON);
 client.ps.stats[STAT_ARMOR]=0;client.ps.powerups[PW_GALAK_SHIELD]=0;GM_UpdateShield();
 CHECK(!(self.flags&FL_SHIELDED)&&self.maxs[0]==20&&ai.investigateDebounceTime==6000);
 level.time=6001;blocked=true;GM_UpdateShield();CHECK(client.ps.stats[STAT_ARMOR]==0);
 blocked=false;GM_UpdateShield();CHECK(client.ps.stats[STAT_ARMOR]==500&&self.maxs[0]==60);
 client.ps.stats[STAT_ARMOR]=0;client.ps.powerups[PW_GALAK_SHIELD]=0;
 self.locationDamage[HL_GENERIC1]=25;
 NPC_GM_Pain(&self,nullptr,nullptr,vec3_origin,25,MOD_BLASTER,HL_GENERIC1);
 CHECK(self.locationDamage[HL_GENERIC1]==Q3_INFINITE&&client.ps.torsoAnim==JAR_GALAK_GENERATOR_BREAK);
 CHECK(surfaces["torso_antenna"]==TURN_OFF&&surfaces["torso_antenna_cap_off"]==TURN_ON);
 int count=animations;NPC_GM_Pain(&self,nullptr,nullptr,vec3_origin,25,MOD_BLASTER,HL_GENERIC1);CHECK(animations==count);
 GM_UpdateShield();level.time+=100000;GM_UpdateShield();CHECK(client.ps.stats[STAT_ARMOR]==0&&!ai.investigateDebounceTime);
 NPC_GM_StartLaser();CHECK(self.lockCount==1&&client.ps.torsoAnim==JAR_GALAK_RAISE);
 count=animations;NPC_GM_StartLaser();CHECK(count==animations);
 exhausted=true;GM_FireLaser();CHECK(self.lockCount==2&&self.s.loopSound&&!ai.coverTarg);
 NPC_GalakMech_ClearBeam(&self);CHECK(!self.lockCount&&!self.s.loopSound);
 exhausted=false;NPC_GM_StartLaser();GM_FireLaser();CHECK(ai.coverTarg==&helper&&helper.owner==&self&&linked==1);
 NPC_GalakMech_ClearBeam(&self);CHECK(freed==1&&!ai.coverTarg&&!helper.inuse);
 NPC_GalakMech_ClearBeam(&self);CHECK(freed==1);
 // Independent helper removal clears the backlink before its slot is reused.
 NPC_GM_StartLaser();GM_FireLaser();G_FreeEntity(&helper);CHECK(!ai.coverTarg&&freed==2);
 NPC_GalakMech_ClearBeam(&self);freed=1;
 // A stale/reused helper slot must never delete a different entity.
 helper.inuse=qtrue;helper.classname="unrelated";ai.coverTarg=&helper;NPC_GalakMech_ClearBeam(&self);CHECK(helper.inuse&&freed==1);

 // The real attack caller must stop the beam when its animation finishes.
 gentity_t enemy{};gclient_t enemyClient{};enemy.client=&enemyClient;enemy.health=100;enemy.s.number=1;self.enemy=&enemy;
 ai.coverTarg=nullptr;NPC_GM_StartLaser();GM_FireLaser();client.ps.torsoAnimTimer=0;
 NPC_BSGM_Attack();CHECK(!self.lockCount&&!self.s.loopSound&&!ai.coverTarg&&client.ps.torsoAnim==JAR_GALAK_DROP);
 // One melee animation may deliver only one hit, including repeated AI ticks.
 client.ps.torsoAnim=BOTH_ATTACK4;ai.blockedDebounceTime=0;timers["smackTime"]=0;int hits=damageCalls;
 NPC_BSGM_Attack();NPC_BSGM_Attack();CHECK(damageCalls==hits+1);
 self.enemy=nullptr;client.ps.torsoAnim=BOTH_STAND1;NPC_GM_StartLaser();GM_FireLaser();
 NPC_BSGM_Attack();CHECK(!self.lockCount&&!ai.coverTarg&&!self.s.loopSound);
 NPC_GM_StartLaser();GM_FireLaser();self.s.time=level.time-5000;GM_Dying(&self);
 CHECK(!self.lockCount&&!ai.coverTarg&&surfaces["torso_shield_off"]==TURN_OFF);
 ai.behaviorState=BS_CINEMATIC;NPC_GalakMech_Init(&self);CHECK(surfaces["torso_shield_off"]==TURN_OFF&&surfaces["torso_galakface_off"]==TURN_ON);
 });}
