#pragma once
#include "qcommon/jar_scene_camera.h"
struct gentity_s;typedef struct gentity_s gentity_t;
inline bool G_SceneCameraPortable(const gentity_t&e){return e.inuse&&jar::sceneCamera::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_scene_camera");}
inline bool G_SceneCameraRead(const gentity_t&e,jar::sceneCamera::Settings&s){if(!G_SceneCameraPortable(e))return false;s.speed=e.speed;s.turn=e.radius;s.wait=e.wait;s.loop=(e.spawnflags&1)!=0;return jar::sceneCamera::Valid(s);}
inline bool G_SceneCameraSet(gentity_t&e,const jar::sceneCamera::Settings&s){if(!G_SceneCameraPortable(e)||!jar::sceneCamera::Valid(s))return false;e.speed=s.speed;e.radius=s.turn;e.wait=s.wait;e.spawnflags=(e.spawnflags&~1)|(s.loop?1:0);return true;}
bool G_SceneCameraSpawn(gentity_t&);
bool G_SceneCameraMoved(gentity_t&);
void G_SceneCameraForget(gentity_t*);
void G_SceneCameraControlledFrame(gentity_t*);
bool G_SceneCameraRestore();
void scene_camera_think(gentity_t*);
void scene_camera_use(gentity_t*,gentity_t*,gentity_t*);
