// Hoth Wasteland's four-leaf base doors use one native mover team. GPL-2.0-only.
#pragma once
#include "g_bsp_section_wasteland.h"
#include "g_entity_team_link.h"
#include "g_subbsp_ownership.h"
#include "g_mover_portal.h"

namespace jar { namespace wasteland {
inline bool DoorTeamMember(const gentity_t& root,const gentity_t& member){
    const auto* leader=member.teammaster;
    if(!leader)return false;
    const auto* part=leader;
    bool found=false;
    for(int model:{5,16,15,6}){
        if(!part||!part->inuse||!part->bmodel||!(part->flags&jarDetachedMoverAreaPortal)||
           part->s.modelindex-root.s.modelindex!=model||G_SubBSPOwner(part)!=&root||
           part->teammaster!=leader||bool(part->flags&FL_TEAMSLAVE)!=(part!=leader))return false;
        found=found||part==&member;
        part=part->teamchain;
    }
    return found&&!part;
}

inline gentity_t* LinkDoorTeam(gentity_t& root,gentity_t* const (&brushes)[models]){
    // Validate all four leaves before changing any. Keep the original entity
    // order, including the reverse slave insertion used by G_FindTeams.
    for(const auto& spec:doorLeaves){
        const auto* part=brushes[spec.model];
        if(!part||!part->inuse||!part->bmodel||part->s.eType!=ET_MOVER||
           part->s.modelindex-root.s.modelindex!=spec.model||G_SubBSPOwner(part)!=&root||
           !part->classname||strcmp(part->classname,"func_door")||!part->team||strcmp(part->team,doorTeam)||
           part->targetname||part->teamchain||part->teammaster||(part->flags&FL_TEAMSLAVE)||
           part->e_ThinkFunc!=thinkF_Think_SpawnNewDoorTrigger)return nullptr;
    }
    auto* leader=brushes[doorLeaves[0].model];
    leader->teammaster=leader;
    for(size_t i=1;i<ARRAY_LEN(doorLeaves);++i)entityTeam::AddMember(*leader,*brushes[doorLeaves[i].model]);
    return leader;
}
} }
