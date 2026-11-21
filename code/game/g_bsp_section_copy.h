// Session-local state for reconstructing qualified portable BSP children. GPL-2.0-only.
#pragma once
#include "g_bsp_section.h"
#include "g_bsp_section_fx.h"
#include "g_bsp_section_hurt.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace jar { namespace bspSection {
struct CopyState {
    struct Lift { int model, elapsed, remaining, duration, loopSound; moverState_t phase; thinkFunc_t think; };
    struct Train { std::string nextCorner; int elapsed, remaining, duration, loopSound; };
    struct Pad { int model, liftModel, remaining; bool pending; };
    struct Pickup { std::string classname; std::array<float, 2> point; int remaining; };
    struct Bob { int model, elapsed; float phase, displacement; bool running; };
    struct Hurt { int model, tag, remaining; std::array<float,3> mins,maxs; bool linked,inactive; };
    struct Fire { std::array<float,3> point; int remaining; thinkFunc_t think; bool loop; };
    struct Speaker { std::array<float,3> point; int remaining; bool playing,timed; };
    struct Gun { std::array<float,3> point; int health,ammo,remaining; thinkFunc_t think; };
    std::string resource;
    std::vector<Lift> lifts;
    std::vector<Train> trains;
    std::vector<Pad> pads;
    std::vector<Pickup> pickups;
    std::vector<Bob> bobbers;
    std::vector<Hurt> hazards;
    std::vector<Fire> fires;
    std::vector<Speaker> speakers;
    std::vector<Gun> guns;
    // Indexed by local inline model minus one. Zero denotes a destroyed brush and -1 a non-breakable slot.
    std::vector<int> brushHealth;
    int dustRemaining=-1;
};

inline std::array<float,3> ChildPoint(const gentity_t& root,const gentity_t& child) {
    vec3_t axis[3],offset;
    AnglesToAxis(root.currentAngles,axis);
    VectorSubtract(child.currentOrigin,root.currentOrigin,offset);
    return {DotProduct(offset,axis[0]),DotProduct(offset,axis[1]),DotProduct(offset,axis[2])};
}

inline int CopyRemaining(int deadline,int limit=60000) {
    return deadline>0?int(std::max<int64_t>(1,std::min<int64_t>(limit,int64_t(deadline)-level.time))):0;
}

inline bool CopyablePad(const gentity_t& child) {
    return (G_BSPSectionBrushPad(child)||
        (G_BSPSectionBoxTagged(child)&&child.s.time2==BSPSectionPadTag))&&
        child.e_UseFunc==useF_Use_Multi&&child.target;
}

inline bool DustTimer(const gentity_t& child){
    return child.e_ThinkFunc==thinkF_func_timer_think&&child.e_UseFunc==useF_func_timer_use;
}

inline bool Torch(const gentity_t& child){
    return child.s.eType==ET_GENERAL&&child.noise_index>0&&child.spawnflags==bspSectionFX::damageFlag&&
        child.classname&&!strcmp(child.classname,"fx_runner");
}

inline bool AmbientSpeaker(const gentity_t& child){
    return child.s.eType==ET_SPEAKER&&child.e_UseFunc==useF_Use_Target_Speaker&&
        (child.spawnflags&3)&&child.noise_index>0;
}
inline bool TrafficSpeaker(const gentity_t& child){
    return child.s.eType==ET_SPEAKER&&child.e_ThinkFunc==thinkF_section_traffic_think&&child.noise_index>0;
}

inline void RestoreSectionTravel(gentity_t& child,moverState_t phase,int elapsed,int duration,bool moving){
    child.moverState=phase;
    child.s.pos.trTime=level.time-elapsed;
    child.s.pos.trDuration=duration;
    child.s.pos.trType=moving?(child.alt_fire?TR_LINEAR_STOP:TR_NONLINEAR_STOP):TR_STATIONARY;
    const bool fromBottom=phase==MOVER_POS1||phase==MOVER_1TO2;
    const float* start=fromBottom?child.pos1:child.pos2;
    const float* end=fromBottom?child.pos2:child.pos1;
    VectorCopy(start,child.s.pos.trBase);
    VectorSubtract(end,start,child.s.pos.trDelta);
    VectorScale(child.s.pos.trDelta,moving?1000.f/duration:0.f,child.s.pos.trDelta);
    EvaluateTrajectory(&child.s.pos,level.time,child.currentOrigin);
}

