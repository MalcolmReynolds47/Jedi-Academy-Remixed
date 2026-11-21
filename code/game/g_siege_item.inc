// Single-player Siege carry, return and delivery callbacks. GPL-2.0-only.
#include "g_siege_item.h"
#include "g_subbsp_ownership.h"
#include "g_siege_korriban.h"
#include "g_bsp_section_hurt.h"

namespace {
constexpr int siegeCheckpointTag=0x4a534750;
constexpr int siegeCheckpointOff=1<<18;
gentity_t* carriedSiegeItem;
uint64_t carriedSiegeLifetime;

gentity_t* SiegeCarried(){
    const auto* item=carriedSiegeItem;
    return item&&G_EntityLifetime(item)==carriedSiegeLifetime&&jar::siege::Item(*item)&&
        item->activator==&g_entities[0]?carriedSiegeItem:nullptr;
}

void SiegeBind(gentity_t& item,gentity_t* player){
    item.activator=player;
    if(player){
        carriedSiegeItem=&item;
        carriedSiegeLifetime=G_EntityLifetime(&item);
    }else G_SiegeForget(&item);
}

bool SiegeUsablePlayer(const gentity_t* player){
    return player==&g_entities[0]&&player->inuse&&player->client&&player->health>0&&
           player->client->ps.pm_type!=PM_SPECTATOR;
}

void SiegeShow(gentity_t& item){
    item.s.eFlags&=~EF_NODRAW;
    const bool pickup=!(item.spawnflags&jar::siege::noPickup);
    item.contents=pickup?CONTENTS_TRIGGER:item.health>0?CONTENTS_SOLID:0;
    item.takedamage=item.health>0?qtrue:qfalse;
    item.e_TouchFunc=pickup?touchF_siege_item_touch:touchF_NULL;
    gi.linkentity(&item);
}

void SiegeHide(gentity_t& item){
    item.s.eFlags|=EF_NODRAW;
    item.contents=0;
    item.takedamage=qfalse;
    item.e_TouchFunc=touchF_NULL;
    item.e_ThinkFunc=thinkF_NULL;
    item.nextthink=0;
    gi.unlinkentity(&item);
}

void SiegeReturn(gentity_t& item,bool notify=true){
    const auto lifetime=G_EntityLifetime(&item);
    SiegeBind(item,nullptr);
    if(notify&&item.fxID){
        const vec3_t up={0,0,1};
        G_PlayEffect(item.fxID,item.currentOrigin,up);
        G_PlayEffect(item.fxID,item.pos1,up);
    }
    G_SetOrigin(&item,item.pos1);
    item.e_ThinkFunc=thinkF_NULL;
    item.nextthink=0;
    item.delay=0;
    SiegeShow(item);
    if(notify&&lifetime&&item.closetarget)G_UseTargets2(&item,&item,item.closetarget);
}

bool SiegeLavaAt(const gentity_t& item,const vec3_t point){
    const auto* room=G_SubBSPOwner(&item);
    if(!G_SiegeKorribanRoom(room))return false;
    std::vector<gentity_t*> members;
    if(!G_SubBSPMembers(room,members))return false;
    for(const auto* volume:members){
        if(volume->s.time2==BSPSectionLavaTag&&volume->linked&&G_BSPSectionBrushHurt(*volume)&&
           gi.EntityContact(point,point,volume))return true;
    }
    return false;
}

void SiegeDrop(gentity_t& item,const gentity_t& carrier){
    const auto lifetime=G_EntityLifetime(&item);
    vec3_t start,end;
    VectorCopy(carrier.client->ps.origin,start);
    const int carrierNum=carrier.s.number;
    SiegeBind(item,nullptr);
    if(item.opentarget)G_UseTargets2(&item,&item,item.opentarget);
    if(!lifetime||G_EntityLifetime(&item)!=lifetime)return;
    start[2]+=30;
    VectorCopy(start,end);
    end[2]-=160;
    trace_t ground{};
    gi.trace(&ground,start,item.mins,item.maxs,end,carrierNum,MASK_SOLID,(EG2_Collision)0,0);
    if(ground.startsolid||ground.allsolid||ground.fraction>=1||
       (gi.pointcontents(ground.endpos,item.s.number)&CONTENTS_NODROP)||SiegeLavaAt(item,ground.endpos)){
        SiegeReturn(item);
        return;
    }
    G_SetOrigin(&item,ground.endpos);
    item.delay=level.time+jar::siege::returnDelay;
    item.e_ThinkFunc=thinkF_siege_item_think;
    item.nextthink=item.delay;
    SiegeShow(item);
}
}

