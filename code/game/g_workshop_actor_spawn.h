// Single-player Sandbox actor spawn checks. GPL-2.0-only.
#pragma once
#include "g_workshop_placement.h"
#include "g_functions.h"

inline const char* G_WorkshopActorSpawnFailure(const gentity_t& actor,const vec3_t requested,
 bool vehicle,float height){
 if(!actor.inuse||!actor.NPC||!actor.client)return "native actor initialization did not finish";
 if(actor.e_ThinkFunc!=thinkF_NPC_Think||(actor.s.eFlags&EF_NODRAW))return "native actor did not become active and visible";
 const float radius=vehicle?std::max(64.f,height*.5f+32.f):64.f;
 if(!(DistanceSquared(actor.currentOrigin,requested)<=radius*radius))return "native actor moved outside the requested spawn area";
 trace_t check{};
 G_JAR_WorkshopSceneryTrace(check,actor.currentOrigin,actor.currentOrigin,actor.mins,actor.maxs,actor.s.number);
 return check.startsolid||check.allsolid?"native collision hull intersects map geometry":nullptr;
}
