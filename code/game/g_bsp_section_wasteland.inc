// Native Hoth Wasteland members, after stock entity spawn. GPL-2.0-only.
static bool BSPSectionInitializeWasteland(gentity_t& root,const jar::bspAdmission::Report& report){
    using namespace jar::wasteland;
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members)||!BSPSectionLinkFlags(root,members))return false;
    gentity_t* brushes[models]{};
    int emplacements=0;
    for(auto* child:members)if(GunEntity(*child)){
        if(child->e_UseFunc!=useF_emplaced_gun_use||child->health!=gunHealth||
           child->count!=gunAmmo||!child->takedamage||!BSPSectionGunFits(root,*child))return false;
        ++emplacements;
    }
    if(emplacements!=ARRAY_LEN(guns))return false;
    for(auto* child:members)if(child->bmodel){
        const int model=child->s.modelindex-root.s.modelindex;
        if(model<=0||model>=models||brushes[model])return false;
        brushes[model]=child;
    }
    for(const auto& leaf:doorLeaves){
        auto* door=brushes[leaf.model];
        if(!door||!BSPSectionTaspirSound(*door,jar::taspir::sounds[0]))return false;
        door->flags|=jarDetachedMoverAreaPortal;
    }
    auto* leader=LinkDoorTeam(root,brushes);
    if(!leader)return false;
    for(const auto& leaf:doorLeaves){
        auto& door=*brushes[leaf.model];
        if(!BSPSectionLiftFits(root,door))return false;
        door.e_ThinkFunc=thinkF_NULL;
        door.nextthink=0;
    }
    for(int index:platforms){
        auto* platform=brushes[index];
        if(!platform||platform->owner!=platform||platform->e_TouchFunc!=touchF_Touch_Plat||
           !platform->soundSet||strcmp(platform->soundSet,"medium_platform"))return false;
        if(gi.bIsFromZone(platform->soundSet,TAG_G_ALLOC))gi.Free(platform->soundSet);
        platform->soundSet=G_NewString(jar::shaft::liftSoundSet);
        platform->flags|=jarDetachedMoverAreaPortal;
        if(!BSPSectionLiftFits(root,*platform))return false;
    }
    for(const auto& spec:lifts){
        auto* lift=brushes[spec.model];
        if(!lift||!lift->targetname||strcmp(lift->targetname,spec.target)||!lift->soundSet||
           strcmp(lift->soundSet,"medium_platform")||lift->speed!=spec.speed||
           lift->damage!=9999||lift->spawnflags!=MOVER_CRUSHER)return false;
        if(gi.bIsFromZone(lift->soundSet,TAG_G_ALLOC))gi.Free(lift->soundSet);
        lift->soundSet=G_NewString(jar::shaft::liftSoundSet);
        lift->flags|=jarDetachedMoverAreaPortal;
        lift->delay=200;
        if(!BSPSectionLiftFits(root,*lift))return false;
    }
    for(const auto& pad:liftPads)
        if(!BSPSectionInitializeCTFPad(brushes[pad.model],root,pad,lifts[pad.lift].target))return false;
    for(const auto& spec:falls){
        auto* fall=brushes[spec.model];
        if(spec.damage<0){
            if(!BSPSectionInitializeCTFFall(fall,root,4))return false;
        }else{
            if(!fall||fall->damage!=spec.damage||fall->spawnflags!=spec.flags||
               fall->e_TouchFunc!=touchF_hurt_touch||fall->wait)return false;
            fall->s.time2=BSPSectionBrushHurtTag;
            if(!G_BSPSectionBrushHurt(*fall))return false;
            G_SetAngles(fall,root.currentAngles);
            gi.linkentity(fall);
        }
    }
    {
        G_SubBSPCapture capture(&root);
        Think_SpawnNewDoorTrigger(leader);
    }
    if(!G_SubBSPMembers(&root,members)||members.size()!=size_t(report.entities-1+helpers))return false;
    for(int model:{doorLeaves[0].model,platforms[0],platforms[1]}){
        const int count=std::count_if(members.begin(),members.end(),[&](const gentity_t* child){
            return child->owner==brushes[model]&&jar::bspSection::SectionMoverTrigger(*child);
        });
        if(count!=1)return false;
    }
    return true;
}