void G_SiegeForget(const gentity_t* item){
    if(carriedSiegeItem!=item)return;
    carriedSiegeItem=nullptr;
    carriedSiegeLifetime=0;
}

void G_SiegeRestore(){
    carriedSiegeItem=nullptr;
    carriedSiegeLifetime=0;
    for(int i=MAX_CLIENTS;i<globals.num_entities;++i){
        auto& item=g_entities[i];
        if(!jar::siege::Item(item)||!item.activator)continue;
        if(item.activator==&g_entities[0]&&!SiegeCarried()&&
           item.e_ThinkFunc==thinkF_siege_item_think&&!(item.spawnflags&jar::siege::delivered)){
            if(!G_EntityLifetime(&item))G_SubBSPAllocated(&item);
            SiegeBind(item,item.activator);
        }else SiegeReturn(item,false);
    }
}

bool G_SiegeForceLimited(const gentity_t& player){
    const auto* item=&player==&g_entities[0]?SiegeCarried():nullptr;
    return item&&(item->spawnflags&jar::siege::forceLimit);
}

bool G_SiegeKorribanRoom(const gentity_t* root){
    return root&&root->inuse&&root->classname&&!strcmp(root->classname,"misc_bsp")&&root->model&&
        !strcmp(root->model,jar::siegeKorriban::resource)&&G_SubBSPOwner(root)==root&&G_SubBSPNamespace(root)!=0;
}

namespace {
bool SiegeKorribanPlayerInside(const gentity_t& room){
    const auto& player=g_entities[0];
    if(!SiegeUsablePlayer(&player))return false;
    vec3_t axis[3],offset;
    AnglesToAxis(room.currentAngles,axis);
    VectorSubtract(player.currentOrigin,room.currentOrigin,offset);
    for(int i=0;i<3;++i){
        const float local=DotProduct(axis[i],offset);
        if(local<room.mins[i]||local>room.maxs[i])return false;
    }
    return true;
}
}

void G_SiegeKorribanEntered(gentity_t& room,gentity_t& player){
    using namespace jar::siegeKorriban;
    if(!G_SiegeKorribanRoom(&room)||!SiegeUsablePlayer(&player))return;
    const auto lifetime=G_EntityLifetime(&room);
    if(auto* item=SiegeCarried()){
        const auto* previous=G_SubBSPOwner(item);
        if(previous!=&room&&G_SiegeKorribanRoom(previous))SiegeReturn(*item);
    }
    if(G_EntityLifetime(&room)!=lifetime||room.count!=int(Round::Waiting))return;
    room.count=int(Round::Active);
    room.e_ThinkFunc=thinkF_siege_korriban_round_think;
    room.nextthink=level.time+roundDuration;
    gi.SendServerCommand(0,"cp \"Siege Korriban: Complete the objectives in 30 minutes.\"");
}

void siege_korriban_round_think(gentity_t* room){
    using namespace jar::siegeKorriban;
    if(!G_SiegeKorribanRoom(room)||room->count!=int(Round::Active))return;
    room->count=int(Round::Lost);
    room->e_ThinkFunc=thinkF_NULL;
    room->nextthink=0;
    G_UseTargets2(room,&g_entities[0],ending);
}

