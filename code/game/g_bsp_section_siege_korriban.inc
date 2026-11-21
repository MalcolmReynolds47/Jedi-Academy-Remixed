// Attach the retail Siege Korriban brush controls to their owning section. GPL-2.0-only.
#include "g_siege_korriban.h"

static bool BSPSectionSiegeKorribanLocation(gentity_t& e){
    if(!e.classname||strcmp(e.classname,"target_location")||!e.message||!e.message[0])return false;
    // Keep authored labels without registering Siege points on the host map.
    if(gi.bIsFromZone(e.classname,TAG_G_ALLOC))gi.Free(e.classname);
    e.classname=G_NewString("info_notnull");
    e.s.time2=jar::siegeKorriban::locationTag;
    e.svFlags|=SVF_NOCLIENT;
    G_SetOrigin(&e,e.s.origin);
    return true;
}

static bool BSPSectionSiegeKorribanSpeaker(gentity_t& e){
    using namespace jar::siegeKorriban;
    if(!e.classname||strcmp(e.classname,"target_speaker")||!e.soundSet||
       strcmp(e.soundSet,lavaSoundSet)||e.spawnflags!=1||e.targetname)return false;
    if(gi.bIsFromZone(e.soundSet,TAG_G_ALLOC))gi.Free(e.soundSet);
    e.soundSet=nullptr;
    AddSpawnField(const_cast<char*>("noise"),const_cast<char*>(lavaLoop));
    SP_target_speaker(&e);
    e.s.time2=speakerTag;
    e.svFlags|=SVF_BROADCAST;
    return true;
}

bool G_SiegeKorribanPrepareMedia(gentity_t& e){
    using namespace jar::siegeKorriban;
    if(!e.classname||strcmp(e.classname,"fx_runner"))return true;
    if(!e.fxFile||e.bmodel||e.contents||e.target2)return false;
    char name[MAX_QPATH];
    const char* source=!Q_stricmpn(e.fxFile,"effects/",8)?e.fxFile+8:e.fxFile;
    COM_StripExtension(source,name,sizeof(name));
    const char* file=nullptr;
    if(e.spawnflags==4&&e.soundSet&&!strcmp(e.soundSet,torchSoundSet)&&!e.targetname&&!e.target){
        for(const auto& flame:torchEffects)if(!Q_stricmp(name,flame.file)){file=flame.file;break;}
    }else if(!e.soundSet&&e.targetname){
        for(const auto& effect:namedEffects){
            if(!strcmp(e.targetname,effect.name)&&!Q_stricmp(name,effect.file)&&
               !strcmp(e.target?e.target:"",effect.target)&&e.spawnflags==effect.flags){
                file=effect.file;
                break;
            }
        }
    }
    if(!file)return false;
    if(gi.bIsFromZone(e.fxFile,TAG_G_ALLOC))gi.Free(e.fxFile);
    e.fxFile=G_NewString(file);
    e.s.time2=effectTag;
    return true;
}

bool G_SiegeKorribanFinishMedia(gentity_t& root){
    using namespace jar::siegeKorriban;
    if(!G_SiegeKorribanRoom(&root))return false;
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members))return false;
    int scenery=0,speakers=0,torches=0,effects=0,locations=0;
    for(const auto* e:members){
        if(e->s.time2==locationTag){
            if(!e->message||!(e->svFlags&SVF_NOCLIENT))return false;
            ++locations;
        }else if(e->s.time2==sceneryTag){
            if(e->s.eType!=ET_GENERAL||e->s.modelindex<=0||!e->linked||e->contents)return false;
            ++scenery;
        }else if(e->s.time2==speakerTag){
            if(e->s.eType!=ET_SPEAKER||e->noise_index<=0||e->s.loopSound!=e->noise_index||!e->linked)return false;
            ++speakers;
        }else if(e->s.time2==effectTag){
            if(e->e_ThinkFunc!=thinkF_fx_runner_link||e->fxID<=0||e->fxID>=MAX_FX||!e->linked||
               e->delay!=200||e->random||e->splashDamage!=5||e->splashRadius!=16)return false;
            if(e->target){
                const auto count=std::count_if(members.begin(),members.end(),[&](const gentity_t* aim){
                    return aim->targetname&&!strcmp(aim->targetname,e->target)&&aim->classname&&
                           !strcmp(aim->classname,"info_notnull")&&DistanceSquared(e->currentOrigin,aim->currentOrigin)>1;
                });
                if(count!=1)return false;
            }
            if(e->spawnflags==4)++torches;
            else ++effects;
        }
    }
    if(locations!=10||scenery!=sceneryCount||speakers!=speakerCount||
       torches!=torchCount||effects!=int(ARRAY_LEN(namedEffects)))return false;
    const int sound=G_SoundIndex(torchLoop);
    if(sound<=0||sound>=MAX_SOUNDS)return false;
    vec3_t angles;
    const vec3_t up={-90,0,0};
    jar::angular::RebaseAngles(up,vec3_origin,root.currentAngles,angles);
    G_SubBSPCapture capture(&root);
    for(auto* e:members)if(e->s.time2==effectTag){
        const bool torch=e->spawnflags==4;
        if(e->fxFile&&gi.bIsFromZone(e->fxFile,TAG_G_ALLOC))gi.Free(e->fxFile);
        if(e->soundSet&&gi.bIsFromZone(e->soundSet,TAG_G_ALLOC))gi.Free(e->soundSet);
        e->fxFile=e->soundSet=nullptr;
        e->s.eType=ET_GENERAL;
        e->svFlags|=SVF_BROADCAST;
        e->noise_index=torch?sound:0;
        G_SetAngles(e,angles);
        fx_runner_link(e);
        // Spread the room's torch effects and damage pulses over their usual interval.
        if(torch)e->nextthink+=e->s.number%e->delay;
    }
    return true;
}

