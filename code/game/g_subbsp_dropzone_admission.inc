// Stock Imperial Drop Zone entity admission. GPL-2.0-only.
inline bool DropZoneBrush(const Entity& e,const std::string& cls,int index,Report& r){
    using namespace dropzone;
    for(const auto& mover:movers){
        if(mover.model!=index)continue;
        const bool platform=mover.Platform(),crusher=mover.damage==5000;
        if(cls!=(platform?"func_plat":"func_door")||strcmp(Value(e,"targetname"),mover.target)||
           Value(e,"speed")!=std::to_string(mover.speed)||strcmp(Value(e,"delay"),mover.delayField)||
           e.size()!=(platform?5u:8u+bool(mover.flags)+crusher)||
           Value(e,"angle")!=(platform?"":std::to_string(mover.angle))||
           strcmp(Value(e,"wait"),platform?"":"3")||strcmp(Value(e,"linear"),platform?"":"1")||
           Value(e,"spawnflags")!=(mover.flags?std::to_string(mover.flags):"")||
           strcmp(Value(e,"dmg"),crusher?"5000":""))
            return Fail(r,"Imperial Drop Zone mover differs from its stock fields");
        return true;
    }
    for(const auto& pad:pads){
        if(pad.model!=index)continue;
        const char* delay=pad.delay==100?"0.1":pad.delay==1000?"1":"2";
        return (cls=="trigger_multiple"&&e.size()==5&&
                !strcmp(Value(e,"target"),movers[pad.lift].target)&&
                !strcmp(Value(e,"delay"),delay)&&Value(e,"wait")==std::to_string(pad.wait))||
            Fail(r,"Imperial Drop Zone trigger differs from its stock timing or link");
    }
    return (cls=="trigger_hurt"&&std::find(std::begin(falls),std::end(falls),index)!=std::end(falls)&&
            e.size()==3&&!strcmp(Value(e,"dmg"),"-1"))||
        Fail(r,"Imperial Drop Zone requires its stock fall hazards");
}

inline bool DropZoneContent(const std::vector<Entity>& entities,Report& r){
    using namespace dropzone;
    if(r.models!=models||entities.size()!=dropzone::entities||strcmp(Value(entities.front(),"classname"),"worldspawn"))
        return Fail(r,"Imperial Drop Zone requires the stock arena");
    uint64_t brushes=0;
    unsigned flags=0,bases=0;
    int pickupCounts[ARRAY_LEN(pickups)]{},soundCounts[ARRAY_LEN(ambience)]{};
    int startsByTeam[2]{},neutral=0,views=0,viewTargets=0;
    for(size_t i=1;i<entities.size();++i){
        r.rejectedEntity=int(i)+1;
        const auto& e=entities[i];
        const std::string cls=Value(e,"classname"),model=Value(e,"model"),angle=Value(e,"angle");
        const char* origin=Value(e,"origin");
        if(!model.empty()&&model.front()=='*'){
            if(!Number(model.c_str()+1,1,models-1)||model!="*"+std::to_string(atoi(model.c_str()+1)))
                return Fail(r,"Imperial Drop Zone has an invalid inline model");
            const int index=atoi(model.c_str()+1);
            const uint64_t bit=uint64_t{1}<<index;
            if(brushes&bit)return Fail(r,"Imperial Drop Zone repeats an inline model");
            brushes|=bit;
            if(!DropZoneBrush(e,cls,index,r))return false;
            continue;
        }
        if(!Vector(origin))return Fail(r,"Imperial Drop Zone entity requires a local origin");
        if(const int team=factory::FlagTeam(cls.c_str())){
            const unsigned bit=1u<<(team-1);
            if((flags&bit)||e.size()!=3||angle!="270"||
               strcmp(origin,team==1?"-6464 0 988":"512 0 988"))
                return Fail(r,"Imperial Drop Zone flag has an invalid home");
            flags|=bit;
        }else if(cls=="misc_model_static"){
            const unsigned bit=!strcmp(origin,"-6464 0 960")?1u:!strcmp(origin,"512 0 960")?2u:0u;
            if(!bit||(bases&bit)||e.size()!=6||model!=factory::baseModel||angle!="270"||
               strcmp(Value(e,"_rs"),"1")||strcmp(Value(e,"_cs"),"1"))
                return Fail(r,"Imperial Drop Zone flag pedestal differs from stock");
            bases|=bit;
        }else if(const int team=factory::StartTeam(cls.c_str())){
            if(e.size()!=3||!Number(angle.c_str(),-360,360))
                return Fail(r,"Imperial Drop Zone team entry has an invalid heading");
            ++startsByTeam[team-1];
        }else if(cls=="info_player_start"){
            if(e.size()!=3||!Number(angle.c_str(),-360,360))
                return Fail(r,"Imperial Drop Zone entry has an invalid heading");
            ++neutral;
        }else if(cls=="info_player_intermission"){
            if(e.size()!=3||strcmp(Value(e,"target"),"v1"))
                return Fail(r,"Imperial Drop Zone view needs its stock target");
            ++views;
        }else if(cls=="info_notnull"){
            if(e.size()!=3||strcmp(Value(e,"targetname"),"v1"))
                return Fail(r,"Imperial Drop Zone view target differs from stock");
            ++viewTargets;
        }else if(cls=="info_null"){
            if(e.size()!=3||!*Value(e,"targetname"))
                return Fail(r,"Imperial Drop Zone marker requires its local name");
        }else if(cls=="target_speaker"){
            const auto* sound=std::find_if(std::begin(ambience),std::end(ambience),
                [&](const Ambient& entry){return !strcmp(Value(e,"noise"),entry.path);});
            if(sound==std::end(ambience)||e.size()!=4||strcmp(Value(e,"spawnflags"),"1"))
                return Fail(r,"Imperial Drop Zone speaker differs from its stock ambience");
            ++soundCounts[sound-ambience];
        }else{
            const auto* spec=std::find_if(std::begin(pickups),std::end(pickups),
                [&](const rift::Pickup& entry){return cls==entry.name;});
            if(spec==std::end(pickups)||e.size()!=2u+!angle.empty()||(!angle.empty()&&!Number(angle.c_str(),-360,360)))
                return Fail(r,"Portable Imperial Drop Zone adapter is pending for "+cls);
            if(!pickupCounts[spec-pickups]++)r.pickupClasses.push_back(cls);
        }
    }
    if(brushes!=(uint64_t{1}<<models)-2||flags!=3||bases!=3||
       startsByTeam[0]!=27||startsByTeam[1]!=17||neutral!=1||views!=1||viewTargets!=1)
        return Fail(r,"Imperial Drop Zone needs every stock brush, flag and entry group");
    for(size_t i=0;i<ARRAY_LEN(pickups);++i)if(pickupCounts[i]!=pickups[i].count)
        return Fail(r,"Imperial Drop Zone pickup counts differ from the stock arena");
    for(size_t i=0;i<ARRAY_LEN(ambience);++i){
        if(soundCounts[i]!=ambience[i].count)return Fail(r,"Imperial Drop Zone ambience counts differ from the stock arena");
        r.resources.push_back(ambience[i].path);
    }
    for(const auto& flag:factory::flags)r.resources.push_back(flag.model);
    r.resources.push_back(factory::baseModel);
    r.entryPoints=starts;r.allocationBudget=int(entities.size());r.rejectedEntity=0;
    return true;
}