inline bool RestoreCopyState(gentity_t& root,const CopyState& state) {
    if(!Portable(root)||state.resource!=root.model)return false;
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members))return false;
    const auto brush=[&](int model){
        const auto match=std::find_if(members.begin(),members.end(),[&](const gentity_t* child){
            return child->bmodel&&child->s.modelindex-root.s.modelindex==model;
        });
        return match==members.end()?nullptr:*match;
    };
    for(const auto& lift:state.lifts){
        auto* child=brush(lift.model);
        if(!child||!SectionLift(*child)||std::abs(child->s.pos.trDuration-lift.duration)>1)return false;
        const bool moving=lift.phase==MOVER_1TO2||lift.phase==MOVER_2TO1;
        RestoreSectionTravel(*child,lift.phase,lift.elapsed,lift.duration,moving);
        if(lift.phase!=MOVER_POS1||lift.think!=thinkF_NULL){
            child->e_ThinkFunc=lift.think;
            child->nextthink=lift.remaining?level.time+lift.remaining:0;
        }
        child->s.loopSound=lift.loopSound;
        child->enemy=child->activator=nullptr;
        gi.linkentity(child);
    }
    for(const auto& saved:state.guns){
        const auto match=std::find_if(members.begin(),members.end(),[&](const gentity_t* child){
            return wasteland::GunEntity(*child)&&
                DistanceSquared(ChildPoint(root,*child).data(),saved.point.data())<.0625f;
        });
        if(match==members.end()||saved.health<0||saved.health>wasteland::gunHealth||
           saved.ammo<0||saved.ammo>wasteland::gunAmmo||
           (saved.health>0?(saved.think!=thinkF_NULL||saved.remaining):
               (saved.think!=thinkF_emplaced_blow&&saved.think!=thinkF_wasteland_gun_respawn)||
               saved.remaining<=0||saved.remaining>wasteland::gunRespawn))return false;
        auto& gun=**match;
        gun.health=saved.health;
        gun.count=saved.ammo;
        gun.takedamage=saved.health>0?qtrue:qfalse;
        gun.e_PainFunc=saved.health>0?painF_emplaced_gun_pain:painF_NULL;
        gun.e_DieFunc=saved.think==thinkF_wasteland_gun_respawn?dieF_NULL:dieF_emplaced_gun_die;
        gun.e_ThinkFunc=saved.think;
        gun.nextthink=saved.remaining?level.time+saved.remaining:0;
    }
    for(const auto& train:state.trains){
        auto* child=brush(taspir::trainModel);
        if(!child||SectionTrainRoot(*child)!=&root||train.duration<=0)return false;
        auto next=std::find_if(members.begin(),members.end(),[&](const gentity_t* candidate){
            return SectionTrainCorner(*candidate,root)&&train.nextCorner==candidate->targetname;
        });
        if(next==members.end())return false;
        const auto from=std::find_if(members.begin(),members.end(),[&](const gentity_t* candidate){
            return SectionTrainCorner(*candidate,root)&&candidate->nextTrain==*next;
        });
        if(from==members.end())return false;
        child->nextTrain=*next;
        VectorCopy((*from)->s.origin,child->pos1);
        VectorCopy((*next)->s.origin,child->pos2);
        RestoreSectionTravel(*child,MOVER_1TO2,train.elapsed,train.duration,true);
        if(train.remaining)child->s.pos.trType=TR_STATIONARY; // Keep the native next segment's delta through the wait.
        child->e_ThinkFunc=train.remaining?thinkF_Think_BeginMoving:thinkF_NULL;
        child->nextthink=train.remaining?level.time+train.remaining:0;
        child->s.loopSound=train.loopSound;
        gi.linkentity(child);
    }
    for(const auto& pad:state.pads){
        const auto* lift=brush(pad.liftModel);
        if(!lift||!SectionLift(*lift)||!lift->targetname)return false;
        const auto match=std::find_if(members.begin(),members.end(),[&](const gentity_t* child){
            return CopyablePad(*child)&&!strcmp(child->target,lift->targetname)&&
                   (!pad.model||child->s.modelindex-root.s.modelindex==pad.model);
        });
        if(match==members.end())return false;
        auto* child=*match;
        child->e_ThinkFunc=pad.pending?thinkF_multi_trigger_run:thinkF_NULL;
        child->nextthink=pad.remaining?level.time+pad.remaining:0;
        child->painDebounceTime=0;
        // A queued stock pad still fires its local targets, without retaining
        // the actor that stood on the original pad.
        child->activator=pad.pending?child:nullptr;
    }
    for(const auto& bob:state.bobbers){
        auto* child=brush(bob.model);
        if(!child||!Bobbing(*child)||child->s.pos.trDuration<=0)return false;
        auto& path=child->s.pos;
        path.trType=bob.running?TR_SINE:TR_INTERPOLATE;
        path.trTime=level.time-bob.elapsed;
        child->radius=bob.phase;
        VectorMA(child->s.origin,bob.running?0.f:bob.displacement,path.trDelta,path.trBase);
        if(bob.running)EvaluateTrajectory(&path,level.time,child->currentOrigin);
        else VectorCopy(path.trBase,child->currentOrigin);
        gi.linkentity(child);
    }
    for(const auto& pickup:state.pickups){
        auto found=std::find_if(members.begin(),members.end(),[&](const gentity_t* child){
            if(G_BSPSectionPickupRoot(*child)!=&root||pickup.classname!=child->classname)return false;
            const auto point=ChildPoint(root,*child);
            return std::hypot(point[0]-pickup.point[0],point[1]-pickup.point[1])<.25f;
        });
        if(found==members.end()||pickup.remaining<=0)return false;
        auto& child=**found;
        child.s.eFlags|=EF_NODRAW;
        child.contents=0;
        child.e_TouchFunc=touchF_NULL;
        child.e_UseFunc=useF_NULL;
        child.svFlags&=~SVF_PLAYER_USABLE;
        child.e_ThinkFunc=thinkF_RespawnItem;
        child.nextthink=level.time+pickup.remaining;
        gi.linkentity(&child);
    }
    for(const auto& hazard:state.hazards){
        const auto found=std::find_if(members.begin(),members.end(),[&](const gentity_t* child){
            if(hazard.model)return G_BSPSectionBrushHurt(*child)&&child->s.modelindex-root.s.modelindex==hazard.model;
            return G_BSPSectionHurtTagged(*child)&&child->s.time2==hazard.tag&&
                VectorCompare(child->s.origin2,hazard.mins.data())&&VectorCompare(child->s.angles2,hazard.maxs.data());
        });
        if(found==members.end())return false;
        auto& child=**found;
        child.painDebounceTime=level.time+hazard.remaining-int(child.wait);
        child.aimDebounceTime=-1;
        if(hazard.inactive)child.svFlags|=SVF_INACTIVE;
        else child.svFlags&=~SVF_INACTIVE;
        if(hazard.linked)gi.linkentity(&child);
        else gi.unlinkentity(&child);
    }
    for(const auto& fire:state.fires){
        const auto found=std::find_if(members.begin(),members.end(),[&](const gentity_t* child){
            return Torch(*child)&&DistanceSquared(ChildPoint(root,*child).data(),fire.point.data())<.0625f;
        });
        if(found==members.end())return false;
        auto& child=**found;
        child.e_ThinkFunc=fire.think;
        child.nextthink=fire.remaining?level.time+fire.remaining:-1;
        child.s.loopSound=fire.loop?child.noise_index:0;
    }
    for(const auto& speaker:state.speakers){
        const auto found=std::find_if(members.begin(),members.end(),[&](const gentity_t* child){
            return (speaker.timed?TrafficSpeaker(*child):AmbientSpeaker(*child))&&
                   DistanceSquared(ChildPoint(root,*child).data(),speaker.point.data())<.0625f;
        });
        if(found==members.end())return false;
        auto& child=**found;
        child.s.loopSound=speaker.playing?child.noise_index:0;
        if(speaker.timed)child.nextthink=speaker.remaining?level.time+speaker.remaining:level.time+FRAMETIME;
        else child.painDebounceTime=speaker.remaining?level.time+speaker.remaining:0;
    }
    std::vector<gentity_t*> destroyed;
    for(size_t index=0;index<state.brushHealth.size();++index){
        if(state.brushHealth[index]==-1)continue;
        auto* child=brush(int(index)+1);
        if(!child||G_BSPSectionBreakableRoot(*child)!=&root)return false;
        const int health=state.brushHealth[index];
        if(health<0||health>child->health)return false;
        if(!health)destroyed.push_back(child);
        else child->health=health;
    }
    if(state.dustRemaining>=0){
        const auto timer=std::find_if(members.begin(),members.end(),[](const gentity_t* child){return DustTimer(*child);});
        if(timer==members.end())return false;
        (*timer)->nextthink=state.dustRemaining?level.time+state.dustRemaining:0;
        (*timer)->activator=*timer;
    }
    // The brush is already broken in the clipboard. Removing the new child
    // directly avoids replaying debris, damage or targets at paste time.
    for(auto* child:destroyed)G_FreeEntity(child);
    return true;
}

