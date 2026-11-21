// Stock Hoth Wasteland entity admission. GPL-2.0-only.
inline bool WastelandBrush(const Entity& e,const std::string& cls,int model,Report& r){
    using namespace wasteland;
    const auto field=[&](const char* key,const char* value){return !strcmp(Value(e,key),value);};
    for(const auto& volume:weatherVolumes)
        if(volume.model==model)return (cls=="misc_weather_zone"&&e.size()==2)||
            Fail(r,"Hoth Wasteland requires its stock weather volume");
    for(const auto& door:doorLeaves)
        if(door.model==model)return (cls=="func_door"&&e.size()==5&&field("team",doorTeam)&&
            Value(e,"angle")==std::to_string(door.angle)&&field("soundset","impdoor1"))||
            Fail(r,"Hoth Wasteland door leaf differs from its stock team, direction or sound");
    for(int platform:platforms)
        if(platform==model)return (cls=="func_plat"&&e.size()==5&&Value(e,"height")==std::to_string(platformHeight)&&
            field("light","20")&&field("soundset","medium_platform"))||
            Fail(r,"Hoth Wasteland platform differs from its stock height or sound");
    for(const auto& lift:lifts)
        if(lift.model==model)return (cls=="func_door"&&e.size()==11&&field("targetname",lift.target)&&
            field("angle","-2")&&field("wait","3")&&field("delay","0.2")&&field("spawnflags","4")&&
            field("soundset","medium_platform")&&field("dmg","9999")&&
            Value(e,"lip")==std::to_string(lift.lip)&&Value(e,"speed")==std::to_string(lift.speed))||
            Fail(r,"Hoth Wasteland crusher lift differs from its stock travel or link");
    for(const auto& pad:liftPads)
        if(pad.model==model)return (cls=="trigger_multiple"&&e.size()==5&&field("target",lifts[pad.lift].target)&&
            field("delay","0.2")&&Value(e,"wait")==std::to_string(pad.wait))||
            Fail(r,"Hoth Wasteland pressure pad differs from its stock timing or link");
    for(const auto& fall:falls)
        if(fall.model==model)return (cls=="trigger_hurt"&&e.size()==3u+bool(fall.flags)&&
            Value(e,"dmg")==std::to_string(fall.damage)&&
            Value(e,"spawnflags")== (fall.flags?std::to_string(fall.flags):""))||
            Fail(r,"Hoth Wasteland fall hazard differs from its stock damage");
    return Fail(r,"Hoth Wasteland has an unrecognized brush");
}

