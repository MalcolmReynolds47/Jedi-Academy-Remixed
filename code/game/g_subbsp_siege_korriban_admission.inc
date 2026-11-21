// Admission for the stock Siege Korriban entity graph. GPL-2.0-only.
inline bool SiegeKorribanContent(const std::vector<Entity>& entities,Report& r){
    using namespace siegeKorriban;
    if(r.models!=models||entities.size()!=siegeKorriban::entities||
       strcmp(Value(entities.front(),"classname"),"worldspawn")||
       strcmp(Value(entities.front(),"message"),"Korriban"))
        return Fail(r,"Siege Korriban requires every stock entity and inline model");
    struct ClassCount {const char* name;int count;};
    constexpr ClassCount classes[]={
        {"misc_model_static",sceneryCount},{"fx_runner",torchCount+ARRAY_LEN(namedEffects)},{"info_player_siegeteam2",48},
        {"info_player_siegeteam1",46},{"target_speaker",speakerCount},{"func_breakable",37},
        {"target_print",29},{"target_relay",26},{"waypoint",18},{"func_door",14},
        {"target_location",10},{"misc_siege_item",9},{"trigger_hurt",8},{"trigger_multiple",8},
        {"info_siege_objective",6},{"target_delay",4},{"info_notnull",4},{"func_usable",2},
        {"target_counter",2},{"NPC_Monster_Rancor",1},{"info_player_start",1},
        {"info_null",1},{"target_siege_end",1},
    };
    int classCounts[ARRAY_LEN(classes)]{},groupCounts[2][ARRAY_LEN(groupNames)]{};
    const auto suffix=[](const std::string& path,const char* end){
        const size_t len=strlen(end);
        return path.size()>=len&&path.compare(path.size()-len,len,end)==0;
    };
    const auto number=[](const Entity& e,const char* key,int expected,int fallback){
        const char* value=Value(e,key);
        return *value?value==std::to_string(expected):expected==fallback;
    };
    std::vector<bool> brushes(models);
    std::vector<std::string> names;
    unsigned objectiveMask=0,deliveryMask=0,itemsMask=0,waypointMask=0;
    unsigned namedEffectMask=0;
    int flameCounts[ARRAY_LEN(torchEffects)]{},offsetCounts[3]{},scaleCounts[2]{};
    const Entity* coffin=nullptr;
    const Entity* counter=nullptr;
    for(size_t n=0;n<entities.size();++n){
        const auto& e=entities[n];
        r.rejectedEntity=int(n)+1;
        for(const auto& field:e)
            if(field.key.size()>=6&&field.key.compare(field.key.size()-6,6,"script")==0&&!field.value.empty())
                return Fail(r,"Script isolation is pending: "+field.key);
        if(!n)continue;
        const std::string cls=Value(e,"classname"),model=Value(e,"model"),name=Value(e,"targetname");
        const auto* kind=std::find_if(std::begin(classes),std::end(classes),[&](const ClassCount& c){return cls==c.name;});
        if(kind==std::end(classes))return Fail(r,"Portable Siege Korriban adapter is pending for "+cls);
        ++classCounts[kind-classes];
        if(!name.empty())names.push_back(name);
        const bool brush=OneOf(cls,{"func_breakable","func_door","func_usable","trigger_hurt","trigger_multiple"});
        int index=0;
        if(brush){
            if(model.size()<2||model[0]!='*'||!Number(model.c_str()+1,1,models-1)||
               model!="*"+std::to_string(atoi(model.c_str()+1)))
                return Fail(r,"Siege Korriban has an invalid inline model");
            index=atoi(model.c_str()+1);
            if(brushes[index])return Fail(r,"Siege Korriban repeats an inline model");
            brushes[index]=true;
        }else if(!Vector(Value(e,"origin")))return Fail(r,"Siege Korriban point requires a local origin");

        if(cls=="info_player_siegeteam1"||cls=="info_player_siegeteam2"){
            const int team=cls=="info_player_siegeteam1"?0:1;
            const auto group=std::find(std::begin(groupNames),std::end(groupNames),name);
            if(group==std::end(groupNames)||strcmp(Value(e,"startoff"),group==std::begin(groupNames)?"":"1"))
                return Fail(r,"Siege Korriban start group differs from its stock phase");
            ++groupCounts[team][group-groupNames];
        }else if(cls==rancorClass){
            if(strcmp(Value(e,"origin"),rancorOrigin)||strcmp(Value(e,"angle"),"360")||
               strcmp(Value(e,"teamowner"),"2")||strcmp(Value(e,"showhealth"),"1")||
               !name.empty()||*Value(e,"NPC_type")||*Value(e,"spawnflags"))
                return Fail(r,"Siege Korriban Rancor differs from its stock setup");
            r.resources.push_back(rancorDefinition);
            r.resources.push_back(rancorModel);
        }else if(cls=="waypoint"){
            const auto* waypoint=std::find_if(std::begin(waypoints),std::end(waypoints),
                [&](const Waypoint& spec){return name==spec.name;});
            if(waypoint==std::end(waypoints)||strcmp(Value(e,"target"),waypoint->target)||
               strcmp(Value(e,"origin"),waypoint->origin)||*Value(e,"target2")||*Value(e,"spawnflags"))
                return Fail(r,"Siege Korriban waypoint differs from its stock route");
            const unsigned bit=1u<<(waypoint-waypoints);
            if(waypointMask&bit)return Fail(r,"Siege Korriban repeats a waypoint");
            waypointMask|=bit;
        }else if(cls=="info_siege_objective"){
            const auto* objective=std::find_if(std::begin(objectives),std::end(objectives),
                [&](const Objective& row){return name==row.name;});
            if(objective==std::end(objectives)||Value(e,"objective")!=std::to_string(objective->number)||
               strcmp(Value(e,"side"),"1")||strcmp(Value(e,"spawnflags"),objective->startsOff?"8":"")||
               strcmp(Value(e,"target"),objective->number>=2&&objective->number<=4?crystalCounter:""))
                return Fail(r,"Siege Korriban objective differs from its stock arming or target");
            const unsigned bit=1u<<(objective-objectives);
            if(objectiveMask&bit)return Fail(r,"Siege Korriban repeats an objective");
            objectiveMask|=bit;
        }else if(cls=="func_door"){
            const auto* door=std::find_if(std::begin(doors),std::end(doors),[&](const Door& spec){return spec.model==index;});
            if(door==std::end(doors)||!number(e,"spawnflags",door->flags,0)||!number(e,"angle",door->angle,0)||
               !number(e,"lip",door->lip,8)||!number(e,"speed",door->speed,400)||!number(e,"wait",door->wait,2)||
               !number(e,"delay",door->delay,0)||!number(e,"dmg",door->damage,2)||name!=door->name||
               strcmp(Value(e,"opentarget"),door->opens)||strcmp(Value(e,"soundset"),door->sound?"stone_door":"")||
               *Value(e,"team")||*Value(e,"health")||*Value(e,"target")||*Value(e,"closetarget"))
                return Fail(r,"Siege Korriban door differs from its stock motion, sounds or targets");
        }else if(cls=="trigger_multiple"&&*Value(e,"siegetrig")){
            const auto* delivery=std::find_if(std::begin(deliveries),std::end(deliveries),
                [&](const Delivery& row){return index==row.model;});
            if(delivery==std::end(deliveries)||name!=delivery->trigger||strcmp(Value(e,"target"),delivery->target)||
               strcmp(Value(e,"siegetrig"),"1")||strcmp(Value(e,"team"),"1")||strcmp(Value(e,"wait"),"-1"))
                return Fail(r,"Siege Korriban delivery gate differs from its stock item or team");
            deliveryMask|=1u<<(delivery-deliveries);
        }else if(cls=="trigger_multiple"){
            const auto* pad=std::find_if(std::begin(pressurePads),std::end(pressurePads),
                [&](const Pressure& spec){return spec.model==index;});
            if(pad==std::end(pressurePads)||strcmp(Value(e,"target"),pad->target)||
               strcmp(Value(e,"wait"),"2")||*Value(e,"delay")||*Value(e,"team")||*Value(e,"spawnflags"))
                return Fail(r,"Siege Korriban pressure volume differs from its stock crusher or wait");
        }else if(cls=="misc_siege_item"||cls=="misc_model_static"){
            if(!Resource(model)||!suffix(model,".md3"))return Fail(r,"Siege Korriban requires a stock MD3");
            r.resources.push_back(model);
            if(cls=="misc_model_static"){
                const std::string scale=Value(e,"modelscale"),offset=Value(e,"zoffset");
                if(*Value(e,"modelscale_vec")||(!scale.empty()&&!OneOf(scale,{"1.5","2"}))||
                   (!offset.empty()&&!OneOf(offset,{"32","64","128"})))
                    return Fail(r,"Siege Korriban scenery differs from its stock scale or culling offset");
                if(!scale.empty())++scaleCounts[scale=="2"];
                if(!offset.empty())++offsetCounts[offset=="32"?0:offset=="64"?1:2];
            }else{
                const std::string goal=Value(e,"goaltarget");
                if(!goal.empty()){
                    const auto* delivery=std::find_if(std::begin(deliveries),std::end(deliveries),
                        [&](const Delivery& row){return goal==row.trigger;});
                    if(delivery==std::end(deliveries)||model!=delivery->itemModel||
                       strcmp(Value(e,"target3"),delivery->itemTarget)||strcmp(Value(e,"teamnotouch"),"2"))
                        return Fail(r,"Siege Korriban carried item differs from its matching objective");
                    const unsigned bit=1u<<(delivery-deliveries);
                    if(itemsMask&bit)return Fail(r,"Siege Korriban repeats a carried objective item");
                    itemsMask|=bit;
                }
                const std::string respawn=Value(e,"respawnfx");
                if(!respawn.empty()){
                    if(!Resource(respawn)||!suffix(respawn,".efx"))return Fail(r,"Siege Korriban has an invalid item return effect");
                    r.resources.push_back("effects/"+Lower(respawn));
                }
                if(name=="ragnos_coffin")coffin=&e;
            }
        }else if(cls=="target_speaker"){
            if(strcmp(Value(e,"soundset"),lavaSoundSet)||strcmp(Value(e,"spawnflags"),"1")||
               *Value(e,"noise")||!name.empty()||*Value(e,"target")||*Value(e,"wait")||*Value(e,"random"))
                return Fail(r,"Siege Korriban ambience differs from its stock local loop");
        }else if(cls=="fx_runner"){
            std::string effect=Lower(Value(e,"fxfile"));
            if(!Resource(effect))return Fail(r,"Siege Korriban requires a local effect asset");
            if(effect.rfind("effects/",0)==0)effect.erase(0,8);
            if(suffix(effect,".efx"))effect.resize(effect.size()-4);
            const std::string sound=Value(e,"soundset"),target=Value(e,"target");
            if(*Value(e,"target2")||*Value(e,"delay")||*Value(e,"random")||
               *Value(e,"splashdamage")||*Value(e,"splashradius"))
                return Fail(r,"Siege Korriban effect differs from its stock timing or damage");
            if(sound==torchSoundSet){
                const auto* flame=std::find_if(std::begin(torchEffects),std::end(torchEffects),
                    [&](const TorchEffect& spec){return effect==spec.file;});
                if(flame==std::end(torchEffects)||!number(e,"spawnflags",4,0)||!name.empty()||!target.empty())
                    return Fail(r,"Siege Korriban torch differs from its stock effect or damage flag");
                ++flameCounts[flame-torchEffects];
            }else{
                const auto* named=std::find_if(std::begin(namedEffects),std::end(namedEffects),
                    [&](const NamedEffect& spec){return name==spec.name;});
                if(named==std::end(namedEffects)||effect!=named->file||target!=named->target||!sound.empty()||
                   !number(e,"spawnflags",named->flags,0))
                    return Fail(r,"Siege Korriban named effect differs from its stock orientation target or state");
                const unsigned bit=1u<<(named-namedEffects);
                if(namedEffectMask&bit)return Fail(r,"Siege Korriban repeats a named effect");
                namedEffectMask|=bit;
            }
            r.resources.push_back("effects/"+effect+".efx");
        }else if(cls=="target_counter"&&name==crystalCounter)counter=&e;
    }
    if(std::find(brushes.begin()+1,brushes.end(),false)!=brushes.end()||
       objectiveMask!=(1u<<ARRAY_LEN(objectives))-1||deliveryMask!=(1u<<ARRAY_LEN(deliveries))-1||
       itemsMask!=(1u<<ARRAY_LEN(deliveries))-1||waypointMask!=(1u<<ARRAY_LEN(waypoints))-1)
        return Fail(r,"Siege Korriban needs every stock brush, objective, delivery and waypoint");
    for(size_t i=0;i<ARRAY_LEN(classes);++i)
        if(classCounts[i]!=classes[i].count)return Fail(r,"Siege Korriban class count differs: "+std::string(classes[i].name));
    if(!std::equal(std::begin(team1GroupCounts),std::end(team1GroupCounts),groupCounts[0])||
       !std::equal(std::begin(team2GroupCounts),std::end(team2GroupCounts),groupCounts[1]))
        return Fail(r,"Siege Korriban start counts differ from the stock phases");
    if(namedEffectMask!=(1u<<ARRAY_LEN(namedEffects))-1||offsetCounts[0]!=1||offsetCounts[1]!=3||offsetCounts[2]!=7||
       scaleCounts[0]!=96||scaleCounts[1]!=31)
        return Fail(r,"Siege Korriban scenery or named effect coverage differs from stock");
    for(size_t i=0;i<ARRAY_LEN(torchEffects);++i)
        if(flameCounts[i]!=torchEffects[i].count)return Fail(r,"Siege Korriban torch colors differ from stock");
    if(!counter||strcmp(Value(*counter,"count"),"3")||strcmp(Value(*counter,"target"),"cyrstalsinplace_relay")||
       !coffin||strcmp(Value(*coffin,"model"),coffinModel)||strcmp(Value(*coffin,"health"),"1800")||
       strcmp(Value(*coffin,"target4"),"jedi_obj_6"))
        return Fail(r,"Siege Korriban requires its three-crystal counter and final coffin");
    for(size_t n=1;n<entities.size();++n){
        r.rejectedEntity=int(n)+1;
        for(const auto& key:{"target","target2","target3","target4","target5","target6","goaltarget","opentarget","closetarget","paintarget"}){
            const char* target=Value(entities[n],key);
            if(*target&&std::find(names.begin(),names.end(),target)==names.end())
                return Fail(r,"Siege Korriban target leaves the room: "+std::string(target));
        }
    }
    for(const char* file:doorSounds)r.resources.push_back(file);
    r.resources.push_back(lavaLoop);
    r.resources.push_back(torchLoop);
    r.resources.push_back(winSound);
    r.resources.push_back(lossSound);
    r.resources.push_back(definition);
    std::sort(r.resources.begin(),r.resources.end());
    r.resources.erase(std::unique(r.resources.begin(),r.resources.end()),r.resources.end());
    r.entryPoints=starts;
    r.allocationBudget=int(entities.size());
    r.rejectedEntity=0;
    return true;
}