inline bool CaptureCopyState(const gentity_t& root,CopyState& state) {
    if(!Portable(root))return false;
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members))return false;
    CopyState next;
    next.resource=root.model;
    const auto mode=Resource(root.model)->mode;
    next.brushHealth.resize(SectionBreakableSlots(mode));
    if(mode==Mode::Factory)next.brushHealth[factory::hurtModel-1]=-1;
    for(const auto* child:members) {
        if(child->bmodel&&G_BSPSectionBreakableRoot(*child)==&root){
            const int index=child->s.modelindex-root.s.modelindex-1;
            if(index<0||size_t(index)>=next.brushHealth.size())return false;
            next.brushHealth[index]=std::max(0,child->health);
        }else if(mode==Mode::Yavin&&DustTimer(*child)){
            next.dustRemaining=CopyRemaining(child->nextthink);
        }else if(SectionTrainRoot(*child)==&root){
            const auto& path=child->s.pos;
            if(!child->nextTrain||!SectionTrainCorner(*child->nextTrain,root)||path.trDuration<=0)return false;
            const bool waiting=path.trType==TR_STATIONARY&&child->e_ThinkFunc==thinkF_Think_BeginMoving&&child->nextthink>0;
            if(!waiting&&path.trType!=TR_LINEAR_STOP&&path.trType!=TR_NONLINEAR_STOP)return false;
            const int elapsed=waiting?0:int(std::max<int64_t>(0,std::min<int64_t>(path.trDuration,int64_t(level.time)-path.trTime)));
            next.trains.push_back({child->nextTrain->targetname,elapsed,waiting?CopyRemaining(child->nextthink):0,
                path.trDuration,child->s.loopSound});
        }else if(mode==Mode::Wasteland&&wasteland::GunEntity(*child)){
            const auto think=static_cast<thinkFunc_t>(child->e_ThinkFunc);
            if((child->health>0&&think!=thinkF_NULL)||
               (child->health<=0&&think!=thinkF_emplaced_blow&&think!=thinkF_wasteland_gun_respawn))return false;
            next.guns.push_back({ChildPoint(root,*child),std::max(0,child->health),child->count,
                think==thinkF_NULL?0:CopyRemaining(child->nextthink,wasteland::gunRespawn),think});
        }else if(SectionLift(*child)) {
            const bool moving=child->moverState==MOVER_1TO2||child->moverState==MOVER_2TO1;
            const int duration=child->s.pos.trDuration;
            if(duration<=0||child->moverState<MOVER_POS1||child->moverState>MOVER_2TO1)return false;
            const int elapsed=moving?int(std::max<int64_t>(-50,std::min<int64_t>(duration,int64_t(level.time)-child->s.pos.trTime))):0;
            const auto think=static_cast<thinkFunc_t>(child->e_ThinkFunc==thinkF_ReturnToPos1||
                child->e_ThinkFunc==thinkF_Use_BinaryMover_Go?child->e_ThinkFunc:thinkF_NULL);
            next.lifts.push_back({child->s.modelindex-root.s.modelindex,elapsed,
                think==thinkF_NULL?0:CopyRemaining(child->nextthink),duration,moving?child->s.loopSound:0,child->moverState,think});
        }else if(CopyablePad(*child)) {
            const auto lift=std::find_if(members.begin(),members.end(),[&](const gentity_t* candidate){
                return SectionLift(*candidate)&&candidate->targetname&&!strcmp(child->target,candidate->targetname);
            });
            if(lift==members.end())return false;
            next.pads.push_back({G_BSPSectionBrushPad(*child)?child->s.modelindex-root.s.modelindex:0,(*lift)->s.modelindex-root.s.modelindex,
                child->nextthink>level.time||child->e_ThinkFunc==thinkF_multi_trigger_run?CopyRemaining(child->nextthink):0,
                child->e_ThinkFunc==thinkF_multi_trigger_run});
        }else if(mode==Mode::Bobbing&&Bobbing(*child)) {
            const auto& path=child->s.pos;
            if(path.trDuration<=0||(path.trType!=TR_SINE&&path.trType!=TR_INTERPOLATE))return false;
            const int elapsed=int((int64_t(level.time)-path.trTime)%path.trDuration);
            const bool running=path.trType==TR_SINE;
            if(!running&&!std::isfinite(child->radius))return false;
            const float phase=running?0.f:std::fmod(child->radius,1.f);
            vec3_t offset;VectorSubtract(child->currentOrigin,child->s.origin,offset);
            const float amplitude=VectorLengthSquared(path.trDelta);
            const float displacement=amplitude>0?DotProduct(offset,path.trDelta)/amplitude:0.f;
            if(!std::isfinite(displacement))return false;
            next.bobbers.push_back({child->s.modelindex-root.s.modelindex,elapsed,phase,displacement,running});
        }else if(G_BSPSectionHurtTagged(*child)||G_BSPSectionBrushHurt(*child)){
            const int wait=int(child->wait);
            const int64_t untilReady=int64_t(child->painDebounceTime)+wait-level.time;
            const int remaining=int(std::max<int64_t>(0,std::min<int64_t>(wait,untilReady)));
            next.hazards.push_back({G_BSPSectionBrushHurt(*child)?child->s.modelindex-root.s.modelindex:0,child->s.time2,remaining,
                {child->s.origin2[0],child->s.origin2[1],child->s.origin2[2]},
                {child->s.angles2[0],child->s.angles2[1],child->s.angles2[2]},
                child->linked!=qfalse,(child->svFlags&SVF_INACTIVE)!=0});
        }else if(Torch(*child)){
            const auto think=static_cast<thinkFunc_t>(child->e_ThinkFunc);
            if(think!=thinkF_fx_runner_link&&think!=thinkF_fx_runner_think)return false;
            next.fires.push_back({ChildPoint(root,*child),CopyRemaining(child->nextthink),think,
                child->nextthink>0&&child->s.loopSound!=0});
        }else if(TrafficSpeaker(*child)){
            next.speakers.push_back({ChildPoint(root,*child),CopyRemaining(child->nextthink),child->s.loopSound!=0,true});
        }else if(AmbientSpeaker(*child)){
            const int remaining=child->painDebounceTime>level.time?CopyRemaining(child->painDebounceTime):0;
            next.speakers.push_back({ChildPoint(root,*child),remaining,child->s.loopSound!=0,false});
        }else if(G_BSPSectionPickupRoot(*child)==&root&&child->e_ThinkFunc==thinkF_RespawnItem) {
            const auto point=ChildPoint(root,*child);
            next.pickups.push_back({child->classname,{point[0],point[1]},CopyRemaining(child->nextthink,120000)});
        }
    }
    state=std::move(next);
    return true;
}

} } // namespace jar::bspSection
