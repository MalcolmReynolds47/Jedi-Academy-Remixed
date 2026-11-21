// Real space callbacks, saved callback dispatcher and native field serializers.
// Collision linking, animation/audio and damage application are service fixtures.
#include "game/g_local.h"
#include "game/b_local.h"
#include "game/g_functions.h"
#include "game/g_vehicles.h"
#include "memory_save.h"
#include <string>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};
gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static int damageCount=0,sounds=0,animations=0,lastDamage=0;static bool kill=false,freeOnDamage=false;
static gentity_t*lastInflictor=nullptr;
void G_Damage(gentity_t*actor,gentity_t*inflictor,gentity_t*attacker,const vec3_t,const vec3_t,int damage,int flags,int mod,int){
 CHECK(inflictor==attacker&&damage>=20&&damage<=40&&flags==DAMAGE_NO_ARMOR&&mod==MOD_SUICIDE);
 lastInflictor=inflictor;lastDamage=damage;++damageCount;actor->health=kill?0:actor->health-damage;
 if(freeOnDamage){actor->client=nullptr;actor->inuse=qfalse;}
}
void G_SoundOnEnt(gentity_t*,soundChannel_t channel,const char*path){CHECK(channel==CHAN_VOICE&&std::string(path).find("*choke")==0);++sounds;}
void NPC_SetAnim(gentity_t*,int parts,int anim,int flags,int){CHECK(parts==SETANIM_BOTH&&anim==BOTH_CHOKE1&&flags==(SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD));++animations;}
qboolean G_PointInBounds(const vec3_t p,const vec3_t low,const vec3_t high){for(int i=0;i<3;++i)if(p[i]<low[i]||p[i]>high[i])return qfalse;return qtrue;}
void InitTrigger(gentity_t*e){e->contents=CONTENTS_TRIGGER;e->svFlags=SVF_NOCLIENT;if(e->spawnflags&128)e->svFlags|=SVF_INACTIVE;}
#include "game/g_space.inc"
#define UNUSED_TOUCH(name) void name(gentity_t*,gentity_t*,trace_t*){throw std::runtime_error("Wrong touch callback: " #name);}
UNUSED_TOUCH(Touch_Item) UNUSED_TOUCH(teleporter_touch) UNUSED_TOUCH(charge_stick) UNUSED_TOUCH(Touch_DoorTrigger)
UNUSED_TOUCH(Touch_PlatCenterTrigger) UNUSED_TOUCH(Touch_Plat) UNUSED_TOUCH(Touch_Button) UNUSED_TOUCH(Touch_Multi)
UNUSED_TOUCH(trigger_push_touch) UNUSED_TOUCH(trigger_teleporter_touch) UNUSED_TOUCH(hurt_touch) UNUSED_TOUCH(NPC_Touch)
UNUSED_TOUCH(touch_ammo_crystal_tigger) UNUSED_TOUCH(funcBBrushTouch) UNUSED_TOUCH(touchLaserTrap) UNUSED_TOUCH(prox_mine_stick)
UNUSED_TOUCH(func_rotating_touch) UNUSED_TOUCH(TouchTieBomb)
UNUSED_TOUCH(bidirectional_touch)
UNUSED_TOUCH(shipboundary_touch)
UNUSED_TOUCH(portable_shield_touch)
UNUSED_TOUCH(factory_flag_touch)
UNUSED_TOUCH(siege_item_touch)
#include "game/g_entity_touch.inc"
static void Link(gentity_t*e){e->linked=qtrue;VectorAdd(e->currentOrigin,e->mins,e->absmin);VectorAdd(e->currentOrigin,e->maxs,e->absmax);}
static int Nearby(const vec3_t lo,const vec3_t hi,gentity_t**out,int limit){
 int count=0;
 for(int i=1;i<globals.num_entities&&count<limit;++i){
  auto&entity=g_entities[i];if(!entity.inuse||!entity.linked)continue;
  bool overlaps=true;
  for(int a=0;a<3;++a)overlaps&=lo[a]<=entity.absmax[a]&&hi[a]>=entity.absmin[a];
  if(overlaps)out[count++]=&entity;
 }
 return count;
}
static gentity_t&Make(int index,const char*name){auto&e=g_entities[index];e.inuse=qtrue;e.s.number=index;e.classname=const_cast<char*>(name);return e;}
static void Touch(gentity_t&volume,gentity_t&actor){GEntity_TouchFunc(&volume,&actor,nullptr);}
static void Geometry(){
 auto&v=Make(20,"trigger_space");v.model=const_cast<char*>("@jar_box");SP_trigger_space(&v);
 vec3_t size={32,128,64};CHECK(jar::volumeEntity::Resize(v,size));VectorSet(v.currentAngles,0,45,0);CHECK(jar::volumeEntity::Refresh(v));Link(&v);
 vec3_t corners[8];jar::volumeEntity::Corners(v,corners);
 for(auto&p:corners){CHECK(spaceContains(v,p));for(int a=0;a<3;++a)CHECK(p[a]>=v.absmin[a]-.001f&&p[a]<=v.absmax[a]+.001f);}
 vec3_t outside={50,50,32};CHECK(G_PointInBounds(outside,v.absmin,v.absmax)&&!spaceContains(v,outside));
 vec3_t ray={-100,0,32},direction={1,0,0};float hit=-1;
 CHECK(jar::volume::Ray(v.pos1,v.pos2,v.currentOrigin,v.currentAngles,ray,direction,200,hit)&&std::fabs(hit-(100-16*std::sqrt(2.f)))<.001f);
 CHECK(!jar::volume::Ray(v.pos1,v.pos2,v.currentOrigin,v.currentAngles,ray,direction,70,hit));
 vec3_t center={0,0,32};CHECK(jar::volume::Ray(v.pos1,v.pos2,v.currentOrigin,v.currentAngles,center,direction,100,hit)&&std::fabs(hit-16*std::sqrt(2.f))<.001f);
 CHECK(!jar::volume::Ray(v.pos1,v.pos2,v.currentOrigin,v.currentAngles,center,direction,10,hit));
 VectorSet(direction,0,0,1);CHECK(!jar::volume::Ray(v.pos1,v.pos2,v.currentOrigin,v.currentAngles,outside,direction,100,hit));
 direction[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!jar::volume::Ray(v.pos1,v.pos2,v.currentOrigin,v.currentAngles,ray,direction,100,hit));
 auto&actor=Make(21,"NPC");gclient_t client{};actor.client=&client;actor.health=100;VectorCopy(outside,client.ps.origin);Touch(v,actor);CHECK(!client.inSpaceIndex);
 VectorCopy(center,client.ps.origin);Touch(v,actor);CHECK(G_SpaceActive(&actor));VectorSet(v.currentAngles,90,0,0);jar::volumeEntity::Refresh(v);Link(&v);CHECK(!G_SpaceActive(&actor));
 CHECK(std::fabs(v.mins[0])<.001f&&std::fabs(v.maxs[0]-64)<.001f&&std::fabs(v.mins[2]+16)<.001f&&std::fabs(v.maxs[2]-16)<.001f);
 MemorySave buffer;ojk::SavedGameHelper helper(&buffer);gentity_t saved{},loaded{};
 VectorCopy(v.pos1,saved.pos1);VectorCopy(v.pos2,saved.pos2);VectorCopy(v.currentAngles,saved.currentAngles);
 saved.sg_export(helper);buffer.reset_buffer_offset();loaded.sg_import(helper);helper.ensure_all_data_read();
 CHECK(VectorCompare(loaded.pos1,v.pos1)&&VectorCompare(loaded.pos2,v.pos2)&&VectorCompare(loaded.currentAngles,v.currentAngles));
 G_SpaceRestore();CHECK(v.pos2[2]==64&&std::fabs(v.maxs[0]-64)<.001f);
 // V12 stored world-aligned mins/maxs. Import them once as local dimensions.
 VectorSet(v.mins,-16,-64,0);VectorSet(v.maxs,16,64,64);VectorClear(v.pos1);VectorClear(v.pos2);VectorSet(v.currentAngles,0,90,0);
 G_SpaceRestore(true);CHECK(v.pos1[0]==-16&&v.pos2[2]==64&&std::fabs(v.maxs[0]-64)<.001f&&std::fabs(v.maxs[1]-16)<.001f);
 G_SpaceRestore();CHECK(v.pos1[0]==-16&&v.pos2[2]==64&&std::fabs(v.maxs[1]-16)<.001f);
 VectorCopy(v.pos1,v.pos2);bool rejected=false;try{G_SpaceRestore();}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);
 v.inuse=actor.inuse=qfalse;actor.client=nullptr;
}
int main(){return RunTests("space volume entry, exposure, protection, saved state and lifetime",[]{
 static_assert(touchF_TouchTieBomb==18&&touchF_space_touch==19,"Keep serialized native callback IDs stable");
 gi.linkentity=Link;gi.EntitiesInBox=Nearby;gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid saved volume");};globals.num_entities=1100;level.time=100;
 auto&v=Make(8,"trigger_space");v.model=const_cast<char*>("@jar_box");SP_trigger_space(&v);
 CHECK(v.e_TouchFunc==touchF_space_touch&&v.e_UseFunc==useF_space_use&&v.contents==CONTENTS_TRIGGER&&v.mins[0]==-64&&v.mins[2]==0&&v.maxs[2]==128&&G_SpaceHasVolumes());
 auto&actor=Make(0,"player");gclient_t client{};actor.client=&client;actor.health=1000;client.NPC_class=CLASS_PLAYER;VectorSet(client.ps.origin,0,0,24);
 Touch(v,actor);CHECK(client.inSpaceIndex==8&&client.inSpaceSuffocation==5100&&G_SpaceActive(&actor));G_SpaceGravity(&actor,800);CHECK(!client.ps.gravity);
 // Repeated/overlapping touches must not postpone damage indefinitely.
 level.time=2000;Touch(v,actor);CHECK(client.inSpaceSuffocation==5100);auto&overlap=Make(9,"trigger_space");overlap.model=v.model;SP_trigger_space(&overlap);Touch(overlap,actor);
 CHECK(client.inSpaceIndex==9&&client.inSpaceSuffocation==5100);level.time=5099;G_SpaceEffects(&actor);CHECK(!damageCount);
 level.time=5100;G_SpaceEffects(&actor);CHECK(damageCount==1&&lastInflictor==&overlap&&actor.health==1000-lastDamage&&sounds==1&&animations==1);
 CHECK(client.inSpaceSuffocation>=6100&&client.inSpaceSuffocation<=7100);G_SpaceEffects(&actor);CHECK(damageCount==1);
 const int inherited=client.inSpaceSuffocation;
 VectorSet(overlap.currentOrigin,100,0,0);Link(&overlap);
 CHECK(G_SpaceActive(&actor)&&client.inSpaceIndex==8&&client.inSpaceSuffocation==inherited);
 VectorClear(overlap.currentOrigin);Link(&overlap);Touch(overlap,actor);
 G_SpaceForget(&overlap);overlap.inuse=qfalse;
 CHECK(G_SpaceActive(&actor)&&client.inSpaceIndex==8&&client.inSpaceSuffocation==inherited);
 client.ps.origin[0]=65;G_SpaceGravity(&actor,600);CHECK(!client.inSpaceIndex&&!client.inSpaceSuffocation&&client.ps.gravity==600);G_SpaceEffects(&actor);CHECK(damageCount==1);
 client.ps.origin[0]=0;Touch(v,actor);CHECK(client.inSpaceSuffocation==10100);
 // Activation retains a selectable marker and drops exposure immediately.
 space_use(&v,&actor,&actor);CHECK(v.contents==0&&v.linked&&!client.inSpaceIndex);Touch(v,actor);CHECK(!client.inSpaceIndex);G_SpaceGravity(&actor,800);CHECK(client.ps.gravity==800);
 space_use(&v,&actor,&actor);Touch(v,actor);CHECK(G_SpaceActive(&actor));v.svFlags|=SVF_INACTIVE;Touch(v,actor);G_SpaceEffects(&actor);CHECK(!client.inSpaceIndex);space_use(&v,&actor,&actor);CHECK(v.contents==CONTENTS_TRIGGER);
 v.svFlags&=~SVF_INACTIVE;Touch(v,actor);v.linked=qfalse;CHECK(!G_SpaceActive(&actor));Link(&v);Touch(v,actor);
 CHECK(G_SpacePortable(v));G_SpaceSet(v,true,false);
 CHECK(v.linked&&v.contents==CONTENTS_TRIGGER&&(v.svFlags&SVF_INACTIVE)&&!client.inSpaceIndex);
 G_SpaceSet(v,true,true);Touch(v,actor);CHECK(G_SpaceActive(&actor));
 G_SpaceSet(v,false,true);CHECK(v.linked&&!v.contents&&!client.inSpaceIndex);
 G_SpaceSet(v,true,true);Touch(v,actor);CHECK(G_SpaceActive(&actor));
 // Noclip, death and custom gravity do not inherit unwanted space motion.
 client.noclip=qtrue;G_SpaceGravity(&actor,800);CHECK(!client.inSpaceIndex&&client.ps.gravity==800);client.noclip=qfalse;
 actor.svFlags|=SVF_CUSTOM_GRAVITY;client.ps.gravity=333;Touch(v,actor);G_SpaceGravity(&actor,800);CHECK(G_SpaceActive(&actor)&&client.ps.gravity==333);actor.svFlags&=~SVF_CUSTOM_GRAVITY;
 actor.health=0;Touch(v,actor);CHECK(!client.inSpaceIndex);actor.health=1000;
 // Full-capacity NPC vehicle slots protect enclosed riders, not just slots
 // below MAX_CLIENTS. Open vehicles and invalid/stale references do not.
 auto&vehicle=Make(1000,"NPC");gclient_t vehicleClient{};vehicle.client=&vehicleClient;vehicleClient.NPC_class=CLASS_VEHICLE;vehicle.health=1000;
 Vehicle_t vehicleState{};vehicleInfo_t info{};vehicle.m_pVehicle=&vehicleState;vehicleState.m_pVehicleInfo=&info;info.hideRider=qtrue;
 actor.s.m_iVehicleNum=1000;Touch(v,actor);CHECK(!client.inSpaceIndex);G_SpaceGravity(&actor,800);CHECK(client.ps.gravity==800);
 info.hideRider=qfalse;Touch(v,actor);CHECK(G_SpaceActive(&actor));info.hideRider=qtrue;CHECK(!G_SpaceActive(&actor));
 for(int index:{-1,ENTITYNUM_WORLD,ENTITYNUM_NONE,MAX_GENTITIES+20}){actor.s.m_iVehicleNum=index;Touch(v,actor);CHECK(G_SpaceActive(&actor));spaceClear(&actor);}
 actor.s.m_iVehicleNum=1000;vehicle.inuse=qfalse;Touch(v,actor);CHECK(G_SpaceActive(&actor));vehicle.inuse=qtrue;actor.s.m_iVehicleNum=0;
 // Mechanical actors retain space membership for movement/vehicle flight,
 // but never choke. Nonhumanoid living actors receive damage without choking anims.
 const int previous=damageCount;for(class_t type:{CLASS_VEHICLE,CLASS_R2D2,CLASS_SABER_DROID,CLASS_ATST,CLASS_INTERROGATOR}){client.NPC_class=type;Touch(v,actor);client.inSpaceSuffocation=level.time;G_SpaceEffects(&actor);CHECK(G_SpaceActive(&actor)&&damageCount==previous);}
 client.NPC_class=CLASS_RANCOR;client.clientInfo.animFileIndex=2;G_SpaceEffects(&actor);CHECK(damageCount==previous+1&&animations==1);client.clientInfo.animFileIndex=0;client.NPC_class=CLASS_PLAYER;
 // Bad saved indices, reused non-space entities and removed volumes cannot
 // be interpreted as space. Removal clears other actors before slot reuse.
 for(int index:{-1,ENTITYNUM_WORLD,ENTITYNUM_NONE,MAX_GENTITIES+100}){client.inSpaceIndex=index;CHECK(!G_SpaceActive(&actor)&&!client.inSpaceIndex&&!client.inSpaceSuffocation);}
 Touch(v,actor);G_SpaceForget(&v);CHECK(!client.inSpaceIndex);v.inuse=qfalse;v.classname=const_cast<char*>("misc_model_static");v.inuse=qtrue;client.inSpaceIndex=8;CHECK(!G_SpaceActive(&actor));v.classname=const_cast<char*>("trigger_space");
 Touch(v,actor);VectorSet(v.currentOrigin,300,0,0);Link(&v);CHECK(!G_SpaceActive(&actor));VectorClear(v.currentOrigin);Link(&v);
 // Native serializer retains exposure timer/index and the appended callback
 // enums/contents; identity strings are resolved by the game's existing loader.
 Touch(v,actor);client.inSpaceSuffocation=level.time+2345;MemorySave buffer;ojk::SavedGameHelper helper(&buffer);client.sg_export(helper);buffer.reset_buffer_offset();gclient_t loadedClient{};loadedClient.sg_import(helper);helper.ensure_all_data_read();
 CHECK(loadedClient.inSpaceIndex==8&&loadedClient.inSpaceSuffocation==level.time+2345);actor.client=&loadedClient;CHECK(G_SpaceActive(&actor));actor.client=&client;
 space_use(&v,&actor,&actor);buffer.reset_buffer();gentity_t savedVolume{},restored{};savedVolume.e_TouchFunc=v.e_TouchFunc;savedVolume.e_UseFunc=v.e_UseFunc;savedVolume.contents=v.contents;VectorCopy(v.mins,savedVolume.mins);VectorCopy(v.maxs,savedVolume.maxs);
 savedVolume.sg_export(helper);buffer.reset_buffer_offset();restored.sg_import(helper);helper.ensure_all_data_read();CHECK(restored.e_TouchFunc==touchF_space_touch&&restored.e_UseFunc==useF_space_use&&restored.contents==0&&restored.maxs[2]==128);
 G_SpaceRestore();CHECK(v.contents==0);v.e_TouchFunc=touchF_NULL;v.e_UseFunc=useF_NULL;v.contents=CONTENTS_TRIGGER;VectorSet(v.mins,-21,-21,0);VectorSet(v.maxs,21,21,22);G_SpaceRestore();CHECK(v.e_TouchFunc==touchF_space_touch&&v.e_UseFunc==useF_space_use&&v.maxs[2]==128);
 auto&authored=Make(10,"trigger_space");authored.model=const_cast<char*>("*4");VectorSet(authored.mins,-400,-400,-400);VectorSet(authored.maxs,400,400,400);authored.spawnflags=128;SP_trigger_space(&authored);CHECK(authored.maxs[0]==400&&(authored.svFlags&SVF_INACTIVE));
 Touch(authored,actor);CHECK(!client.inSpaceIndex);
 // Death or script deletion during damage cannot schedule choking on corpses.
 Touch(v,actor);client.inSpaceSuffocation=level.time;kill=true;const int beforeAnim=animations;G_SpaceEffects(&actor);CHECK(!client.inSpaceIndex&&animations==beforeAnim);kill=false;actor.health=1000;
 Touch(v,actor);client.inSpaceSuffocation=level.time;freeOnDamage=true;G_SpaceEffects(&actor);CHECK(!actor.inuse&&!actor.client&&animations==beforeAnim);
 G_SpaceForget(&v);v.inuse=overlap.inuse=authored.inuse=qfalse;CHECK(!G_SpaceHasVolumes());Geometry();CHECK(!G_SpaceHasVolumes());
 });}
