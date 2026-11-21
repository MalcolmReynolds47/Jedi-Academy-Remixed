// Optional synchronous entity allocation and rollback. GPL-2.0-only.
#ifndef G_SPAWN_TRANSACTION_H
#define G_SPAWN_TRANSACTION_H
#include <cstddef>
#include <cstdint>
#include <vector>
struct gentity_s;typedef struct gentity_s gentity_t;
// Only optional transactions convert required allocation exhaustion to this
// exception. Other native engine errors retain their normal behavior.
struct G_SpawnCapacity {};
class G_SpawnTransaction {
    struct Allocation {int slot;uint64_t life;};
    G_SpawnTransaction*previous;
    std::vector<Allocation> allocations;
    size_t limit;
    uint64_t epoch;
    bool committed=false,exhausted=false;
    friend bool G_SpawnTransactionCanAllocate();
    friend void G_RecordSpawnAllocation(gentity_t*);
    friend void G_RejectSpawnAllocation();
public:
    explicit G_SpawnTransaction(size_t allocationLimit=0);
    ~G_SpawnTransaction();
    bool Commit();
    bool Exhausted()const{return exhausted;}
    void ForEachLiveAllocation(void (*visit)(gentity_t*,gentity_t*),gentity_t* root) const;
    G_SpawnTransaction(const G_SpawnTransaction&)=delete;
    G_SpawnTransaction&operator=(const G_SpawnTransaction&)=delete;
};
bool G_OptionalSpawnActive();
bool G_SpawnTransactionCanAllocate();
void G_RecordSpawnAllocation(gentity_t*);
void G_RejectSpawnAllocation();
void G_ResetSpawnTransactions();
#endif
