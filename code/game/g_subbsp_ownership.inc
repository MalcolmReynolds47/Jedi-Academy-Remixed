// Map-section ownership preserves native names and includes delayed helpers.
// Native entity memory remains owned by G_Spawn/G_FreeEntity. GPL-2.0-only.
#include "g_subbsp_ownership.h"
#include "g_randomizer.h"
#include "qcommon/ojk_saved_game_helper.h"
#include <algorithm>
#include <map>
#include <vector>
#include <limits>
namespace {
using BspLife=uint64_t;
struct BspGroup {int root;std::map<int,BspLife> members;bool isolated=false;};
std::map<BspLife,BspGroup> bspGroups;
BspLife bspLife[MAX_GENTITIES]{},bspOwner[MAX_GENTITIES]{},bspNextLife=0,bspCapture=0,bspNameCapture=0;
bool bspFree[MAX_GENTITIES]{};
int bspSlot(const gentity_t*e){
    if(!e)return -1;
    const auto address=reinterpret_cast<uintptr_t>(e),begin=reinterpret_cast<uintptr_t>(g_entities);
    if(address<begin||address-begin>=sizeof(g_entities)||(address-begin)%sizeof(*e))return -1;
    const int n=int((address-begin)/sizeof(*e));return n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD?n:-1;
}
BspLife bspNewLife(){if(bspNextLife==std::numeric_limits<BspLife>::max())G_Error("JA:R: sub-BSP entity lifetimes exhausted");return ++bspNextLife;}
bool bspLive(int n,BspLife life){return n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&life&&bspLife[n]==life&&g_entities[n].inuse;}
bool bspGroupLive(BspLife id){auto it=bspGroups.find(id);return it!=bspGroups.end()&&bspLive(it->second.root,id);}
BspLife bspGroup(const gentity_t*e){const int n=bspSlot(e);if(n<0||!e->inuse)return 0;return bspGroupLive(bspLife[n])?bspLife[n]:bspGroupLive(bspOwner[n])?bspOwner[n]:0;}
uint32_t bspClassHash(const char*name){uint32_t hash=2166136261u;if(name)for(;*name;++name)hash=(hash^uint8_t(*name))*16777619u;return hash;}
void bspDetach(int n){auto it=bspGroups.find(bspOwner[n]);if(it!=bspGroups.end())it->second.members.erase(n);bspOwner[n]=0;}
}
void G_SubBSPOwnershipReset(){bspGroups.clear();std::fill(std::begin(bspLife),std::end(bspLife),0);std::fill(std::begin(bspOwner),std::end(bspOwner),0);std::fill(std::begin(bspFree),std::end(bspFree),false);bspCapture=bspNameCapture=0;}
uint64_t G_EntityLifetime(const gentity_t*e){const int n=bspSlot(e);return n>=0&&e->inuse?bspLife[n]:0;}
void G_SubBSPAllocated(gentity_t*e){
    const int n=bspSlot(e);if(n<0)return;
    // Native allocation must follow free. Discard stale bookkeeping rather than
    // adopting a previous occupant when loading/reinitializing an entity slot.
    auto old=bspGroups.find(bspLife[n]);if(old!=bspGroups.end()){
        for(const auto&m:old->second.members)if(bspOwner[m.first]==old->first)bspOwner[m.first]=0;
        bspGroups.erase(old);
    }
    bspDetach(n);bspLife[n]=bspNewLife();bspFree[n]=false;
    if(bspGroupLive(bspCapture)){bspOwner[n]=bspCapture;bspGroups.at(bspCapture).members[n]=bspLife[n];}
}
bool G_SubBSPBegin(gentity_t*e,bool isolated){
    const int n=bspSlot(e);if(n<0||!e->inuse||!e->classname||strcmp(e->classname,"misc_bsp")||e->s.modelindex<=0)return false;
    if(!bspLife[n])bspLife[n]=bspNewLife();
    if(bspGroupLive(bspLife[n]))return false; // A live section cannot be spawned twice.
    BspLife parent=bspOwner[n];unsigned depth=0;
    for(BspLife id=parent;id;){auto it=bspGroups.find(id);if(it==bspGroups.end()||++depth>=MAX_SUB_BSP)return false;id=bspOwner[it->second.root];}
    bspGroups.emplace(bspLife[n],BspGroup{n,{},isolated});return true;
}
namespace {
BspLife bspNamespace(BspLife group){
    for(unsigned depth=0;group&&depth<MAX_SUB_BSP;++depth){auto it=bspGroups.find(group);if(it==bspGroups.end()||!bspLive(it->second.root,group))return 0;
        if(it->second.isolated)return group;group=bspOwner[it->second.root];}
    return 0;
}
}
uint64_t G_SubBSPNamespace(const gentity_t*e){return bspNamespace(bspGroup(e));}
bool G_SubBSPNameVisible(const gentity_t*e){
    // The player is an intentional cross-section target. Named mission entities
    // remain outside isolated copies; physical/AI queries are not filtered.
    if(bspNameCapture&&!bspGroupLive(bspNameCapture))return false;
    return e==&g_entities[0] || G_SubBSPNamespace(e)==bspNameCapture;
}
bool G_SubBSPNameField(int offset){
    switch(offset){case FOFS(targetname):case FOFS(script_targetname):case FOFS(team):case FOFS(cameraGroup):
    case FOFS(target):case FOFS(target2):case FOFS(target3):case FOFS(target4):case FOFS(targetJump):
    case FOFS(opentarget):case FOFS(closetarget):case FOFS(paintarget):
    case FOFS(NPC_targetname):case FOFS(NPC_target):case FOFS(ownername):return true;
    default:return false;}
}
gentity_t*G_SubBSPOwner(const gentity_t*e){auto it=bspGroups.find(bspGroup(e));return it==bspGroups.end()?nullptr:&g_entities[it->second.root];}
bool G_SubBSPMembers(const gentity_t*e,std::vector<gentity_t*>&members){
    members.clear();auto it=bspGroups.find(bspGroup(e));if(it==bspGroups.end()||&g_entities[it->second.root]!=e)return false;
    for(const auto&m:it->second.members)if(bspLive(m.first,m.second)&&bspOwner[m.first]==it->first)members.push_back(&g_entities[m.first]);
    return true;
}
bool G_SubBSPHasOwnership(){return !bspGroups.empty();}
G_SubBSPCapture::G_SubBSPCapture(const gentity_t*e):previous(bspCapture),previousNamespace(bspNameCapture){bspCapture=bspGroup(e);bspNameCapture=bspNamespace(bspCapture);}
G_SubBSPCapture::~G_SubBSPCapture(){bspCapture=bspGroupLive(previous)?previous:0;bspNameCapture=previousNamespace;}
bool G_SubBSPFreeInProgress(const gentity_t*e){const int n=bspSlot(e);return n>=0&&bspFree[n];}
G_SubBSPFreeScope::G_SubBSPFreeScope(gentity_t*e):slot(bspSlot(e)){
    if(slot<0)return;
    bspFree[slot]=true;bspDetach(slot);
    auto it=bspGroups.find(bspLife[slot]);
    if(it!=bspGroups.end()){
        const auto id=it->first;const auto children=std::move(it->second.members);bspGroups.erase(it);
        // Remove the group before callbacks can reenter; never free a reused slot.
        JarActorRecoverySuspension recovery;
        for(const auto&m:children)if(bspLive(m.first,m.second)&&bspOwner[m.first]==id){
            bspOwner[m.first]=0;G_FreeEntity(&g_entities[m.first]);
        }
    }
    bspLife[slot]=0;
}
G_SubBSPFreeScope::~G_SubBSPFreeScope(){if(slot>=0)bspFree[slot]=false;}
void G_SubBSPOwnershipExport(bool autosave,ojk::SavedGameHelper&h){
    std::vector<int> rows;
    if(!autosave)for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD;++n)if(bspGroup(&g_entities[n]))rows.push_back(n);
    h.write<uint32_t>(uint32_t(rows.size()));
    for(int n:rows){auto parent=bspGroups.find(bspOwner[n]);h.write<uint32_t>(uint32_t(n));h.write<int32_t>(parent==bspGroups.end()?-1:parent->second.root);
        h.write<uint32_t>(bspGroupLive(bspLife[n])?(bspGroups.at(bspLife[n]).isolated?3:1):0);h.write<uint32_t>(bspClassHash(g_entities[n].classname));}
}
bool G_SubBSPOwnershipImport(bool autosave,ojk::SavedGameHelper&h,bool namespaces){
    uint32_t count;h.read<uint32_t>(count);if(count>ENTITYNUM_WORLD-MAX_CLIENTS||(autosave&&count))return false;
    if(autosave)return true; // Auto-load keeps the fresh map's authored ownership.
    struct Row {int owner;bool root,isolated;};std::map<int,Row> rows;
    for(unsigned i=0;i<count;++i){uint32_t n,root,hash;int32_t owner;h.read<uint32_t>(n);h.read<int32_t>(owner);h.read<uint32_t>(root);h.read<uint32_t>(hash);
        if(n<MAX_CLIENTS||n>=unsigned(ENTITYNUM_WORLD)||root>unsigned(namespaces?3:1)||root==2||owner<-1||owner>=ENTITYNUM_WORLD||owner==int(n)||(!root&&owner<MAX_CLIENTS)||!g_entities[n].inuse||hash!=bspClassHash(g_entities[n].classname))return false;
        if(root&&(!g_entities[n].classname||strcmp(g_entities[n].classname,"misc_bsp")||g_entities[n].s.modelindex<=0))return false;
        if(!rows.emplace(int(n),Row{owner,root!=0,(root&2)!=0}).second)return false;
    }
    for(const auto&row:rows){int n=row.second.owner;unsigned depth=0;while(n!=-1){auto it=rows.find(n);if(it==rows.end()||!it->second.root||++depth>MAX_SUB_BSP||n==row.first)return false;n=it->second.owner;}}
    // Publish only after all identities, owners and ancestry have been checked.
    G_SubBSPOwnershipReset();
    for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD;++n)if(g_entities[n].inuse)bspLife[n]=bspNewLife();
    for(const auto&row:rows)if(row.second.root){const int n=row.first;bspGroups.emplace(bspLife[n],BspGroup{n,{},row.second.isolated});}
    for(const auto&row:rows)if(row.second.owner!=-1){const int n=row.first;bspOwner[n]=bspLife[row.second.owner];bspGroups.at(bspOwner[n]).members[n]=bspLife[n];}
    return true;
}
