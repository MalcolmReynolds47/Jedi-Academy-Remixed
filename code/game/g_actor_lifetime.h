// Detect native callbacks that free, replace, or detach an entity's state.
// GPL-2.0-only. Include after g_local.h.
#pragma once
#include "g_subbsp_ownership.h"

class G_ActorLifetime {
    const gentity_t& entity;
    const uint64_t lifetime;
    const gclient_t* const client;
    const gNPC_t* const npc;
    const Vehicle_t* const vehicle;

public:
    explicit G_ActorLifetime(const gentity_t& ent)
        : entity(ent), lifetime(G_EntityLifetime(&ent)), client(ent.client), npc(ent.NPC), vehicle(ent.m_pVehicle) {}

    bool Current() const {
        return entity.inuse && entity.client==client && entity.NPC==npc && entity.m_pVehicle==vehicle &&
            G_EntityLifetime(&entity)==lifetime;
    }
};