inline bool WastelandContent(const std::vector<Entity>& entities,Report& r){
    using namespace wasteland;
    if(r.models!=models||entities.size()!=wasteland::entities||strcmp(Value(entities.front(),"classname"),"worldspawn")||
       strcmp(Value(entities.front(),"fog"),fog)||strcmp(Value(entities.front(),"distancecull"),"1800"))
        return Fail(r,"Hoth Wasteland requires the stock arena and fog");
    unsigned brushes=0,flags=0,bases=0,snow=0,gunSlots=0;
    int pickupCounts[ARRAY_LEN(pickups)]{},startsByTeam[2]{},neutral=0,views=0,viewTargets=0;
    for(size_t i=1;i<entities.size();++i){
        r.rejectedEntity=int(i)+1;
        const auto& e=entities[i];
        const std::string cls=Value(e,"classname"),model=Value(e,"model"),angle=Value(e,"angle"),origin=Value(e,"origin");
        if(!model.empty()&&model.front()=='*'){
            if(!Number(model.c_str()+1,1,models-1)||model!="*"+std::to_string(atoi(model.c_str()+1)))
                return Fail(r,"Hoth Wasteland has an invalid inline model");
            const int index=atoi(model.c_str()+1);
            const unsigned bit=1u<<index;
            if(brushes&bit)return Fail(r,"Hoth Wasteland repeats an inline model");
            brushes|=bit;
            if(!WastelandBrush(e,cls,index,r))return false;
            continue;
        }
        if(!Vector(origin.c_str()))return Fail(r,"Hoth Wasteland entity requires a local origin");
        if(const int team=factory::FlagTeam(cls.c_str())){
            const unsigned bit=1u<<(team-1);
            if((flags&bit)||e.size()!=2||origin!=flagOrigins[team-1])
                return Fail(r,"Hoth Wasteland flag differs from its stock home");
            flags|=bit;
        }else if(cls=="misc_model_static"){
            const auto* spec=std::find_if(std::begin(scenery),std::end(scenery),[&](const Scenery& s){return origin==s.origin;});
            if(spec==std::end(scenery)||e.size()!=5u+!angle.empty()||angle!=spec->heading||
               model!=(spec->pedestal?factory::baseModel:crateModel)||strcmp(Value(e,"_rs"),"1")||strcmp(Value(e,"_cs"),"1"))
                return Fail(r,"Hoth Wasteland scenery differs from its stock model or position");
            const unsigned bit=1u<<(spec-scenery);
            if(bases&bit)return Fail(r,"Hoth Wasteland repeats a scenery model");
            bases|=bit;
        }else if(const int team=factory::StartTeam(cls.c_str())){
            if(e.size()!=3||!Number(angle.c_str(),-360,360))
                return Fail(r,"Hoth Wasteland team entry has an invalid heading");
            ++startsByTeam[team-1];
        }else if(cls=="info_player_start"){
            if(e.size()!=3||!Number(angle.c_str(),-360,360))
                return Fail(r,"Hoth Wasteland entry has an invalid heading");
            ++neutral;
        }else if(cls=="info_player_intermission"){
            if(e.size()!=3||strcmp(Value(e,"target"),"t296"))
                return Fail(r,"Hoth Wasteland view needs its stock target");
            ++views;
        }else if(cls=="info_null"){
            const char* name=Value(e,"targetname");
            if(e.size()!=3||!*name)return Fail(r,"Hoth Wasteland marker requires its local name");
            if(!strcmp(name,"t296")){
                if(origin!="-1760 1184 128")return Fail(r,"Hoth Wasteland view target differs from stock");
                ++viewTargets;
            }
        }else if(cls=="fx_snow"){
            const auto it=std::find(std::begin(snowOrigins),std::end(snowOrigins),origin);
            if(it==std::end(snowOrigins)||e.size()!=3||Value(e,"count")!=std::to_string(snowCount))
                return Fail(r,"Hoth Wasteland snowfall differs from the stock emitters");
            const unsigned bit=1u<<(it-snowOrigins);
            if(snow&bit)return Fail(r,"Hoth Wasteland repeats a snow emitter");
            snow|=bit;
        }else if(cls=="emplaced_gun"){
            const auto* gun=std::find_if(std::begin(guns),std::end(guns),[&](const Gun& g){return origin==g.origin;});
            if(gun==std::end(guns)||e.size()!=6||angle!=gun->heading||
               Value(e,"count")!=std::to_string(gunRespawn)||strcmp(Value(e,"spawnflags"),"1")||strcmp(Value(e,"showhealth"),"1"))
                return Fail(r,"Hoth Wasteland emplacement differs from the stock location or respawn rule");
            const unsigned bit=1u<<(gun-guns);
            if(gunSlots&bit)return Fail(r,"Hoth Wasteland repeats an emplaced gun");
            gunSlots|=bit;
        }else{
            const auto* pickup=std::find_if(std::begin(pickups),std::end(pickups),
                [&](const rift::Pickup& p){return cls==p.name;});
            if(pickup==std::end(pickups)||e.size()!=2u+!angle.empty()||
               (!angle.empty()&&(cls!="item_medpak_instant"||!Number(angle.c_str(),-360,360))))
                return Fail(r,"Portable Hoth Wasteland adapter is pending for "+cls);
            if(!pickupCounts[pickup-pickups]++)r.pickupClasses.push_back(cls);
        }
    }
    if(brushes!=(1u<<models)-2||flags!=3||bases!=(1u<<ARRAY_LEN(scenery))-1||snow!=3||gunSlots!=3||
       startsByTeam[0]!=11||startsByTeam[1]!=13||neutral!=1||views!=1||viewTargets!=1)
        return Fail(r,"Hoth Wasteland needs every stock brush, flag, entry and controller");
    for(size_t i=0;i<ARRAY_LEN(pickups);++i)
        if(pickupCounts[i]!=pickups[i].count)return Fail(r,"Hoth Wasteland pickup counts differ from the stock arena");
    for(const auto& flag:factory::flags)r.resources.push_back(flag.model);
    for(const char* model:{factory::baseModel,crateModel})r.resources.push_back(model);
    for(const char* file:{gunModel,gunSound,gunExitSound,gunMoveSound,gunExplosion,snowImage,
        "effects/emplaced/dead_smoke.efx"})r.resources.push_back(file);
    for(const char* file:taspir::sounds[0].files)r.resources.push_back(file);
    for(const char* file:shaft::liftSounds)r.resources.push_back(file);
    r.entryPoints=starts;
    r.ownedHelpers=ARRAY_LEN(platforms); // The linked door trigger is created after all leaves exist.
    r.allocationBudget=int(entities.size())+helpers;
    r.rejectedEntity=0;
    return true;
}
