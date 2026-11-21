// Native map-section entity lifetimes and ownership. GPL-2.0-only.
#ifndef G_SUBBSP_OWNERSHIP_H
#define G_SUBBSP_OWNERSHIP_H
#include <cstdint>
#include <vector>
struct gentity_s;typedef struct gentity_s gentity_t;
namespace ojk {class SavedGameHelper;}
void G_SubBSPOwnershipReset();
void G_SubBSPAllocated(gentity_t*);
uint64_t G_EntityLifetime(const gentity_t*);
bool G_SubBSPBegin(gentity_t*,bool isolated=false);
uint64_t G_SubBSPNamespace(const gentity_t*);
bool G_SubBSPNameVisible(const gentity_t*);
bool G_SubBSPNameField(int);
gentity_t *G_SubBSPOwner(const gentity_t*);
bool G_SubBSPMembers(const gentity_t*,std::vector<gentity_t*>&);
bool G_SubBSPHasOwnership();
bool G_SubBSPFreeInProgress(const gentity_t*);
class G_SubBSPCapture {
    uint64_t previous,previousNamespace;
public:
    explicit G_SubBSPCapture(const gentity_t*);
    ~G_SubBSPCapture();
    G_SubBSPCapture(const G_SubBSPCapture&)=delete;
    G_SubBSPCapture&operator=(const G_SubBSPCapture&)=delete;
};
class G_SubBSPFreeScope {
    int slot;
public:
    explicit G_SubBSPFreeScope(gentity_t*);
    ~G_SubBSPFreeScope();
    G_SubBSPFreeScope(const G_SubBSPFreeScope&)=delete;
    G_SubBSPFreeScope&operator=(const G_SubBSPFreeScope&)=delete;
};
void G_SubBSPOwnershipExport(bool,ojk::SavedGameHelper&);
bool G_SubBSPOwnershipImport(bool,ojk::SavedGameHelper&,bool namespaces=true);
#endif
