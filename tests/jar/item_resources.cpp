// Production retail item parser, resource repair and holocron pickup effects.
#include "game/g_local.h"
#include "game/g_carried_weapon_art.h"
#include "game/g_items.h"
#include "game/g_bsp_section_korriban.h"
#include "game/g_bsp_section_sentinel.h"
#include "game/g_bsp_section_pickup_specs.h"
#include "game/g_bsp_section_fx.h"
#include "game/wp_saber.h"
#include "memory_save.h"
#include "test_support.h"
#include <map>
#include <deque>
#include <fstream>
#include <iterator>
game_import_t gi{};
gitem_t bg_itemlist[ITM_NUM_ITEMS]{};extern const int bg_numItems=ITM_NUM_ITEMS;
static std::map<std::string,std::string> files,vars;
static std::deque<std::string> strings;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
#include "game/g_itemLoad.cpp"
qboolean missionInfo_Updated=qfalse;
vmCvar_t cg_updatedDataPadForcePower1{},cg_updatedDataPadForcePower2{},cg_updatedDataPadForcePower3{};
#include "game/g_holocron_pickup.inc"
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
int G_ModelIndex(const char*){return 7;}
static std::string probed;static bool probeOK=true;
#include "game/g_workshop_pickup.inc"
int main(int argc,char**argv){return RunTests("retail item resource repair and native holocron grant semantics",[&]{
 gi.Printf=Com_Printf;gi.cvar_set=[](const char*k,const char*v){vars[k]=v;};
 gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};
 gi.FS_ReadFile=[](const char*n,void**out)->long{const auto it=files.find(n);if(it==files.end()){if(out)*out=nullptr;return -1;}if(out){auto*b=new char[it->second.size()+1];memcpy(b,it->second.c_str(),it->second.size()+1);*out=b;}return long(it->second.size());};
 gi.FS_FreeFile=[](void*p){delete[] static_cast<char*>(p);};
 gi.G2API_InitGhoul2Model=[](CGhoul2Info_v&,const char*name,int index,qhandle_t,qhandle_t,int,int){CHECK(index==7);probed=name;return probeOK?0:-1;};
 if(argc==3){
  std::ifstream dat(argv[1],std::ios::binary);CHECK(dat.good());files["ext_data/items.dat"]=std::string(std::istreambuf_iterator<char>(dat),{});
  std::ifstream paths(argv[2]);CHECK(paths.good());for(std::string path;std::getline(paths,path);)files.emplace(path,"asset");
  IT_LoadItemParms();int matched=0;
  const auto& boon=bg_itemlist[ITM_JAR_FORCE_BOON];
  CHECK(boon.giType==IT_POWERUP&&boon.giTag==PW_FORCE_BOON&&boon.quantity==25&&
        files.count(boon.world_model)&&files.count(boon.pickup_sound));
  const std::string icon=boon.icon;
  CHECK(files.count(icon+".tga")||files.count(icon+".png")||files.count(icon+".jpg"));
  for (unsigned i=0;i<ARRAY_LEN(jar::enlightenment::pickups);++i) {
   const auto& powerup=bg_itemlist[ITM_JAR_LIGHT_ENLIGHTENMENT+i];
   const auto& spec=jar::enlightenment::pickups[i];
   CHECK(powerup.classname==spec.classname&&powerup.giType==IT_POWERUP&&powerup.giTag==spec.tag);
   CHECK(files.count(powerup.world_model)&&files.count(powerup.pickup_sound));
   const std::string image=powerup.icon;
   CHECK(files.count(image+".tga")||files.count(image+".png")||files.count(image+".jpg"));
  }
  for(const auto&r:jar::itemResources::replacements)for(int i=1;i<bg_numItems;++i){auto&item=bg_itemlist[i];if(item.classname&&!strcmp(item.classname,r.name)){CHECK(item.giType==r.type&&item.giTag==r.tag&&!strcmp(item.world_model,r.model));++matched;printf("Retail pickup admitted: %s -> %s\n",r.name,item.world_model);}}
  CHECK(matched==13);CHECK(bg_itemlist[ITM_SHIELD_PICKUP].giTag==INV_PORTABLE_SHIELD);
  const auto&pad=bg_itemlist[ITM_DATAPAD_PICKUP];CHECK(pad.giTag==INV_DATAPAD&&pad.giType==IT_HOLDABLE&&pad.quantity==1&&!strcmp(pad.icon,"models/items/datapad_screen"));
  for(const auto& spec:jar::korriban::pickups){
   const auto* item=std::find_if(bg_itemlist+1,bg_itemlist+bg_numItems,[&](const gitem_t& row){
    return row.classname&&!strcmp(row.classname,spec.name);
   });
   CHECK(item!=bg_itemlist+bg_numItems&&item->world_model&&files.count(item->world_model));
   CHECK(item->giType==IT_WEAPON||item->giType==IT_AMMO||item->giType==IT_HEALTH||item->giType==IT_ARMOR);
  }
  CHECK(files.count(jar::bspSectionFX::effectFile)&&files.count(jar::bspSectionFX::loop)&&
        files.count("sound/ambience/prototype/windmix1.wav"));
  for(const auto& spec:jar::sentinel::pickups){
   const auto* item=std::find_if(bg_itemlist+1,bg_itemlist+bg_numItems,[&](const gitem_t& row){
    return row.classname&&!strcmp(row.classname,spec.name);
   });
   CHECK(item!=bg_itemlist+bg_numItems&&item->world_model&&files.count(item->world_model));
   CHECK(item->giType==IT_WEAPON||item->giType==IT_AMMO||item->giType==IT_HEALTH||item->giType==IT_ARMOR);
  }
  for(const char* sound:jar::sentinel::pistonSounds)CHECK(files.count(sound));
  for(const auto& sound:jar::taspir::sounds)for(const char* file:sound.files)CHECK(files.count(file));
  for(const auto& scenery:jar::taspir::scenery)CHECK(files.count(std::string(jar::taspir::sceneryPrefix)+scenery.model));
  CHECK(files.count(jar::factory::baseModel)&&files.count(jar::factory::glassSound)&&files.count(jar::factory::glassImpact));
  for(const auto& flag:jar::factory::flags)CHECK(files.count(flag.model));
  for(const auto& sound:jar::coruscant::sounds)CHECK(files.count(sound.file));
  CHECK(files.count(jar::hilltops::exterior)&&files.count(jar::hilltops::wind));
  for(const char* call:jar::hilltops::birdCalls)CHECK(files.count(call));
  for(const auto& sound:jar::dropzone::ambience)CHECK(files.count(sound.path));
  for(const char* file:{jar::wasteland::crateModel,jar::wasteland::gunModel,jar::wasteland::gunSound,
                       jar::wasteland::gunExitSound,jar::wasteland::gunMoveSound,jar::wasteland::gunExplosion,
                       jar::wasteland::snowImage})CHECK(files.count(file));
  for(const auto mode:{jar::bspSection::Mode::Tatooine,jar::bspSection::Mode::Taspir,jar::bspSection::Mode::Factory,
                       jar::bspSection::Mode::Coruscant,jar::bspSection::Mode::Hilltops,jar::bspSection::Mode::DropZone,
                       jar::bspSection::Mode::Wasteland}){
   size_t count;
   const auto* specs=jar::bspSection::PickupSpecs(mode,count);
   for(size_t i=0;i<count;++i){
   const auto* item=std::find_if(bg_itemlist+1,bg_itemlist+bg_numItems,[&](const gitem_t& row){
    return row.classname&&!strcmp(row.classname,specs[i].name);
   });
   CHECK(item!=bg_itemlist+bg_numItems&&item->world_model&&files.count(item->world_model));
   CHECK(item->giType==IT_WEAPON||item->giType==IT_AMMO||item->giType==IT_HEALTH||item->giType==IT_ARMOR||item->giType==IT_POWERUP);
   }
  }
  for(const auto& entry:jar::carriedWeapon::Entries()){
   auto* item=std::find_if(bg_itemlist+1,bg_itemlist+bg_numItems,[&](const gitem_t& row){
    return row.classname&&!strcmp(row.classname,entry.item);
   });
   CHECK(item!=bg_itemlist+bg_numItems&&jar::carriedWeapon::StockPickupArt(*item)==&entry.art);
   CHECK(files.count(entry.art.pickup));
   gentity_t drop{};drop.s.eType=ET_ITEM;drop.item=item;
   CHECK(workshopPickupModel(drop)&&probed==entry.art.pickup);
  }
  return;
 }
 // The actual parser and post-load repair, including preserved stock quantity/tag.
 files["ext_data/items.dat"]="{\nitemname ITM_FORCE_HEAL_PICKUP\nclassname holocron_force_heal\nworldmodel models/map_objects/force_holocrons/heal.md3\ncount 1\ntype IT_HOLOCRON\ntag FP_HEAL\n}\n";
 const auto&r=jar::itemResources::replacements[0];files[r.model]="model";IT_LoadItemParms();auto&item=bg_itemlist[ITM_FORCE_HEAL_PICKUP];
 CHECK(!strcmp(item.world_model,r.model)&&item.quantity==1&&item.giTag==FP_HEAL&&item.giType==IT_HOLOCRON);
 {
  using namespace jar::forceBoon;
  const auto& boon=bg_itemlist[ITM_JAR_FORCE_BOON];
  CHECK(!strcmp(boon.classname,classname)&&boon.giTag==PW_FORCE_BOON&&boon.giType==IT_POWERUP);
  const int expires=Deadline(1250,0,boon.quantity);
  CHECK(expires==26000&&Deadline(1500,expires,boon.quantity)==51000);
  CHECK(Deadline(32000,expires,boon.quantity)==57000);
  CHECK(Deadline(std::numeric_limits<int>::max()-1000,0,boon.quantity)==std::numeric_limits<int>::max());
  CHECK(Drain(expires,1500,9)==4&&Drain(expires,1500,1)==1&&Drain(expires,1500,0)==0);
  CHECK(Recharge(expires,1500,2)==6&&Recharge(expires,expires,2)==2&&Drain(expires,expires,9)==9);
  CHECK(PW_REMOVE_AT_DEATH&(1<<PW_FORCE_BOON));
  MemorySave saved;ojk::SavedGameHelper helper(&saved);playerState_t player{},restored{};
  player.powerups[PW_FORCE_BOON]=expires;player.powerups[PW_SEEKER]=3000;
  player.sg_export(helper);saved.reset_buffer_offset();restored.sg_import(helper);helper.ensure_all_data_read();
  CHECK(restored.powerups[PW_FORCE_BOON]==expires&&restored.powerups[PW_SEEKER]==3000);
 }
 files["ext_data/items.dat"]="{\nitemname ITM_SHIELD_PICKUP\nclassname item_enviro\nworldmodel models/items/shield.md3\ncount 100\ntype IT_HOLDABLE\ntag ITM_SHIELD_PICKUP\n}\n";
 IT_LoadItemParms();CHECK(bg_itemlist[ITM_SHIELD_PICKUP].giTag==INV_PORTABLE_SHIELD&&bg_itemlist[ITM_SHIELD_PICKUP].giType==IT_HOLDABLE);
 files["ext_data/items.dat"]="{\nitemname ITM_DATAPAD_PICKUP\nclassname item_datapad\nworldmodel models/items/datapad.md3\ncount 1\ntype IT_HOLDABLE\ntag ITM_DATAPAD_PICKUP\n}\n";
 files["models/items/datapad_screen.jpg"]="stock image";IT_LoadItemParms();auto&pad=bg_itemlist[ITM_DATAPAD_PICKUP];
 CHECK(pad.giTag==INV_DATAPAD&&pad.giType==IT_HOLDABLE&&pad.quantity==1&&!strcmp(pad.icon,"models/items/datapad_screen"));
 pad.icon="custom/icon";JAR_RepairItemResources();CHECK(!strcmp(pad.icon,"custom/icon"));
 pad.icon=nullptr;pad.giTag=INV_SEEKER;JAR_RepairItemResources();CHECK(!pad.icon);pad.giTag=INV_DATAPAD;
 pad.giType=IT_BAD;JAR_RepairItemResources();CHECK(!pad.icon);pad.giType=IT_HOLDABLE;
 files.erase("models/items/datapad_screen.jpg");JAR_RepairItemResources();CHECK(!pad.icon);
 // Every mapping is gated on the exact authored path/type/tag and file presence.
 for(const auto&entry:jar::itemResources::replacements){auto&i=bg_itemlist[1];i={};i.classname=entry.name;i.world_model=entry.original;i.giType=itemType_t(entry.type);i.giTag=entry.tag;i.quantity=17;
  files.erase(entry.original);files.erase(entry.model);JAR_RepairItemResources();CHECK(!strcmp(i.world_model,entry.original));
  files[entry.model]="model";files[entry.original]="mod-supplied model";JAR_RepairItemResources();CHECK(!strcmp(i.world_model,entry.original));
  files.erase(entry.original);i.world_model="custom/model.md3";JAR_RepairItemResources();CHECK(!strcmp(i.world_model,"custom/model.md3"));
  i.world_model=entry.original;i.giType=IT_BAD;JAR_RepairItemResources();CHECK(!strcmp(i.world_model,entry.original));
  i.giType=itemType_t(entry.type);i.giTag=999;JAR_RepairItemResources();CHECK(!strcmp(i.world_model,entry.original));
  i.giTag=entry.tag;JAR_RepairItemResources();CHECK(!strcmp(i.world_model,entry.model)&&i.quantity==17);
 }
 gentity_t pickup{},player{};gclient_t client{};player.client=&client;gitem_t holo{};holo.giType=IT_HOLOCRON;pickup.item=&holo;
 {
  using namespace jar::enlightenment;
  auto& ps=client.ps;
  CHECK(CanCollect(ps,Light)&&CanCollect(ps,Dark));
  ps.forcePowerLevel[FP_PROTECT]=1;
  ps.forcePowerLevel[FP_SABER_OFFENSE]=4;
  ps.forcePowersKnown=(1<<FP_PROTECT)|(1<<FP_SABER_OFFENSE);
  CHECK(Alignment(ps)==Light&&CanCollect(ps,Light)&&!CanCollect(ps,Dark));
  Extend(ps,Light,1250,25);
  CHECK(Active(ps)==Light&&Deadline(ps)==26000);
  CHECK(Rank(ps,FP_PROTECT)==3&&Rank(ps,FP_GRIP)==0&&Rank(ps,FP_SABER_OFFENSE)==4);
  CHECK(Known(ps)&(1<<FP_TELEPATHY));
  CHECK(ps.forcePowerLevel[FP_PROTECT]==1);
  Extend(ps,Light,1500,25);CHECK(Deadline(ps)==51000);
  ps.forcePowersActive=(1<<FP_PROTECT)|(1<<FP_SABER_OFFENSE);
  CHECK(AffectedActivePowers(ps)==(1<<FP_PROTECT));
  ps.clientNum=1;CHECK(!CanCollect(ps,Light)&&Rank(ps,FP_PROTECT)==1);ps.clientNum=0;

  // Campaign gains during Enlightenment still update the canonical ranks.
  holo.giTag=FP_HEAL;pickup.count=2;Pickup_Holocron(&pickup,&player);
  CHECK(Rank(ps,FP_HEAL)==3&&ps.forcePowerLevel[FP_HEAL]==2);
  MemorySave saved;ojk::SavedGameHelper helper(&saved);playerState_t restored{};
  ps.sg_export(helper);saved.reset_buffer_offset();restored.sg_import(helper);helper.ensure_all_data_read();
  CHECK(Deadline(restored)==51000&&Rank(restored,FP_HEAL)==3&&restored.forcePowerLevel[FP_HEAL]==2);
  restored.powerups[PW_FORCE_ENLIGHTENMENT]=0;
  CHECK(Rank(restored,FP_HEAL)==2&&!(Known(restored)&(1<<FP_TELEPATHY)));
  ps={};ps.forcePowerLevel[FP_GRIP]=1;ps.forcePowersKnown=1<<FP_GRIP;
  CHECK(Alignment(ps)==Dark&&CanCollect(ps,Dark)&&!CanCollect(ps,Light));
  Extend(ps,Dark,2000,25);CHECK(Rank(ps,FP_LIGHTNING)==3&&Rank(ps,FP_PROTECT)==0&&Rank(ps,FP_LEVITATION)==3);
  ps={};
 }
 holo.world_model="models/map_objects/mp/lt_heal.md3";CHECK(workshopPickupModel(pickup)&&probed.empty());
 holo.world_model="models/weapons2/stun_baton/baton_w.glm";CHECK(workshopPickupModel(pickup)&&probed==holo.world_model);probeOK=false;CHECK(!workshopPickupModel(pickup));
 gitem_t sentry{};
 sentry.classname="weapon_turret";sentry.giTag=WP_TURRET;sentry.giType=IT_WEAPON;
 sentry.world_model=jar::carriedWeapon::placeholderView;
 gentity_t drop{};drop.s.eType=ET_ITEM;drop.item=&sentry;
 const auto& art=jar::carriedWeapon::Find(WP_TURRET)->art;
 probed.clear();CHECK(workshopPickupModel(drop)&&probed.empty()); // Missing stock media falls back.
 files[art.pickup]="stock";probeOK=true;
 CHECK(workshopPickupModel(drop)&&probed==art.pickup);
 probed.clear();workshopRestoreCarriedPickup(drop);CHECK(probed==art.pickup);
 probeOK=false;CHECK(!workshopPickupModel(drop));probeOK=true;
 sentry.world_model="models/custom/turret.glm";
 CHECK(workshopPickupModel(drop)&&probed==sentry.world_model);
 sentry.world_model=jar::carriedWeapon::placeholderView;
 sentry.classname="weapon_custom";
 probed.clear();CHECK(workshopPickupModel(drop)&&probed.empty());
 for(int power:{FP_HEAL,FP_LEVITATION,FP_SPEED,FP_PUSH,FP_PULL,FP_TELEPATHY,FP_GRIP,FP_LIGHTNING,FP_SABERTHROW}){
  client.ps={};holo.giTag=power;pickup.count=1;missionInfo_Updated=qfalse;vars.clear();Pickup_Holocron(&pickup,&player);
  CHECK(client.ps.forcePowersKnown==(1<<power)&&client.ps.forcePowerLevel[power]==1&&missionInfo_Updated&&vars["cg_updatedDataPadForcePower1"]==std::to_string(power+1));
  pickup.count=3;Pickup_Holocron(&pickup,&player);CHECK(client.ps.forcePowerLevel[power]==3);
  pickup.count=2;missionInfo_Updated=qfalse;vars.clear();Pickup_Holocron(&pickup,&player);CHECK(client.ps.forcePowerLevel[power]==3&&!missionInfo_Updated&&vars.empty());
 }
 const auto before=client.ps;for(int bad:std::initializer_list<int>{-1,NUM_FORCE_POWERS,100}){holo.giTag=bad;Pickup_Holocron(&pickup,&player);CHECK(!memcmp(&before,&client.ps,sizeof(before)));}
 holo.giTag=FP_HEAL;for(int bad:std::initializer_list<int>{-1,NUM_FORCE_POWER_LEVELS,100}){pickup.count=bad;Pickup_Holocron(&pickup,&player);CHECK(!memcmp(&before,&client.ps,sizeof(before)));}
 player.client=nullptr;Pickup_Holocron(&pickup,&player);pickup.item=nullptr;Pickup_Holocron(&pickup,&player);
 // The level uses the native gentity serializer, with no new save layout.
 MemorySave saved;ojk::SavedGameHelper helper(&saved);gentity_t source{},restored{};
 source.count=3;source.s.eType=ET_ITEM;source.sg_export(helper);saved.reset_buffer_offset();restored.sg_import(helper);saved.ensure_all_data_read();CHECK(restored.count==3&&restored.s.eType==ET_ITEM);
 });}
