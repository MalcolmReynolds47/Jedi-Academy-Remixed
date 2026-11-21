// Model/frame collision shared by the MP server, client prediction and Box3D.
// GPL-2.0-only.
#pragma once

#include "q_shared.h"
#include "../../code/game/jar_physics.h"

// Emit up to 26 posed model vertices that reconstruct the exact support planes.
qboolean JAR_MP_VisitModelHull(const char* model, int frame, void* context,
                              void (*emit)(void*, const float*));

// The result is for synchronous use and can be invalidated by the next lookup
// for the same entity or a map or renderer restart.
const jar::physics::NativePropHull* JAR_MP_TraceHull(const char* model, int frame,
                                                  const vec3_t angles,
                                                  unsigned entity = ENTITYNUM_NONE,
                                                  bool client = false);
void JAR_MP_ClearModelHulls();
