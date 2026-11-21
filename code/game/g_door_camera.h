#ifndef G_JAR_DOOR_CAMERA_H
#define G_JAR_DOOR_CAMERA_H
#include "qcommon/jar_door_camera.h"
#include <algorithm>
inline bool G_DoorCameraPortable(const gentity_t&e){return e.inuse&&jar::doorCamera::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_door_camera");}
inline bool G_DoorCameraPart(const gentity_t*e,const gentity_t*root,const char*model){return e&&e->inuse&&e->owner==root&&e->model&&!strcmp(e->model,model)&&e->classname&&!strcmp(e->classname,"info_notnull");}
inline bool G_DoorCameraParts(const gentity_t&e){return G_DoorCameraPart(e.lastEnemy,&e,jar::doorCamera::shaft)&&G_DoorCameraPart(e.lastEnemy->lastEnemy,&e,jar::doorCamera::head);}
inline gentity_t*G_DoorCameraRoot(gentity_t*e){
 if(!e||!e->inuse)return nullptr;if(G_DoorCameraPortable(*e))return e;
 auto*r=e->owner;if(!r||!G_DoorCameraPortable(*r)||!G_DoorCameraPart(r->lastEnemy,r,jar::doorCamera::shaft))return nullptr;
 return e==r->lastEnemy||(e==r->lastEnemy->lastEnemy&&G_DoorCameraPart(e,r,jar::doorCamera::head))?r:nullptr;
}
inline bool G_DoorCameraShaft(const gentity_t*e){return e&&e->owner&&G_DoorCameraPortable(*e->owner)&&G_DoorCameraParts(*e->owner)&&e->owner->lastEnemy==e;}
inline bool G_DoorCameraRead(const gentity_t&e,jar::doorCamera::Settings&s){if(!G_DoorCameraPortable(e))return false;s.travel=e.radius;s.speed=e.speed;s.extended=(e.spawnflags&1)!=0;return jar::doorCamera::Valid(s);}
bool G_DoorCameraMoved(gentity_t&);
inline bool G_DoorCameraSet(gentity_t&e,const jar::doorCamera::Settings&s){
 if(!G_DoorCameraPortable(e)||!jar::doorCamera::Valid(s))return false;
 e.radius=s.travel;e.speed=s.speed;e.spawnflags=s.extended?e.spawnflags|1:e.spawnflags&~1;
 e.random=std::max(0.f,std::min(s.travel,e.random));e.painDebounceTime=level.time;e.s.loopSound=0;e.alt_fire=qfalse;return G_DoorCameraMoved(e);
}
bool G_DoorCameraSpawn(gentity_t*);
void G_DoorCameraUse(gentity_t*);
void G_DoorCameraFrame();
void G_DoorCameraForget(gentity_t*);
void G_DoorCameraReset();
void G_DoorCameraRestore();
#endif
