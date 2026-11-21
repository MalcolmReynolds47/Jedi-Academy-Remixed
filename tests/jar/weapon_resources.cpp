// Native weapon parser and projectile path with engine services captured.
#include "game/g_local.h"
#include "game/b_local.h"
#include "game/w_local.h"
#include "game/g_carried_weapon_art.h"
#include "test_support.h"
#include <map>
#include <array>
#include <fstream>
#include <iterator>
game_import_t gi{};level_locals_t level{};
weaponData_t weaponData[WP_NUM_WEAPONS]{};ammoData_t ammoData[AMMO_MAX]{};
static std::map<std::string,std::string> files;
#include "game/g_weaponLoad.cpp"
#define STUB_FX(name) void name(centity_t*,const weaponInfo_s*){CHECK(false);}
STUB_FX(FX_BryarProjectileThink)
STUB_FX(FX_BryarAltProjectileThink)
STUB_FX(FX_BlasterProjectileThink)
STUB_FX(FX_BlasterAltFireThink)
STUB_FX(FX_BowcasterProjectileThink)
STUB_FX(FX_RepeaterProjectileThink)
STUB_FX(FX_RepeaterAltProjectileThink)
STUB_FX(FX_DEMP2_ProjectileThink)
STUB_FX(FX_DEMP2_AltProjectileThink)
STUB_FX(FX_FlechetteProjectileThink)
STUB_FX(FX_FlechetteAltProjectileThink)
STUB_FX(FX_RocketProjectileThink)
STUB_FX(FX_RocketAltProjectileThink)
STUB_FX(FX_ConcProjectileThink)
STUB_FX(FX_EmplacedProjectileThink)
STUB_FX(FX_TurretProjectileThink)
STUB_FX(FX_ATSTMainProjectileThink)
STUB_FX(FX_ATSTSideMainProjectileThink)
STUB_FX(FX_ATSTSideAltProjectileThink)
STUB_FX(FX_TuskenShotProjectileThink)
STUB_FX(FX_NoghriShotProjectileThink)

