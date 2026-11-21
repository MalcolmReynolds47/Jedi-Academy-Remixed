// Exercise the production original-mech lifecycle without launching the game.
#include "game/NPC_AI_GalakMech.c"
#include "test_support.h"
#include <map>

gameImport_t imports{};gameImport_t*trap=&imports;level_locals_t level{};npcStatic_t NPCS{};
int gPainHitLoc,gPainMOD;vec3_t gPainPoint;
gentity_t *NPC; gNPC_t *NPCInfo;
static std::map<std::string,int> timers,surfaces;
static int animations,sounds,effects,freed,linked;
static bool blocked,exhausted;
static gentity_t helper;
gentity_t g_entities[MAX_GENTITIES];vmCvar_t g_npcspskill{};

static int damageCalls;
qboolean NPC_CheckPlayerTeamStealth(){return qfalse;}
void NPC_Pain(gentity_t*,gentity_t*,int){}
int G_EffectIndex(const char*){return 1;}
gentity_t* G_PlayEffectID(const int,vec3_t,vec3_t){++effects;return nullptr;}
void G_Sound(gentity_t*,int,int){++sounds;}
void G_Damage(gentity_t*,gentity_t*,gentity_t*,vec3_t,vec3_t,int,int,int){++damageCalls;}
void G_Throw(gentity_t*,vec3_t,float){}
qboolean G_ClearLOS(gentity_t*,gentity_t*){return qtrue;}
void NPC_ChangeWeapon(int){}
void WeaponThink(qboolean){}
int NPC_ShotEntity(gentity_t*,vec3_t){return ENTITYNUM_WORLD;}
void NPC_ReachedGoal(){}
gentity_t*UpdateGoal(){return nullptr;}
qboolean NPC_MoveToGoal(qboolean){return qfalse;}
void CalcEntitySpot(const gentity_t*e,spot_t,vec3_t point){VectorCopy(e->r.currentOrigin,point);}
qboolean NPC_UpdateAngles(qboolean,qboolean){return qtrue;}
qboolean NPC_FreeCombatPoint(int,qboolean){return qtrue;}
qboolean NPC_CheckEnemyExt(qboolean){return NPC->enemy?qtrue:qfalse;}
qboolean G_BoundsOverlap(const vec3_t,const vec3_t,const vec3_t,const vec3_t){return qfalse;}
qboolean NPC_FaceEnemy(qboolean){return qtrue;}
qboolean NAV_HitNavGoal(vec3_t,vec3_t,vec3_t,vec3_t,int,qboolean){return qfalse;}
void NAV_GetLastMove(navInfo_t*info){memset(info,0,sizeof(*info));}
qboolean G_StandardHumanoid(gentity_t*){return qfalse;}
qboolean Q3_TaskIDPending(gentity_t*,taskID_t){return qfalse;}
void NPC_AimAdjust(int){}
qboolean WP_LobFire(gentity_t*,vec3_t,vec3_t,vec3_t,vec3_t,int,vec3_t,qboolean,int,int,float,float,float,qboolean){return qfalse;}
qboolean InFront(vec3_t,vec3_t,vec3_t,float){return qtrue;}
gentity_t*G_TempEntity(vec3_t,int){return &helper;}
qboolean BG_CrouchAnim(int){return qfalse;}
void TIMER_Set(gentity_t*,const char*n,int t){timers[n]=level.time+t;}
qboolean TIMER_Done(gentity_t*,const char*n){return timers[n]<=level.time?qtrue:qfalse;}
void NPC_SetAnim(gentity_t*e,int,int anim,int){e->client->ps.torsoAnim=anim;e->client->ps.torsoTimer=1500;++animations;}
int G_SoundIndex(const char*){return 1;}
void G_SoundOnEnt(gentity_t*,soundChannel_t,const char*){++sounds;}
void G_PlayEffect(const char*,int){++effects;}
void G_PlayEffect(const char*,const vec3_t,const vec3_t){++effects;}
void G_AddEvent(gentity_t*,int,int){}
void G_AddVoiceEvent(gentity_t*,int,int){}
void NPC_SetPainEvent(gentity_t*){}
void G_SetOrigin(gentity_t*e,vec3_t p){VectorCopy(p,e->r.currentOrigin);}
gentity_t*G_TrySpawn(void){if(exhausted)return nullptr;helper.inuse=qtrue;return &helper;}
void G_FreeEntity(gentity_t*e){NPC_GalakMech_ClearBeam(e);e->inuse=qfalse;++freed;}

