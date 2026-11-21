// Preserve native AI scratch state across synchronous menu spawns.
// Include after g_local.h. GPL-2.0-only.
#ifndef G_NPC_CONTEXT_H
#define G_NPC_CONTEXT_H
#include "g_subbsp_ownership.h"
extern gentity_t *NPC,*_saved_NPC;
extern gNPC_t *NPCInfo,*_saved_NPCInfo;
extern gclient_t *client,*_saved_client;
extern usercmd_t ucmd,_saved_ucmd;
class G_NPCContext {
    struct State {
        gentity_t*actor;gNPC_t*info;gclient_t*client;usercmd_t cmd;uint64_t life;
        bool Live()const{return !actor||(actor->inuse&&G_EntityLifetime(actor)==life&&actor->NPC==info&&actor->client==client);}
    } active,saved;
public:
    G_NPCContext():active{NPC,NPCInfo,client,ucmd,G_EntityLifetime(NPC)},
        saved{_saved_NPC,_saved_NPCInfo,_saved_client,_saved_ucmd,G_EntityLifetime(_saved_NPC)}{}
    ~G_NPCContext(){
        const bool live=active.Live(),savedLive=saved.Live();
        NPC=live?active.actor:nullptr;NPCInfo=live?active.info:nullptr;client=live?active.client:nullptr;ucmd=active.cmd;
        _saved_NPC=savedLive?saved.actor:nullptr;_saved_NPCInfo=savedLive?saved.info:nullptr;_saved_client=savedLive?saved.client:nullptr;_saved_ucmd=saved.cmd;
    }
    G_NPCContext(const G_NPCContext&)=delete;
    G_NPCContext&operator=(const G_NPCContext&)=delete;
};
#endif
