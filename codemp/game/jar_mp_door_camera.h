// Portable Jabba door camera snapshot fields and bounded settings. GPL-2.0-only.
#pragma once

#define JAR_MP_DOOR_CAMERA_ID 86873624u
#define JAR_MP_DOOR_CAMERA_BASE_MODEL "models/map_objects/kejim/sec_panel.md3"
#define JAR_MP_DOOR_CAMERA_SHAFT_MODEL "models/map_objects/kejim/impcam_base.md3"
#define JAR_MP_DOOR_CAMERA_HEAD_MODEL "models/map_objects/kejim/impcam.md3"
#define JAR_MP_DOOR_CAMERA_SERVO "sound/movers/objects/cameramove_lp2.wav"
#define JAR_MP_DOOR_CAMERA_CLOSED 20.0f
#define JAR_MP_DOOR_CAMERA_MASS 20.0f
#define JAR_MP_DOOR_CAMERA_SHAFT_LENGTH 17.546875f
#define JAR_MP_DOOR_CAMERA_MIN_SCALE (JAR_MP_DOOR_CAMERA_CLOSED/JAR_MP_DOOR_CAMERA_SHAFT_LENGTH)
#define JAR_MP_DOOR_CAMERA_MAX_SCALE ((JAR_MP_DOOR_CAMERA_CLOSED+256.0f)/JAR_MP_DOOR_CAMERA_SHAFT_LENGTH)

enum {
 JAR_MP_DOOR_CAMERA_ROOT_TAG=6, JAR_MP_DOOR_CAMERA_PART_TAG=247,
 JAR_MP_DOOR_CAMERA_SHAFT=1, JAR_MP_DOOR_CAMERA_HEAD=2,
 JAR_MP_DOOR_CAMERA_MILLI=1000,
 JAR_MP_DOOR_CAMERA_MIN_TRAVEL=8000, JAR_MP_DOOR_CAMERA_MAX_TRAVEL=256000,
 JAR_MP_DOOR_CAMERA_MIN_SPEED=1000, JAR_MP_DOOR_CAMERA_MAX_SPEED=512000,
 JAR_MP_DOOR_CAMERA_DEFAULT_TRAVEL=48000, JAR_MP_DOOR_CAMERA_DEFAULT_SPEED=120000
};
enum { JAR_MP_DOOR_CAMERA_REST, JAR_MP_DOOR_CAMERA_MOVING, JAR_MP_DOOR_CAMERA_BLOCKED };

static inline qboolean JAR_MP_DoorCameraRootState(const entityState_t *state){
 return state->eType==ET_GENERAL&&state->generic1==250&&state->bolt2==JAR_MP_DOOR_CAMERA_ROOT_TAG?qtrue:qfalse;
}
static inline int JAR_MP_DoorCameraPartState(const entityState_t *state){
 if(state->eType!=ET_GENERAL||state->generic1!=JAR_MP_DOOR_CAMERA_PART_TAG||
    state->otherEntityNum<MAX_CLIENTS||state->otherEntityNum>=ENTITYNUM_WORLD||!state->time2)return 0;
 return state->bolt2==JAR_MP_DOOR_CAMERA_SHAFT||state->bolt2==JAR_MP_DOOR_CAMERA_HEAD?state->bolt2:0;
}
