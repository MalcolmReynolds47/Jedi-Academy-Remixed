// Portable portal state using existing entity-state fields.
#pragma once
#include "../../shared/qcommon/jar_portal_limits.h"
#include "../qcommon/jar_portal_size.h"

#define JAR_MP_PORTAL_SURFACE_ID 88142876u
#define JAR_MP_PORTAL_CAMERA_ID 97890286u
#define JAR_MP_PORTAL_SURFACE_TAG 17
#define JAR_MP_PORTAL_CAMERA_TAG 18
enum { JAR_PORTAL_VIEW_OFF, JAR_PORTAL_VIEW_CAMERA, JAR_PORTAL_VIEW_MIRROR };

static inline int JAR_MP_PortalSurface(const entityState_t* state) {
 return state->eType==ET_GENERAL && state->generic1==250 && state->bolt2==JAR_MP_PORTAL_SURFACE_TAG;
}
static inline void JAR_MP_PortalHull(int width,int height,const vec3_t angles,vec3_t mins,vec3_t maxs) {
 vec3_t axis[3];
 AnglesToAxis(angles,axis);
 for(int a=0;a<3;++a) {
  maxs[a]=2.f*fabsf(axis[0][a])+.5f*width*fabsf(axis[1][a])+.5f*height*fabsf(axis[2][a]);
  mins[a]=-maxs[a];
 }
}
