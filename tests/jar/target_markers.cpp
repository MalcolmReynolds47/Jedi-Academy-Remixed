// Actual native interest selection, marker lifecycle and location lookup.
#include "game/g_local.h"
#include "game/b_local.h"
#include "game/g_functions.h"
#include "game/g_reference_tags.h"
#include "game/g_combat_points.h"
#include "game/g_target_markers.h"
#include "memory_save.h"
#include <deque>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;static int activations=0;static bool reuse=false,contact=false,allBlocked=false,nearBlocked=false;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);}
qboolean G_ClearLineOfSight(const vec3_t,const vec3_t to,int,int){return allBlocked||(nearBlocked&&to[0]==40)?qfalse:qtrue;}
void CalcEntitySpot(gentity_t*e,spot_t,vec3_t p){VectorCopy(e->currentOrigin,p);}
void G_FreeEntity(gentity_t*e){G_CombatPointForget(e);G_ReferenceForget(e);G_TargetMarkersForget(e);e->inuse=qfalse;}
void G_UseTargets2(gentity_t*,gentity_t*,const char*name){CHECK(!strcmp(name,"looked"));++activations;if(reuse){auto&e=g_entities[1000];G_FreeEntity(&e);e.inuse=qtrue;G_RegisterInterest(&e);}}
gentity_t*G_Find(gentity_t*from,int offset,const char*match){for(int i=from?int(from-g_entities)+1:0;i<globals.num_entities;++i){auto&e=g_entities[i];const auto*name=*reinterpret_cast<char**>(reinterpret_cast<char*>(&e)+offset);if(e.inuse&&name&&!strcmp(name,match))return &e;}return nullptr;}
#include "reference_fixture.h"
static bool waypointAdmission(){return true;}
#include "game/g_waypoint_properties.inc"
#include "game/g_target_markers.inc"
#include "game/g_interest.inc"
#include "game/g_location_lookup.inc"
static gentity_t&Make(int slot,const char*name,float x,const char*message=nullptr){auto&e=g_entities[slot];e.inuse=qtrue;e.s.number=slot;e.classname=G_NewString(name);VectorSet(e.currentOrigin,x,0,0);VectorCopy(e.currentOrigin,e.s.origin);e.message=message?G_NewString(message):nullptr;return e;}
int main(){return RunTests("interest and location markers, native priority, cleanup, slot reuse and restore",[]{
 globals.num_entities=1500;gi.Printf=[](const char*,...){};gi.inPVS=[](const vec3_t,const vec3_t){return qtrue;};
 gi.EntityContact=[](const vec3_t,const vec3_t,const gentity_t*){return contact?qtrue:qfalse;};
 gi.linkentity=[](gentity_t*){};
 auto&player=Make(0,"player",0);gclient_t client{};player.client=&client;client.renderInfo.lookMode=LM_ENT;client.renderInfo.lookTarget=ENTITYNUM_NONE;
 auto&authored=Make(8,"target_interest",120);SP_target_interest(&authored);CHECK(!authored.inuse&&level.numInterestPoints==1&&level.interestPoints[0].origin[0]==120);
 lookMode_t mode=LM_ENT;CHECK(G_FindLocalInterestPoint(&player,mode)==0&&mode==LM_INTEREST);
 auto&point=Make(1000,"target_interest",80);point.model=G_NewString("@jar_interest");point.target=G_NewString("looked");SP_target_interest(&point);
 CHECK(point.inuse&&point.e_UseFunc==useF_marker_use&&level.numInterestPoints==1&&G_TargetMarkersPresent());
 CHECK(G_FindLocalInterestPoint(&player,mode)==1000&&mode==LM_ENT&&activations==1);client.renderInfo.lookTarget=1000;
 CHECK(G_FindLocalInterestPoint(&player,mode)==1000&&activations==1); // No repeated target firing while already looking.
 point.currentOrigin[0]=200;CHECK(G_FindLocalInterestPoint(&player,mode)==0&&mode==LM_INTEREST);point.currentOrigin[0]=80;
 allBlocked=true;CHECK(G_FindLocalInterestPoint(&player,mode)==ENTITYNUM_NONE);allBlocked=false;
 gi.inPVS=[](const vec3_t,const vec3_t){return qfalse;};CHECK(G_FindLocalInterestPoint(&player,mode)==ENTITYNUM_NONE);gi.inPVS=[](const vec3_t,const vec3_t){return qtrue;};
 VectorSet(point.currentOrigin,20,0,80);CHECK(G_FindLocalInterestPoint(&player,mode)==0);VectorSet(point.currentOrigin,300,0,0);CHECK(G_FindLocalInterestPoint(&player,mode)==0);VectorSet(point.currentOrigin,80,0,0);
 marker_use(&point,&player,&player);CHECK(client.renderInfo.lookTarget==ENTITYNUM_NONE&&G_FindLocalInterestPoint(&player,mode)==0);marker_use(&point,&player,&player);
 client.renderInfo.lookTarget=1000;G_FreeEntity(&point);CHECK(client.renderInfo.lookTarget==ENTITYNUM_NONE&&G_FindLocalInterestPoint(&player,mode)==0);
 point.inuse=qtrue;point.classname=G_NewString("misc_model_static");CHECK(G_FindLocalInterestPoint(&player,mode)==0);point.classname=G_NewString("target_interest");G_RegisterInterest(&point);
 reuse=true;CHECK(G_FindLocalInterestPoint(&player,mode)==ENTITYNUM_NONE);reuse=false;CHECK(G_FindLocalInterestPoint(&player,mode)==1000);
 G_TargetMarkersReset();CHECK(G_FindLocalInterestPoint(&player,mode)==0);G_TargetMarkersRestore();CHECK(G_FindLocalInterestPoint(&player,mode)==1000);
 for(int i=level.numInterestPoints;i<MAX_INTEREST_POINTS;++i)VectorSet(level.interestPoints[i].origin,1000,0,0);level.numInterestPoints=MAX_INTEREST_POINTS;
 auto&overflow=Make(9,"target_interest",30);SP_target_interest(&overflow);CHECK(!overflow.inuse&&level.numInterestPoints==MAX_INTEREST_POINTS&&G_FindLocalInterestPoint(&player,mode)==1000);
 // The location list accepts later spawns and prefers visible points, retaining
 // authored trigger_location precedence for native mission location checks.
 auto&near=Make(10,"target_location",40,"Near");SP_target_location(&near);auto&far=Make(11,"target_location",100,"Far");SP_target_location(&far);
 CHECK(level.locationHead==&far&&far.nextTrain==&near&&!strcmp(G_GetLocationForEnt(&player),"Near"));nearBlocked=true;CHECK(!strcmp(G_GetLocationForEnt(&player),"Far"));allBlocked=true;CHECK(!strcmp(G_GetLocationForEnt(&player),"Near"));allBlocked=nearBlocked=false;
 auto&volume=Make(12,"trigger_location",0,"Authored volume");contact=true;CHECK(!strcmp(G_GetLocationForEnt(&player),"Authored volume"));contact=false;
 marker_use(&near,&player,&player);CHECK(!strcmp(G_GetLocationForEnt(&player),"Far"));marker_use(&near,&player,&player);near.currentOrigin[0]=200;CHECK(!strcmp(G_GetLocationForEnt(&player),"Far"));
 G_FreeEntity(&far);CHECK(level.locationHead==&near&&!strcmp(G_GetLocationForEnt(&player),"Near"));far.inuse=qtrue;far.classname=G_NewString("misc_model_static");CHECK(!strcmp(G_GetLocationForEnt(&player),"Near"));
 level.locationHead=reinterpret_cast<gentity_t*>(1);CHECK(!G_NearestLocation(&player));G_TargetMarkersRestore();CHECK(level.locationHead==&near&&!strcmp(G_NearestLocation(&player),"Near"));
 // Native saved marker state requires no new table/pointer chunk. Strings are
 // resolved by the engine's existing field loader; callback/flags/origin stay native.
 MemorySave save;ojk::SavedGameHelper helper(&save);gentity_t before{},loaded{};before.e_UseFunc=point.e_UseFunc;before.svFlags=SVF_INACTIVE;VectorCopy(point.currentOrigin,before.currentOrigin);
 before.sg_export(helper);save.reset_buffer_offset();loaded.sg_import(helper);helper.ensure_all_data_read();CHECK(loaded.e_UseFunc==useF_marker_use&&(loaded.svFlags&SVF_INACTIVE)&&loaded.currentOrigin[0]==80);
 // Old portable locations had no local bounds. Restore the original footprint,
 // then exercise rotated contact and an edited volume after native save/load.
 auto&portable=Make(13,"trigger_location",0,"Hangar");portable.model=G_NewString("@jar_box");
 G_TargetMarkersRestore();vec3_t dimensions;
 CHECK(jar::volumeEntity::Dimensions(portable,dimensions)&&dimensions[0]==42&&dimensions[1]==42&&dimensions[2]==22);
 CHECK(!strcmp(G_GetLocationForEnt(&player),"Hangar"));
 const vec3_t editedDimensions={80,48,32};
 CHECK(jar::volumeEntity::Resize(portable,editedDimensions));
 VectorSet(portable.currentAngles,0,45,0);CHECK(jar::volumeEntity::Refresh(portable));
 VectorSet(player.currentOrigin,32,32,40);
 CHECK(!jar::volume::Contains(portable.pos1,portable.pos2,portable.currentOrigin,portable.currentAngles,player.currentOrigin));
 CHECK(!strcmp(G_GetLocationForEnt(&player),"Near"));
 VectorSet(player.mins,-15,-15,-24);VectorSet(player.maxs,15,15,32);
 CHECK(!strcmp(G_GetLocationForEnt(&player),"Hangar"));
 VectorSet(player.currentOrigin,60,60,40);CHECK(!strcmp(G_GetLocationForEnt(&player),"Near"));
 VectorSet(player.currentOrigin,32,32,40);
 portable.svFlags|=SVF_INACTIVE;CHECK(!strcmp(G_GetLocationForEnt(&player),"Near"));portable.svFlags&=~SVF_INACTIVE;
 MemorySave volumeSave;ojk::SavedGameHelper volumeHelper(&volumeSave);gentity_t restoredVolume{};
 portable.sg_export(volumeHelper);volumeSave.reset_buffer_offset();restoredVolume.sg_import(volumeHelper);volumeHelper.ensure_all_data_read();
 VectorCopy(restoredVolume.pos1,portable.pos1);VectorCopy(restoredVolume.pos2,portable.pos2);
 VectorCopy(restoredVolume.currentAngles,portable.currentAngles);
 G_TargetMarkersRestore();CHECK(jar::volumeEntity::Dimensions(portable,dimensions)&&VectorCompare(dimensions,editedDimensions));
 CHECK(!strcmp(G_GetLocationForEnt(&player),"Hangar"));
 G_FreeEntity(&portable);CHECK(!strcmp(G_GetLocationForEnt(&player),"Near"));
 G_FreeEntity(&point);G_FreeEntity(&near);G_FreeEntity(&volume);CHECK(!G_TargetMarkersPresent()&&!level.locationHead);
 });}
