#include "g_start_marker.h"

bool G_JAR_VisitStart(gentity_t* marker, gentity_t* actor)
{
    if (!marker || !G_JAR_PortableStart(*marker)) return false;
    if (!actor || !actor->inuse || !actor->client || actor->health <= 0 || actor->m_pVehicle ||
        actor->s.m_iVehicleNum || (marker->svFlags & SVF_INACTIVE)) return true;

    vec3_t origin, arrival;
    VectorCopy(marker->currentOrigin, origin);
    origin[2] -= actor->mins[2];
    VectorCopy(origin, arrival);
    arrival[2] += 1; // TeleportPlayer raises the origin by one unit.
    trace_t space{};
    gi.trace(&space, arrival, actor->mins, actor->maxs, arrival, actor->s.number,
        MASK_PLAYERSOLID, (EG2_Collision)0, 0);
    if (space.startsolid || space.allsolid) {
        gi.Printf("JA:R: start marker is obstructed. Move it to a clear place.\n");
        return true;
    }
    TeleportPlayer(actor, origin, marker->currentAngles);
    return true;
}

void portable_start_use(gentity_t* self, gentity_t*, gentity_t* activator)
{
    G_JAR_VisitStart(self, activator);
}