bool G_SiegeKorribanPrint(gentity_t& notice){
    const auto* room=G_SubBSPOwner(&notice);
    if(!G_SiegeKorribanRoom(room))return false;
    if(room->count==int(jar::siegeKorriban::Round::Active)&&SiegeKorribanPlayerInside(*room)&&
       !(notice.spawnflags&2)&&notice.message&&level.time>=notice.aimDebounceTime){
        notice.aimDebounceTime=level.time+int(notice.wait);
        gi.SendServerCommand(0,"cp \"%s\"",notice.message);
    }
    return true;
}

bool G_SiegeKorribanHeavyHit(const gentity_t& brush,int mod){
    if(!brush.classname||strcmp(brush.classname,"func_breakable")||
       !G_SiegeKorribanRoom(G_SubBSPOwner(&brush)))return false;
    // Map the stock Siege heavy-weapon rule to the single-player MOD values.
    switch(mod){
    case MOD_SABER:
    case MOD_REPEATER_ALT:
    case MOD_FLECHETTE_ALT:
    case MOD_ROCKET:
    case MOD_ROCKET_ALT:
    case MOD_THERMAL:
    case MOD_THERMAL_ALT:
    case MOD_DETPACK:
    case MOD_LASERTRIP:
    case MOD_LASERTRIP_ALT:
    case MOD_CONC:
    case MOD_CONC_ALT:
    case MOD_EMPLACED:
    case MOD_SUICIDE:
    case MOD_FALLING:
    case MOD_CRUSH:
    case MOD_TRIGGER_HURT:
        return true;
    default:
        return false;
    }
}

bool G_SiegeKorribanStart(gentity_t& point){
    if(!point.classname||!G_SiegeKorribanRoom(G_SubBSPOwner(&point)))return false;
    const int team=!strcmp(point.classname,"info_player_siegeteam1")?1:
        !strcmp(point.classname,"info_player_siegeteam2")?2:0;
    if(!team)return false;
    int off;
    G_SpawnInt("startoff","0",&off);
    if(off)point.spawnflags|=siegeCheckpointOff;
    point.count=team;
    point.s.time2=siegeCheckpointTag;
    point.e_UseFunc=useF_siege_checkpoint_use;
    point.svFlags|=SVF_NOCLIENT;
    G_SetOrigin(&point,point.s.origin);
    return true;
}

bool G_SiegeKorribanEntry(const gentity_t& point){
    return point.inuse&&point.s.time2==siegeCheckpointTag&&point.count==1&&
        !(point.spawnflags&siegeCheckpointOff)&&G_SiegeKorribanRoom(G_SubBSPOwner(&point));
}

void siege_checkpoint_use(gentity_t* point,gentity_t*,gentity_t*){
    if(point&&point->s.time2==siegeCheckpointTag&&G_SiegeKorribanRoom(G_SubBSPOwner(point)))
        point->spawnflags^=siegeCheckpointOff;
}

void siege_item_touch(gentity_t* item,gentity_t* player,trace_t*){
    using namespace jar::siege;
    if(!Item(*item)||!SiegeUsablePlayer(player)||SiegeCarried()||item->activator||
       (item->spawnflags&(delivered|noPickup))||(item->s.eFlags&EF_NODRAW))return;
    const bool fire=item->target2&&(!(item->spawnflags&pickupOnce)||!(item->spawnflags&picked));
    item->spawnflags|=picked;
    SiegeBind(*item,player);
    item->contents=0;
    item->takedamage=qfalse;
    item->e_TouchFunc=touchF_NULL;
    item->e_ThinkFunc=thinkF_siege_item_think;
    item->nextthink=level.time+FRAMETIME;
    item->delay=0;
    if(item->noise_index)G_Sound(player,item->noise_index);
    gi.SendServerCommand(0,"cp \"Siege item picked up.\"");
    if(fire)G_UseTargets2(item,player,item->target2);
}

