// Shared additive facial bone controller. Include after native Ghoul2 types.
#ifndef JAR_FACE_CONTROLLER_H
#define JAR_FACE_CONTROLLER_H
#include "jar_face.h"
#include <cstring>
namespace jar {
struct FaceState { float amount=0,previousEnergy=0,energyChange=0; int time=0,handle=0,model=0,epoch=0; };
template<class Model,class Stop> inline void ClearFace(Model& model,Stop stop) {
    // The ownership bit survives save/renderer reload in the existing bone
    // flags, so Stock can also remove accents restored from a save.
    for (int i=int(model.mBlist.size())-1; i>=0; --i) {
        if (!(model.mBlist[i].flags&BONE_ANGLES_JAR_FACE)) continue;
        model.mBlist[i].flags &= ~BONE_ANGLES_JAR_FACE;
        if(model.mBlist[i].flags&(BONE_ANGLES_IK|BONE_ANGLES_RAGDOLL)) continue;
        stop(i);
    }
}

template<class Model> inline const mdxaSkel_t *FaceBone(const Model& model,int index) {
    const auto *header=model.aHeader;
    if(!header || index<0 || index>=int(model.mBlist.size())) return nullptr;
    const int number=model.mBlist[index].boneNumber;
    if(number<0 || number>=header->numBones || header->numBones<=0 ||
       header->ofsEnd<int(sizeof(*header)) || header->ofsEnd>model.currentAnimModelSize ||
       size_t(header->numBones)>(size_t(header->ofsEnd)-sizeof(*header))/sizeof(int)) return nullptr;
    const unsigned char *data=reinterpret_cast<const unsigned char *>(header);
    const int *offsets=reinterpret_cast<const int *>(data+sizeof(*header));
    const int offset=offsets[number];
    if(offset<0 || size_t(offset)+sizeof(*header)+sizeof(mdxaSkel_t)>size_t(header->ofsEnd)) return nullptr;
    const auto *bone=reinterpret_cast<const mdxaSkel_t *>(data+sizeof(*header)+offset);
    if(bone->parent<0 || bone->parent>=header->numBones) return nullptr;
    const int parentOffset=offsets[bone->parent];
    if(parentOffset<0 || size_t(parentOffset)+sizeof(*header)+sizeof(mdxaSkel_t)>size_t(header->ofsEnd)) return nullptr;
    const auto *parent=reinterpret_cast<const mdxaSkel_t *>(data+sizeof(*header)+parentOffset);
    if(std::strncmp(parent->name,"face",sizeof(parent->name))) return nullptr;
    for(int r=0;r<3;++r) if(!std::isfinite(bone->BasePoseMat.matrix[r][3])) return nullptr;
    return bone;
}

template<class Model,class Index,class Set,class Stop>
inline void ApplyFace(Model&model,FaceState&state,int entity,int time,int handle,int epoch,
 int preset,bool eligible,float energy,float sensitivity,Index indexOf,Set set,Stop stop){
    constexpr int ragdollStarted=0x0010;
    if(preset<1 || preset>3 || !eligible ||
       (model.mFlags&ragdollStarted)) {
        ClearFace(model,stop); state.amount=state.previousEnergy=state.energyChange=0; state.time=time; return;
    }
    const bool reset=state.epoch!=epoch || state.handle!=handle || state.model!=model.mModel ||
        time<state.time || time-state.time>250;
    if(reset) { state.amount=state.previousEnergy=state.energyChange=0; state.time=time; }
    state.handle=handle; state.model=model.mModel; state.epoch=epoch;
    const float target=jar::FaceTarget(energy,sensitivity);
    if(time!=state.time||reset){state.energyChange=target-state.previousEnergy;state.previousEnergy=target;}
    const auto shape=jar::FaceShape(entity,time,target,state.energyChange);
    state.amount=jar::SmoothFace(state.amount,target,time-state.time);
    state.time=time;
    if(state.amount<0.001f && target==0) { ClearFace(model,stop); return; }

    static const char *names[]={"jaw","lblip2","rblip2","ltlip2","rtlip2","ceyebrow"};
    // GetBoneIndex refreshes model pointers after video restarts. No borrowed
    // renderer pointers/indices are retained between calls or model lifetimes.
    const int jaw=indexOf(names[0]);
    const auto *jawBone=FaceBone(model,jaw);
    if(!jawBone) { ClearFace(model,stop); return; }
    float jawPivot[3]; for(int r=0;r<3;++r) jawPivot[r]=jawBone->BasePoseMat.matrix[r][3];
    for(int role=0;role<6;++role) {
        const int index=role==0 ? jaw : indexOf(names[role]);
        const auto *bone=FaceBone(model,index);
        if(!bone) continue;
        const int flags=model.mBlist[index].flags;
        // Respect an independently controlled bone (script, IK, ragdoll).
        if((flags&(BONE_ANGLES_IK|BONE_ANGLES_RAGDOLL)) ||
           ((flags&BONE_ANGLES_TOTAL) && !(flags&BONE_ANGLES_JAR_FACE))) continue;
        float pivot[3]; for(int r=0;r<3;++r) pivot[r]=bone->BasePoseMat.matrix[r][3];
        mdxaBone_t accent;
        jar::FaceMatrix(accent.matrix,role,pivot,jawPivot,state.amount,preset,shape.spread,shape.asymmetry);
        set(index,accent,BONE_ANGLES_POSTMULT|BONE_ANGLES_JAR_FACE,time);
    }
}
}
#endif
