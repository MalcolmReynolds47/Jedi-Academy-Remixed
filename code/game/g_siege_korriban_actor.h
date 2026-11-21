// Stock Siege Korriban's section-owned native Rancor. GPL-2.0-only.
#pragma once
#include "g_siege_item.h"
#include "g_siege_korriban.h"
#include "g_subbsp_ownership.h"

inline bool G_SiegeKorribanRancorSpawner(const gentity_t& entity){
    return entity.inuse&&entity.classname&&!strcmp(entity.classname,jar::siegeKorriban::rancorClass)&&
        G_SiegeKorribanRoom(G_SubBSPOwner(&entity));
}

inline bool G_SiegeKorribanRancorTagged(const gentity_t& entity){
    return entity.inuse&&entity.s.time2==jar::siegeKorriban::rancorTag&&entity.NPC&&entity.client&&
        entity.client->NPC_class==CLASS_RANCOR;
}

inline bool G_SiegeKorribanRancor(const gentity_t& entity){
    return G_SiegeKorribanRancorTagged(entity)&&G_SiegeKorribanRoom(G_SubBSPOwner(&entity));
}

inline bool G_SiegeKorribanRancorHolding(const gentity_t& actor){
    const auto* victim=actor.activator;
    return G_SiegeKorribanRancorTagged(actor)&&victim&&victim->inuse&&victim->client&&
        victim->activator==&actor&&(victim->client->ps.eFlags&EF_HELD_BY_RANCOR);
}

inline void G_SiegeKorribanRancorAllegiance(gentity_t& actor){
    if(!G_SiegeKorribanRancor(actor))return;
    actor.svFlags|=SVF_BROADCAST;
    actor.client->playerTeam=TEAM_ENEMY;
    actor.client->enemyTeam=TEAM_PLAYER;
    actor.client->ps.persistant[PERS_TEAM]=TEAM_ENEMY;
}
