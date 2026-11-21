// Shared portable CTF lift groups and ambient speakers. GPL-2.0-only.
static int SectionTrafficDeadline(const gentity_t& e){
    return level.time+std::max(FRAMETIME,int(e.wait+1000*e.random*Q_flrand(-1,1)));
}

void section_traffic_think(gentity_t* speaker){
    if(!speaker->inuse||!G_SubBSPOwner(speaker)||speaker->noise_index<=0)return;
    const bool birds=speaker->s.time2==jar::hilltops::birdSpeakerTag;
    if(!(speaker->svFlags&SVF_INACTIVE)&&(!birds||speaker->s.loopSound)){
        int sound=speaker->noise_index;
        if(birds){
            const int index=Q_irand(0,ARRAY_LEN(jar::hilltops::birdCalls)-1);
            sound=G_SoundIndex(jar::hilltops::birdCalls[index]);
        }
        if(sound>0)G_AddEvent(speaker,EV_GENERAL_SOUND,sound);
    }
    speaker->nextthink=SectionTrafficDeadline(*speaker);
}

static bool BSPSectionInitializeCTFPad(gentity_t* pad,const gentity_t& root,
    const jar::sectionCTF::Pad& spec,const char* target){
    if(!pad||!pad->target||strcmp(pad->target,target)||pad->wait!=spec.wait||
       pad->e_UseFunc!=useF_Use_Multi||pad->e_TouchFunc!=touchF_Touch_Multi)return false;
    pad->s.time2=BSPSectionBrushPadTag;
    if(!G_BSPSectionBrushPad(*pad))return false;
    pad->delay=spec.delay;
    G_SetAngles(pad,root.currentAngles);
    if(spec.yaw>=0){
        vec3_t basis[3];AnglesToAxis(root.currentAngles,basis);
        const float yaw=DEG2RAD(spec.yaw);
        VectorScale(basis[0],cosf(yaw),pad->movedir);
        VectorMA(pad->movedir,sinf(yaw),basis[1],pad->movedir);
    }
    gi.linkentity(pad);
    return true;
}

static bool BSPSectionInitializeCTFFall(gentity_t* hazard,const gentity_t& root,int flags){
    if(!hazard||hazard->e_TouchFunc!=touchF_hurt_touch||hazard->damage!=-1||hazard->wait||
       hazard->spawnflags!=flags)return false;
    hazard->s.time2=BSPSectionBrushHurtTag;
    if(!G_BSPSectionBrushHurt(*hazard))return false;
    hazard->damage=Q3_INFINITE;
    hazard->spawnflags|=32|8;
    hazard->noise_index=0;
    G_SetAngles(hazard,root.currentAngles);
    gi.linkentity(hazard);
    return true;
}