void siege_item_think(gentity_t* item){
    if(!jar::siege::Item(*item)||item->spawnflags&jar::siege::delivered)return;
    if(!item->activator){
        if(item->delay&&item->delay<=level.time)SiegeReturn(*item);
        return;
    }
    if(item->activator!=&g_entities[0]||!item->activator->inuse||!item->activator->client){
        SiegeReturn(*item);
        return;
    }
    const auto& player=*item->activator;
    if(player.health<=0){
        SiegeDrop(*item,player);
        return;
    }
    vec3_t source,back;
    const vec3_t angles={0,player.client->ps.viewangles[YAW],0};
    AngleVectors(angles,back,nullptr,nullptr);
    VectorMA(player.client->ps.origin,-24,back,source);
    source[2]+=24;
    G_SetOrigin(item,source);
    gi.linkentity(item);
    item->nextthink=level.time+FRAMETIME;
}

bool G_SiegeTryDeliver(gentity_t& goal,gentity_t* player){
    auto* item=SiegeCarried();
    if(!item||!SiegeUsablePlayer(player)||!goal.targetname||!item->target||
       (goal.noDamageTeam&&player->client->playerTeam!=goal.noDamageTeam)||
       Q_stricmp(goal.targetname,item->target)||G_SubBSPOwner(&goal)!=G_SubBSPOwner(item))return false;
    item->spawnflags|=jar::siege::delivered;
    SiegeBind(*item,nullptr);
    SiegeHide(*item);
    if(item->target3)G_UseTargets2(item,item,item->target3);
    return true;
}

void siege_item_use(gentity_t* item,gentity_t*,gentity_t*){
    if(!jar::siege::Item(*item)||(item->spawnflags&jar::siege::delivered)||item->activator)return;
    item->spawnflags&=~jar::siege::offRadar;
    if(item->s.eFlags&EF_NODRAW)SiegeShow(*item);
}

void siege_item_die(gentity_t* item,gentity_t*,gentity_t* attacker,int,int,int,int){
    if(!jar::siege::Item(*item)||(item->spawnflags&jar::siege::delivered))return;
    item->spawnflags|=jar::siege::delivered;
    SiegeBind(*item,nullptr);
    SiegeHide(*item);
    if(item->target4)G_UseTargets2(item,attacker,item->target4);
}

void SP_misc_siege_item(gentity_t* item){
    using namespace jar::siege;
    const auto* model=item->model;
    if(!model||Q_stricmp(COM_GetExtension(model),"md3")||gi.FS_ReadFile(model,nullptr)<=0){G_FreeEntity(item);return;}
    int canPickup,limit,once;
    G_SpawnInt("canpickup","1",&canPickup);
    G_SpawnInt("forcelimit","0",&limit);
    G_SpawnInt("pickuponlyonce","1",&once);
    if(!canPickup)item->spawnflags|=noPickup;
    if(limit)item->spawnflags|=forceLimit;
    if(once)item->spawnflags|=pickupOnce;
    G_SpawnVector("mins","-16 -16 -24",item->mins);
    G_SpawnVector("maxs","16 16 32",item->maxs);
    for(int i=0;i<3;++i){
        if(!std::isfinite(item->mins[i])||!std::isfinite(item->maxs[i])||item->mins[i]>=item->maxs[i]||
           std::fabs(item->mins[i])>256||std::fabs(item->maxs[i])>256){G_FreeEntity(item);return;}
    }
    char* resource;
    G_SpawnString("respawnfx","",&resource);
    if(*resource)item->fxID=G_EffectIndex(resource);
    G_SpawnString("pickupsound","",&resource);
    if(*resource)item->noise_index=G_SoundIndex(resource);
    G_SpawnString("goaltarget","",&resource);
    if(*resource&&!item->target)item->target=G_NewString(resource);
    G_SpawnString("target5","",&resource);
    if(*resource&&!item->closetarget)item->closetarget=G_NewString(resource);
    G_SpawnString("target6","",&resource);
    if(*resource&&!item->opentarget)item->opentarget=G_NewString(resource);
    item->s.time2=itemTag;
    item->s.eType=ET_GENERAL;
    item->s.modelindex=G_ModelIndex(model);
    item->svFlags|=SVF_BROADCAST|SVF_PLAYER_USABLE;
    item->e_UseFunc=useF_siege_item_use;
    if(item->health>0)item->e_DieFunc=dieF_siege_item_die;
    VectorCopy(item->s.origin,item->pos1);
    G_SetOrigin(item,item->s.origin);
    G_SetAngles(item,item->s.angles);
    if(item->targetname&&!(item->spawnflags&(offRadar|immediate)))SiegeHide(*item);
    else SiegeShow(*item);
}

