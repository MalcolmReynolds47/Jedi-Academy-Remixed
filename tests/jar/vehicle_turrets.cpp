// Production SP turret and projectile code with explicit renderer/world boundaries.
#include "game/g_local.h"
#include "game/g_vehicles.h"
#include "game/g_vehicle_shields.h"
#include "game/g_functions.h"
#include "game/w_local.h"
#include "test_support.h"
#include "memory_save.h"
#include <map>

vehWeaponInfo_t g_vehWeaponInfo[MAX_VEH_WEAPONS]{};int numVehicleWeapons=2;
struct TestPool:IGhoul2InfoArray{
 std::map<int,std::vector<CGhoul2Info>> entries;int next=1;
 int New()override{entries[next]={};return next++;}void Delete(int h)override{entries.erase(h);}
 bool IsValid(int h)const override{return entries.count(h)!=0;}
 std::vector<CGhoul2Info>&Get(int h)override{return entries.at(h);}
 const std::vector<CGhoul2Info>&Get(int h)const override{return entries.at(h);}
}pool;
IGhoul2InfoArray&_TheGhoul2InfoArray(){return pool;}
game_import_t gi{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
static Vehicle_t vehicle{};static vehicleInfo_t info{};static gclient_t clients[8]{};
static std::vector<int> candidates;static float yaw[2]{},pitch[2]{};
static bool blocked=false,pvs=true,badMatrix=false,badBone=false,exhausted=false;
static int shots=0,flashes=0,queries=0,shotBolt=-1,lastBolt=-1;
static gentity_t projectile{};static vec3_t shotStart{},shotDir{};
qboolean OnSameTeam(gentity_t*a,gentity_t*b){return a->client&&b->client&&a->client->playerTeam==b->client->playerTeam?qtrue:qfalse;}
int G_RadiusList(vec3_t,float,gentity_t*,qboolean,gentity_t**out){++queries;for(unsigned i=0;i<candidates.size();++i)out[i]=&g_entities[candidates[i]];return int(candidates.size());}
void G_PlayEffect(int,const vec3_t,const vec3_t){++flashes;}
bool G_JAR_ProjectilesEnabled(){return false;}
void WP_TraceSetStart(const gentity_t*,vec3_t,const vec3_t,const vec3_t){}
void WP_RocketLock(gentity_t*,float){}
gentity_t *CreateMissile(vec3_t start,vec3_t dir,float speed,int life,gentity_t*owner,qboolean){
 CHECK(owner==&g_entities[2]&&speed==25000&&life==10000);
 if(exhausted)return nullptr;
 ++shots;shotBolt=lastBolt;VectorCopy(start,shotStart);VectorCopy(dir,shotDir);projectile={};return &projectile;
}
#define VEH_HOMING_MISSILE_THINK_TIME 100
#include "game/g_vehicle_projectile.inc"
#include "game/g_vehicle_turret.inc"

static void Reset()
{
 for(int i=0;i<8;++i){g_entities[i]=gentity_t{};clients[i]={};auto&e=g_entities[i];e.s.number=i;e.inuse=qtrue;e.health=100;e.takedamage=qtrue;e.client=&clients[i];e.client->playerTeam=i==4?TEAM_ENEMY:TEAM_PLAYER;e.client->enemyTeam=i==4?TEAM_PLAYER:TEAM_ENEMY;}
 vehicle={};info={};vehicle.m_pParentEntity=&g_entities[2];vehicle.m_pVehicleInfo=&info;vehicle.m_pPilot=&g_entities[0];vehicle.m_fTimeModifier=1;
 auto&parent=g_entities[2];parent.m_pVehicle=&vehicle;parent.playerModel=0;parent.ghoul2.resize(1);parent.client->playerTeam=TEAM_NEUTRAL;
 VectorSet(g_entities[4].currentOrigin,1000,0,0);VectorSet(g_entities[5].currentOrigin,300,0,0);
 for(int i=0;i<MAX_VEHICLE_MUZZLES;++i)vehicle.m_iMuzzleTag[i]=i<4?i:-1;
 for(int i=0;i<2;++i){auto&t=info.turret[i];t.iWeapon=1;t.iAmmoMax=120;t.iDelay=100;t.iAmmoRechargeMS=500;t.fTurnSpeed=10;t.bAI=qtrue;t.bAILead=qtrue;t.fAIRange=15000;
  t.yawBone=const_cast<char*>(i?"bottom_yaw":"top_yaw");t.pitchBone=const_cast<char*>(i?"bottom_pitch":"top_pitch");t.yawAxis=2;t.pitchAxis=1;
  t.iMuzzle[0]=i?1:3;t.iMuzzle[1]=i?2:4;t.pitchClampUp=i?-20:-90;t.pitchClampDown=i?90:20;
  vehicle.turretStatus[i].ammo=120;vehicle.turretStatus[i].nextMuzzle=t.iMuzzle[0]-1;vehicle.turretStatus[i].enemyEntNum=ENTITYNUM_NONE;
  yaw[i]=pitch[i]=0;
 }
 g_vehWeaponInfo[1]={};auto&w=g_vehWeaponInfo[1];w.bIsProjectile=qtrue;w.iAmmoPerShot=1;w.fSpeed=25000;w.iDamage=1000;w.iSplashDamage=320;w.fSplashRadius=200;w.iLifeTime=2000;w.iMuzzleFX=42;
 candidates={4,5};blocked=badMatrix=badBone=exhausted=false;pvs=true;shots=flashes=queries=0;level.time=1000;shotBolt=-1;
}
static void Tick(int milliseconds=0,int turret=0){level.time+=milliseconds;G_VehicleTurretThink(&vehicle,&g_entities[2],turret);}

int main(){return RunTests("SP native turret targeting, transforms, projectile emission, exhaustion, cooldown and saved state",[]{
 gi.inPVS=[](const vec3_t,const vec3_t){return pvs?qtrue:qfalse;};
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int pass,int mask,EG2_Collision,int){CHECK(pass==2&&mask==MASK_SHOT);*t={};t->fraction=blocked?.3f:1.0f;t->entityNum=blocked?ENTITYNUM_WORLD:ENTITYNUM_NONE;};
 gi.G2API_SetBoneAngles=[](CGhoul2Info*,const char*name,const vec3_t angles,int flags,Eorientations up,Eorientations left,Eorientations forward,qhandle_t*,int blend,int time){
  CHECK(flags==BONE_ANGLES_POSTMULT&&up==POSITIVE_X&&left==NEGATIVE_Y&&forward==NEGATIVE_Z&&blend==0&&time==level.time);
  if(badBone)return qfalse;int index=!strncmp(name,"bottom",6)?1:0;
  if(strstr(name,"yaw"))yaw[index]=angles[2];else pitch[index]=angles[1];return qtrue;
 };
 gi.G2API_GetBoltMatrix=[](CGhoul2Info_v&,int model,int bolt,mdxaBone_t*m,const vec3_t rotation,const vec3_t origin,int time,qhandle_t*,const vec3_t){
  CHECK(model==0&&bolt>=0&&bolt<4&&time==level.time);if(badMatrix)return qfalse;
  lastBolt=bolt;const int turret=bolt<2?1:0;vec3_t angles={pitch[turret],yaw[turret],0},local,f,right,up,world;
  AngleVectors(angles,local,nullptr,nullptr);AngleVectors(rotation,f,right,up);
  for(int a=0;a<3;++a){world[a]=local[0]*f[a]-local[1]*right[a]+local[2]*up[a];m->matrix[a][1]=-world[a];m->matrix[a][3]=origin[a]+10*f[a]-(bolt%2?4:-4)*right[a];}
  return qtrue;
 };
 gi.G2API_GiveMeVectorFromMatrix=[](mdxaBone_t&m,Eorientations axis,vec3_t&v){CHECK(axis==ORIGIN||axis==NEGATIVE_Y);for(int a=0;a<3;++a)v[a]=axis==ORIGIN?m.matrix[a][3]:-m.matrix[a][1];};

 Reset();Tick();CHECK(shots==1&&flashes==1&&shotBolt==2&&vehicle.turretStatus[0].ammo==119&&vehicle.turretStatus[0].nextMuzzle==3);
 CHECK(projectile.owner==&g_entities[0]&&projectile.s.otherEntityNum==2&&projectile.s.otherEntityNum2==1);
 CHECK(projectile.damage==1000&&projectile.splashDamage==320&&projectile.splashRadius==200&&projectile.nextthink==3000);
 CHECK(projectile.methodOfDeath==MOD_EMPLACED&&projectile.bounceCount==8&&g_entities[2].client->ps.ammo[2]==119);
 Tick(99);CHECK(shots==1);Tick(1);CHECK(shots==2&&shotBolt==3&&vehicle.turretStatus[0].nextMuzzle==2);
 Tick(0,1);CHECK(shots==3&&shotBolt==0&&vehicle.turretStatus[1].ammo==119);

 // Friendly, neutral, dead, untargetable, attached and obstructed actors are excluded.
 for(int scenario=0;scenario<9;++scenario){Reset();
  if(scenario==0)g_entities[4].client->playerTeam=TEAM_PLAYER;
  if(scenario==1)g_entities[4].client->playerTeam=TEAM_NEUTRAL;
  if(scenario==2)g_entities[4].health=0;
  if(scenario==3)g_entities[4].flags|=FL_NOTARGET;
  if(scenario==4)g_entities[4].owner=&g_entities[2];
  if(scenario==5)vehicle.m_pDroidUnit=&g_entities[4];
  if(scenario==6)blocked=true;
  if(scenario==7)pvs=false;
  if(scenario==8)vehicle.m_pPilot=nullptr;
  Tick();CHECK(shots==0&&vehicle.turretStatus[0].enemyEntNum==ENTITYNUM_NONE);Tick(100);CHECK(queries==1);
 }
 Reset();Tick();g_entities[4].client->playerTeam=TEAM_PLAYER;Tick(100);CHECK(shots==1&&vehicle.turretStatus[0].enemyEntNum==ENTITYNUM_NONE);
 Reset();g_entities[4].client=nullptr;Tick();CHECK(shots==0);g_entities[2].enemy=&g_entities[4];Tick(500);CHECK(shots==1);

 // Native geometry is sampled without a rendered frame. Track yaw/lead and banked ships.
 Reset();VectorSet(g_entities[4].currentOrigin,0,1000,0);Tick();CHECK(shots==0&&yaw[0]>9&&yaw[0]<11);
 for(int i=0;i<10;++i)Tick(20);CHECK(shots>0&&yaw[0]>89);
 Reset();VectorSet(g_entities[4].client->ps.velocity,0,500,0);Tick();CHECK(shots==1&&shotDir[1]>.01f);
 Reset();g_entities[4].client->ps.velocity[1]=std::numeric_limits<float>::quiet_NaN();Tick();CHECK(shots==0);
 Reset();VectorSet(vehicle.m_vOrientation,20,45,30);vec3_t f;AngleVectors(vehicle.m_vOrientation,f,nullptr,nullptr);VectorScale(f,1000,g_entities[4].currentOrigin);Tick();CHECK(shots==1&&DotProduct(f,shotDir)>.999f);
 Reset();VectorSet(g_entities[4].currentOrigin,0,0,-1000);Tick();CHECK(shots==0);Tick(0,1);CHECK(shots==0&&pitch[1]>0);

 for(int scenario=0;scenario<9;++scenario){Reset();
  if(scenario==0)exhausted=true;
  if(scenario==1)badMatrix=true;
  if(scenario==2)badBone=true;
  if(scenario==3)vehicle.m_iMuzzleTag[2]=vehicle.m_iMuzzleTag[3]=-1;
  if(scenario==4)info.turret[0].iWeapon=MAX_VEH_WEAPONS;
  if(scenario==5)info.turret[0].yawAxis=3;
  if(scenario==6)vehicle.m_iDieTime=1;
  if(scenario==7)vehicle.m_iRemovedSurfaces=1;
  if(scenario==8)info.turret[0].bAI=qfalse;
  Tick();CHECK(shots==0&&flashes==0&&vehicle.turretStatus[0].ammo==120);
 }
 Reset();vehicle.turretStatus[0].ammo=0;Tick();CHECK(shots==0);vehicle.turretStatus[0].ammo=1;Tick(500);CHECK(shots==1&&vehicle.turretStatus[0].ammo==0);
 Reset();vehicle.turretStatus[0].nextMuzzle=999;Tick();CHECK(shots==1&&shotBolt==2);

 // Existing save serializers retain target, barrel, ammunition and cooldown.
 Reset();Tick();MemorySave memory;ojk::SavedGameHelper save(&memory);vehicle.turretStatus[0].sg_export(save);vehicle.m_Muzzles[3].sg_export(save);
 const auto status=vehicle.turretStatus[0];vehicle.turretStatus[0]={};vehicle.m_Muzzles[3]={};memory.reset_buffer_offset();vehicle.turretStatus[0].sg_import(save);vehicle.m_Muzzles[3].sg_import(save);
 CHECK(vehicle.turretStatus[0].ammo==status.ammo&&vehicle.turretStatus[0].nextMuzzle==3&&vehicle.turretStatus[0].enemyEntNum==4&&vehicle.turretStatus[0].enemyHoldTime==status.enemyHoldTime);
 Tick(99);CHECK(shots==1);Tick(1);CHECK(shots==2&&shotBolt==3);

 // The X-wing definition recharges one shield point every 400 ms.
 Reset();info.shields=700;info.shieldRechargeMS=400;
 auto& ps=g_entities[2].client->ps;
 ps.stats[STAT_ARMOR]=vehicle.m_iShields=698;
 vehicle.lastShieldInc=1000;
 G_VehicleRechargeShields(vehicle,ps,1399);CHECK(vehicle.m_iShields==698);
 G_VehicleRechargeShields(vehicle,ps,1400);CHECK(vehicle.m_iShields==699&&ps.stats[STAT_ARMOR]==699&&vehicle.lastShieldInc==1400);
 G_VehicleRechargeShields(vehicle,ps,1400);G_VehicleRechargeShields(vehicle,ps,1450);
 CHECK(vehicle.m_iShields==699&&vehicle.lastShieldInc==1400);

 MemorySave shieldMemory;ojk::SavedGameHelper shieldSave(&shieldMemory);
 vehicle.sg_export(shieldSave);shieldMemory.reset_buffer_offset();
 Vehicle_t restored{};restored.sg_import(shieldSave);shieldSave.ensure_all_data_read();
 restored.m_pVehicleInfo=&info;
 CHECK(restored.lastShieldInc==1400&&restored.m_iShields==699);
 G_VehicleRechargeShields(restored,ps,1799);CHECK(restored.m_iShields==699);
 G_VehicleRechargeShields(restored,ps,1800);CHECK(restored.m_iShields==700&&ps.stats[STAT_ARMOR]==700);
 G_VehicleRechargeShields(restored,ps,2400);CHECK(restored.m_iShields==700&&restored.lastShieldInc==1800);

 ps.stats[STAT_ARMOR]=restored.m_iShields=0;
 G_VehicleRechargeShields(restored,ps,2600);CHECK(restored.m_iShields==0);
 ps.stats[STAT_ARMOR]=restored.m_iShields=15;info.shieldRechargeMS=0;
 G_VehicleRechargeShields(restored,ps,3000);CHECK(restored.m_iShields==15);
 });}
