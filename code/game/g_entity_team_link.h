// Shared native mover team links. GPL-2.0-only.
#pragma once

namespace jar { namespace entityTeam {
inline void AddMember(gentity_t& leader,gentity_t& part){
    part.teamchain=leader.teamchain;
    leader.teamchain=&part;
    part.teammaster=&leader;
    part.flags|=FL_TEAMSLAVE;
    if(part.targetname){
        leader.targetname=G_NewString(part.targetname);
        part.targetname=nullptr;
    }
}
} }