void siege_objective_use(gentity_t* goal,gentity_t*,gentity_t* player){
    if(goal->setTime)return;
    auto* room=G_SubBSPOwner(goal);
    const bool siegeRoom=G_SiegeKorribanRoom(room);
    if(siegeRoom&&(room->count==int(jar::siegeKorriban::Round::Lost)||
                   room->count==int(jar::siegeKorriban::Round::Won)))return;
    if(goal->spawnflags&jar::siege::offRadar){
        goal->spawnflags&=~jar::siege::offRadar;
        return;
    }
    const auto lifetime=G_EntityLifetime(goal);
    if((goal->spawnflags&jar::siege::requiresItem)&&!G_SiegeTryDeliver(*goal,player))return;
    if(!lifetime||G_EntityLifetime(goal)!=lifetime)return;
    goal->setTime=1;
    if(siegeRoom&&goal->targetname){
        for(const auto& objective:jar::siegeKorriban::objectives){
            if(goal->count!=objective.number||strcmp(goal->targetname,objective.name))continue;
            if(SiegeKorribanPlayerInside(*room))gi.SendServerCommand(0,"cp \"%s\"",objective.announcement);
            if(*objective.output)G_UseTargets2(goal,player,objective.output);
            if(G_EntityLifetime(goal)==lifetime)G_UseTargets(goal,player);
            return;
        }
    }
    gi.SendServerCommand(0,"cp \"Siege objective completed.\"");
    G_UseTargets(goal,player);
}

void siege_end_use(gentity_t* goal,gentity_t*,gentity_t* player){
    if(goal->setTime)return;
    using namespace jar::siegeKorriban;
    auto* room=G_SubBSPOwner(goal);
    if(G_SiegeKorribanRoom(room)){
        const bool expired=room->count==int(Round::Lost);
        if(!expired&&room->count!=int(Round::Active))return;
        room->count=int(expired?Round::Lost:Round::Won);
        room->nextthink=0;
        room->e_ThinkFunc=thinkF_NULL;
        if(SiegeKorribanPlayerInside(*room)){
            G_Sound(&g_entities[0],G_SoundIndex(expired?lossSound:winSound));
            gi.SendServerCommand(0,"cp \"%s\"",expired?"Siege Korriban: Time expired.":"Siege Korriban: All objectives completed.");
        }
        goal->setTime=1;
        G_UseTargets(goal,player);
        return;
    }
    goal->setTime=1;
    gi.SendServerCommand(0,"cp \"Siege round completed.\"");
    G_UseTargets(goal,player);
}

void SP_info_siege_objective(gentity_t* goal){
    G_SpawnInt("objective","0",&goal->count);
    goal->e_UseFunc=useF_siege_objective_use;
    goal->svFlags|=SVF_PLAYER_USABLE;
    if(goal->model&&*goal->model){
        goal->s.eType=ET_GENERAL;
        goal->s.modelindex=G_ModelIndex(goal->model);
        goal->contents=CONTENTS_ITEM;
    }
    G_SetOrigin(goal,goal->s.origin);
    gi.linkentity(goal);
}

void SP_target_siege_end(gentity_t* goal){
    SP_info_siege_objective(goal);
    goal->e_UseFunc=useF_siege_end_use;
}
