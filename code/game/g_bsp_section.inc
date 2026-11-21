// Qualified stock sections with native, owned children and local entry points.
// Other BSP profiles must supply their own controller-state/copy adapters.
// Included after native root/admission setup. GPL-2.0-only.
#include "g_bsp_section.h"
#include "g_siege_item.h"
#include "g_vehicles.h"
#include <cstdlib>
#include "g_spawn_context.h"
#include "g_subbsp_ownership.h"
#include "g_subbsp_movers.h"
#include "qcommon/jar_angular_trajectory.h"
#include "qcommon/jar_workshop_state.h"
#include "g_bsp_section_fx.h"
#include "g_bsp_section_hurt.h"
#include "g_bsp_section_bob.inc"
gentity_t* G_BSPSectionBreakableRoot(const gentity_t&child){
    if(jar::bspSection::SectionGlass(child))return G_SubBSPOwner(&child);
    if(!child.inuse||!child.classname||strcmp(child.classname,"func_breakable"))return nullptr;
    auto*root=G_SubBSPOwner(&child);
    if(G_SiegeKorribanRoom(root))return root;
    if(!root||!jar::bspSection::Portable(*root))return nullptr;
    return jar::bspSection::Breakable(jar::bspSection::Resource(root->model)->mode)?root:nullptr;
}
bool G_BSPSectionBreakable(const gentity_t&child){return G_BSPSectionBreakableRoot(child)!=nullptr;}
#include "g_bsp_section_motion.inc"
#include "g_bsp_section_factory.inc"
void AddSpawnField(char*,char*);
#include "g_bsp_section_ctf.inc"
#include "g_bsp_section_siege_korriban.inc"
bool G_BSPSectionSupportsActors(const gentity_t& root){
    if(!jar::bspSection::Portable(root))return false;
    const auto supported=[&](int ground){
        if(ground<MAX_CLIENTS||ground>=globals.num_entities)return false;
        const auto& support=g_entities[ground];
        return support.inuse&&support.linked&&(&support==&root||
            ((support.contents&(MASK_SOLID|CONTENTS_BODY))&&G_SubBSPOwner(&support)==&root));
    };
    for(int n=0;n<globals.num_entities;++n){
        const auto& actor=g_entities[n];
        if(!actor.inuse||!actor.client)continue;
        if(supported(actor.client->ps.groundEntityNum)||supported(actor.s.groundEntityNum)||
           (actor.m_pVehicle&&actor.m_pVehicle->m_LandTrace.fraction<1&&
            supported(actor.m_pVehicle->m_LandTrace.entityNum)))return true;
    }
    return false;
}
bool G_BSPSectionTeleportMembers(const gentity_t& root,std::vector<gentity_t*>& members){
    if(!jar::bspSection::Portable(root)||!root.linked||root.s.eType!=ET_MOVER||root.s.modelindex<=0||
       root.s.pos.trType!=TR_STATIONARY||
       !jar::angular::Stationary(root.s.apos.trType)||!VectorCompare(root.pos1,root.currentOrigin)||
       !VectorCompare(root.pos2,root.currentAngles)||!G_SubBSPMembers(&root,members))return false;
    const bool yavin=jar::bspSection::Resource(root.model)->mode==jar::bspSection::Mode::Yavin;
    const bool wasteland=jar::bspSection::Resource(root.model)->mode==jar::bspSection::Mode::Wasteland;
    for(const auto* child:members){
        if(child->client||child->freeAfterEvent||(child->svFlags&SVF_NO_TELEPORT)||
           !child->classname||!BSPSectionUncontrolled(*child))return false;
        if(jar::bspSection::Bobbing(*child)){
            if(!BSPSectionBobbingFits(root,*child))return false;
        }else if(G_BSPSectionBreakable(*child)){
            if(!BSPSectionBreakableFits(root,*child))return false;
        }else if(jar::bspSection::SectionLift(*child)){
            if(!BSPSectionLiftFits(root,*child))return false;
        }else if(jar::wasteland::GunEntity(*child)){
            if(!BSPSectionGunFits(root,*child))return false;
        }else if(G_BSPSectionBrushPad(*child)||G_BSPSectionBrushHurt(*child)){
            if(child->s.pos.trType!=TR_STATIONARY||!BSPSectionContains(root,*child,child->currentOrigin,vec3_origin))return false;
        }else if(jar::bspSection::SectionTrainRoot(*child)){
            if(!BSPSectionTrainFits(root,*child))return false;
        }else if(jar::bspSection::SectionMoverTrigger(*child)){
            if(child->s.pos.trType!=TR_STATIONARY||!BSPSectionBoxValid(child->pos1,child->pos2))return false;
        }else{
            const char* name=child->classname;
            if(child->bmodel||(child->contents&(MASK_SOLID|CONTENTS_BODY))||
               (child->s.pos.trType!=TR_STATIONARY&&child->s.pos.trType!=TR_INTERPOLATE))return false;
            if(strcmp(name,"info_null")&&strcmp(name,"info_notnull")&&strcmp(name,"target_position")&&
               strcmp(name,"target_speaker")&&strcmp(name,"fx_runner")&&strcmp(name,"misc_model_static")&&
               !(yavin&&(!strcmp(name,"target_random")||!strcmp(name,"func_timer")))&&
               !(wasteland&&(!strcmp(name,"misc_weather_zone")||!strcmp(name,"fx_snow")))&&
               !jar::bspSection::SectionTrainCorner(*child,root)&&
               FactoryFlagRoot(*child)!=&root&&
               !G_BSPSectionBoxTagged(*child)&&!G_BSPSectionPickupRoot(*child))return false;
        }
    }
    return true;
}
bool G_BSPSectionBounds(const jar::bspSection::Profile&profile,const vec3_t angles,vec3_t mins,vec3_t maxs,std::string&reason){
    jar::bspAdmission::Report report;
    if(!G_AdmitPortableSubBSP(profile.model,report,profile.mode)){reason=report.reason;return false;}
    if(profile.mode==jar::bspSection::Mode::Tatooine)
        report.mins[2]=std::min(report.mins[2],jar::tatooine::minimumTravelZ);
    jar::bspSection::Hull(report.mins,report.maxs,angles,mins,maxs);return true;
}
bool G_BSPSectionPublish(gentity_t& root){
    if(!jar::bspSection::Portable(root))return false;
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members))return false;
    // Permanent entities are sent only through the map-start baseline. These
    // assemblies spawn and move at runtime, independently of the host PVS.
    root.s.eFlags&=~EF_PERMANENT;
    root.svFlags|=SVF_BROADCAST;
    for(auto*child:members){
        if(!child->bmodel||child->s.eType!=ET_MOVER||child->s.modelindex<=0||
           (child->svFlags&SVF_NOCLIENT))continue;
        child->s.eFlags&=~EF_PERMANENT;
        child->svFlags|=SVF_BROADCAST;
    }
    return true;
}
void G_BSPSectionRestoreWorkshop(gentity_t& root,jar::workshop::State& state){
    if(!G_BSPSectionPublish(root)||state.ownedCount>=jar::workshop::ownedLimit||
       !root.targetname||strncmp(root.targetname,"jar_sb_",7))return;
    // Older builds could discard a named room's workshop record after its
    // native classname changed. Reclaim only its original unused owner ID.
    char* end=nullptr;
    const unsigned long id=std::strtoul(root.targetname+7,&end,10);
    if(*end||!id||id>=state.nextOwned)return;
    char name[32];
    Com_sprintf(name,sizeof(name),"jar_sb_%u",unsigned(id));
    if(strcmp(root.targetname,name))return;
    const auto insert=std::find_if(state.owned.begin(),state.owned.end(),
        [id](const jar::workshop::Owned& entry){return entry.id>id;});
    const auto& profile=*jar::bspSection::Resource(root.model);
    for(const auto& card:jar::catalog::entries)if(card.kind==5&&!strcmp(card.name,profile.card)){
        jar::workshop::InsertOwnership(state,size_t(insert-state.owned.begin()),
            {uint32_t(id),uint32_t(root.s.number),card.id,1,0});
        return;
    }
}
const gentity_t* G_BSPSectionPickupRoot(const gentity_t& item) {
    if (!item.inuse || item.s.time2 != jar::rift::pickupTag || !item.item || !item.classname)
        return nullptr;
    const auto* root = G_SubBSPOwner(&item);
    if(!root||!jar::bspSection::Portable(*root))return nullptr;
    size_t count;
    return jar::bspSection::PickupSpecs(jar::bspSection::Resource(root->model)->mode,count) ? root : nullptr;
}

