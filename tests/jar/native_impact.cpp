#include "game/g_local.h"
#include "game/g_vehicles.h"
#include "game/g_functions.h"
#include "game/wp_saber.h"
#include "test_support.h"

game_import_t gi{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
gentity_t*player=nullptr;
void DoImpact(gentity_t*,gentity_t*,qboolean,trace_t*);
static cvar_t gravity{};cvar_t*g_gravity=&gravity;
static int damages;static gentity_t*damaged;static int lastDamage,lastFlags;
static bool despawnOnImpact,reuseOnImpact;static int fatalImpactEvents;
static uint64_t impactLifetime=1;
uint64_t G_EntityLifetime(const gentity_t*){return impactLifetime;}
static void ImpactLog(int,const char*,const char* event,const char*,...){
 CHECK(!strcmp(event, fatalImpactEvents==0 ? "fatal_impact_begin" : "fatal_impact_end"));
 ++fatalImpactEvents;
}
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
bool G_JAR_IsActorStandin(const gentity_t*){return false;}
Vehicle_t*G_IsRidingVehicle(gentity_t*){return nullptr;}
void G_Throw(gentity_t*,const vec3_t,float){}
void G_Knockdown(gentity_t*,gentity_t*,const vec3_t,float,qboolean){}
void G_StartMatrixEffect(gentity_t*,int=0,int=1000,float=0.f,int=0){}
qboolean PM_InKnockDown(playerState_t*){return qfalse;}
qboolean TIMER_Done(gentity_t*,const char*){return qtrue;}
void TIMER_Set(gentity_t*,const char*,int){}
void G_PlayEffect(int,const vec3_t,const vec3_t){}
int G_EffectIndex(const char*){return 1;}
int G_SoundIndex(const char*){return 1;}
void G_Sound(gentity_t*,int){}
void GEntity_PainFunc(gentity_t*,gentity_t*,gentity_t*,const vec3_t,int,int,int){}
void G_Damage(gentity_t*target,gentity_t*,gentity_t*,const vec3_t,const vec3_t,int damage,int flags,int,int){
 ++damages;damaged=target;lastDamage=damage;lastFlags=flags;
 if(despawnOnImpact){target->inuse=qfalse;target->client=nullptr;target->m_pVehicle=nullptr;}
 if(reuseOnImpact)++impactLifetime;
}
#include "game/g_impact.inc"

int main(){return RunTests("native prop impact without a client record",[]{
 gi.pointcontents=[](const vec3_t,int){return 0;};
 auto&prop=g_entities[8];auto&wall=g_entities[ENTITYNUM_WORLD];
 prop.s.number=8;prop.inuse=qtrue;prop.health=40;prop.takedamage=qtrue;prop.mass=50;prop.forcePuller=ENTITYNUM_NONE;
 wall.s.number=ENTITYNUM_WORLD;wall.bmodel=qtrue;trace_t trace{};trace.plane.normal[2]=1;
 // Below the break threshold, the old vehicle check dereferenced prop.client.
 for(float speed:{0.f,40.f,80.f,139.f}){prop.s.pos.trDelta[0]=speed;DoImpact(&prop,&wall,qtrue,&trace);CHECK(damages==0);}
 // The null guard must not disable impact damage for destructible props.
 prop.s.pos.trDelta[0]=1000;DoImpact(&prop,&wall,qtrue,&trace);CHECK(damages==1 && damaged==&prop && lastDamage>0);
 DoImpact(&prop,&wall,qfalse,&trace);CHECK(damages==1);
 // Preserve the native vehicle-class branch when a client record exists.
 gclient_t client{};prop.client=&client;client.NPC_class=CLASS_VEHICLE;client.ps.velocity[0]=80;level.time=1000;
 DoImpact(&prop,&wall,qtrue,&trace);CHECK(damages==2 && damaged==&prop);

 // The Physgun can release an empty speeder at 1024 units/sec. The floor
 // contact uses native vehicle damage even with generic self-damage disabled.
 Vehicle_t swoop{};vehicleInfo_t speeder{};
 prop.m_pVehicle=&swoop;swoop.m_pVehicleInfo=&speeder;
 speeder.type=VH_SPEEDER;speeder.mass=80;speeder.toughness=1;
 VectorSet(client.ps.velocity,0,0,-1024);
 DoImpact(&prop,&wall,qfalse,&trace);
 CHECK(damages==3&&damaged==&prop&&lastDamage==100000&&(lastFlags&DAMAGE_IMPACT_DIE));
 CHECK(swoop.m_ulFlags&VEH_CRASHING);

 // A scripted impact callback can remove its target before impact returns.
 gi.JAR_Log=ImpactLog;
 gclient_t obstacleClient{};auto& obstacle=g_entities[9];
 obstacle.inuse=qtrue;obstacle.client=&obstacleClient;obstacleClient.NPC_class=CLASS_VEHICLE;
 obstacle.m_pVehicle=&swoop;obstacle.s.number=9;obstacle.currentOrigin[2]=-64;
 // Architecture still needs to trigger the fatal impact. Supply a vehicle
 // controller with no definition first, then test synchronous removal.
 Vehicle_t uninitialized{};obstacle.m_pVehicle=&uninitialized;prop.painDebounceTime=0;
 VectorClear(client.ps.velocity);
 DoImpact(&prop,&obstacle,qfalse,&trace);
 CHECK(damages==3);
 obstacle.client=nullptr;prop.painDebounceTime=0;despawnOnImpact=true;
 VectorSet(client.ps.velocity,0,0,-1024);
 DoImpact(&prop,&obstacle,qtrue,&trace);
 CHECK(damages==4&&damaged==&prop&&!prop.inuse&&!prop.client&&fatalImpactEvents==2);

 // The death callback may immediately allocate a new occupant in the same
 // slot. The old impact must never enter its owner/vehicle callbacks.
 despawnOnImpact=false;reuseOnImpact=true;fatalImpactEvents=0;
 prop.inuse=qtrue;prop.client=&client;prop.m_pVehicle=&swoop;prop.owner=&obstacle;
 prop.painDebounceTime=0;
 DoImpact(&prop,&obstacle,qtrue,&trace);
 CHECK(damages==5&&damaged==&prop&&prop.inuse&&fatalImpactEvents==2&&impactLifetime==2);
});}