static bool BSPSectionSiegeKorribanWaypoint(gentity_t& point){
    if(!point.classname||strcmp(point.classname,"waypoint")||!point.targetname||!point.target)return false;
    for(const auto& spec:jar::siegeKorriban::waypoints){
        if(strcmp(spec.name,point.targetname)||strcmp(spec.target,point.target))continue;
        point.model=G_NewString(jar::siegeKorriban::waypointModel);
        point.svFlags|=SVF_NOCLIENT;
        G_SetOrigin(&point,point.s.origin);
        return true;
    }
    return false;
}

bool G_SiegeKorribanPrepareBrush(gentity_t& entity){
    using namespace jar::siegeKorriban;
    const auto* root=G_SubBSPOwner(&entity);
    if(!G_SiegeKorribanRoom(root)||!entity.bmodel||!entity.classname)return true;
    const int model=entity.s.modelindex-root->s.modelindex;
    if(!strcmp(entity.classname,"func_door")){
        const auto* door=std::find_if(std::begin(doors),std::end(doors),[&](const Door& spec){return spec.model==model;});
        if(door==std::end(doors)||entity.e_ThinkFunc!=thinkF_Think_MatchTeam||
           entity.e_UseFunc!=useF_Use_BinaryMover||entity.e_ReachedFunc!=reachedF_Reached_BinaryMover||
           entity.e_BlockedFunc!=blockedF_Blocked_Door||entity.speed!=door->speed||
           entity.wait!=door->wait*1000||entity.delay!=door->delay*1000||entity.damage!=door->damage||
           entity.spawnflags!=door->flags||strcmp(entity.targetname?entity.targetname:"",door->name)||
           strcmp(entity.opentarget?entity.opentarget:"",door->opens)||
           strcmp(entity.soundSet?entity.soundSet:"",door->sound?"stone_door":""))return false;
        entity.flags|=jarDetachedMoverAreaPortal;
        if(door->sound){
            if(gi.bIsFromZone(entity.soundSet,TAG_G_ALLOC))gi.Free(entity.soundSet);
            entity.soundSet=G_NewString(doorSoundSet);
        }
        return true;
    }
    if(!strcmp(entity.classname,"trigger_multiple")){
        const auto* pad=std::find_if(std::begin(pressurePads),std::end(pressurePads),
            [&](const Pressure& spec){return spec.model==model;});
        const auto* delivery=std::find_if(std::begin(deliveries),std::end(deliveries),
            [&](const Delivery& spec){return spec.model==model;});
        if(entity.e_TouchFunc!=touchF_Touch_Multi||entity.e_UseFunc!=useF_Use_Multi||!entity.target||
           (pad==std::end(pressurePads)&&delivery==std::end(deliveries)))return false;
        if(pad!=std::end(pressurePads)){
            if(strcmp(entity.target,pad->target)||entity.wait!=2||entity.delay||entity.spawnflags||
               entity.noDamageTeam!=TEAM_FREE)return false;
        }else if(strcmp(entity.target,delivery->target)||!entity.targetname||
                 strcmp(entity.targetname,delivery->trigger)||entity.wait!=-1||entity.delay||
                 entity.spawnflags!=jar::siege::requiresItem||entity.noDamageTeam!=TEAM_PLAYER)return false;
        entity.s.time2=BSPSectionBrushPadTag;
        if(!G_BSPSectionBrushPad(entity))return false;
    }else if(!strcmp(entity.classname,"trigger_hurt")){
        const bool lava=std::find(std::begin(lavaModels),std::end(lavaModels),model)!=std::end(lavaModels);
        if(!lava&&std::find(std::begin(hazardModels),std::end(hazardModels),model)==std::end(hazardModels))return false;
        if(entity.e_TouchFunc!=touchF_hurt_touch||entity.damage!=(lava?25:9999)||
           entity.wait||entity.delay||entity.spawnflags!=(8|4))return false;
        entity.s.time2=lava?BSPSectionLavaTag:BSPSectionBrushHurtTag;
        entity.wait=FRAMETIME;
        if(!G_BSPSectionBrushHurt(entity))return false;
    }else return true;
    G_SetAngles(&entity,root->currentAngles);
    gi.linkentity(&entity);
    return true;
}