static bool BSPSectionInitializePickupsAndFalls(gentity_t& root,jar::bspSection::Mode mode) {
    std::vector<gentity_t*> members;
    if (!G_SubBSPMembers(&root, members)) return false;
    const bool korribanRoom=mode==jar::bspSection::Mode::Korriban;
    const bool sentinelRoom=mode==jar::bspSection::Mode::Sentinel;
    const bool tatooineRoom=mode==jar::bspSection::Mode::Tatooine;
    const bool taspirRoom=mode==jar::bspSection::Mode::Taspir;
    const bool factoryRoom=mode==jar::bspSection::Mode::Factory;
    const bool coruscantRoom=mode==jar::bspSection::Mode::Coruscant;
    const bool hilltopsRoom=mode==jar::bspSection::Mode::Hilltops;
    const bool dropzoneRoom=mode==jar::bspSection::Mode::DropZone;
    const bool wastelandRoom=mode==jar::bspSection::Mode::Wasteland;
    const bool separateHazards=sentinelRoom||taspirRoom||jar::bspSection::CapturePractice(mode);
    const int fallCount=tatooineRoom?0:korribanRoom?int(ARRAY_LEN(jar::korriban::fallBoxes)):2;
    unsigned hazards=0;
    int pickups=0;
    for (auto* child : members) {
        if (child->classname && !strcmp(child->classname, "trigger_hurt")) {
            const int model = child->s.modelindex - root.s.modelindex;
            if(separateHazards)continue;
            if(model<1||model>fallCount)return false;
            const bool compound = !korribanRoom && model == 1;
            const auto& box = korribanRoom?jar::korriban::fallBoxes[model-1]:jar::rift::fallBoxes[compound?0:2];
            if (!G_BSPSectionInitializeHurt(*child, box.mins, box.maxs, compound)) return false;
            hazards |= 1u << (model - 1);
        } else if (child->item) {
            child->s.time2 = jar::rift::pickupTag;
            child->svFlags |= SVF_BROADCAST;
            child->e_ThinkFunc = thinkF_NULL;
            child->nextthink = 0;
            FinishSpawningItem(child);
            if (!child->inuse || child->s.eType != ET_ITEM || !(child->contents & CONTENTS_ITEM))
                return false;
            ++pickups;
        }
    }
    return (separateHazards||hazards==(1u<<fallCount)-1) &&
           pickups==(wastelandRoom?jar::wasteland::pickupCount:dropzoneRoom?jar::dropzone::pickupCount:hilltopsRoom?jar::hilltops::pickupCount:coruscantRoom?jar::coruscant::pickupCount:factoryRoom?jar::factory::pickupCount:taspirRoom?jar::taspir::pickupCount:tatooineRoom?jar::tatooine::pickupCount:sentinelRoom?jar::sentinel::pickupCount:
                     korribanRoom?jar::korriban::pickupCount:jar::rift::pickupCount);
}

