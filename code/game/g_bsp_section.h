// Portable stock BSP section controller. Include after g_local.h.
// GPL-2.0-only.
#ifndef G_BSP_SECTION_H
#define G_BSP_SECTION_H
#include <string>
#include "../../shared/qcommon/jar_bsp_section_policy.h"
#include "../../shared/qcommon/jar_hoth_sky.h"
#include "../../shared/qcommon/jar_yavin_sky.h"
#include "g_functions.h"
#include "g_bsp_section_shaft.h"
#include "g_bsp_section_pickup_specs.h"
#include "g_bsp_section_motion.h"
#include "g_mover_portal.h"
#include "g_subbsp_ownership.h"
#include "g_bsp_section_wasteland.h"
#include "g_bsp_section_wasteland_gun.h"
#include "g_siege_korriban.h"
namespace jar { namespace bspSection {
struct Profile {const char* card;const char* model;const char* title;Mode mode;};
// New cards use the retail arena title. The original misc_bsp ID still
// reconstructs Vjun in old saves and in copied assemblies.
constexpr Profile profiles[]={
    {"misc_bsp","#mp/duel10","Vjun Fuel Processing",Mode::Stationary},
    {"Imperial Control Room","#mp/duel4","Imperial Control Room",Mode::Stationary},
    {"Bespin Courtyard","#mp/duel1","Bespin Courtyard",Mode::Bobbing},
    {"Rancor Pit","#mp/duel7","Rancor Pit",Mode::Fire},
    {"Generator Room","#mp/duel2","Generator Room",Mode::Hurt},
    {"Taspir Landing","#mp/duel5","Taspir Landing",Mode::Hurt},
    {"Abandoned City","#mp/duel8","Abandoned City",Mode::Breakable},
    {"Hoth Canyon","#mp/duel9","Hoth Canyon",Mode::Hurt},
    {"Imperial Shaft","#mp/duel3","Imperial Shaft",Mode::Shaft},
    {"Rift Sanctuary","#mp/ffa4","Rift Sanctuary",Mode::Rift},
    {"Korriban FFA","#mp/ffa2","Korriban FFA",Mode::Korriban},
    {"Yavin Training Grounds","#mp/duel6","Yavin Training Grounds",Mode::Yavin},
    {"Vjun Sentinel","#mp/ffa1","Vjun Sentinel",Mode::Sentinel},
    {"Tatooine FFA","#mp/ffa3","Tatooine FFA",Mode::Tatooine},
    {"Taspir","#mp/ffa5","Taspir",Mode::Taspir},
    {"Factory","#mp/ctf5","Factory",Mode::Factory},
    {"Coruscant Streets","#mp/ctf4","Coruscant Streets",Mode::Coruscant},
    {"Yavin Hilltops","#mp/ctf3","Yavin Hilltops",Mode::Hilltops},
    {"Imperial Drop Zone","#mp/ctf1","Imperial Drop Zone",Mode::DropZone},
    {"Hoth Wasteland","#mp/ctf2","Hoth Wasteland",Mode::Wasteland},
};
inline const Profile* Catalog(const char*name){if(name)for(const auto&profile:profiles)if(!strcmp(name,profile.card))return &profile;return nullptr;}
// The Siege runtime is exercised with stock data while its workshop card
// awaits qualification of full-room save, copy, and movement.
constexpr Profile siegeKorribanProfile={"Siege Korriban",siegeKorriban::resource,"Siege Korriban",Mode::SiegeKorriban};
inline const Profile* Resource(const char*model){
    if(!model)return nullptr;
    for(const auto&profile:profiles)if(!strcmp(model,profile.model))return &profile;
    return !strcmp(model,siegeKorribanProfile.model)?&siegeKorribanProfile:nullptr;
}
inline bool SectionSky(const char* model){return model&&(!strcmp(model,hothSky::resource)||!strcmp(model,yavinSky::resource));}
inline bool Class(const char*name){return Catalog(name)!=nullptr;}
inline bool Portable(const gentity_t&e){return e.inuse&&e.bmodel&&e.classname&&!strcmp(e.classname,"misc_bsp")&&e.e_UseFunc==useF_target_teleporter_use&&Resource(e.model);}
inline bool MatchesCard(const gentity_t& e,const Profile& profile){return Portable(e)&&!strcmp(e.model,profile.model);}
inline const factory::Window* SectionGlass(const gentity_t& e){
    if(!e.inuse||!e.bmodel||!e.classname||strcmp(e.classname,"func_glass"))return nullptr;
    const auto* root=G_SubBSPOwner(&e);
    if(!root||!Portable(*root)||Resource(root->model)->mode!=Mode::Factory)return nullptr;
    return factory::Glass(e.s.modelindex-root->s.modelindex);
}
inline int SectionGlassVertices(const gentity_t& e,vec3_t verts[4],vec3_t normal){
    const auto* window=SectionGlass(e);
    if(!window)return 0;
    vec3_t basis[3];AnglesToAxis(e.currentAngles,basis);
    for(int i=0;i<4;++i){
        VectorCopy(e.currentOrigin,verts[i]);
        for(int axis=0;axis<3;++axis)VectorMA(verts[i],window->points[i][axis],basis[axis],verts[i]);
    }
    vec3_t a,b;VectorSubtract(verts[1],verts[0],a);VectorSubtract(verts[3],verts[0],b);
    CrossProduct(a,b,normal);VectorNormalize(normal);
    return window->shards;
}
inline int SectionBreakableSlots(Mode mode){
    if(mode==Mode::Factory)return factory::modelCount-1;
    return Breakable(mode)?(mode==Mode::Yavin?yavinBreakables:cityBreakables):0;
}
inline bool Bobbing(const gentity_t& child){
    return child.inuse&&child.bmodel&&child.s.eType==ET_MOVER&&child.classname&&
        !strcmp(child.classname,"func_bobbing")&&child.model&&child.model[0]=='*';
}
inline bool ShaftLift(const gentity_t& e) {
    return e.inuse && e.bmodel && e.classname && !strcmp(e.classname, "func_door") &&
        e.e_UseFunc == useF_Use_BinaryMover && e.soundSet && !strcmp(e.soundSet, shaft::liftSoundSet);
}
inline const gentity_t* SectionLiftRoot(const gentity_t& child){
    if(!child.inuse||!child.bmodel||!(child.flags&jarDetachedMoverAreaPortal)||
       !child.classname)return nullptr;
    const auto* root=G_SubBSPOwner(&child);
    if(root&&root->model&&!strcmp(root->model,siegeKorriban::resource)&&
       !strcmp(child.classname,"func_door")&&child.e_ReachedFunc==reachedF_Reached_BinaryMover){
        for(const auto& door:siegeKorriban::doors)
            if(door.model==child.s.modelindex-root->s.modelindex)return root;
    }
    if(child.e_UseFunc!=useF_Use_BinaryMover)return nullptr;
    if(!root||!Portable(*root))return nullptr;
    const auto mode=Resource(root->model)->mode;
    if((mode==Mode::Sentinel||mode==Mode::Tatooine||mode==Mode::Taspir||mode==Mode::Coruscant||mode==Mode::Hilltops)&&
       !strcmp(child.classname,"func_door"))return root;
    if(mode==Mode::Tatooine&&!strcmp(child.classname,"func_plat")&&
       child.s.modelindex-root->s.modelindex==tatooine::platform)return root;
    if(mode==Mode::Taspir&&!strcmp(child.classname,"func_plat"))return root;
    if(mode==Mode::DropZone){
        const int model=child.s.modelindex-root->s.modelindex;
        for(const auto& mover:dropzone::movers)
            if(model==mover.model&&!strcmp(child.classname,mover.Platform()?"func_plat":"func_door"))return root;
    }
    if(mode==Mode::Wasteland){
        const int model=child.s.modelindex-root->s.modelindex;
        for(int index:wasteland::platforms)if(model==index&&!strcmp(child.classname,"func_plat"))return root;
        if(!strcmp(child.classname,"func_door")){
            for(const auto& lift:wasteland::lifts)if(model==lift.model)return root;
            for(const auto& leaf:wasteland::doorLeaves)if(model==leaf.model)return root;
        }
    }
    return nullptr;
}
inline bool SectionLift(const gentity_t& e){return ShaftLift(e)||SectionLiftRoot(e);}
inline bool SectionMoverTrigger(const gentity_t& e){
    const auto* root=G_SubBSPOwner(&e);
    return root&&e.inuse&&!e.bmodel&&e.owner&&e.owner->classname&&
        ((e.e_TouchFunc==touchF_Touch_PlatCenterTrigger&&!strcmp(e.owner->classname,"func_plat"))||
         (e.e_TouchFunc==touchF_Touch_DoorTrigger&&!strcmp(e.owner->classname,"func_door")))&&
        e.model&&!strcmp(e.model,"@jar_bsp_trigger")&&e.contents==CONTENTS_TRIGGER&&
        (e.svFlags&SVF_NOCLIENT)&&SectionLiftRoot(*e.owner)==root;
}
inline const gentity_t* SectionTrainRoot(const gentity_t& e){
    if(!e.inuse||!e.bmodel||!e.classname||strcmp(e.classname,"func_train")||e.e_ReachedFunc!=reachedF_Reached_Train)
        return nullptr;
    const auto* root=G_SubBSPOwner(&e);
    return root&&Portable(*root)&&Resource(root->model)->mode==Mode::Taspir&&
        e.s.modelindex-root->s.modelindex==taspir::trainModel?root:nullptr;
}
inline bool SectionTrainCorner(const gentity_t& e,const gentity_t& root){
    return e.inuse&&!e.linked&&!e.bmodel&&e.classname&&!strcmp(e.classname,"path_corner")&&
        e.targetname&&G_SubBSPOwner(&e)==&root;
}
inline void Hull(const vec3_t low,const vec3_t high,const vec3_t angles,vec3_t mins,vec3_t maxs){
    vec3_t axis[3];AnglesToAxis(angles,axis);VectorSet(mins,1e9f,1e9f,1e9f);VectorSet(maxs,-1e9f,-1e9f,-1e9f);
    for(int mask=0;mask<8;++mask){vec3_t p{};for(int a=0;a<3;++a)VectorMA(p,mask&(1<<a)?high[a]:low[a],axis[a],p);
        for(int a=0;a<3;++a){if(p[a]<mins[a])mins[a]=p[a];if(p[a]>maxs[a])maxs[a]=p[a];}}
}
inline void Corners(const gentity_t&e,float out[8][3],float padding=0){
    vec3_t axis[3];AnglesToAxis(e.currentAngles,axis);
    for(int mask=0;mask<8;++mask){VectorCopy(e.currentOrigin,out[mask]);for(int a=0;a<3;++a)VectorMA(out[mask],mask&(1<<a)?e.maxs[a]+padding:e.mins[a]-padding,axis[a],out[mask]);}
}
inline void Center(const gentity_t&e,vec3_t point){
    VectorCopy(e.currentOrigin,point);vec3_t axis[3];AnglesToAxis(e.currentAngles,axis);
    for(int a=0;a<3;++a)VectorMA(point,(e.mins[a]+e.maxs[a])*.5f,axis[a],point);
}
} }
bool G_BSPSectionBounds(const jar::bspSection::Profile&,const vec3_t,vec3_t,vec3_t,std::string&);
bool G_BSPSectionSpawn(gentity_t*);
namespace jar { namespace workshop { struct State; } }
bool G_BSPSectionPublish(gentity_t&);
void G_BSPSectionRestoreWorkshop(gentity_t&,jar::workshop::State&);
bool G_BSPSectionMoved(gentity_t&);
bool G_BSPSectionSupportsActors(const gentity_t&);
bool G_BSPSectionTeleportMembers(const gentity_t&,std::vector<gentity_t*>&);
inline void G_BSPSectionTranslateChild(gentity_t& child,const vec3_t arrival){
 vec3_t offset;
 VectorSubtract(arrival,child.currentOrigin,offset);
 VectorCopy(arrival,child.currentOrigin);
 VectorAdd(child.s.origin,offset,child.s.origin);
 VectorAdd(child.s.pos.trBase,offset,child.s.pos.trBase);
 if(jar::bspSection::SectionLift(child)||jar::bspSection::SectionTrainRoot(child)){
  VectorAdd(child.pos1,offset,child.pos1);
  VectorAdd(child.pos2,offset,child.pos2);
 }
 child.s.eFlags^=EF_TELEPORT_BIT;
 if(child.linked)gi.linkentity(&child);
}
bool G_BSPSectionEnter(gentity_t*,gentity_t*);
bool G_BSPSectionPosition(gentity_t*,gentity_t*);
bool G_BSPSectionScenery(gentity_t*,gentity_t*);
const gentity_t* G_BSPSectionPickupRoot(const gentity_t&);
bool G_BSPSectionSkyOrigin(const gentity_t&, vec3_t);
gentity_t* G_BSPSectionBreakableRoot(const gentity_t&);
bool G_BSPSectionBreakable(const gentity_t&);
#endif
