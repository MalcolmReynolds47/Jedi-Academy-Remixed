// World-space query shared by MP server collision and client prediction.
// GPL-2.0-only.
#pragma once
#include "jar_mp_model_hull.h"

inline bool JAR_MP_TraceModelHull(trace_t& trace, const char* model, const entityState_t& state,
 const vec3_t origin, const vec3_t angles, const vec3_t start, const vec3_t end,
 const vec3_t mins, const vec3_t maxs, int contents, bool client)
{
 const auto* hull = JAR_MP_TraceHull(model, state.frame, angles, state.number, client);
 if (!hull) return false;
 const auto vector = [](const float* point) {
  return point ? jar::physics::Vec{point[0], point[1], point[2]} : jar::physics::Vec{};
 };
 vec3_t first, last, travel;
 VectorSubtract(start, origin, first);
 VectorSubtract(end, origin, last);
 const auto hit = hull->Trace(vector(first), vector(last), vector(mins), vector(maxs));
 trace = {};
 trace.fraction = hit.fraction;
 trace.startsolid = hit.startSolid ? qtrue : qfalse;
 trace.allsolid = hit.allSolid ? qtrue : qfalse;
 trace.entityNum = ENTITYNUM_NONE;
 VectorSubtract(end, start, travel);
 VectorMA(start, hit.fraction, travel, trace.endpos);
 if (hit.startSolid || hit.fraction < 1) {
  trace.entityNum = state.number;
  trace.contents = contents;
  VectorSet(trace.plane.normal, hit.normal.x, hit.normal.y, hit.normal.z);
  trace.plane.dist = hit.planeDistance + DotProduct(trace.plane.normal, origin);
  trace.plane.type = PLANE_NON_AXIAL;
  SetPlaneSignbits(&trace.plane);
 }
 return true;
}
