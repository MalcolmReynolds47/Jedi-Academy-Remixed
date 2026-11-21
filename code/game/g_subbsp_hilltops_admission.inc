// Stock Yavin Hilltops entity admission. GPL-2.0-only.
inline bool HilltopsContent(const std::vector<Entity>& entities,Report& r){
    using namespace hilltops;
    if(r.models!=models||entities.size()!=hilltops::entities||strcmp(Value(entities.front(),"classname"),"worldspawn"))
        return Fail(r,"Yavin Hilltops requires the stock arena");
    unsigned mask=0,flags=0,bases=0;
    int pickupCounts[ARRAY_LEN(pickups)]{},soundCounts[4]{},startsByTeam[2]{},neutral=0,views=0;
    for(size_t i=1;i<entities.size();++i){
        r.rejectedEntity=int(i)+1;
        const auto& e=entities[i];
        const std::string cls=Value(e,"classname"),model=Value(e,"model"),angle=Value(e,"angle");
        const char* origin=Value(e,"origin");
        if(!model.empty()&&model.front()=='*'){
            if(!Number(model.c_str()+1,1,models-1)||model!="*"+std::to_string(atoi(model.c_str()+1)))
                return Fail(r,"Yavin Hilltops has an invalid inline model");
            const int index=atoi(model.c_str()+1);
            if(mask&(1u<<index))return Fail(r,"Yavin Hilltops repeats an inline model");
            mask|=1u<<index;
            if(!CtfBrush(e,cls,index,lifts,pads,falls,"",r))return false;
            continue;
        }
        if(!Vector(origin))return Fail(r,"Yavin Hilltops entity requires a local origin");
        if(const int team=factory::FlagTeam(cls.c_str())){
            const unsigned bit=1u<<(team-1);
            if((flags&bit)||e.size()!=3||angle!=(team==1?"225":"45")||
               strcmp(origin,team==1?"1712 880 685":"-3760 -4976 685"))return Fail(r,"Yavin Hilltops flag has an invalid home");
            flags|=bit;
        }else if(cls=="misc_model_static"){
            const unsigned bit=!strcmp(origin,"1712 880 656")?1u:!strcmp(origin,"-3760 -4976 656")?2u:0u;
            if(!bit||(bases&bit)||e.size()!=6||model!=factory::baseModel||angle!=(bit==1?"225":"45")||
               strcmp(Value(e,"_rs"),"1")||strcmp(Value(e,"_cs"),"1"))return Fail(r,"Yavin Hilltops flag pedestal differs from stock");
            bases|=bit;
        }else if(const int team=factory::StartTeam(cls.c_str())){
            if(e.size()!=2u+!angle.empty()||(!angle.empty()&&!Number(angle.c_str(),-360,360)))return Fail(r,"Yavin Hilltops team entry has an invalid heading");
            ++startsByTeam[team-1];
        }else if(cls=="info_player_start"){
            if(e.size()!=3||!Number(angle.c_str(),-360,360))return Fail(r,"Yavin Hilltops entry has an invalid heading");
            ++neutral;
        }else if(cls=="info_player_intermission"){
            if(e.size()!=3||strcmp(Value(e,"target"),"v1"))return Fail(r,"Yavin Hilltops view needs its stock target");
            ++views;
        }else if(cls=="info_null"||cls=="info_notnull"){
            if(e.size()!=3||!*Value(e,"targetname"))return Fail(r,"Yavin Hilltops marker requires a local name");
        }else if(cls=="target_speaker"){
            const std::string noise=Value(e,"noise"),set=Value(e,"soundset");
            int kind=-1;
            if(e.size()==4&&set.empty()&&noise==exterior)kind=0;
            if(e.size()==4&&set.empty()&&noise==wind)kind=1;
            if(e.size()==4&&set==exteriorSet&&noise.empty())kind=2;
            if(e.size()==5&&set=="medium_platform"&&noise==wind)kind=3;
            if(kind<0||strcmp(Value(e,"spawnflags"),"1"))return Fail(r,"Yavin Hilltops speaker differs from its stock ambience");
            ++soundCounts[kind];
        }else{
            const auto* spec=std::find_if(std::begin(pickups),std::end(pickups),[&](const rift::Pickup& p){return cls==p.name;});
            if(spec==std::end(pickups)||e.size()!=2u+!angle.empty()||(!angle.empty()&&!Number(angle.c_str(),-360,360)))
                return Fail(r,"Portable Yavin Hilltops adapter is pending for "+cls);
            if(!pickupCounts[spec-pickups]++)r.pickupClasses.push_back(cls);
        }
    }
    if(mask!=(1u<<models)-2||flags!=3||bases!=3||startsByTeam[0]!=27||startsByTeam[1]!=27||neutral!=1||views!=1)
        return Fail(r,"Yavin Hilltops needs every stock brush, flag and entry group");
    for(size_t i=0;i<ARRAY_LEN(pickups);++i)if(pickupCounts[i]!=pickups[i].count)
        return Fail(r,"Yavin Hilltops pickup counts differ from the stock arena");
    const int expected[]={18,8,8,2};
    if(!std::equal(std::begin(expected),std::end(expected),soundCounts))return Fail(r,"Yavin Hilltops ambience counts differ from the stock arena");
    for(const auto& flag:factory::flags)r.resources.push_back(flag.model);
    for(const char* file:{factory::baseModel,exterior,wind})r.resources.push_back(file);
    for(const char* file:birdCalls)r.resources.push_back(file);
    for(const char* sound:shaft::liftSounds)r.resources.push_back(sound);
    r.entryPoints=starts;r.allocationBudget=int(entities.size());r.rejectedEntity=0;
    return true;
}
