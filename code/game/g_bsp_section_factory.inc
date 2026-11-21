// Factory flags and breakable brush setup. GPL-2.0-only.
static gentity_t* FactoryFlagRoot(const gentity_t& flag){
    if(!flag.inuse||flag.e_UseFunc!=useF_factory_flag_use||!jar::factory::FlagTeam(flag.classname))return nullptr;
    auto* root=G_SubBSPOwner(&flag);
    return root&&jar::bspSection::Portable(*root)&&jar::bspSection::CapturePractice(jar::bspSection::Resource(root->model)->mode)?root:nullptr;
}

static bool FactorySpawnFlag(gentity_t& flag,int team){
    flag.s.modelindex=G_ModelIndex(jar::factory::flags[team-1].model);
    if(!flag.s.modelindex){G_FreeEntity(&flag);return true;}
    flag.s.eType=ET_GENERAL;
    VectorSet(flag.s.modelScale,1,1,1);
    VectorSet(flag.mins,-16,-16,-20);VectorSet(flag.maxs,16,16,32);
    flag.contents=CONTENTS_TRIGGER|CONTENTS_ITEM;
    flag.svFlags|=SVF_BROADCAST|SVF_PLAYER_USABLE;
    flag.e_UseFunc=useF_factory_flag_use;
    flag.e_TouchFunc=touchF_factory_flag_touch;
    G_SetOrigin(&flag,flag.s.origin);
    gi.linkentity(&flag);
    return true;
}

static bool FactoryPlayerInSection(const gentity_t* player,const gentity_t& root){
    if(!player||player!=&g_entities[0]||!player->inuse||!player->client||player->health<=0||player->s.m_iVehicleNum)return false;
    vec3_t basis[3],delta;AnglesToAxis(root.currentAngles,basis);VectorSubtract(player->currentOrigin,root.currentOrigin,delta);
    for(int i=0;i<3;++i){
        const float local=DotProduct(delta,basis[i]);
        if(local<root.mins[i]||local>root.maxs[i])return false;
    }
    return true;
}

static gentity_t* FactoryOtherFlag(const gentity_t& flag,const gentity_t& root){
    auto* other=flag.enemy;
    return other&&other!=&flag&&FactoryFlagRoot(*other)==&root?other:nullptr;
}

static void FactoryReturnFlag(gentity_t& flag){
    flag.s.eFlags&=~EF_NODRAW;
    flag.contents=CONTENTS_TRIGGER|CONTENTS_ITEM;
    flag.svFlags|=SVF_PLAYER_USABLE;
    flag.e_ThinkFunc=thinkF_NULL;
    flag.nextthink=0;
    flag.activator=nullptr;
    gi.linkentity(&flag);
}

void factory_flag_use(gentity_t* flag,gentity_t*,gentity_t* player){
    auto* root=FactoryFlagRoot(*flag);
    if(!root||!FactoryPlayerInSection(player,*root))return;
    auto* other=FactoryOtherFlag(*flag,*root);
    if(!other||flag->activator||other->activator)return;
    root->count=jar::factory::FlagTeam(flag->classname);
    gi.SendServerCommand(0,"cp \"%s base selected. Take the opposing flag and return here.\"",jar::factory::flags[root->count-1].color);
}

void factory_flag_touch(gentity_t* flag,gentity_t* player,trace_t*){
    auto* root=FactoryFlagRoot(*flag);
    if(!root||root->count<1||root->count>int(ARRAY_LEN(jar::factory::flags))||
       !FactoryPlayerInSection(player,*root)||flag->activator)return;
    auto* other=FactoryOtherFlag(*flag,*root);
    if(!other)return;
    if(root->count==jar::factory::FlagTeam(flag->classname)){
        if(other->activator!=player)return;
        FactoryReturnFlag(*other);
        if(flag->count<INT_MAX)++flag->count;
        gi.SendServerCommand(0,"cp \"Flag captured. %s captures: %d.\"",jar::factory::flags[root->count-1].color,flag->count);
        return;
    }
    flag->activator=player;
    flag->contents=0;
    flag->s.eFlags|=EF_NODRAW;
    flag->svFlags&=~SVF_PLAYER_USABLE;
    flag->e_ThinkFunc=thinkF_factory_flag_think;
    flag->nextthink=level.time+250;
    gi.linkentity(flag);
    gi.SendServerCommand(0,"cp \"Flag taken. Return to your base.\"");
}

void factory_flag_think(gentity_t* flag){
    const auto* root=FactoryFlagRoot(*flag);
    if(!root)return;
    if(FactoryPlayerInSection(flag->activator,*root)){
        flag->nextthink=level.time+250;
        return;
    }
    FactoryReturnFlag(*flag);
    gi.SendServerCommand(0,"cp \"Flag returned to its base.\"");
}

static bool BSPSectionLinkFlags(const gentity_t& root,const std::vector<gentity_t*>& members){
    gentity_t* flags[2]{};
    for(auto* child:members)if(FactoryFlagRoot(*child)==&root){
        const int team=jar::factory::FlagTeam(child->classname);
        if(flags[team-1])return false;
        flags[team-1]=child;
    }
    if(!flags[0]||!flags[1])return false;
    flags[0]->enemy=flags[1];flags[1]->enemy=flags[0];
    return true;
}

static bool BSPSectionInitializeFactory(gentity_t& root,const jar::bspAdmission::Report& report){
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(&root,members))return false;
    int panes=0,hazards=0;
    for(auto* child:members){
        if(jar::bspSection::SectionGlass(*child)){
            if(!BSPSectionBreakableFits(root,*child))return false;
            ++panes;
        }else if(child->bmodel&&child->s.modelindex-root.s.modelindex==jar::factory::hurtModel){
            if(!G_BSPSectionInitializeHurt(*child,report.hurtMins,report.hurtMaxs))return false;
            ++hazards;
        }
    }
    return panes==int(ARRAY_LEN(jar::factory::windows))&&hazards==1&&BSPSectionLinkFlags(root,members);
}