template<size_t LiftCount,size_t PadCount>
static bool BSPSectionInitializeLiftGroups(gentity_t& root,const jar::bspAdmission::Report& report,
        const jar::sentinel::Lift (&lifts)[LiftCount],const jar::sentinel::Pad (&pads)[PadCount],
        int fallModel,int platformModel){
    using namespace jar::sentinel;
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members))return false;
    std::vector<gentity_t*> brushes(report.models);
    for(auto* child:members)if(child->bmodel){
        const int model=child->s.modelindex-root.s.modelindex;
        if(model<=0||model>=report.models||brushes[model])return false;
        brushes[model]=child;
    }
    if(fallModel&&(!brushes[fallModel]||
       !G_BSPSectionInitializeHurt(*brushes[fallModel],report.hurtMins,report.hurtMaxs)))return false;
    if(platformModel){
        auto* platform=brushes[platformModel];
        if(!platform||!platform->classname||strcmp(platform->classname,"func_plat")||
           platform->e_TouchFunc!=touchF_Touch_Plat||platform->owner!=platform||
           !(platform->contents&CONTENTS_SOLID))return false;
        platform->flags|=jarDetachedMoverAreaPortal;
        const int triggerCount=std::count_if(members.begin(),members.end(),[&](const gentity_t* child){
            return child->owner==platform&&jar::bspSection::SectionMoverTrigger(*child);
        });
        if(triggerCount!=1||!BSPSectionLiftFits(root,*platform))return false;
    }
    for(const auto& spec:pads){
        auto* pad=brushes[spec.model];
        if(!pad||pad->e_UseFunc!=useF_Use_Multi||pad->e_TouchFunc!=touchF_Touch_Multi||!pad->target||
           pad->contents!=CONTENTS_TRIGGER||pad->wait!=spec.wait)return false;
        pad->delay=200;
        pad->s.time2=BSPSectionBrushPadTag;
        G_SetAngles(pad,root.currentAngles);
        gi.linkentity(pad);
    }
    for(const auto& spec:lifts){
        auto* lift=brushes[spec.model];
        const auto* pad=brushes[pads[spec.group].model];
        if(!lift||!lift->targetname||strcmp(lift->targetname,pad->target)||!(lift->contents&CONTENTS_SOLID))return false;
        if(lift->soundSet){
            const bool piston=!strcmp(lift->soundSet,"piston");
            if(gi.bIsFromZone(lift->soundSet,TAG_G_ALLOC))gi.Free(lift->soundSet);
            lift->soundSet=G_NewString(piston?pistonSet:jar::shaft::liftSoundSet);
        }
        lift->flags|=jarDetachedMoverAreaPortal;
        lift->delay=200;
        if(!BSPSectionLiftFits(root,*lift))return false;
    }
    return true;
}

