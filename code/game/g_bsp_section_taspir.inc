// Runtime setup for the retail Taspir arena. GPL-2.0-only.
void Think_SpawnNewDoorTrigger(gentity_t*);
void SetMoverState(gentity_t*,moverState_t,int);

static bool BSPSectionTaspirSound(gentity_t& e,const jar::taspir::SoundSet& sound){
    if(!e.soundSet||strcmp(e.soundSet,sound.stock))return false;
    if(gi.bIsFromZone(e.soundSet,TAG_G_ALLOC))gi.Free(e.soundSet);
    e.soundSet=G_NewString(sound.runtime);
    return true;
}

static bool BSPSectionInitializeTaspir(gentity_t& root,const jar::bspAdmission::Report& report){
    using namespace jar::taspir;
    using namespace jar::bspSection;
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members))return false;
    gentity_t* brushes[modelCount]{},*route[ARRAY_LEN(corners)]{};
    for(auto* child:members){
        if(child->bmodel){
            const int model=child->s.modelindex-root.s.modelindex;
            if(model<=0||model>=modelCount||brushes[model])return false;
            brushes[model]=child;
        }else if(SectionTrainCorner(*child,root)){
            const auto* corner=std::find_if(std::begin(corners),std::end(corners),[&](const Corner& c){return !strcmp(c.name,child->targetname);});
            if(corner==std::end(corners)||route[corner-corners]||child->speed!=corner->speed||child->wait!=corner->wait)return false;
            route[corner-corners]=child;
        }
    }
    if(std::any_of(std::begin(brushes)+1,std::end(brushes),[](const gentity_t* e){return !e;})||
       std::any_of(std::begin(route),std::end(route),[](const gentity_t* e){return !e;}))return false;
    auto& train=*brushes[trainModel];
    if(SectionTrainRoot(train)!=&root||!BSPSectionTaspirSound(train,sounds[2])||
       !G_BSPSectionInitializeHurt(*brushes[fallModel],report.hurtMins,report.hurtMaxs))return false;
    G_SetAngles(&train,root.currentAngles);
    for(size_t i=0;i<ARRAY_LEN(corners);++i){
        auto* next=route[(i+1)%ARRAY_LEN(corners)];
        if(!route[i]->target||strcmp(route[i]->target,next->targetname)||
           !BSPSectionContains(root,train,route[i]->s.origin,vec3_origin))return false;
        route[i]->nextTrain=next;
    }
    train.nextTrain=route[1];
    VectorCopy(route[0]->s.origin,train.pos1);
    VectorCopy(route[1]->s.origin,train.pos2);
    train.s.pos.trDuration=1; // The first two stock corners share the bottom stop.
    SetMoverState(&train,MOVER_1TO2,level.time);
    train.s.pos.trType=TR_STATIONARY;
    train.nextthink=level.time+corners[0].wait*1000;
    train.e_ThinkFunc=thinkF_Think_BeginMoving;
    if(!BSPSectionTrainFits(root,train))return false;
    G_SubBSPCapture capture(&root);
    for(int index:doors){
        auto& door=*brushes[index];
        if(door.e_ThinkFunc!=thinkF_Think_SpawnNewDoorTrigger||!BSPSectionTaspirSound(door,sounds[0]))return false;
        door.flags|=jarDetachedMoverAreaPortal;
        if(!BSPSectionLiftFits(root,door))return false;
        door.e_ThinkFunc=thinkF_NULL;
        door.nextthink=0;
        Think_SpawnNewDoorTrigger(&door);
    }
    for(const auto& spec:platforms){
        auto& platform=*brushes[spec.model];
        if(platform.owner!=&platform||platform.e_TouchFunc!=touchF_Touch_Plat||!BSPSectionTaspirSound(platform,sounds[1]))return false;
        platform.flags|=jarDetachedMoverAreaPortal;
        if(!BSPSectionLiftFits(root,platform))return false;
    }
    if(!G_SubBSPMembers(&root,members)||members.size()!=size_t(entityCount-1+helperCount))return false;
    bool triggered[modelCount]{};
    for(auto* child:members)if(SectionMoverTrigger(*child)){
        const int index=child->owner->s.modelindex-root.s.modelindex;
        if(index<=0||index>=modelCount||triggered[index])return false;
        triggered[index]=true;
    }
    for(int index:doors)if(!triggered[index])return false;
    for(const auto& spec:platforms)if(!triggered[spec.model])return false;
    return true;
}
