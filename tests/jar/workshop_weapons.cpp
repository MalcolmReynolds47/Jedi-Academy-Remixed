// Exercise the production SP catalog grant and variant-drop controller.
#include "cgame/cg_local.h"
#include "game/g_local.h"
#include "game/wp_saber.h"
#include "game/w_local.h"
#include "game/g_carried_weapon_art.h"
#include "qcommon/jar_catalog_generated.h"
#include "test_support.h"
#include <algorithm>
#include <map>
cg_t cg{};game_import_t gi{};
gitem_t bg_itemlist[WP_NUM_WEAPONS]{};
weaponData_t weaponData[WP_NUM_WEAPONS]{};
int numSpawnVars=3;qboolean spawning=qfalse;
static gentity_t fixturePlayer,dropped;static gclient_t client;
static bool allowed=true,validSaber=true,knockdown=false,roll=false,dropFailed=false;
static int registrations,stops,ammoGrants,lists;static std::string status,melee,dropName;
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected model allocation");}
gitem_t*FindItemForWeapon(weapon_t w){return &bg_itemlist[w];}
void RegisterItem(gitem_t*){++registrations;}
void CG_RegisterItemVisuals(int){} void CG_RegisterWeapon(int){}
void Add_Ammo(gentity_t*p,int w,int count){++ammoGrants;auto&a=p->client->ps.ammo[weaponData[w].ammoIndex];a=std::min(300,a+count);}
qboolean PM_InKnockDown(playerState_t*){return knockdown?qtrue:qfalse;}
qboolean PM_InRoll(playerState_t*){return roll?qtrue:qfalse;}
gentity_t*G_DropSaberItem(const char*name,saber_colors_t color,vec3_t,vec3_t,vec3_t,gentity_t*){
 CHECK(numSpawnVars==0&&spawning);dropName=name;CHECK(color==(!strcmp(name,"jar_shortblade")?SABER_PURPLE:SABER_YELLOW));
 dropped.inuse=dropFailed?qfalse:qtrue;return &dropped;
}
static bool sandboxReady(){return allowed;}
static gentity_t*workshopPlayer(){return &fixturePlayer;}
static void message(const char*s){status=s;}
static void list(){++lists;}
static void stopTool(){++stops;}
static bool saberValid(const char*){return validSaber;}
static void grant(const char*name,bool select){CHECK(select);melee=name;}
struct Supply {uint32_t id;};static Supply supplied;static gitem_t*suppliedItem;
static const Supply*supply(uint32_t id){return id==supplied.id?&supplied:nullptr;}
static gitem_t*supplyItem(const Supply&){return suppliedItem;}
#include "game/g_workshop_weapons.inc"

vec3_t forwardVec,vrightVec,up,muzzle;
static gentity_t turretBolt;
static bool missileCapacity=true;
static gentity_t* lastShooter=nullptr;
gentity_t* CreateMissile(vec3_t origin,vec3_t direction,float speed,int life,gentity_t* owner,qboolean alt){
 CHECK(VectorCompare(origin,muzzle)&&VectorCompare(direction,forwardVec));
 CHECK(speed==900&&life==10000&&!alt);
 lastShooter=owner;
 return missileCapacity?&turretBolt:nullptr;
}
#include "game/g_carried_weapon_fire.inc"

