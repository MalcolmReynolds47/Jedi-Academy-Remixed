// Content admission for the retail Factory arena. GPL-2.0-only.
inline bool FactoryContent(const std::vector<Entity>& entities,Report& r){
    using namespace factory;
    if(r.models!=modelCount||entities.size()!=entityCount||strcmp(Value(entities.front(),"classname"),"worldspawn"))
        return Fail(r,"Factory requires the complete stock arena");
    unsigned brushes=0,flagMask=0,baseMask=0;
    int counts[ARRAY_LEN(pickups)]{};
    int teamStarts[2]{},ordinaryStarts=0,views=0;
    for(size_t i=1;i<entities.size();++i){
        r.rejectedEntity=int(i)+1;
        const auto& e=entities[i];
        const std::string cls=Value(e,"classname");
        const char* origin=Value(e,"origin");
        const char* model=Value(e,"model");
        if(*model=='*'){
            if(strlen(model)!=2||model[1]<'1'||model[1]>'5')return Fail(r,"Factory inline model is out of range");
            const int index=model[1]-'0';
            const unsigned bit=1u<<index;
            if(brushes&bit)return Fail(r,"Factory repeats an inline model");
            brushes|=bit;
            if(const auto* window=Glass(index)){
                if(cls!="func_glass"||e.size()!=3||Value(e,"maxshards")!=std::to_string(window->shards))
                    return Fail(r,"Factory glass requires its stock model and shard count");
            }else if(index!=hurtModel||cls!="trigger_hurt"||e.size()!=4||strcmp(Value(e,"dmg"),"-1")||
                     strcmp(Value(e,"spawnflags"),"12"))return Fail(r,"Factory requires its stock fall trigger");
        }else if(!Vector(origin))return Fail(r,"Factory entity requires a local origin");
        else if(const int flag=FlagTeam(cls.c_str())){
            const unsigned bit=1u<<(flag-1);
            if((flagMask&bit)||e.size()!=2||strcmp(origin,flags[flag-1].origin))return Fail(r,"Factory flag has an invalid home");
            flagMask|=bit;
        }else if(cls=="misc_model_static"){
            if(e.size()!=3||strcmp(model,baseModel))return Fail(r,"Factory requires its stock flag pedestals");
            const unsigned bit=!strcmp(origin,"1548 2592 -344")?1u:!strcmp(origin,"7220 2592 -344")?2u:0u;
            if(!bit||(baseMask&bit))return Fail(r,"Factory flag pedestal has an invalid home");
            baseMask|=bit;
        }else if(cls=="info_player_intermission"){
            if(e.size()!=3||!OneOf(Value(e,"target"),{"v1","t163"}))return Fail(r,"Factory view needs its stock local target");
            ++views;
        }else if(const int team=StartTeam(cls.c_str())){
            const char* angle=Value(e,"angle");
            if(e.size()!=2u+bool(*angle)||(*angle&&!Number(angle,-360,360)))return Fail(r,"Factory team entry has an invalid heading");
            ++teamStarts[team-1];
        }else if(cls=="info_player_deathmatch"){
            const char* angle=Value(e,"angle");
            if(e.size()!=2u+bool(*angle)||(*angle&&!Number(angle,-360,360)))return Fail(r,"Factory deathmatch entry has an invalid heading");
            ++ordinaryStarts;
        }else if(cls=="info_null"||cls=="info_notnull"){
            if(e.size()!=3||!*Value(e,"targetname"))return Fail(r,"Factory marker requires a local name");
        }else{
            const auto* spec=std::find_if(std::begin(pickups),std::end(pickups),[&](const rift::Pickup& p){return cls==p.name;});
            const bool disruptor=cls=="weapon_disruptor";
            if(spec==std::end(pickups)||e.size()!=2u+disruptor||(disruptor&&strcmp(Value(e,"angle"),"45")))
                return Fail(r,"Portable Factory adapter is pending for "+cls);
            if(!counts[spec-pickups]++)r.pickupClasses.push_back(cls);
        }
    }
    if(brushes!=(1u<<modelCount)-2||flagMask!=3||baseMask!=3||teamStarts[0]!=13||teamStarts[1]!=14||ordinaryStarts!=2||views!=2)
        return Fail(r,"Factory needs every stock window, flag base and arrival group");
    for(size_t i=0;i<ARRAY_LEN(pickups);++i)if(counts[i]!=pickups[i].count)
        return Fail(r,"Factory pickup counts differ from the stock arena");
    for(const char* resource:{baseModel,flags[0].model,flags[1].model,glassSound,glassImpact})r.resources.push_back(resource);
    r.entryPoints=entryCount;
    r.allocationBudget=entityCount;
    r.rejectedEntity=0;
    return true;
}