#undef STUB_FX
int G_EffectIndex(const char*){return 1;}
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
vec3_t forwardVec={1,0,0},muzzle={0,0,64};static gentity_t missile;static bool exhausted;static int fired;
void WP_TraceSetStart(const gentity_t*,vec3_t,const vec3_t,const vec3_t){}
bool WP_MissileTargetHint(gentity_t*,vec3_t,vec3_t){return false;}
gentity_t*CreateMissile(vec3_t start,vec3_t direction,float speed,int life,gentity_t*owner,qboolean alt){
 CHECK(VectorCompare(start,muzzle)&&VectorCompare(direction,forwardVec)&&speed==BRYAR_PISTOL_VEL&&life==10000&&!alt&&owner->s.weapon==WP_JAWA);++fired;return exhausted?nullptr:&missile;
}
int main(int argc,char**argv){return RunTests("Jawa retail weapon load, missing-only stock art repair and native firing",[&]{
 gi.Printf=Com_Printf;
 gi.FS_ReadFile=[](const char*n,void**out)->long{auto i=files.find(n);if(i==files.end()){if(out)*out=nullptr;return -1;}if(out){char*b=new char[i->second.size()+1];memcpy(b,i->second.c_str(),i->second.size()+1);*out=b;}return long(i->second.size());};
 gi.FS_FreeFile=[](void*p){delete[] static_cast<char*>(p);};
 files["ext_data/weapons.dat"]="{\nweapontype WP_JAWA\nweaponclass weapon_jawa\nweaponmodel models/weapons2/jawa/jawa_gun.md3\nmissileFuncName bryar_func\naltmissileFuncName bryar_alt_func\nammotype 2\nenergypershot 2\naltenergypershot 2\nfiretime 400\naltfiretime 400\nrange 8192\naltrange 8192\nmuzzleEffect bryar/muzzle_flash\n}\n";
 if(argc==3){std::ifstream dat(argv[1],std::ios::binary);CHECK(dat.good());files["ext_data/weapons.dat"]=std::string(std::istreambuf_iterator<char>(dat),{});std::ifstream paths(argv[2]);CHECK(paths.good());for(std::string path;std::getline(paths,path);)files.emplace(path,"asset");}
 const auto*r=jar::itemResources::Find("weapon_jawa");const char*view="models/weapons2/briar_pistol/briar_pistol.md3";const char*hand="models/weapons2/briar_pistol/briar_pistol_hand.md3";
 const char*required[]={view,hand,r->model};for(const char*p:required){if(argc==3)CHECK(files.count(p));files[p]="asset";}
 files[r->original]="original";WP_LoadWeaponParms();std::array<weaponData_t,WP_NUM_WEAPONS> baseline;memcpy(baseline.data(),weaponData,sizeof(weaponData));files.erase(r->original);WP_LoadWeaponParms();
 auto&w=weaponData[WP_JAWA];CHECK(!strcmp(w.weaponMdl,view)&&!strcmp(w.weaponIcon,"gfx/hud/w_icon_briar")&&!strcmp(w.selectSnd,"sound/weapons/bryar/select.wav"));
 CHECK(w.ammoIndex==2&&w.energyPerShot==2&&w.altEnergyPerShot==2&&w.fireTime==400&&w.altFireTime==400&&w.range==8192&&w.func==reinterpret_cast<void*>(FX_BryarProjectileThink));
 auto normalized=w;memcpy(normalized.weaponMdl,baseline[WP_JAWA].weaponMdl,sizeof(w.weaponMdl));memcpy(normalized.weaponIcon,baseline[WP_JAWA].weaponIcon,sizeof(w.weaponIcon));memcpy(normalized.selectSnd,baseline[WP_JAWA].selectSnd,sizeof(w.selectSnd));CHECK(!memcmp(&normalized,&baseline[WP_JAWA],sizeof(w)));
 for(int i=0;i<WP_NUM_WEAPONS;++i)if(i!=WP_JAWA)CHECK(!memcmp(&weaponData[i],&baseline[i],sizeof(w)));
 for(const char*p:required){files.erase(p);WP_LoadWeaponParms();CHECK(!memcmp(&w,&baseline[WP_JAWA],sizeof(w)));files[p]="asset";}
 w=baseline[WP_JAWA];strcpy(w.weaponMdl,"custom/view.md3");auto original=w;JAR_RepairWeaponResources();CHECK(!memcmp(&w,&original,sizeof(w)));
 w=baseline[WP_JAWA];strcpy(w.classname,"custom");original=w;JAR_RepairWeaponResources();CHECK(!memcmp(&w,&original,sizeof(w)));
 w=baseline[WP_JAWA];strcpy(w.weaponIcon,"custom/icon");strcpy(w.selectSnd,"custom/select.wav");JAR_RepairWeaponResources();CHECK(!strcmp(w.weaponMdl,view)&&!strcmp(w.weaponIcon,"custom/icon")&&!strcmp(w.selectSnd,"custom/select.wav"));
 WP_LoadWeaponParms();
 if(argc==3)for(const auto& entry:jar::carriedWeapon::Entries()){
  CHECK(jar::carriedWeapon::StockWeaponArt(entry.weapon)==&entry.art);
  CHECK(files.count(entry.art.view)&&files.count(entry.art.hands)&&files.count(entry.art.pickup));
  CHECK(!entry.art.barrel||files.count(entry.art.barrel));
 }
 gentity_t shooter{};gclient_t client{};gNPC_t npc{};shooter.client=&client;shooter.s.weapon=client.ps.weapon=WP_JAWA;weaponData[WP_BRYAR_PISTOL].damage=14;
 for(int kind=0;kind<2;++kind){shooter.NPC=kind?&npc:nullptr;npc.currentAim=5;WP_FireBryarPistol(&shooter,qfalse);CHECK(missile.damage==14&&missile.s.weapon==WP_JAWA&&missile.methodOfDeath==MOD_BRYAR&&missile.clipmask==(MASK_SHOT|CONTENTS_LIGHTSABER));}
 exhausted=true;missile.damage=-1;WP_FireBryarPistol(&shooter,qfalse);CHECK(missile.damage==-1&&fired==3);
 });}
