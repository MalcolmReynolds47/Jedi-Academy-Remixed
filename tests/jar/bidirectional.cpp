// Production crossing callbacks, dispatcher and native saved fields. Services
// replace engine linking/target actions, without a game launch. GPL-2.0-only.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "memory_save.h"
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::string output;static int fires=0;static bool destroy=false;
void G_UseTargets2(gentity_t*self,gentity_t*actor,const char*name){CHECK(actor->client);output=name;++fires;if(destroy)self->inuse=qfalse;}
void InitTrigger(gentity_t*e){e->contents=CONTENTS_TRIGGER;e->svFlags=SVF_NOCLIENT;if(e->spawnflags&128)e->svFlags|=SVF_INACTIVE;}
qboolean G_PointInBounds(const vec3_t p,const vec3_t lo,const vec3_t hi){for(int a=0;a<3;++a)if(p[a]<lo[a]||p[a]>hi[a])return qfalse;return qtrue;}
#include "game/g_bidirectional.inc"
#define UNUSED_TOUCH(name) void name(gentity_t*,gentity_t*,trace_t*){throw std::runtime_error("Wrong touch callback: " #name);}
UNUSED_TOUCH(Touch_Item) UNUSED_TOUCH(teleporter_touch) UNUSED_TOUCH(charge_stick) UNUSED_TOUCH(Touch_DoorTrigger)
UNUSED_TOUCH(Touch_PlatCenterTrigger) UNUSED_TOUCH(Touch_Plat) UNUSED_TOUCH(Touch_Button) UNUSED_TOUCH(Touch_Multi)
UNUSED_TOUCH(trigger_push_touch) UNUSED_TOUCH(trigger_teleporter_touch) UNUSED_TOUCH(hurt_touch) UNUSED_TOUCH(NPC_Touch)
UNUSED_TOUCH(touch_ammo_crystal_tigger) UNUSED_TOUCH(funcBBrushTouch) UNUSED_TOUCH(touchLaserTrap) UNUSED_TOUCH(prox_mine_stick)
UNUSED_TOUCH(func_rotating_touch) UNUSED_TOUCH(TouchTieBomb) UNUSED_TOUCH(space_touch)
UNUSED_TOUCH(shipboundary_touch)
UNUSED_TOUCH(factory_flag_touch)
UNUSED_TOUCH(siege_item_touch)
void portable_shield_touch(gentity_t*,gentity_t*,trace_t*){throw std::runtime_error("Unexpected shield callback");}
#include "game/g_entity_touch.inc"
static void Link(gentity_t*e){e->linked=qtrue;VectorAdd(e->currentOrigin,e->mins,e->absmin);VectorAdd(e->currentOrigin,e->maxs,e->absmax);}
int main(){return RunTests("bidirectional crossings, filters, limits, native fields and lifetime",[]{
 static_assert(touchF_space_touch==19&&touchF_bidirectional_touch==20,"Saved callbacks append only");
 gi.linkentity=Link;gi.Error=[](int,const char*,...){throw std::runtime_error("Rejected saved trigger");};globals.num_entities=30;level.time=100;
 auto&v=g_entities[8];v.inuse=qtrue;v.s.number=8;v.classname=const_cast<char*>("trigger_bidirectional");v.model=const_cast<char*>("@jar_box");v.wait=.25f;v.target=const_cast<char*>("forward");v.paintarget=const_cast<char*>("reverse");SP_trigger_bidirectional(&v);
 CHECK(v.e_TouchFunc==touchF_bidirectional_touch&&v.e_UseFunc==useF_bidirectional_use&&v.maxs[2]==128&&G_BidirectionalHasVolumes());
 auto&actor=g_entities[0];gclient_t client{};actor.inuse=qtrue;actor.client=&client;actor.health=100;
 auto move=[&](float previous,float current,float y=0){VectorSet(actor.lastOrigin,previous,y,24);VectorSet(client.ps.origin,current,y,24);GEntity_TouchFunc(&v,&actor,nullptr);};
 move(-100,100);CHECK(fires==1&&output=="forward"&&v.painDebounceTime==350);move(100,-100);CHECK(fires==1);
 level.time=350;move(100,-100);CHECK(fires==2&&output=="reverse");level.time=600;move(-20,-10);move(20,20);move(-20,20,70);move(-1024,1024);CHECK(fires==2);
 move(0,10);CHECK(fires==3);level.time+=250;move(10,0);CHECK(fires==3);move(0,-10);CHECK(fires==4);
 level.time+=250;actor.health=0;move(-20,20);actor.health=100;client.noclip=qtrue;move(-20,20);client.noclip=qfalse;CHECK(fires==4);
 v.spawnflags=1;actor.s.number=1000;move(-20,20);CHECK(fires==4);actor.s.number=0;v.spawnflags=0;
 gNPC_t npc{};actor.NPC=&npc;actor.script_targetname=const_cast<char*>("allowed");v.NPC_targetname=const_cast<char*>("other");move(-20,20);CHECK(fires==4);v.NPC_targetname=actor.script_targetname;move(-20,20);CHECK(fires==5);v.NPC_targetname=nullptr;actor.NPC=nullptr;
 level.time+=250;v.svFlags|=SVF_INACTIVE;move(-20,20);bidirectional_use(&v,&actor,&actor);CHECK(fires==5&&(v.contents&CONTENTS_TRIGGER));v.svFlags&=~SVF_INACTIVE;
 bidirectional_use(&v,&actor,&actor);CHECK(!v.contents&&v.linked);move(-20,20);CHECK(fires==5);bidirectional_use(&v,&actor,&actor);
 v.count=1;move(-20,20);CHECK(fires==6&&v.count==-1&&!v.contents);level.time+=250;move(20,-20);CHECK(fires==6);bidirectional_use(&v,&actor,&actor);CHECK(v.count==1);move(20,-20);CHECK(fires==7&&!v.contents);
 v.count=0;v.wait=-1;bidirectional_use(&v,&actor,&actor);level.time+=250;move(-20,20);CHECK(fires==8&&!v.contents);v.wait=.25f;bidirectional_use(&v,&actor,&actor);
 // The center plane and extent follow the same local axes as C and Physgun.
 level.time+=250;VectorSet(v.currentAngles,0,90,0);jar::volumeEntity::Refresh(v);Link(&v);VectorSet(actor.lastOrigin,0,-80,24);VectorSet(client.ps.origin,0,80,24);GEntity_TouchFunc(&v,&actor,nullptr);CHECK(fires==9&&output=="forward");
 vec3_t outside1={80,-80,24},outside2={80,80,24};CHECK(!bidirectionalCrossing(v,outside1,outside2));
 // Save native cooldown/count/callbacks/local bounds and previous actor origin.
 MemorySave save;ojk::SavedGameHelper helper(&save);gentity_t before{},loaded{};before.e_TouchFunc=v.e_TouchFunc;before.e_UseFunc=v.e_UseFunc;before.count=3;before.wait=.25f;before.painDebounceTime=v.painDebounceTime;
 VectorCopy(v.pos1,before.pos1);VectorCopy(v.pos2,before.pos2);VectorCopy(v.currentAngles,before.currentAngles);VectorCopy(actor.lastOrigin,before.lastOrigin);
 before.sg_export(helper);save.reset_buffer_offset();loaded.sg_import(helper);helper.ensure_all_data_read();CHECK(loaded.count==3&&loaded.wait==.25f&&loaded.painDebounceTime==v.painDebounceTime&&loaded.e_TouchFunc==touchF_bidirectional_touch&&VectorCompare(loaded.lastOrigin,actor.lastOrigin)&&VectorCompare(loaded.pos2,v.pos2));
 G_BidirectionalRestore();CHECK(v.linked&&v.maxs[1]>63);v.wait=std::numeric_limits<float>::quiet_NaN();bool rejected=false;try{G_BidirectionalRestore();}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);v.wait=.25f;
 // Authored BSP bounds keep their map direction, independent of portable axes.
 auto&native=g_entities[9];native.inuse=qtrue;native.s.number=9;native.classname=v.classname;native.model=const_cast<char*>("*3");native.bmodel=qtrue;VectorSet(native.mins,-20,-40,0);VectorSet(native.maxs,20,40,80);VectorSet(native.movedir,0,1,0);SP_trigger_bidirectional(&native);
 vec3_t a={0,-60,24},b={0,60,24};CHECK(bidirectionalCrossing(native,a,b)==1&&native.maxs[0]==20);
 level.time+=250;destroy=true;VectorSet(actor.lastOrigin,0,-80,24);VectorSet(client.ps.origin,0,80,24);GEntity_TouchFunc(&v,&actor,nullptr);CHECK(!v.inuse&&fires==10);GEntity_TouchFunc(&v,&actor,nullptr);CHECK(fires==10);
 v.inuse=qtrue;v.classname=const_cast<char*>("misc_model_static");GEntity_TouchFunc(&v,&actor,nullptr);CHECK(fires==10);native.inuse=qfalse;CHECK(!G_BidirectionalHasVolumes());
 });}
