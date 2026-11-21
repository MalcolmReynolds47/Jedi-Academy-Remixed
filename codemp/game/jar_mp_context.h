// Shared context display contract. All permissions and mutations live on server.
#pragma once
#include "jar_mp_controller.h"
#include "jar_mp_number.h"
enum { JAR_CONTEXT_NONE, JAR_CONTEXT_PROP, JAR_CONTEXT_NPC, JAR_CONTEXT_VEHICLE,
       JAR_CONTEXT_PLAYER, JAR_CONTEXT_PICKUP, JAR_CONTEXT_OTHER };
enum { JAR_CONTEXT_FREEZE=1, JAR_CONTEXT_RELEASE=2, JAR_CONTEXT_GRAVITY=4,
       JAR_CONTEXT_MASS=8, JAR_CONTEXT_ROTATE=16, JAR_CONTEXT_REMOVE=32,
       JAR_CONTEXT_DETACH=64, JAR_CONTEXT_HEALTH=128, JAR_CONTEXT_QUANTITY=256,
       JAR_CONTEXT_POSE=512, JAR_CONTEXT_UNDO=1024, JAR_CONTEXT_TRANSFORM=2048, JAR_CONTEXT_CONTROLLER=4096,
       JAR_CONTEXT_MOVEMENT=8192, JAR_CONTEXT_ARMOR=16384 };
enum { JAR_PROPERTY_NONE, JAR_PROPERTY_MASS, JAR_PROPERTY_GRAVITY,
       JAR_PROPERTY_HEALTH, JAR_PROPERTY_QUANTITY, JAR_PROPERTY_TRANSFORM, JAR_PROPERTY_CONTROLLER, JAR_PROPERTY_DISTANCE_LINK,
       JAR_PROPERTY_ACTOR_GRAVITY, JAR_PROPERTY_ACTOR_FRICTION, JAR_PROPERTY_ARMOR };
static inline int JAR_ContextMassValid(float mass) {
    return isfinite(mass) && mass > 0 && isfinite(1.f / mass);
}

typedef struct { int entity,field; unsigned identity,revision; double before,after;
 unsigned poseRevision;int frozen;vec3_t oldOrigin,oldAngles,newOrigin,newAngles;
 int distanceRole,distanceBefore;unsigned distanceEpoch;
 jarControllerState_t controllerBefore,controllerAfter;
} jarPropertyUndo_t;
