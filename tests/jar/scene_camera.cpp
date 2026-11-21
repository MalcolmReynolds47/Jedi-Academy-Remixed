// Production camera controller with deterministic collision and view boundaries.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "test_support.h"
#include "memory_save.h"
#include <deque>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
gentity_t*G_Find(gentity_t*,int,const char*name){for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(e.inuse&&e.targetname&&!strcmp(e.targetname,name))return &e;}return nullptr;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);e->s.pos.trType=TR_STATIONARY;}
void G_SetAngles(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentAngles);VectorCopy(p,e->s.apos.trBase);e->s.apos.trType=TR_STATIONARY;}
qboolean G_ClearViewEntity(gentity_t*p){if(p->client->ps.viewEntity)g_entities[p->client->ps.viewEntity].svFlags&=~SVF_BROADCAST;p->client->ps.viewEntity=0;return qtrue;}
void G_SetViewEntity(gentity_t*p,gentity_t*e){G_ClearViewEntity(p);p->client->ps.viewEntity=e->s.number;e->svFlags|=SVF_BROADCAST;}
#include "game/g_scene_camera.inc"
static gentity_t&Make(int n,const char*name,const char*target,float x=0,float y=0,float z=0){auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=G_NewString(name);e.targetname=G_NewString(target);VectorSet(e.currentOrigin,x,y,z);return e;}
int main(){return RunTests("scene camera routes, focus, exits, controlled views and native save fields",[]{
 globals.num_entities=12;level.time=1000;static gclient_t client{};auto&player=Make(0,"player","player");player.client=&client;player.health=100;
 gi.linkentity=[](gentity_t*){};gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid save");};
 static float fraction=1;static bool solid=false;static int traces=0;
 gi.trace=[](trace_t*t,const vec3_t start,const vec3_t lo,const vec3_t hi,const vec3_t end,int pass,int mask,EG2_Collision,int){CHECK(lo[0]==-2&&hi[0]==2&&pass==2&&mask==MASK_SOLID);++traces;*t={};t->fraction=fraction;t->startsolid=solid?qtrue:qfalse;for(int a=0;a<3;++a)t->endpos[a]=start[a]+fraction*(end[a]-start[a]);};
 auto&camera=Make(2,"misc_camera_track","track");CHECK(!G_SceneCameraSpawn(camera)&&!camera.e_UseFunc);camera.model=G_NewString("@jar_scene_camera");CHECK(G_SceneCameraSpawn(camera)&&camera.speed==64&&camera.radius==180&&camera.wait==1&&camera.contents==CONTENTS_ITEM);
 auto&a=Make(3,"path_corner","a"),&b=Make(4,"path_corner","b",128);a.target=b.targetname;camera.target=a.targetname;
 auto&focus=Make(5,"misc_camera_focus","focus",0,100);focus.model=camera.model;CHECK(G_SceneCameraSpawn(focus));focus.target2=camera.targetname;camera.target2=focus.targetname;
 scene_camera_use(&camera,nullptr,&focus);CHECK(!camera.count);player.health=0;scene_camera_use(&camera,nullptr,&player);CHECK(!camera.count);player.health=100;
 scene_camera_use(&focus,nullptr,&player);CHECK(camera.count&&client.ps.viewEntity==2&&(camera.s.eFlags&EF_NODRAW));
 auto step=[&](int ms){level.time+=ms;scene_camera_think(&camera);};
 step(100);CHECK(!strcmp(camera.target3,"b")&&camera.currentOrigin[0]==0&&camera.currentAngles[YAW]==18&&camera.s.apos.trType==TR_INTERPOLATE);
 step(900);CHECK(camera.currentOrigin[0]==0);step(100);CHECK(std::fabs(camera.currentOrigin[0]-6.4f)<.001f&&camera.s.pos.trType==TR_INTERPOLATE);
 // Collision and long frames cannot teleport a camera through the scenery.
 solid=true;step(100);CHECK(std::fabs(camera.currentOrigin[0]-6.4f)<.001f);solid=false;fraction=0;step(100);CHECK(std::fabs(camera.currentOrigin[0]-6.4f)<.001f);fraction=1;step(10000);CHECK(std::fabs(camera.currentOrigin[0]-19.2f)<.001f);
 jar::sceneCamera::Settings settings{640,180,0,false};CHECK(G_SceneCameraSet(camera,settings));step(200);CHECK(camera.currentOrigin[0]==128&&!camera.target3);step(200);CHECK(camera.currentOrigin[0]==128);
 scene_camera_use(&camera,nullptr,&player);CHECK(!client.ps.viewEntity&&!camera.count&&!(camera.s.eFlags&EF_NODRAW));
 settings.loop=true;G_SceneCameraSet(camera,settings);scene_camera_use(&camera,nullptr,&player);step(200);CHECK(camera.currentOrigin[0]==0&&!strcmp(camera.target3,"b"));step(200);CHECK(camera.currentOrigin[0]==128&&!strcmp(camera.target3,"a"));
 // Zero-length cyclic routes advance once per frame, with no unbounded loop.
 VectorCopy(a.currentOrigin,b.currentOrigin);step(200);const int before=traces;step(200);CHECK(traces==before+1);
 settings.speed=0;settings.turn=90;G_SceneCameraSet(camera,settings);focus.radius=90;auto&subject=Make(6,"NPC","subject",0,-100);focus.target=subject.targetname;VectorClear(camera.currentAngles);step(200);CHECK(std::fabs(camera.currentAngles[YAW]+18)<.001f);
 // Removed subjects fall back to the focus point. An unrelated reused slot is ignored.
 subject.inuse=qfalse;step(200);CHECK(std::fabs(camera.currentAngles[YAW])<.001f);subject.inuse=qtrue;subject.targetname=G_NewString("new_generation");step(200);CHECK(std::fabs(camera.currentAngles[YAW]-18)<.001f);
 auto invalid=settings;invalid.turn=std::numeric_limits<float>::quiet_NaN();CHECK(!G_SceneCameraSet(camera,invalid)&&camera.radius==90);invalid=settings;invalid.wait=31;CHECK(!G_SceneCameraSet(camera,invalid));invalid=settings;invalid.speed=-1;CHECK(!G_SceneCameraSet(camera,invalid));
 auto restart=[&](){client.usercmd={};if(camera.count)scene_camera_use(&camera,nullptr,&player);scene_camera_use(&camera,nullptr,&player);level.time+=500;};
 restart();client.usercmd.buttons=BUTTON_USE;G_SceneCameraControlledFrame(&camera);CHECK(!client.ps.viewEntity&&!camera.count);
 restart();client.usercmd.forwardmove=127;step(1);CHECK(!client.ps.viewEntity&&!camera.count);
 restart();const int heldTraces=traces;G_SceneCameraControlledFrame(&camera);CHECK(camera.count&&traces==heldTraces);client.usercmd.upmove=127;G_SceneCameraControlledFrame(&camera);CHECK(!client.ps.viewEntity);
 restart();client.ps.viewEntity=8;step(1);CHECK(client.ps.viewEntity==8&&!camera.count);client.ps.viewEntity=0;
 restart();player.health=0;G_SceneCameraControlledFrame(&camera);CHECK(!client.ps.viewEntity);player.health=100;
 restart();G_SceneCameraForget(&camera);CHECK(!client.ps.viewEntity&&!camera.count);
 restart();camera.nextthink=0;CHECK(G_SceneCameraRestore()&&camera.count&&!camera.nextthink);camera.nextthink=1;CHECK(G_SceneCameraRestore()&&camera.nextthink==level.time+FRAMETIME);
 camera.count=2;CHECK(!G_SceneCameraRestore());camera.count=1;camera.e_UseFunc=useF_NULL;CHECK(!G_SceneCameraRestore());camera.e_UseFunc=useF_scene_camera_use;
 // Native serialization covers scalars and callback IDs. String relocation belongs to the engine save layer.
 MemorySave save;ojk::SavedGameHelper helper(&save);gentity_t stored{},loaded{};stored.speed=camera.speed;stored.radius=camera.radius;stored.wait=camera.wait;stored.spawnflags=camera.spawnflags;stored.count=camera.count;stored.painDebounceTime=camera.painDebounceTime;stored.fly_sound_debounce_time=camera.fly_sound_debounce_time;stored.attackDebounceTime=camera.attackDebounceTime;stored.e_ThinkFunc=camera.e_ThinkFunc;stored.e_UseFunc=camera.e_UseFunc;stored.sg_export(helper);save.reset_buffer_offset();loaded.sg_import(helper);helper.ensure_all_data_read();CHECK(loaded.speed==0&&loaded.radius==90&&loaded.wait==0&&loaded.count==1&&loaded.spawnflags==1&&loaded.e_ThinkFunc==thinkF_scene_camera_think&&loaded.e_UseFunc==useF_scene_camera_use&&loaded.painDebounceTime==camera.painDebounceTime&&loaded.fly_sound_debounce_time==camera.fly_sound_debounce_time&&loaded.attackDebounceTime==camera.attackDebounceTime);
 client.ps.viewEntity=5;focus.count=1;CHECK(G_SceneCameraRestore()&&!client.ps.viewEntity&&!focus.count&&!camera.count);
 camera.model=G_NewString("authored_camera");camera.count=2;CHECK(G_SceneCameraRestore()&&camera.count==2);CHECK(!G_SceneCameraMoved(camera));
 });}