static bool BSPSectionInitializeShaft(gentity_t& root, const jar::bspAdmission::Report& report) {
    std::vector<gentity_t*> members;
    if (!G_SubBSPMembers(&root, members)) return false;
    gentity_t* lifts[2]{};
    gentity_t* pads[2]{};
    for (auto* child : members) {
        if (!child->bmodel || !child->classname) continue;
        const int model = child->s.modelindex - root.s.modelindex;
        if (!strcmp(child->classname, "func_door") && (model == 2 || model == 3))
            lifts[model - 2] = child;
        if (!strcmp(child->classname, "trigger_multiple") && (model == 1 || model == 4))
            pads[model == 1 ? 0 : 1] = child;
    }
    for (int i = 0; i < 2; ++i) {
        auto* lift = lifts[i];
        auto* pad = pads[i];
        if (!lift || !pad || !lift->targetname || !pad->target || strcmp(lift->targetname, pad->target) ||
            !lift->soundSet || strcmp(lift->soundSet, "medium_platform") || !(lift->contents & CONTENTS_SOLID))
            return false;
        // The lifts belong to the room, including sounds and area-portal state.
        if (gi.bIsFromZone(lift->soundSet, TAG_G_ALLOC)) gi.Free(lift->soundSet);
        lift->soundSet = G_NewString(jar::shaft::liftSoundSet);
        lift->flags |= jarDetachedMoverAreaPortal;
        // SP parses the native delay field as an integer. Restore the
        // qualified fractional delay in milliseconds after native setup.
        lift->delay = pad->delay = 200;
        if (!BSPSectionLiftFits(root, *lift) ||
            !G_BSPSectionInitializePad(*pad, report.padMins[i], report.padMaxs[i]))
            return false;
    }
    return true;
}