const char *bgToggleableSurfaces[BG_NUM_TOGGLEABLE_SURFACES]={"vehicle_body",nullptr};
#include "game/g_npc_surfaces.inc"
void BG_GiveMeVectorFromMatrix(mdxaBone_t*m,int mode,vec3_t v){for(int i=0;i<3;++i)v[i]=mode==ORIGIN?m->matrix[i][3]:-m->matrix[i][1];}
void QDECL TestError(int,const char*,...){throw std::runtime_error("Com_Error");}
void QDECL TestPrint(const char*,...){}
void (*Com_Error)(int,const char*,...)=TestError;
void (*Com_Printf)(const char*,...)=TestPrint;
void QDECL Com_DPrintf(const char*,...){}
int QDECL Com_sprintf(char*b,int size,const char*f,...){va_list a;va_start(a,f);int n=vsnprintf(b,size,f,a);va_end(a);return n;}
int main(){return RunTests("MP Galak native shield, weakpoint, charge, fire and cleanup",[]{
 gentity_t self{};gclient_t client{};gNPC_t ai{};
 self.client=&client;self.NPC=&ai;self.inuse=qtrue;self.ghoul2=(void*)1;
 NPC=&self;NPCInfo=&ai;NPCS.NPC=&self;NPCS.NPCInfo=&ai;
 client.NPC_class=CLASS_GALAKMECH;self.health=800;level.time=1000;
 trap->G2API_SetSurfaceOnOff=[](void*,const char*n,int f){surfaces[n]=f;return qtrue;};
 trap->G2API_AddBolt=[](void*,int,const char*){return -1;};
 trap->Trace=[](trace_t*t,const vec3_t,const vec3_t lo,const vec3_t hi,const vec3_t,int,int,int,int,int){CHECK(lo[0]==-60&&hi[2]==80);*t={};t->startsolid=blocked?qtrue:qfalse;};
 trap->ICARUS_TaskIDPending=[](sharedEntity_t*,int){return qfalse;};
 trap->LinkEntity=[](sharedEntity_t*){++linked;};
 NPC_GalakMech_Init(&self);
 CHECK(client.ps.stats[STAT_ARMOR]==500&&self.r.mins[0]==-60&&self.r.maxs[2]==80&&client.ps.standheight==80);
 CHECK((self.flags&FL_SHIELDED)&&surfaces["torso_shield_off"]==TURN_ON);
 client.ps.stats[STAT_ARMOR]=0;GM_UpdateShield();
 CHECK(!(self.flags&FL_SHIELDED)&&self.r.maxs[0]==20&&ai.investigateDebounceTime==6000);
 level.time=6001;blocked=true;GM_UpdateShield();CHECK(client.ps.stats[STAT_ARMOR]==0);
 blocked=false;GM_UpdateShield();CHECK(client.ps.stats[STAT_ARMOR]==500&&self.r.maxs[0]==60);
 client.ps.stats[STAT_ARMOR]=0;
 self.locationDamage[HL_GENERIC1]=25;gPainHitLoc=HL_GENERIC1;gPainMOD=MOD_BLASTER;
 NPC_GM_Pain(&self,nullptr,25);
 CHECK(self.locationDamage[HL_GENERIC1]==Q3_INFINITE&&client.ps.torsoAnim==JAR_GALAK_GENERATOR_BREAK);
 CHECK(surfaces["torso_antenna"]==TURN_OFF&&surfaces["torso_antenna_cap_off"]==TURN_ON);
 int count=animations;NPC_GM_Pain(&self,nullptr,25);CHECK(animations==count);
 GM_UpdateShield();level.time+=100000;GM_UpdateShield();CHECK(client.ps.stats[STAT_ARMOR]==0&&!ai.investigateDebounceTime);
 NPC_GM_StartLaser();CHECK((unsigned(self.s.surfacesOn)&JAR_GALAK_BEAM_MASK)==JAR_GALAK_CHARGING);CHECK(self.lockCount==1&&client.ps.torsoAnim==JAR_GALAK_RAISE);
 count=animations;NPC_GM_StartLaser();CHECK(count==animations);
 exhausted=true;GM_FireLaser();CHECK((unsigned(self.s.surfacesOn)&JAR_GALAK_BEAM_MASK)==JAR_GALAK_FIRING);CHECK(self.lockCount==2&&self.s.loopSound&&!ai.coverTarg);
 NPC_GalakMech_ClearBeam(&self);CHECK(!self.lockCount&&!self.s.loopSound&&!(unsigned(self.s.surfacesOn)&JAR_GALAK_BEAM_MASK));
 exhausted=false;NPC_GM_StartLaser();GM_FireLaser();CHECK(ai.coverTarg==&helper&&helper.parent==&self&&linked==1);
 NPC_GalakMech_ClearBeam(&self);CHECK(freed==1&&!ai.coverTarg&&!helper.inuse);
 NPC_GalakMech_ClearBeam(&self);CHECK(freed==1);
 // Independent helper removal clears the backlink before its slot is reused.
 NPC_GM_StartLaser();GM_FireLaser();G_FreeEntity(&helper);CHECK(!ai.coverTarg&&freed==2);
 NPC_GalakMech_ClearBeam(&self);freed=1;
 // A stale/reused helper slot must never delete a different entity.
 helper.inuse=qtrue;helper.classname="unrelated";ai.coverTarg=&helper;NPC_GalakMech_ClearBeam(&self);CHECK(helper.inuse&&freed==1);

 // The real attack caller must stop the beam when its animation finishes.
 gentity_t enemy{};gclient_t enemyClient{};enemy.client=&enemyClient;enemy.health=100;enemy.s.number=1;self.enemy=&enemy;
 ai.coverTarg=nullptr;NPC_GM_StartLaser();GM_FireLaser();client.ps.torsoTimer=0;
 NPC_BSGM_Attack();CHECK(!self.lockCount&&!self.s.loopSound&&!ai.coverTarg&&client.ps.torsoAnim==JAR_GALAK_DROP);
 // One melee animation may deliver only one hit, including repeated AI ticks.
 client.ps.torsoAnim=BOTH_ATTACK4;ai.blockedDebounceTime=0;timers["smackTime"]=0;int hits=damageCalls;
 NPC_BSGM_Attack();NPC_BSGM_Attack();CHECK(damageCalls==hits+1);
 self.enemy=nullptr;client.ps.torsoAnim=BOTH_STAND1;NPC_GM_StartLaser();GM_FireLaser();
 NPC_BSGM_Attack();CHECK(!self.lockCount&&!ai.coverTarg&&!self.s.loopSound);
 NPC_GM_StartLaser();GM_FireLaser();self.s.time=level.time-5000;GM_Dying(&self);
 CHECK(!self.lockCount&&!ai.coverTarg&&surfaces["torso_shield_off"]==TURN_OFF);
 gentity_t vehicle{};gclient_t vehicleClient{};vehicle.client=&vehicleClient;vehicleClient.NPC_class=CLASS_VEHICLE;vehicle.ghoul2=(void*)1;NPC_SetSurfaceOnOff(&vehicle,"vehicle_body",TURN_OFF);CHECK(vehicle.s.surfacesOff==1&&!vehicle.s.surfacesOn);
 ai.behaviorState=BS_CINEMATIC;NPC_GalakMech_Init(&self);CHECK(surfaces["torso_shield_off"]==TURN_OFF&&surfaces["torso_galakface_off"]==TURN_ON);
 });}

void G_SoundAtLoc(vec3_t,int,int){++sounds;}
qboolean NPC_ClearLOS4(gentity_t*){return qtrue;}
