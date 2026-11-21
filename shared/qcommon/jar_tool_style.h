// SP is the common tool presentation and distance-control specification.
#pragma once
#define JAR_TOOL_AIM_RANGE 4096.f
#define JAR_PHYSGUN_MP_AIM_MASK (MASK_PLAYERSOLID | CONTENTS_ITEM)
#define JAR_PHYSGUN_MIN_DISTANCE 48.f
#define JAR_PHYSGUN_MAX_DISTANCE 2048.f
#define JAR_PHYSGUN_WHEEL_STEP 24.f
// A grab starts at its hit depth, even outside the normal wheel range.
// Wheel input may bring it back into range without snapping it across a limit.
static inline float JAR_PhysgunWheelDistance(float distance,float maximum,int direction){
 const float low=distance<JAR_PHYSGUN_MIN_DISTANCE?distance:JAR_PHYSGUN_MIN_DISTANCE;
 const float high=distance>maximum?distance:maximum;
 const float next=distance+direction*JAR_PHYSGUN_WHEEL_STEP;
 return next<low?low:next>high?high:next;
}
#define JAR_PHYSGUN_SHADER "gfx/effects/blueLine"
#define JAR_TOOLGUN_SHADER "gfx/effects/redLine"
#define JAR_TOOLGUN_SOUND "sound/weapons/bryar/fire.mp3"
#define JAR_PHYSGUN_WIDTH 1.4f
#define JAR_TOOLGUN_WIDTH 1.1f
#define JAR_PHYSGUN_COLOR .35f,.7f,1.f
#define JAR_TOOLGUN_COLOR 1.f,.7f,.2f
