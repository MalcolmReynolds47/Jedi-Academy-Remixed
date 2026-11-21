// Production portable properties, spawn and activation; native transition boundary.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "qcommon/jar_hub_transition.h"
#include "qcommon/jar_player_transfer.h"
#include "memory_save.h"
#include <deque>
#include <memory>
game_import_t gi{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;
static std::string map,spawn,status;static bool hub=false,missing=false;static unsigned transitions=0,behavior=0;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);}
void G_ChangeMap(const char*m,const char*s,qboolean h){map=m;spawn=s;hub=h!=qfalse;++transitions;}
qboolean G_ActivateBehavior(gentity_t*e,int){if(behavior==1)e->inuse=qfalse;if(behavior==2)e->targetname=G_NewString("reused_slot");if(behavior==3)e->message=G_NewString("hoth2;quit");return qfalse;}
#include "game/g_level_change_use.inc"
int main(){return RunTests("portable level change destinations, activation, lifetime and native saved fields",[]{
 gi.FS_ReadFile=[](const char*p,void**out)->long{CHECK(!out);return !missing&&(!strcmp(p,"maps/hoth2.bsp")||!strcmp(p,"maps/t1_surprise.bsp"))?100:-1;};
 gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};gi.cvar_set=[](const char*,const char*s){status=s;};
 using namespace jar::levelChange;
 Settings s;s.map="hoth2";CHECK(Valid(s));s.map="custom/my-map_1";s.spawn="entry-2";CHECK(Valid(s));
 for(const char*bad:{"","../hoth2","/hoth2","hoth2/","a//b","hoth2.bsp","+menu","hoth2;quit","hoth2\nquit","hoth2\rquit","hoth2 quit","a\\b","$map","a\"b"}){s.map=bad;CHECK(!Valid(s));}
 s.map=std::string(55,'a');CHECK(!Valid(s));s.map=std::string("hoth2\0evil",10);CHECK(!Valid(s));s.map="hoth2";
 for(const char*bad:{"entry;quit","entry\nquit","two words","/entry","+menu"}){s.spawn=bad;CHECK(!Valid(s));}s.spawn=std::string(64,'a');CHECK(!Valid(s));s.spawn="";
 auto&e=g_entities[10];e.inuse=qtrue;e.s.number=10;e.classname=G_NewString("target_level_change");e.model=G_NewString("@jar_level_change");e.message=G_NewString("hoth2");e.targetname=G_NewString("owner_10");e.s.origin[0]=42;
 CHECK(G_LevelChangeSpawn(e)&&e.e_UseFunc==useF_target_level_change_use&&e.count==-1&&e.noise_index==0&&e.currentOrigin[0]==42&&!transitions);
 s.map="t1_surprise";s.spawn="arrival";s.hub=true;CHECK(G_LevelChangeSet(e,s)&&!transitions);Settings read;CHECK(G_LevelChangeRead(e,read)&&read.map==s.map&&read.spawn==s.spawn&&read.hub);
 G_LevelChangeUse(&e);CHECK(transitions==1&&map==s.map&&spawn==s.spawn&&hub);
 const char*before=e.message;missing=true;CHECK(!G_LevelChangeSet(e,s)&&e.message==before);G_LevelChangeUse(&e);CHECK(transitions==1&&status.find("unavailable")!=std::string::npos);missing=false;
 s.map="hoth2";s.spawn.clear();s.hub=false;CHECK(G_LevelChangeSet(e,s)&&!e.target);G_LevelChangeUse(&e);CHECK(transitions==2&&map=="hoth2"&&spawn.empty()&&!hub);
 behavior=1;G_LevelChangeUse(&e);CHECK(transitions==2);e.inuse=qtrue;behavior=2;G_LevelChangeUse(&e);CHECK(transitions==2);behavior=3;G_LevelChangeUse(&e);CHECK(transitions==2);behavior=0;
 CHECK(!G_LevelChangeSpawn(e));e.model=nullptr;CHECK(!G_LevelChangeSet(e,s)&&!G_LevelChangeRead(e,read));G_LevelChangeUse(&e);CHECK(transitions==2);
 // Native saving first substitutes string indices, then restores strings on load.
 // Exercise that same gentity wire format; no new field, callback ID or schema.
 gentity_t source{},restored{};source.inuse=qtrue;source.s.number=11;source.count=-1;source.spawnflags=1;source.e_UseFunc=useF_target_level_change_use;
 source.classname=reinterpret_cast<char*>(1);source.model=reinterpret_cast<char*>(2);source.message=reinterpret_cast<char*>(3);source.target=reinterpret_cast<char*>(4);source.targetname=reinterpret_cast<char*>(5);
 MemorySave saved;ojk::SavedGameHelper helper(&saved);source.sg_export(helper);saved.reset_buffer_offset();restored.sg_import(helper);saved.ensure_all_data_read();
 CHECK(restored.count==-1&&restored.spawnflags==1&&restored.e_UseFunc==useF_target_level_change_use&&restored.classname==source.classname&&restored.model==source.model&&restored.message==source.message&&restored.target==source.target&&restored.targetname==source.targetname);
 restored.classname=G_NewString("target_level_change");restored.model=G_NewString("@jar_level_change");restored.message=G_NewString("hoth2");restored.target=G_NewString("saved_entry");restored.targetname=G_NewString("owner_11");
 G_LevelChangeUse(&restored);CHECK(transitions==3&&map=="hoth2"&&spawn=="saved_entry"&&hub);

 // Loading a destination's level chunk resumes its world time while its
 // arrival name remains the current visit's request.
 auto savedHub=std::make_unique<level_locals_t>();
 auto arrival=std::make_unique<level_locals_t>();
 std::strcpy(savedHub->mapname,"hoth2");std::strcpy(savedHub->spawntarget,"earlier_visit");
 std::strcpy(arrival->spawntarget,"saved_entry");savedHub->time=45000;
 MemorySave levelBytes;ojk::SavedGameHelper levelIO(&levelBytes);
 savedHub->sg_export(levelIO);levelBytes.reset_buffer_offset();arrival->sg_import(levelIO);levelBytes.ensure_all_data_read();
 CHECK(arrival->time==45000&&!strcmp(arrival->mapname,"hoth2")&&!strcmp(arrival->spawntarget,"saved_entry"));
 for(const char* key:{"playersave","playerammo","playerinv","playerfplvl","spawntarget","tiers_complete",
                      "jar_toolMode","jar_inventoryTransferCount","jar_inventoryTransfer0","jar_inventoryTransfer119"})
  CHECK(jar::hubTransition::RetainOutgoingCvar(key));
 for(const char* key:{"tier_storyinfo","mapname","jar_worldSeed","jar_worldMode","g_gravity"})
  CHECK(!jar::hubTransition::RetainOutgoingCvar(key));

 using jar::playerTransfer::Decode;
 using jar::playerTransfer::Encode;
 jar::playerTransfer::State carry;
 // Original retail layout, including the older entity sentinel and null
 // saber name, remains readable through the production transfer parser.
 const std::string header="87 28 6 128 2 0 1420 11.25 -22.5 3.75 129 42 120 150 3 ";
 const std::string blades=" 1 0 1 0 0 0 0 0 3 4 1 0 0 0 0 0 ";
 const std::string secondary="(null) 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 1 1023 0";
 const auto retail=header+"single_1"+blades+secondary;
 CHECK(Decode(retail.c_str(),carry)&&carry.health==87&&carry.armor==28&&carry.weapon==WP_BLASTER_PISTOL&&
       carry.view[1]==-22.5f&&carry.forceMax==120&&carry.sabers[0].active[2]&&
       carry.sabers[0].color[1]==SABER_BLUE&&!carry.sabers[1].name[0]&&carry.lockEnemy==1023);
 for(const auto& broken:{retail.substr(0,retail.find_last_of(' ')),header+std::string(MAX_QPATH,'x')+blades+secondary,
                        std::string("2147483648")+retail.substr(2),retail+" 0",
                        std::string("87 28 6 128 2 0 1420 nan")+retail.substr(retail.find(" -22.5"))}){
  carry.health=412;
  CHECK(!Decode(broken.c_str(),carry)&&carry.health==412);
 }
 playerState_t writer{};
 writer.stats[STAT_HEALTH]=75;writer.stats[STAT_WEAPONS]=int((1u<<WP_PHYSGUN)|(1u<<WP_SABER));
 writer.weapon=WP_PHYSGUN;writer.weaponstate=WEAPON_READY;writer.saberLockEnemy=ENTITYNUM_NONE;
 writer.saber[0].name="single_2";writer.saber[0].blade[7].active=qtrue;
 writer.saber[0].blade[7].color=SABER_PURPLE;
 writer.saber[1].name="";
 const auto emitted=Encode(writer);
 CHECK(Decode(emitted.c_str(),carry)&&carry.health==75&&carry.weapon==WP_PHYSGUN&&
       carry.weapons==writer.stats[STAT_WEAPONS]&&carry.lockEnemy==ENTITYNUM_NONE&&
       !strcmp(carry.sabers[0].name,"single_2")&&!carry.sabers[1].name[0]&&
       carry.sabers[0].active[7]&&carry.sabers[0].color[7]==SABER_PURPLE);
 std::array<int,AMMO_MAX> ammo{};
 CHECK(jar::playerTransfer::ReadAmmo(" 0 -1 125 \t",ammo)&&ammo[1]==-1&&ammo[2]==125&&ammo.back()==0);
 const auto priorAmmo=ammo;
 std::string tooManyAmmo;
 for(int i=0;i<=AMMO_MAX;++i)tooManyAmmo+=" 2";
 for(const auto& brokenAmmo:{tooManyAmmo,std::string("0 -5"),std::string("1 25junk"),std::string("1 999999999999")})
  CHECK(!jar::playerTransfer::ReadAmmo(brokenAmmo.c_str(),ammo)&&ammo==priorAmmo);
 std::array<int,NUM_FORCE_POWERS> ranks{};
 CHECK(jar::playerTransfer::ReadForceRanks("0 3 2 1 0 4 3 0 3 3 7",ranks)&&
       ranks[FP_TELEPATHY]==4&&ranks[FP_SABER_OFFENSE]==7&&ranks.back()==0);
 const auto priorRanks=ranks;
 CHECK(!jar::playerTransfer::ReadForceRanks("0 4",ranks)&&ranks==priorRanks);
 CHECK(!jar::playerTransfer::ReadForceRanks("0 3 2 1 0 5",ranks)&&ranks==priorRanks);
 });}