#include "g_bsp_section_taspir.inc"
#include "g_bsp_section_wasteland.inc"

static bool BSPSectionSpawn(gentity_t*e,const jar::bspSection::Profile*profile){
    G_SpawnContext context;
    AddSpawnField(const_cast<char*>("bspmodel"),const_cast<char*>(profile->model+1));
    AddSpawnField(const_cast<char*>("_jar_portable"),const_cast<char*>("1"));
    if(e->model&&gi.bIsFromZone(e->model,TAG_G_ALLOC))gi.Free(e->model);
    e->model=G_NewString(profile->model);
    if(strcmp(e->classname,"misc_bsp")){
        if(gi.bIsFromZone(e->classname,TAG_G_ALLOC))gi.Free(e->classname);
        e->classname=G_NewString("misc_bsp");
    }
    e->e_UseFunc=useF_target_teleporter_use;e->svFlags|=SVF_PLAYER_USABLE;
    jar::bspAdmission::Report report;
    G_SpawnSubBSP(e,&report);if(!e->inuse)return false;
    VectorCopy(e->currentOrigin,e->pos1);VectorCopy(e->currentAngles,e->pos2);
    bool moversReady=true;
    if(profile->mode==jar::bspSection::Mode::Sentinel)
        moversReady=BSPSectionInitializeLiftGroups(*e,report,jar::sentinel::lifts,jar::sentinel::pads,jar::sentinel::fallModel,0);
    if(profile->mode==jar::bspSection::Mode::Tatooine){
        e->mins[2]=std::min(e->mins[2],jar::tatooine::minimumTravelZ);
        gi.linkentity(e);
        moversReady=BSPSectionInitializeLiftGroups(*e,report,jar::tatooine::lifts,jar::tatooine::pads,0,jar::tatooine::platform);
    }
    if(profile->mode==jar::bspSection::Mode::Taspir)moversReady=BSPSectionInitializeTaspir(*e,report);
    if(profile->mode==jar::bspSection::Mode::Coruscant)
        moversReady=BSPSectionInitializeCTF<jar::coruscant::models>(*e,jar::coruscant::lifts,jar::coruscant::pads,jar::coruscant::falls,false);
    if(profile->mode==jar::bspSection::Mode::Hilltops)
        moversReady=BSPSectionInitializeCTF<jar::hilltops::models>(*e,jar::hilltops::lifts,jar::hilltops::pads,jar::hilltops::falls,true);
    if(profile->mode==jar::bspSection::Mode::DropZone)moversReady=BSPSectionInitializeDropZone(*e);
    if(profile->mode==jar::bspSection::Mode::Wasteland)moversReady=BSPSectionInitializeWasteland(*e,report);
    if(profile->mode==jar::bspSection::Mode::Factory&&!BSPSectionInitializeFactory(*e,report)){
        gi.Printf("JA:R: Factory brush or flag setup failed; nothing spawned.\n");G_FreeEntity(e);return false;
    }
    if(!moversReady){
        gi.Printf("JA:R: %s lift or trigger setup failed; nothing spawned.\n",profile->title);G_FreeEntity(e);return false;
    }
    size_t pickupKinds;
    if(jar::bspSection::PickupSpecs(profile->mode,pickupKinds)&&
       !BSPSectionInitializePickupsAndFalls(*e,profile->mode)){
        gi.Printf("JA:R: %s pickup or hazard setup failed; nothing spawned.\n",profile->title);G_FreeEntity(e);return false;
    }
    if(jar::bspSection::Breakable(profile->mode)){
        std::vector<gentity_t*> members;int brushes=0;
        if(G_SubBSPMembers(e,members))for(auto*child:members){
            if(!G_BSPSectionBreakable(*child))continue;
            if(child->s.modelindex<=e->s.modelindex||child->s.modelindex>=e->s.modelindex+report.models||
               !(child->contents&CONTENTS_SOLID)||!BSPSectionBreakableFits(*e,*child))break;
            ++brushes;
        }
        if(brushes!=report.models-1){
            gi.Printf("JA:R: section breakable brush setup failed; nothing spawned.\n");G_FreeEntity(e);return false;
        }
    }
    if(profile->mode==jar::bspSection::Mode::Shaft&&!BSPSectionInitializeShaft(*e,report)){
        gi.Printf("JA:R: Imperial Shaft lift setup failed; nothing spawned.\n");G_FreeEntity(e);return false;
    }
    if(profile->mode==jar::bspSection::Mode::Hurt||profile->mode==jar::bspSection::Mode::Shaft){
        std::vector<gentity_t*> members;int hazards=0;
        if(G_SubBSPMembers(e,members))for(auto*child:members)if(child->classname&&!strcmp(child->classname,"trigger_hurt")){
            if(!G_BSPSectionInitializeHurt(*child,report.hurtMins,report.hurtMaxs))break;
            ++hazards;
        }
        if(hazards!=1){gi.Printf("JA:R: section hazard registration failed; nothing spawned.\n");G_FreeEntity(e);return false;}
    }
    if(profile->mode==jar::bspSection::Mode::Fire||profile->mode==jar::bspSection::Mode::Korriban||profile->mode==jar::bspSection::Mode::Yavin){
        std::vector<gentity_t*> members;
        if(!G_SubBSPMembers(e,members)){G_FreeEntity(e);return false;}
        for(auto* child:members){
            if(!child->classname||strcmp(child->classname,"fx_runner"))continue;
            const auto aim=std::find_if(members.begin(),members.end(),[&](const gentity_t* candidate){
                return child->target&&candidate->targetname&&!Q_stricmp(child->target,candidate->targetname)&&
                       candidate->classname&&!strcmp(candidate->classname,"info_notnull");
            });
            if(!G_BSPSectionInitializeFX(*child,e->currentAngles,aim==members.end()?nullptr:*aim)){
                gi.Printf("JA:R: section effect setup failed; nothing spawned.\n");G_FreeEntity(e);return false;
            }
        }
    }
    e->s.pos.trType=TR_STATIONARY;e->s.apos.trType=TR_STATIONARY;
    VectorClear(e->s.pos.trDelta);VectorClear(e->s.apos.trDelta);
    G_BSPSectionPublish(*e);
    return true;
}
bool G_BSPSectionSpawn(gentity_t*e){
    const auto* profile=jar::bspSection::Catalog(e->classname);
    return profile&&BSPSectionSpawn(e,profile);
}
bool G_BSPSectionScenery(gentity_t* e, gentity_t* root) {
    if (!root || !e->classname || strcmp(e->classname,"misc_model_static"))
        return false;
    const bool siege=G_SiegeKorribanRoom(root);
    if(!siege&&!jar::bspSection::Portable(*root))return false;
    const auto mode=siege?jar::bspSection::Mode::SiegeKorriban:jar::bspSection::Resource(root->model)->mode;
    if(!siege&&mode!=jar::bspSection::Mode::Shaft&&mode!=jar::bspSection::Mode::Yavin&&mode!=jar::bspSection::Mode::Taspir&&
       mode!=jar::bspSection::Mode::Factory&&mode!=jar::bspSection::Mode::Hilltops&&mode!=jar::bspSection::Mode::DropZone&&
       mode!=jar::bspSection::Mode::Wasteland)
        return false;

    // Keep the authored MD3s as owned runtime models instead of inserting
    // them into the host client's permanent scenery array.
    G_SpawnVector("modelscale_vec", "1 1 1", e->s.modelScale);
    float scale;
    if (G_SpawnFloat("modelscale", "1", &scale))
        VectorSet(e->s.modelScale, scale, scale, scale);
    if(siege){
        e->s.time2=jar::siegeKorriban::sceneryTag;
        G_SpawnFloat("zoffset","0",&e->s.angles2[2]);
    }
    e->s.modelindex = e->model ? G_ModelIndex(e->model) : 0;
    if (!e->s.modelindex) {
        G_FreeEntity(e);
        return true;
    }
    e->s.eType = ET_GENERAL;
    e->svFlags |= SVF_BROADCAST;
    G_SetOrigin(e, e->s.origin);
    gi.linkentity(e);
    return true;
}

