// Allocation rollback does not undo scripts or changes to existing entities.
// Included by g_utils.cpp before the native allocator. GPL-2.0-only.
#include "g_spawn_transaction.h"
#include "g_subbsp_ownership.h"
#include "g_randomizer.h"
#include <algorithm>
namespace {
G_SpawnTransaction*spawnTransaction=nullptr;
uint64_t spawnTransactionEpoch=0;
}
void G_ResetSpawnTransactions(){spawnTransaction=nullptr;++spawnTransactionEpoch;}
bool G_OptionalSpawnActive(){return spawnTransaction!=nullptr;}
G_SpawnTransaction::G_SpawnTransaction(size_t allocationLimit):previous(spawnTransaction),
    limit(allocationLimit?std::min(allocationLimit,size_t(ENTITYNUM_WORLD-JAR_ENTITY_RESERVE-MAX_CLIENTS)):size_t(ENTITYNUM_WORLD-JAR_ENTITY_RESERVE-MAX_CLIENTS)),epoch(spawnTransactionEpoch){
    spawnTransaction=this;
}
bool G_SpawnTransactionCanAllocate(){
    for(auto*t=spawnTransaction;t;t=t->previous)if(t->exhausted||t->allocations.size()>=t->limit){
        t->exhausted=true;spawnTransaction->exhausted=true;return false;
    }
    return true;
}
void G_RejectSpawnAllocation(){if(spawnTransaction)spawnTransaction->exhausted=true;}
void G_RecordSpawnAllocation(gentity_t*e){
    // Ancestors retain records even if a nested transaction commits. Generation
    // checks make records from a rolled-back or reused child harmless.
    for(auto*t=spawnTransaction;t;t=t->previous)t->allocations.push_back({e->s.number,G_EntityLifetime(e)});
}
bool G_SpawnTransaction::Commit(){return committed=epoch==spawnTransactionEpoch&&!exhausted;}
void G_SpawnTransaction::ForEachLiveAllocation(void (*visit)(gentity_t*,gentity_t*),gentity_t*root) const{
    for(const auto& allocation:allocations){
        auto* child=&g_entities[allocation.slot];
        if(child!=root&&allocation.life&&G_EntityLifetime(child)==allocation.life)
            visit(root,child);
    }
}
G_SpawnTransaction::~G_SpawnTransaction(){
    // A level reset can follow a native ERR_DROP without C++ stack unwinding.
    if(epoch!=spawnTransactionEpoch)return;
    spawnTransaction=nullptr;
    if(!committed||exhausted){
        JarActorRecoverySuspension recovery;
        for(auto i=allocations.rbegin();i!=allocations.rend();++i){
            auto*e=&g_entities[i->slot];
            if(i->life&&G_EntityLifetime(e)==i->life)G_FreeEntity(e);
        }
    }
    // Cleanup may emit native stop-effect events. Those allocations belong to
    // normal cleanup, not to this or an enclosing optional spawn.
    if(epoch==spawnTransactionEpoch)spawnTransaction=previous;
}