int main(){return RunTests("SP weapon card grants and melee world pickups",[]{
 fixturePlayer.client=&client;cg.time=750;
 for(int w=1;w<WP_NUM_WEAPONS;++w){bg_itemlist[w].giType=IT_WEAPON;bg_itemlist[w].giTag=w;bg_itemlist[w].quantity=25;weaponData[w].ammoIndex=AMMO_BLASTER;}
 for(uint32_t id=2002;id<=2013;++id){client.ps.weaponTime=800;client.ps.weaponstate=WEAPON_CHARGING;weaponCard(id);CHECK(cg.weaponSelect==int(id-2000)&&cg.weaponSelectTime==750&&client.ps.weaponTime==0&&client.ps.weaponstate==WEAPON_READY&&(client.ps.stats[STAT_WEAPONS]&(1u<<(id-2000))));}
 CHECK(registrations==12&&stops==12&&ammoGrants==12&&client.ps.ammo[AMMO_BLASTER]==300);
 // Each generated weapon card takes the direct grant route, including native NPC weapons.
 for(const auto&e:jar::catalog::entries)if(e.tab==2&&e.kind==4&&!e.reason[0]){supplied.id=e.id;suppliedItem=&bg_itemlist[WP_NOGHRI_STICK];weaponCard(e.id);CHECK(cg.weaponSelect==WP_NOGHRI_STICK);}
 const int before=registrations;for(unsigned id:{0u,1001u,2501u,0xffffffffu})weaponCard(id);CHECK(registrations==before);
 for(int refusal=0;refusal<4;++refusal){allowed=refusal!=0;client.ps.saberInFlight=refusal==1?qtrue:qfalse;knockdown=refusal==2;roll=refusal==3;weaponCard(2003);CHECK(registrations==before);}
 allowed=true;client.ps.saberInFlight=qfalse;knockdown=roll=false;
 weaponData[WP_BLASTER].ammoIndex=AMMO_MAX;weaponCard(2003);CHECK(registrations==before);weaponData[WP_BLASTER].ammoIndex=AMMO_BLASTER;
 for(unsigned id=2101;id<=2103;++id){weaponCard(id);CHECK(melee==workshopMelee(id));vec3_t origin{},angles{};CHECK(workshopDropMelee(id,origin,angles)==&dropped&&dropName==workshopMelee(id)&&numSpawnVars==3&&!spawning);}
 validSaber=false;melee.clear();weaponCard(2101);CHECK(melee.empty());vec3_t v{};CHECK(!workshopDropMelee(2101,v,v));
 validSaber=true;dropFailed=true;CHECK(!workshopDropMelee(2102,v,v)&&numSpawnVars==3&&!spawning);

 static float traceFraction=1;
 static bool solid=false;
 static const char* missingModel=nullptr;
 gi.FS_ReadFile=[](const char* path,void**)->long{
  return missingModel&&!strcmp(path,missingModel)?-1:100;
 };
 gi.trace=[](trace_t* result,const vec3_t start,const vec3_t mins,const vec3_t maxs,
             const vec3_t end,int skip,int mask,EG2_Collision collision,int){
  CHECK(skip==0&&mask==(MASK_SOLID|CONTENTS_SHOTCLIP)&&collision==G2_NOCOLLIDE);
  CHECK(start[0]==50&&start[1]==60&&start[2]==90);
  CHECK(mins[0]==-5&&maxs[2]==5);
  CHECK(std::fabs(end[0]-42)<.001f&&std::fabs(end[1]-72)<.001f&&end[2]==90);
  result->fraction=traceFraction;
  result->startsolid=solid?qtrue:qfalse;
  vec3_t step;
  VectorSubtract(end,start,step);
  VectorMA(start,traceFraction,step,result->endpos);
 };
 VectorSet(fixturePlayer.currentOrigin,50,60,70);
 VectorSet(client.ps.viewangles,0,90,0);
 client.ps.viewheight=26;client.ps.leanofs=-14;
 for(const auto& entry:jar::carriedWeapon::Entries()){
  const auto* match=std::find_if(std::begin(jar::catalog::entries),std::end(jar::catalog::entries),
   [&](const auto& card){return card.tab==2&&card.kind==4&&!strcmp(card.name,entry.item);});
  CHECK(match!=std::end(jar::catalog::entries)&&!match->reason[0]);
  auto& native=weaponData[entry.weapon];
  Q_strncpyz(native.weaponMdl,jar::carriedWeapon::placeholderView,sizeof(native.weaponMdl));
  supplied.id=match->id;suppliedItem=&bg_itemlist[entry.weapon];weaponCard(match->id);
  CHECK(cg.weaponSelect==entry.weapon);
  CHECK(jar::carriedWeapon::StockWeaponArt(entry.weapon)==&entry.art);
  // A native attachment request can arrive before ps.weapon changes.
  CHECK(!strcmp(jar::carriedWeapon::HandModel(fixturePlayer,native.weaponMdl),entry.art.pickup));
  CHECK(jar::carriedWeapon::HandModel(fixturePlayer,jar::carriedWeapon::placeholderView)==jar::carriedWeapon::placeholderView);
  missingModel=entry.art.pickup;
  CHECK(jar::carriedWeapon::HandModel(fixturePlayer,native.weaponMdl)==native.weaponMdl);
  missingModel=nullptr;
  Q_strncpyz(native.weaponMdl,"models/custom/handheld.md3",sizeof(native.weaponMdl));
  CHECK(!jar::carriedWeapon::StockWeaponArt(entry.weapon));
  CHECK(jar::carriedWeapon::HandModel(fixturePlayer,native.weaponMdl)==native.weaponMdl);
  Q_strncpyz(native.weaponMdl,jar::carriedWeapon::placeholderView,sizeof(native.weaponMdl));
  fixturePlayer.s.weapon=client.ps.weapon=cg.weaponSelect; // Native weapon draw completed.
  CHECK(G_UseCarriedMountedWeapon(fixturePlayer));
  CHECK(G_PrepareCarriedMountedWeaponShot(fixturePlayer));
  CHECK(std::fabs(forwardVec[0])<.001f&&forwardVec[1]>.999f);
  CHECK(muzzle[0]==42&&muzzle[1]==72&&muzzle[2]==90);
 }
 for(int flag:{EF_IN_ATST,EF_LOCKED_TO_WEAPON}){
  client.ps.eFlags=flag;
  CHECK(!G_UseCarriedMountedWeapon(fixturePlayer));
  const auto* model=weaponData[fixturePlayer.s.weapon].weaponMdl;
  CHECK(jar::carriedWeapon::HandModel(fixturePlayer,model)==model);
 }
 client.ps.eFlags=0;
 fixturePlayer.s.m_iVehicleNum=12;CHECK(!G_UseCarriedMountedWeapon(fixturePlayer));
 fixturePlayer.s.m_iVehicleNum=0;
 fixturePlayer.s.number=19;CHECK(!G_UseCarriedMountedWeapon(fixturePlayer));
 fixturePlayer.s.number=0;
 traceFraction=.5f;
 CHECK(G_PrepareCarriedMountedWeaponShot(fixturePlayer));
 CHECK(muzzle[0]==46&&muzzle[1]==66&&muzzle[2]==90);
 solid=true;CHECK(!G_PrepareCarriedMountedWeaponShot(fixturePlayer));solid=false;
 fixturePlayer.s.weapon=client.ps.weapon=WP_TURRET;
 G_FireCarriedTurret(&fixturePlayer);
 CHECK(lastShooter==&fixturePlayer&&turretBolt.s.weapon==WP_TURRET&&turretBolt.damage==2);
 CHECK(!strcmp(turretBolt.classname,"turret_proj")&&turretBolt.clipmask==(MASK_SHOT|CONTENTS_LIGHTSABER));
 CHECK(turretBolt.methodOfDeath==MOD_ENERGY&&turretBolt.dflags==DAMAGE_NO_KNOCKBACK&&turretBolt.mins[2]==-1);
 weaponData[WP_TURRET].damage=10;G_FireCarriedTurret(&fixturePlayer);CHECK(turretBolt.damage==10);
 missileCapacity=false;weaponData[WP_TURRET].damage=12;G_FireCarriedTurret(&fixturePlayer);CHECK(turretBolt.damage==10);
 });}
