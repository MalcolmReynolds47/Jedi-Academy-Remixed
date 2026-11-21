// Original JA:R carried inventory and owned Sandbox state. GPL-2.0-only.
#ifndef JAR_WORKSHOP_STATE_H
#define JAR_WORKSHOP_STATE_H
#include "jar_constraint.h"
#include "jar_catalog_generated.h"
#include "jar_capacity.h"
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
namespace jar { namespace workshop {
constexpr unsigned inventoryLimit=64, ownedLimit=JAR_ENTITY_SLOTS-2, nameSize=64;
constexpr unsigned consoleCatalog=1;
constexpr unsigned wireLimit=32+inventoryLimit*160+ownedLimit*20+constraint::limit*184;
struct Weapon {
    uint32_t id=0, family=0, sandbox=0, tool=0;
    char saber[2][nameSize]{};
    uint8_t colors[2][8]{};
};
constexpr unsigned maxPropMass=100000;
struct PropPhysics {
    uint32_t mass=0; // Zero uses the shape's default mass.
    bool noGravity=false;
};
struct Owned { uint32_t id=0, entity=0, catalog=0, frozen=1, parent=0; PropPhysics physics{}; };
struct State {
    uint32_t count=0, nextWeapon=1, active=0, ownedCount=0, nextOwned=1, jointCount=0, nextJoint=1;
    std::array<Weapon,inventoryLimit> weapons{};
    std::vector<Owned> owned;
    // Derived lookup data. Save records stay in their existing order.
    std::unordered_map<uint32_t,uint32_t> ownershipByEntity;
    std::unordered_map<uint32_t,std::vector<uint32_t>> childrenByParent;
    bool ownershipIdsOrdered=true;
    std::array<constraint::Joint,constraint::limit> joints{};
};
inline bool NameValid(const char *s,bool empty=true) {
    if (!s || (!empty && !*s)) return false;
    for(unsigned i=0;i<nameSize;++i) {
        const unsigned char c=s[i]; if(!c)return true;
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;
    }
    return false;
}
inline bool FamilyValid(uint32_t f) { return (f>=1 && f<=14) || f==29 || f==30; } // Native IDs plus appended tools.
inline bool WeaponValid(const Weapon& w) {
    if(!w.id || !FamilyValid(w.family) || w.sandbox>1 || !NameValid(w.saber[0],w.family!=1) || !NameValid(w.saber[1]))return false;
    if(w.tool>2 || (w.tool && (!w.sandbox || w.family!=(w.tool==1?29u:30u))))return false;
    if((w.family==29 || w.family==30) && !w.tool)return false;
    if(w.family!=1 && (w.saber[0][0] || w.saber[1][0]))return false;
    for(auto &slot:w.colors)for(auto c:slot)if(c>5)return false;
    return true;
}
inline bool SameChoice(const Weapon& a,const Weapon& b) {
    return a.family==b.family && a.tool==b.tool && !std::strcmp(a.saber[0],b.saber[0]) && !std::strcmp(a.saber[1],b.saber[1]) &&
        (a.family!=1 || !std::memcmp(a.colors,b.colors,sizeof(a.colors)));
}
inline Weapon *Find(State& s,uint32_t id) { for(unsigned i=0;i<s.count;++i)if(s.weapons[i].id==id)return &s.weapons[i];return nullptr; }
inline uint32_t Add(State& s,Weapon w) {
    w.id=s.nextWeapon;
    if(!WeaponValid(w) || s.nextWeapon==UINT32_MAX)return 0;
    for(unsigned i=0;i<s.count;++i)if(SameChoice(s.weapons[i],w))return s.weapons[i].id;
    if(s.count>=inventoryLimit)return 0;
    s.weapons[s.count++]=w;++s.nextWeapon;return w.id;
}
inline bool PropCatalog(uint32_t c){const auto*e=catalog::Find(c);return (c>=1001&&c<=1020)||(e&&e->kind==1);}
inline bool ValidPropPhysics(uint32_t catalog,const PropPhysics& physics){
    return physics.mass<=maxPropMass&&((!physics.mass&&!physics.noGravity)||PropCatalog(catalog));
}
inline bool RigidCatalog(uint32_t c){const auto*e=catalog::Find(c);return (c>=1001&&c<=1020)||(e&&e->kind==1&&e->rigid);}
inline bool CatalogValid(uint32_t c) { return c==consoleCatalog || catalog::Find(c) || (c>=1001 && c<=1020) || (c>=2002 && c<=2013) || (c>=2101 && c<=2103) || (c>=2501 && c<=2504) || (c>=3001 && c<=3012) || c==4001 || c==4002; }
inline const Owned* OwnedId(const State& s,uint32_t id){
    if(s.ownershipIdsOrdered){
        const auto match=std::lower_bound(s.owned.begin(),s.owned.end(),id,
            [](const Owned& entry,uint32_t value){return entry.id<value;});
        return match!=s.owned.end()&&match->id==id?&*match:nullptr;
    }
    // Older saves can legally list records out of ownership order.
    for(const auto& entry:s.owned)if(entry.id==id)return &entry;
    return nullptr;
}
inline Owned* OwnedId(State& s,uint32_t id){
    return const_cast<Owned*>(OwnedId(static_cast<const State&>(s),id));
}
inline const Owned* OwnedEntity(const State& s,uint32_t entity){
    const auto indexed=s.ownershipByEntity.find(entity);
    if(indexed==s.ownershipByEntity.end())return nullptr;
    const auto* entry=OwnedId(s,indexed->second);
    return entry&&entry->entity==entity?entry:nullptr;
}
inline Owned* OwnedEntity(State& s,uint32_t entity){
    return const_cast<Owned*>(OwnedEntity(static_cast<const State&>(s),entity));
}
inline const std::vector<uint32_t>* OwnedChildren(const State& s,uint32_t parent){
    const auto found=s.childrenByParent.find(parent);
    return found==s.childrenByParent.end()?nullptr:&found->second;
}
inline void IndexOwnershipChild(State& s,uint32_t parent,const Owned& child){
    if(!parent)return;
    auto& siblings=s.childrenByParent[parent];
    const auto before=[&](uint32_t id,uint32_t){
        return s.ownershipIdsOrdered?id<child.id:OwnedId(s,id)<&child;
    };
    const auto at=std::lower_bound(siblings.begin(),siblings.end(),child.id,before);
    try {
        siblings.insert(at,child.id);
    } catch(...) {
        if(siblings.empty())s.childrenByParent.erase(parent);
        throw;
    }
}
inline void UnindexOwnershipChild(State& s,uint32_t parent,uint32_t child){
    if(!parent)return;
    const auto found=s.childrenByParent.find(parent);
    if(found==s.childrenByParent.end())return;
    auto& siblings=found->second;
    const auto at=s.ownershipIdsOrdered?std::lower_bound(siblings.begin(),siblings.end(),child):
        std::find(siblings.begin(),siblings.end(),child);
    if(at!=siblings.end()&&*at==child)siblings.erase(at);
    if(siblings.empty())s.childrenByParent.erase(found);
}
inline bool SetOwnershipParent(State& s,Owned& child,uint32_t parent){
    if(parent>=child.id)return false;
    if(parent==child.parent)return true;
    IndexOwnershipChild(s,parent,child);
    UnindexOwnershipChild(s,child.parent,child.id);
    child.parent=parent;
    return true;
}
inline bool InsertOwnership(State& s,size_t index,Owned owned){
    if(s.ownedCount>=ownedLimit||index>s.owned.size()||!owned.id||!owned.entity||
       owned.id>=constraint::nativeBit||owned.parent>=owned.id||!CatalogValid(owned.catalog)||OwnedId(s,owned.id))return false;
    const auto indexed=s.ownershipByEntity.emplace(owned.entity,owned.id);
    if(!indexed.second)return false;
    const bool wasOrdered=s.ownershipIdsOrdered;
    const bool ordered=wasOrdered&&
        (!index||s.owned[index-1].id<owned.id)&&
        (index==s.owned.size()||owned.id<s.owned[index].id);
    try {
        s.owned.insert(s.owned.begin()+index,owned);
        s.ownershipIdsOrdered=ordered;
        try {
            IndexOwnershipChild(s,owned.parent,s.owned[index]);
        } catch(...) {
            s.owned.erase(s.owned.begin()+index);
            s.ownershipIdsOrdered=wasOrdered;
            throw;
        }
    } catch(...) {
        s.ownershipByEntity.erase(indexed.first);
        throw;
    }
    ++s.ownedCount;
    return true;
}
inline bool IndexOwnership(State& s){
    s.ownershipByEntity.clear();
    s.ownershipByEntity.reserve(s.owned.size());
    s.childrenByParent.clear();
    s.ownershipIdsOrdered=true;
    uint32_t previous=0;
    for(const auto& entry:s.owned){
        if(!s.ownershipByEntity.emplace(entry.entity,entry.id).second)return false;
        if(entry.parent)s.childrenByParent[entry.parent].push_back(entry.id);
        s.ownershipIdsOrdered&=entry.id>previous;
        previous=entry.id;
    }
    return true;
}
inline bool Constrained(const State&s,uint32_t id){for(unsigned i=0;i<s.jointCount;++i)if(s.joints[i].kind!=constraint::Thruster && (s.joints[i].first==id||s.joints[i].second==id))return true;return false;}
inline bool RigidConstrained(const State&s,uint32_t id){for(unsigned i=0;i<s.jointCount;++i)if(s.joints[i].kind<=constraint::Hinge&&(s.joints[i].first==id||s.joints[i].second==id))return true;return false;}
inline constraint::Joint* ThrusterFor(State& state,uint32_t endpoint){
 for(unsigned i=0;i<state.jointCount;++i){auto& joint=state.joints[i];
  if(joint.kind==constraint::Thruster&&joint.first==endpoint)return &joint;
 }
 return nullptr;
}
inline bool JointPair(const State&s,uint32_t a,uint32_t b){for(unsigned i=0;i<s.jointCount;++i){const auto&j=s.joints[i];if((j.first==a&&j.second==b)||(j.first==b&&j.second==a))return true;}return false;}
inline bool JointRefs(const State&s,const constraint::Joint&j,uint32_t maxEntities=65535){
    auto valid=[&](uint32_t id){
        if(constraint::Native(id))return constraint::NativeEntity(id)<=maxEntities;
        const auto*o=OwnedId(s,id);return o!=nullptr;
    };
    return valid(j.first)&&valid(j.second);
}
inline uint32_t AddJoint(State&s,const constraint::Joint&record){auto j=record;j.id=s.nextJoint;
    if(s.jointCount>=constraint::limit||s.nextJoint==UINT32_MAX||!constraint::Valid(j)||!JointRefs(s,j)||JointPair(s,j.first,j.second))return 0;
    s.joints[s.jointCount++]=j;++s.nextJoint;return j.id;
}
inline bool ForgetJoint(State&s,uint32_t id){for(unsigned i=0;i<s.jointCount;++i)if(s.joints[i].id==id){
    for(unsigned k=i+1;k<s.jointCount;++k)s.joints[k-1]=s.joints[k];s.joints[--s.jointCount]={};return true;}return false;}
inline bool Valid(const State& s,uint32_t maxEntities) {
    if(s.count>inventoryLimit || s.ownedCount>ownedLimit || s.ownedCount!=s.owned.size() ||
       s.ownedCount!=s.ownershipByEntity.size() || !s.nextWeapon || !s.nextOwned ||
       s.nextOwned>constraint::nativeBit || s.jointCount>constraint::limit || !s.nextJoint)return false;
    bool active=!s.active;
    for(unsigned i=0;i<s.count;++i){const auto&w=s.weapons[i];
        if(!WeaponValid(w) || w.id>=s.nextWeapon)return false;
        active|=w.id==s.active;
        for(unsigned j=0;j<i;++j)if(w.id==s.weapons[j].id || SameChoice(w,s.weapons[j]))return false;
    }
    if(!active)return false;
    std::vector<uint32_t> unorderedIds;
    size_t childCount=0;
    if(!s.ownershipIdsOrdered)unorderedIds.reserve(s.ownedCount);
    for(unsigned i=0;i<s.ownedCount;++i){const auto&o=s.owned[i];
        if(!o.id || o.id>=s.nextOwned || !o.entity || o.entity>=maxEntities || !CatalogValid(o.catalog) || o.frozen>1 || o.parent>=o.id || !ValidPropPhysics(o.catalog,o.physics))return false;
        const auto indexed=s.ownershipByEntity.find(o.entity);
        if(indexed==s.ownershipByEntity.end()||indexed->second!=o.id)return false;
        if(o.parent)++childCount;
        if(!s.ownershipIdsOrdered)unorderedIds.push_back(o.id);
        else if(i&&s.owned[i-1].id>=o.id)return false;
    }
    if(!s.ownershipIdsOrdered){
        std::sort(unorderedIds.begin(),unorderedIds.end());
        if(std::adjacent_find(unorderedIds.begin(),unorderedIds.end())!=unorderedIds.end())return false;
    }
    size_t indexedChildren=0;
    for(const auto& parent:s.childrenByParent){
        const Owned* previous=nullptr;
        for(const auto child:parent.second){
            const auto* entry=OwnedId(s,child);
            if(!entry||entry->parent!=parent.first||(previous&&entry<=previous))return false;
            previous=entry;
            ++indexedChildren;
        }
    }
    if(indexedChildren!=childCount)return false;
    for(unsigned i=0;i<s.jointCount;++i){const auto&j=s.joints[i];
        if(!constraint::Valid(j)||j.id>=s.nextJoint||!JointRefs(s,j,maxEntities))return false;
        for(unsigned k=0;k<i;++k){const auto&b=s.joints[k];if(j.id==b.id || (j.first==b.first&&j.second==b.second) || (j.first==b.second&&j.second==b.first))return false;}
    }
    return true;
}
inline void Forget(State& s,uint32_t entity) {
    const auto native=constraint::NativeId(entity);
    for(unsigned k=0;k<s.jointCount;)if(s.joints[k].first==native||s.joints[k].second==native)ForgetJoint(s,s.joints[k].id);else ++k;
    const auto* owned=OwnedEntity(s,entity);
    if(!owned)return;
    const auto id=owned->id;
    for(unsigned k=0;k<s.jointCount;)if(s.joints[k].first==id||s.joints[k].second==id)ForgetJoint(s,s.joints[k].id);else ++k;
    s.ownershipByEntity.erase(entity);
    UnindexOwnershipChild(s,owned->parent,id);
    s.owned.erase(s.owned.begin()+(owned-s.owned.data()));
    --s.ownedCount;
}
inline bool Own(State& s,uint32_t entity,uint32_t catalog,uint32_t parent=0) {
    if(!InsertOwnership(s,s.owned.size(),{s.nextOwned,entity,catalog,1,parent}))return false;
    ++s.nextOwned;
    return true;
}
inline std::vector<uint8_t> Encode(const State& s,bool autosave=false) {
    std::vector<uint8_t> b;
    if(!Valid(s,65536))return b;
    auto word=[&](uint32_t v){for(int i=0;i<4;++i)b.push_back(uint8_t(v>>(i*8)));};
    const bool propSettings=!autosave&&std::any_of(s.owned.begin(),s.owned.begin()+s.ownedCount,[](const Owned& o){return o.physics.mass||o.physics.noGravity;});
    word(propSettings?0x4a575309:0x4a575308);word(s.count);word(s.nextWeapon);word(s.active);word(autosave?0:s.ownedCount);word(s.nextOwned);word(autosave?0:s.jointCount);word(s.nextJoint);
    for(unsigned i=0;i<s.count;++i){const auto&w=s.weapons[i];word(w.id);word(w.family);word(w.sandbox);word(w.tool);
        for(auto &name:w.saber)for(auto c:name)b.push_back(uint8_t(c));
        for(auto &slot:w.colors)for(auto c:slot)b.push_back(c);
    }
    if(!autosave)for(unsigned i=0;i<s.ownedCount;++i){const auto&o=s.owned[i];
        word(o.id);word(o.entity);word(o.catalog);
        // Pack alongside the existing freeze bit to keep the save size bound.
        word(o.frozen|(uint32_t(o.physics.noGravity)<<1)|(o.physics.mass<<2));
        word(o.parent);
    }
    if(!autosave)for(unsigned i=0;i<s.jointCount;++i){const auto&j=s.joints[i];word(j.id);word(j.kind);word(j.first);word(j.second);
        for(const auto*f:{&j.a,&j.b}){for(float v:f->position){uint32_t bits;std::memcpy(&bits,&v,4);word(bits);}for(float v:f->rotation){uint32_t bits;std::memcpy(&bits,&v,4);word(bits);}}
        for(float v:{j.length,j.force}){uint32_t bits;std::memcpy(&bits,&v,4);word(bits);}word(j.enabled);word(uint32_t(j.path.count));
        for(const auto&point:j.path.bend)for(float v:point){uint32_t bits;std::memcpy(&bits,&v,4);word(bits);}
    }
    return b;
}
inline bool Decode(const uint8_t *p,size_t size,State& out,uint32_t maxEntities) {
    if(!p || size<24 || size>wireLimit)return false;
    State s;size_t at=0;
    auto word=[&](){uint32_t v=0;for(int i=0;i<4;++i)v|=uint32_t(p[at++])<<(8*i);return v;};
    const auto version=word();if(version<0x4a575301||version>0x4a575309)return false;
    const bool tools=version>=0x4a575302,joints=version>=0x4a575304,devices=version>=0x4a575305;
    if(joints && size<32)return false;
    s.count=word();s.nextWeapon=word();s.active=word();s.ownedCount=word();s.nextOwned=word();
    if(joints){s.jointCount=word();s.nextJoint=word();}
    if(s.count>inventoryLimit || s.ownedCount>ownedLimit || s.jointCount>constraint::limit || size!=(joints?32:24)+s.count*(tools?160:156)+s.ownedCount*(tools?20:16)+s.jointCount*(version>=0x4a575308?184:devices?84:72))return false;
    for(unsigned i=0;i<s.count;++i){auto&w=s.weapons[i];w.id=word();w.family=word();w.sandbox=word();w.tool=tools?word():0;
        if(version<0x4a575303 && w.tool){
            if(w.tool>2 || w.family!=(w.tool==1?13u:2u))return false;
            w.family=w.tool==1?29:30;
        }
        for(auto &name:w.saber)for(auto &c:name)c=char(p[at++]);
        for(auto &slot:w.colors)for(auto &c:slot)c=p[at++];
    }
    s.owned.resize(s.ownedCount);
    for(auto&o:s.owned){o.id=word();o.entity=word();o.catalog=word();
        const auto settings=word();o.frozen=settings;
        if(version>=0x4a575309){o.frozen=settings&1;o.physics={settings>>2,(settings&2)!=0};}
        o.parent=tools?word():0;
        if(!tools && o.catalog!=1001 && o.catalog!=1002 && o.catalog!=2003 && o.catalog!=2006)return false;}
    if(!IndexOwnership(s))return false;
    for(unsigned i=0;i<s.jointCount;++i){auto&j=s.joints[i];j.id=word();j.kind=word();j.first=word();j.second=word();
        for(auto*f:{&j.a,&j.b}){for(float&v:f->position){const uint32_t bits=word();std::memcpy(&v,&bits,4);}for(float&v:f->rotation){const uint32_t bits=word();std::memcpy(&v,&bits,4);}}
        if(devices){for(float*v:{&j.length,&j.force}){const uint32_t bits=word();std::memcpy(v,&bits,4);}j.enabled=word();}else if(j.kind>constraint::Hinge)return false;
        if(version>=0x4a575308){const auto bends=word();if(bends>JAR_ROPE_PATH_BENDS)return false;j.path.count=int(bends);
            for(auto&point:j.path.bend)for(float&v:point){const uint32_t bits=word();std::memcpy(&v,&bits,4);}}
    }
    if(version<0x4a575306)for(unsigned i=0;i<s.jointCount;++i){const auto&j=s.joints[i];const auto*a=OwnedId(s,j.first),*b=OwnedId(s,j.second);
        if(constraint::Native(j.first)||constraint::Native(j.second)||!a||!b||!RigidCatalog(a->catalog)||!RigidCatalog(b->catalog))return false;}
    if(version<0x4a575307)for(unsigned i=0;i<s.jointCount;++i){const auto&j=s.joints[i];if(j.kind>=constraint::Rope)continue;const auto*a=OwnedId(s,j.first),*b=OwnedId(s,j.second);
        if(!a||!b||!RigidCatalog(a->catalog)||!RigidCatalog(b->catalog))return false;}
    if(!Valid(s,maxEntities))return false;
    out=std::move(s);return true; // Transactional: invalid input never changes the destination.
}
inline bool ImportCarriedInventory(State& destination,const uint8_t* bytes,size_t size,uint32_t maxEntities) {
    State incoming;
    if(!Decode(bytes,size,incoming,maxEntities)||incoming.ownedCount||incoming.jointCount)return false;
    destination.weapons=std::move(incoming.weapons);
    destination.count=incoming.count;
    destination.active=incoming.active;
    destination.nextWeapon=incoming.nextWeapon;
    return true;
}
inline std::string Hex(const std::string& s) {
    static const char digits[]="0123456789abcdef";std::string out;
    for(unsigned char c:s){out+=digits[c>>4];out+=digits[c&15];}return out;
}
inline bool Unhex(const std::string& s,std::string& out,size_t maximum) {
    if(s.size()%2 || s.size()>maximum*2)return false;
    auto digit=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    std::string result;for(size_t i=0;i<s.size();i+=2){int a=digit(s[i]),b=digit(s[i+1]);if(a<0||b<0)return false;result+=char(a*16+b);}
    out=result;return true;
}
// Restore only a binding still owned by this feature. A user's edit wins.
inline bool RestoreQ(bool owned,const char* current) {return owned && current && (!std::strcmp(current,"jar_spawnmenu") || !std::strcmp(current,"+jar_spawnmenu"));}
} }
#endif