template<size_t ModelCount,size_t LiftCount,size_t PadCount,size_t FallCount>
static bool BSPSectionInitializeCTF(gentity_t& root,const jar::sectionCTF::Lift (&lifts)[LiftCount],
    const jar::sectionCTF::Pad (&pads)[PadCount],const int (&falls)[FallCount],bool hilltops){
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members)||!BSPSectionLinkFlags(root,members))return false;
    gentity_t* brush[ModelCount]{};
    int speakerCount=0,trafficCount=0;
    for(auto* child:members){
        if(child->bmodel){
            const int model=child->s.modelindex-root.s.modelindex;
            if(model<=0||size_t(model)>=ModelCount||brush[model])return false;
            brush[model]=child;
        }else if(child->s.eType==ET_SPEAKER){
            if(child->noise_index<=0||child->e_UseFunc!=useF_Use_Target_Speaker)return false;
            ++speakerCount;
            const bool bird=hilltops&&child->s.time2==jar::hilltops::birdSpeakerTag;
            if(bird||(!hilltops&&child->wait==8000&&child->random==10&&!child->spawnflags)){
                ++trafficCount;
                if(bird){
                    child->wait=(jar::hilltops::callMin+jar::hilltops::callMax)/2;
                    child->random=(jar::hilltops::callMax-jar::hilltops::callMin)/2000.f;
                }
                child->s.clientNum=0;
                child->e_ThinkFunc=thinkF_section_traffic_think;
                child->nextthink=SectionTrafficDeadline(*child);
            }else if(child->spawnflags!=1||!child->s.loopSound)return false;
        }
    }
    if(speakerCount!=(hilltops?36:41)||trafficCount!=(hilltops?8:6))return false;
    if(hilltops)for(const char* call:jar::hilltops::birdCalls)if(G_SoundIndex(call)<=0)return false;
    for(const auto& spec:lifts){
        auto* lift=brush[spec.model];
        if(!lift||!lift->targetname||strcmp(lift->targetname,spec.target)||!lift->soundSet||
           strcmp(lift->soundSet,"medium_platform")||!(lift->contents&CONTENTS_SOLID)||
           lift->speed!=spec.speed||bool(lift->alt_fire)!=spec.linear)return false;
        if(gi.bIsFromZone(lift->soundSet,TAG_G_ALLOC))gi.Free(lift->soundSet);
        lift->soundSet=G_NewString(jar::shaft::liftSoundSet);
        lift->flags|=jarDetachedMoverAreaPortal;
        lift->delay=200;
        if(!BSPSectionLiftFits(root,*lift))return false;
    }
    for(const auto& spec:pads)
        if(!BSPSectionInitializeCTFPad(brush[spec.model],root,spec,lifts[spec.lift].target))return false;
    for(int index:falls)
        if(!BSPSectionInitializeCTFFall(brush[index],root,hilltops?4:12))return false;
    return true;
}

static bool BSPSectionInitializeDropZone(gentity_t& root){
    using namespace jar::dropzone;
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members)||!BSPSectionLinkFlags(root,members))return false;
    gentity_t* brush[models]{};
    int speakers=0;
    for(auto* child:members){
        if(child->bmodel){
            const int model=child->s.modelindex-root.s.modelindex;
            if(model<=0||model>=models||brush[model])return false;
            brush[model]=child;
        }else if(child->s.eType==ET_SPEAKER){
            if(child->noise_index<=0||child->e_UseFunc!=useF_Use_Target_Speaker||
               child->spawnflags!=1||!child->s.loopSound)return false;
            ++speakers;
        }
    }
    if(speakers!=speakerCount)return false;
    for(const auto& spec:movers){
        auto* mover=brush[spec.model];
        const bool platform=spec.Platform();
        if(!mover||!mover->classname||strcmp(mover->classname,platform?"func_plat":"func_door")||
           !mover->targetname||strcmp(mover->targetname,spec.target)||!(mover->contents&CONTENTS_SOLID)||
           mover->e_UseFunc!=useF_Use_BinaryMover||mover->speed!=spec.speed||mover->wait!=(platform?1000:3000)||
           mover->spawnflags!=spec.flags||mover->damage!=spec.damage||bool(mover->alt_fire)==platform)return false;
        if(platform&&(mover->e_TouchFunc!=touchF_Touch_Plat||mover->owner!=mover))return false;
        mover->flags|=jarDetachedMoverAreaPortal;
        mover->delay=spec.delay;
        if(!BSPSectionLiftFits(root,*mover))return false;
    }
    for(const auto& spec:pads)
        if(!BSPSectionInitializeCTFPad(brush[spec.model],root,spec,movers[spec.lift].target))return false;
    for(int index:falls)
        if(!BSPSectionInitializeCTFFall(brush[index],root,4))return false;
    return true;
}

void SP_target_speaker(gentity_t*);
static bool BSPSectionHilltopsSpeaker(gentity_t& e){
    if(!e.classname||strcmp(e.classname,"target_speaker")||!e.soundSet)return false;
    if(!strcmp(e.soundSet,jar::hilltops::exteriorSet)){
        e.s.time2=jar::hilltops::birdSpeakerTag;
        AddSpawnField(const_cast<char*>("noise"),const_cast<char*>(jar::hilltops::exterior));
    }else if(strcmp(e.soundSet,"medium_platform"))return false;
    if(gi.bIsFromZone(e.soundSet,TAG_G_ALLOC))gi.Free(e.soundSet);
    e.soundSet=nullptr;
    SP_target_speaker(&e);
    return true;
}