bool G_BSPSectionPosition(gentity_t*e,gentity_t*root){
    if(G_SiegeKorribanRoom(root)&&(G_SiegeKorribanStart(*e)||BSPSectionSiegeKorribanWaypoint(*e)||
       BSPSectionSiegeKorribanSpeaker(*e)||BSPSectionSiegeKorribanLocation(*e)))return true;
    if(!root||!jar::bspSection::Portable(*root))return false;
    const auto mode=jar::bspSection::Resource(root->model)->mode;
    const bool capturePractice=jar::bspSection::CapturePractice(mode);
    if(mode==jar::bspSection::Mode::Wasteland&&e->classname&&
       (!strcmp(e->classname,"misc_weather_zone")||!strcmp(e->classname,"fx_snow"))){
        // Weather uses the verified local volume planes, never the host's
        // global world-effect commands. Retain the authored points as children.
        G_SetOrigin(e,e->s.origin);
        e->svFlags|=SVF_NOCLIENT;
        return true;
    }
    if(mode==jar::bspSection::Mode::Wasteland&&e->classname&&!strcmp(e->classname,"emplaced_gun"))
        e->s.time2=jar::wasteland::gunTag;
    if(mode==jar::bspSection::Mode::Hilltops&&BSPSectionHilltopsSpeaker(*e))return true;
    if(capturePractice)if(const int team=jar::factory::FlagTeam(e->classname))return FactorySpawnFlag(*e,team);
    // Stock info_nulls ordinarily discard themselves (some immediately). Keep
    // their reference points local to the section instead of losing them.
    if(e->classname&&!strcmp(e->classname,"info_null")){G_SetOrigin(e,e->s.origin);return true;}
    const bool skyPoint=jar::bspSection::SectionSky(root->model)&&
                        e->classname&&!strcmp(e->classname,"misc_skyportal");
    if(!skyPoint&&!jar::bspSection::Position(e->classname)&&!(capturePractice&&jar::factory::StartTeam(e->classname)))return false;
    // Retain owned points and their original class without registering a
    // host respawn or running the map-global sky camera callback.
    e->message=e->classname;e->classname=G_NewString("info_notnull");
    G_SetOrigin(e,e->s.origin);return true;
}
bool G_BSPSectionSkyOrigin(const gentity_t& root, vec3_t local) {
    if (!jar::bspSection::Portable(root) || !jar::bspSection::SectionSky(root.model))
        return false;
    std::vector<gentity_t*> members;
    if (!G_SubBSPMembers(&root, members))
        return false;
    for (const auto* point : members) {
        if (!point->message || strcmp(point->message, "misc_skyportal") ||
            !point->classname || strcmp(point->classname, "info_notnull"))
            continue;
        vec3_t axis[3], offset;
        AnglesToAxis(root.currentAngles, axis);
        VectorSubtract(point->currentOrigin, root.currentOrigin, offset);
        for (int i = 0; i < 3; ++i)
            local[i] = DotProduct(offset, axis[i]);
        return true;
    }
    return false;
}
bool G_BSPSectionMoved(gentity_t&e){
    if(!jar::bspSection::Portable(e))return false;
    if(VectorCompare(e.pos1,e.currentOrigin)&&VectorCompare(e.pos2,e.currentAngles))return true;
    std::vector<gentity_t*> members;if(!G_SubBSPMembers(&e,members))return false;
    vec3_t before[3],after[3];AnglesToAxis(e.pos2,before);AnglesToAxis(e.currentAngles,after);
    auto point=[&](const vec3_t input,vec3_t output){vec3_t delta,local;VectorSubtract(input,e.pos1,delta);
        for(int a=0;a<3;++a)local[a]=DotProduct(delta,before[a]);VectorCopy(e.currentOrigin,output);
        for(int a=0;a<3;++a)VectorMA(output,local[a],after[a],output);
    };
    for(auto*child:members){
        if(BSPSectionRebaseBobbing(*child,e.pos1,e.pos2,e.currentOrigin,e.currentAngles))continue;
        if(jar::bspSection::SectionLift(*child)||jar::bspSection::SectionTrainRoot(*child)){
            point(child->currentOrigin,child->currentOrigin);
            point(child->s.origin,child->s.origin);
            point(child->pos1,child->pos1);point(child->pos2,child->pos2);
            point(child->s.pos.trBase,child->s.pos.trBase);
            jar::angular::RebaseVector(child->s.pos.trDelta,e.pos2,e.currentAngles,child->s.pos.trDelta);
            jar::angular::RebaseVector(child->movedir,e.pos2,e.currentAngles,child->movedir);
            G_SetAngles(child,e.currentAngles);
            if(child->linked)gi.linkentity(child);
            continue;
        }
        vec3_t pos,angles;point(child->currentOrigin,pos);
        jar::angular::RebaseAngles(child->currentAngles,e.pos2,e.currentAngles,angles);
        G_SetOrigin(child,pos);G_SetAngles(child,angles);
        VectorCopy(pos,child->s.origin);VectorCopy(angles,child->s.angles);
        if(jar::wasteland::GunEntity(*child)){
            jar::angular::RebaseAngles(child->pos1,e.pos2,e.currentAngles,child->pos1);
            jar::angular::RebaseAngles(child->lastAngles,e.pos2,e.currentAngles,child->lastAngles);
        }
        if(G_BSPSectionBrushPad(*child)&&(child->spawnflags&2))
            jar::angular::RebaseVector(child->movedir,e.pos2,e.currentAngles,child->movedir);
        if(jar::bspSection::SectionMoverTrigger(*child))
            jar::volume::Hull(child->pos1,child->pos2,child->currentAngles,child->mins,child->maxs);
        else G_BSPSectionRefreshBoxBounds(*child);
        if(child->linked)gi.linkentity(child);
    }
    VectorCopy(e.currentOrigin,e.pos1);VectorCopy(e.currentAngles,e.pos2);
    VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);return true;
}
bool G_BSPSectionEnter(gentity_t*root,gentity_t*player){
    if(!root||!jar::bspSection::Portable(*root))return false;
    if(!player||!player->client||player->health<=0||player->s.m_iVehicleNum)return true;
    std::vector<gentity_t*> members;G_SubBSPMembers(root,members);
    const bool capturePractice=jar::bspSection::CapturePractice(jar::bspSection::Resource(root->model)->mode);
    const bool siege=G_SiegeKorribanRoom(root);
    for(auto*point:members)if(siege?G_SiegeKorribanEntry(*point):
        jar::bspSection::Start(point->message)||(capturePractice&&jar::factory::StartTeam(point->message))){
        // TeleportPlayer raises the origin by one unit before its killbox.
        // Test that exact hull so entering a section never telefrags its occupants.
        vec3_t arrival;VectorCopy(point->currentOrigin,arrival);arrival[2]+=1;
        trace_t check{};gi.trace(&check,arrival,player->mins,player->maxs,arrival,player->s.number,MASK_PLAYERSOLID,(EG2_Collision)0,0);
        if(check.startsolid||check.allsolid)continue;
        vec3_t lo,hi;VectorAdd(arrival,player->mins,lo);VectorAdd(arrival,player->maxs,hi);
        if(std::any_of(members.begin(),members.end(),[&](const gentity_t*child){
            return G_BSPSectionHurtContact(*child,lo,hi)||(G_BSPSectionBrushHurt(*child)&&child->linked&&
                !(child->svFlags&SVF_INACTIVE)&&gi.EntityContact(lo,hi,child));
        }))continue;
        TeleportPlayer(player,point->currentOrigin,point->currentAngles);
        if(siege)G_SiegeKorribanEntered(*root,*player);
        if(capturePractice&&!root->count)gi.SendServerCommand(0,"cp \"Use either flag to choose your home base.\"");
        return true;
    }
    gi.Printf("JA:R: section entry points are obstructed or unsafe. Clear a destination before entering.\n");return true;
}
