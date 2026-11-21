// Stock Coruscant Streets entity admission. GPL-2.0-only.
inline bool CoruscantContent(const std::vector<Entity>& entities,Report& r){
    using namespace coruscant;
    if(r.models!=models||entities.size()!=coruscant::entities||strcmp(Value(entities.front(),"classname"),"worldspawn")||
       strcmp(Value(entities.front(),"fog"),fog))return Fail(r,"Coruscant requires the stock arena and fog");
    unsigned mask=0,flagMask=0;
    int pickupCounts[ARRAY_LEN(pickups)]{},soundCounts[ARRAY_LEN(sounds)]{};
    int startsByTeam[2]{},dm=0,views=0;
    for(size_t i=1;i<entities.size();++i){
        r.rejectedEntity=int(i)+1;
        const auto& e=entities[i];
        const std::string cls=Value(e,"classname"),model=Value(e,"model"),angle=Value(e,"angle");
        const char* origin=Value(e,"origin");
        if(!model.empty()){
            if(model.front()!='*'||!Number(model.c_str()+1,1,models-1)||model!="*"+std::to_string(atoi(model.c_str()+1)))
                return Fail(r,"Coruscant has an invalid inline model");
            const int index=atoi(model.c_str()+1);
            if(mask&(1u<<index))return Fail(r,"Coruscant repeats an inline model");
            mask|=1u<<index;
            if(!CtfBrush(e,cls,index,lifts,pads,falls,"12",r))return false;
            continue;
        }
        if(!Vector(origin))return Fail(r,"Coruscant entity requires a local origin");
        if(const int team=factory::FlagTeam(cls.c_str())){
            const unsigned bit=1u<<(team-1);
            if((flagMask&bit)||e.size()!=3||angle!=(team==1?"180":"270")||
               strcmp(origin,team==1?"4672 -2560 -3":"-576 2688 -3"))return Fail(r,"Coruscant flag has an invalid home");
            flagMask|=bit;
        }else if(const int team=factory::StartTeam(cls.c_str())){
            if(e.size()!=3||!Number(angle.c_str(),-360,360))return Fail(r,"Coruscant team entry has an invalid heading");
            ++startsByTeam[team-1];
        }else if(cls=="info_player_deathmatch"){
            if(e.size()!=3||!Number(angle.c_str(),-360,360))return Fail(r,"Coruscant deathmatch entry has an invalid heading");
            ++dm;
        }else if(cls=="info_player_intermission"){
            if(e.size()!=4||angle!="90"||!OneOf(Value(e,"target"),{"v1","t201"}))return Fail(r,"Coruscant view needs its stock target");
            ++views;
        }else if(cls=="info_null"||cls=="info_notnull"){
            if(e.size()!=3||!*Value(e,"targetname"))return Fail(r,"Coruscant marker requires a local name");
        }else if(cls=="target_speaker"){
            std::string noise=Value(e,"noise");std::replace(noise.begin(),noise.end(),'\\','/');
            const auto* sound=std::find_if(std::begin(sounds),std::end(sounds),[&](const Sound& row){return noise==row.file;});
            const bool traffic=noise==flyby;
            if(sound==std::end(sounds)||e.size()!=(traffic?5u:4u)||
               (traffic?(strcmp(Value(e,"wait"),"8")||strcmp(Value(e,"random"),"10")):strcmp(Value(e,"spawnflags"),"1")))
                return Fail(r,"Coruscant speaker differs from its stock sound or timing");
            if(!soundCounts[sound-sounds]++)r.resources.push_back(noise);
        }else{
            const auto* spec=std::find_if(std::begin(pickups),std::end(pickups),[&](const rift::Pickup& p){return cls==p.name;});
            const bool rocket=cls=="weapon_rocket_launcher";
            if(spec==std::end(pickups)||e.size()!=2u+rocket||(rocket&&angle!="135"))return Fail(r,"Portable Coruscant adapter is pending for "+cls);
            if(!pickupCounts[spec-pickups]++)r.pickupClasses.push_back(cls);
        }
    }
    if(mask!=(1u<<models)-2||flagMask!=3||startsByTeam[0]!=25||startsByTeam[1]!=25||dm!=2||views!=2)
        return Fail(r,"Coruscant needs every stock brush, flag and entry group");
    for(size_t i=0;i<ARRAY_LEN(pickups);++i)if(pickupCounts[i]!=pickups[i].count)
        return Fail(r,"Coruscant pickup counts differ from the stock arena");
    for(size_t i=0;i<ARRAY_LEN(sounds);++i)if(soundCounts[i]!=sounds[i].count)
        return Fail(r,"Coruscant sound counts differ from the stock arena");
    for(const char* sound:shaft::liftSounds)r.resources.push_back(sound);
    for(const auto& flag:factory::flags)r.resources.push_back(flag.model);
    r.entryPoints=starts;r.allocationBudget=int(entities.size());r.rejectedEntity=0;
    return true;
}
