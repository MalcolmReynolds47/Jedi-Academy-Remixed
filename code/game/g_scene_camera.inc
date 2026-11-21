// Owned scene view and route controller. Never changes authored map cameras.
#include "g_scene_camera.h"
#include <algorithm>
extern qboolean G_ClearViewEntity(gentity_t*);
extern void G_SetViewEntity(gentity_t*,gentity_t*);
namespace {
gentity_t*sceneNamed(const char*name){return name&&name[0]?G_Find(nullptr,FOFS(targetname),name):nullptr;}
void sceneString(char*&field,const char*value){char*next=value&&value[0]?G_NewString(value):nullptr;if(field&&gi.bIsFromZone(field,TAG_G_ALLOC))gi.Free(field);field=next;}
void sceneStop(gentity_t&e){if(g_entities[0].client&&g_entities[0].client->ps.viewEntity==e.s.number)G_ClearViewEntity(&g_entities[0]);e.count=0;e.nextthink=0;e.s.eFlags&=~EF_NODRAW;}
bool sceneContinue(gentity_t&e){
 auto&player=g_entities[0];
 if(!e.count||!player.client||player.health<=0||player.client->ps.viewEntity!=e.s.number){sceneStop(e);return false;}
 const auto&cmd=player.client->usercmd;
 if(level.time>=e.painDebounceTime&&(cmd.forwardmove||cmd.rightmove||cmd.upmove||(cmd.buttons&BUTTON_USE))){sceneStop(e);return false;}
 return true;
}
gentity_t*sceneCorner(const char*name){auto*e=sceneNamed(name);return e&&e->inuse&&e->classname&&!strcmp(e->classname,"path_corner")?e:nullptr;}
void sceneAim(gentity_t&e,float dt){
 auto*focus=sceneNamed(e.target2);if(!focus||!focus->inuse||focus==&e)return;
 float turn=e.radius;
 if(G_SceneCameraPortable(*focus)&&jar::sceneCamera::Focus(focus->classname)){
  turn=focus->radius;if(auto*subject=sceneNamed(focus->target))if(subject->inuse&&subject!=&e&&subject!=focus)focus=subject;
 }
 vec3_t point,direction,desired,angles;VectorCopy(focus->currentOrigin,point);
 if(focus->client)point[2]+=(focus->mins[2]+focus->maxs[2])*.5f;
 VectorSubtract(point,e.currentOrigin,direction);if(VectorNormalize(direction)<.001f)return;
 vectoangles(direction,desired);VectorCopy(e.currentAngles,angles);
 for(int a=0;a<2;++a){float delta=AngleSubtract(desired[a],angles[a]),step=turn*dt;angles[a]+=std::max(-step,std::min(step,delta));}
 G_SetAngles(&e,angles);e.s.apos.trType=TR_INTERPOLATE;
}
}
bool G_SceneCameraSpawn(gentity_t&e){
 if(!G_SceneCameraPortable(e))return false;
 G_SceneCameraSet(e,{});e.count=0;e.takedamage=qfalse;e.contents=CONTENTS_ITEM;e.s.eType=ET_GENERAL;
 e.e_UseFunc=useF_scene_camera_use;e.e_ThinkFunc=thinkF_scene_camera_think;e.nextthink=0;e.svFlags|=SVF_PLAYER_USABLE;
 VectorSet(e.mins,-8,-8,-8);VectorSet(e.maxs,8,8,8);return true;
}
void scene_camera_use(gentity_t*e,gentity_t*,gentity_t*activator){
 if(!e||!G_SceneCameraPortable(*e)||!activator||activator->s.number||!activator->client||activator->health<=0)return;
 if(jar::sceneCamera::Focus(e->classname)){
  auto*camera=sceneNamed(e->target2);if(camera&&G_SceneCameraPortable(*camera)&&jar::sceneCamera::Track(camera->classname))scene_camera_use(camera,e,activator);
  return;
 }
 if(activator->client->ps.viewEntity==e->s.number){sceneStop(*e);return;}
 G_SetViewEntity(activator,e);e->count=1;e->s.eFlags|=EF_NODRAW;e->painDebounceTime=level.time+500;
 e->fly_sound_debounce_time=level.time;e->attackDebounceTime=0;sceneString(e->target3,e->target);
 e->nextthink=level.time+FRAMETIME;
}
void scene_camera_think(gentity_t*e){
 if(!e||!G_SceneCameraPortable(*e)||!jar::sceneCamera::Track(e->classname))return;
 if(!sceneContinue(*e))return;
 const float dt=std::max(0.f,std::min(.2f,(level.time-e->fly_sound_debounce_time)*.001f));e->fly_sound_debounce_time=level.time;e->nextthink=level.time+FRAMETIME;
 if(level.time>=e->attackDebounceTime&&e->speed>0){
  if(auto*corner=sceneCorner(e->target3)){
   vec3_t delta,point;VectorSubtract(corner->currentOrigin,e->currentOrigin,delta);const float distance=VectorNormalize(delta);
   const float travel=e->speed*dt;VectorMA(e->currentOrigin,std::min(distance,travel),delta,point);
   // Camera rails stop at scenery; they never move the player body through it.
   trace_t tr;vec3_t low={-2,-2,-2},high={2,2,2};gi.trace(&tr,e->currentOrigin,low,high,point,e->s.number,MASK_SOLID,(EG2_Collision)0,0);
   if(!tr.startsolid&&!tr.allsolid){G_SetOrigin(e,tr.endpos);e->s.pos.trType=TR_INTERPOLATE;VectorCopy(tr.endpos,e->s.origin);}
   if(!tr.startsolid&&!tr.allsolid&&tr.fraction==1&&distance<=travel+.001f){
    const char*next=corner->target;if(!sceneCorner(next))next=(e->spawnflags&1)?e->target:nullptr;if(!(e->spawnflags&1)&&next&&e->target&&!strcmp(next,e->target))next=nullptr;
    // Broken routes remain still. At most one corner advances per frame.
    sceneString(e->target3,next);e->attackDebounceTime=level.time+int(e->wait*1000);
   }
  }
 }
 sceneAim(*e,dt);gi.linkentity(e);
}
void G_SceneCameraControlledFrame(gentity_t*e){
 if(e&&G_SceneCameraPortable(*e)&&jar::sceneCamera::Track(e->classname)){sceneContinue(*e);e->fly_sound_debounce_time=level.time;}
}
bool G_SceneCameraMoved(gentity_t&e){if(!G_SceneCameraPortable(e))return false;VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);e.fly_sound_debounce_time=level.time;return true;}
void G_SceneCameraForget(gentity_t*e){if(e&&G_SceneCameraPortable(*e))sceneStop(*e);}
bool G_SceneCameraRestore(){
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_SceneCameraPortable(e))continue;jar::sceneCamera::Settings s;
  if(!G_SceneCameraRead(e,s)||(e.count!=0&&e.count!=1)||e.e_UseFunc!=useF_scene_camera_use||e.e_ThinkFunc!=thinkF_scene_camera_think)return false;
  if(!e.count||!jar::sceneCamera::Track(e.classname)||!g_entities[0].client||g_entities[0].client->ps.viewEntity!=i)sceneStop(e);
  else {e.fly_sound_debounce_time=level.time;if(e.nextthink>0)e.nextthink=level.time+FRAMETIME;}
 }return true;
}
