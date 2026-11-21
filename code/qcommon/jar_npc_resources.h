// Missing retail legacy NPC bodies with a compatible shipped replacement.
// GPL-2.0-only. Definitions and AI identity are not replaced.
#pragma once
#include "q_shared.h"
#include <cstring>

// Segmented model animation files live in the model directory. A suffix
// selects a skin from that directory without changing the animation key.
inline const char* JAR_NpcSplitLegacyModel(const char* selection, char (&directory)[MAX_QPATH]){
 Q_strncpyz(directory,selection,sizeof(directory));
 if(char* suffix=std::strchr(directory,'/')){
  *suffix='\0';
  return suffix+1;
 }
 return "default";
}

namespace jar { namespace npcResources {
struct LegacyLegs { const char *name, *original, *replacement; };
static const LegacyLegs legacyLegs[]={
 {"seeker","remote","remote_sp"},
};
struct LegacyBody { const char *name, *original, *skin, *replacement; };
static const LegacyBody legacyBodies[]={
 {"remote","remote","default","remote_sp"},
};
}}
