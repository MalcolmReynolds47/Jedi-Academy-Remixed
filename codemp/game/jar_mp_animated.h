// Stock animated Sandbox props use the normal MD3 frame and sound fields.
// GPL-2.0-only.
#pragma once
#include "qcommon/jar_button.h"

enum { JAR_MP_BOMB_ID = 92740395, JAR_MP_BEACON_ID = 88018492 };
enum { JAR_MP_BOMB_TAG = 3, JAR_MP_BEACON_TAG = 4, JAR_MP_ANIM_TICK_MS = 50 };
#define JAR_MP_BUTTON_TAG 12
enum { JAR_MP_BOMB_ARMED, JAR_MP_BOMB_DISARMING, JAR_MP_BOMB_DISARMED };
enum { JAR_MP_BEACON_HIDDEN, JAR_MP_BEACON_DEPLOYING, JAR_MP_BEACON_SIGNALLING };
enum { JAR_MP_BOMB_LAST_FRAME = 8, JAR_MP_BEACON_DEPLOY_LAST = 30,
       JAR_MP_BEACON_LOOP_FIRST = 31, JAR_MP_BEACON_LOOP_LAST = 60 };

static inline int JAR_MP_AnimatedCatalogTag(unsigned id){
 return id==JAR_MP_BOMB_ID?JAR_MP_BOMB_TAG:id==JAR_MP_BEACON_ID?JAR_MP_BEACON_TAG:id==JAR_BUTTON_ID?JAR_MP_BUTTON_TAG:0;
}

static inline int JAR_MP_AnimatedTag(const entityState_t*state){
 if(state->number<MAX_CLIENTS||state->number>=ENTITYNUM_WORLD||state->eType!=ET_GENERAL||state->generic1!=250)return 0;
 return state->bolt2==JAR_MP_BOMB_TAG||state->bolt2==JAR_MP_BEACON_TAG||state->bolt2==JAR_MP_BUTTON_TAG?state->bolt2:0;
}

static inline qboolean JAR_MP_BombState(const entityState_t*state){return JAR_MP_AnimatedTag(state)==JAR_MP_BOMB_TAG;}
static inline qboolean JAR_MP_BeaconState(const entityState_t*state){return JAR_MP_AnimatedTag(state)==JAR_MP_BEACON_TAG;}

static inline qboolean JAR_MP_AnimatedFrameValid(int tag,int phase,int frame){
 if(tag==JAR_MP_BUTTON_TAG)return JAR_ButtonFrameValid(phase,frame)?qtrue:qfalse;
 if(tag==JAR_MP_BOMB_TAG){
  switch(phase){
   case JAR_MP_BOMB_ARMED:return frame==0;
   case JAR_MP_BOMB_DISARMING:return frame>=0&&frame<JAR_MP_BOMB_LAST_FRAME;
   case JAR_MP_BOMB_DISARMED:return frame==JAR_MP_BOMB_LAST_FRAME;
  }
 }
 if(tag==JAR_MP_BEACON_TAG){
  switch(phase){
   case JAR_MP_BEACON_HIDDEN:return frame==0;
   case JAR_MP_BEACON_DEPLOYING:return frame>=0&&frame<=JAR_MP_BEACON_DEPLOY_LAST;
   case JAR_MP_BEACON_SIGNALLING:return frame>=JAR_MP_BEACON_LOOP_FIRST&&frame<=JAR_MP_BEACON_LOOP_LAST;
  }
 }
 return qfalse;
}

static inline qboolean JAR_MP_AnimatedFrameRuns(int tag,int phase){
 return (tag==JAR_MP_BOMB_TAG&&phase==JAR_MP_BOMB_DISARMING)||
        (tag==JAR_MP_BEACON_TAG&&(phase==JAR_MP_BEACON_DEPLOYING||phase==JAR_MP_BEACON_SIGNALLING));
}
