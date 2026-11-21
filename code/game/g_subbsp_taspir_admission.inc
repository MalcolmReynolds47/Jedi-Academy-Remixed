// Validate Taspir before running its native movers or wiring its path. GPL-2.0-only.
inline bool TaspirContent(const std::vector<Entity>& entities,Report& r){
    using namespace taspir;
    if(r.models!=modelCount||entities.size()!=entityCount||r.redundantSoundKeys!=1||
       strcmp(Value(entities.front(),"classname"),"worldspawn"))
        return Fail(r,"Taspir requires the complete stock arena");
    unsigned brushMask=0,cornerMask=0;
    int sceneryCounts[ARRAY_LEN(scenery)]{},pickupCounts[ARRAY_LEN(pickups)]{};
    for(size_t i=1;i<entities.size();++i){
        r.rejectedEntity=int(i)+1;
        const auto& e=entities[i];
        const std::string cls=Value(e,"classname");
        const char* model=Value(e,"model");
        const char* origin=Value(e,"origin");
        if(*model=='*'){
            if(!Number(model+1,1,modelCount-1)||strspn(model+1,"0123456789")!=strlen(model+1))
                return Fail(r,"Taspir inline model is out of range");
            const int index=atoi(model+1);
            const unsigned bit=1u<<index;
            if(brushMask&bit)return Fail(r,"Taspir repeats an inline model");
            brushMask|=bit;
            if(cls=="func_door"&&std::find(std::begin(doors),std::end(doors),index)!=std::end(doors)&&
               e.size()==5&&!strcmp(Value(e,"angle"),"-1")&&!strcmp(Value(e,"lip"),"24")&&
               !strcmp(Value(e,"soundset"),sounds[0].stock))continue;
            if(cls=="func_plat"){
                const auto* p=std::find_if(std::begin(platforms),std::end(platforms),[&](const Platform& row){return row.model==index;});
                if(p!=std::end(platforms)&&e.size()==5u+bool(*p->lip)&&!strcmp(Value(e,"lip"),p->lip)&&
                   Value(e,"height")==std::to_string(p->height)&&!strcmp(Value(e,"dmg"),"5000")&&
                   !strcmp(Value(e,"soundset"),sounds[1].stock))continue;
            }
            if(cls=="func_train"&&index==trainModel&&e.size()==7&&!strcmp(origin,"-416 2656 64")&&
               !strcmp(Value(e,"spawnflags"),"33")&&!strcmp(Value(e,"dmg"),"5000")&&
               !strcmp(Value(e,"target"),corners[0].name)&&!strcmp(Value(e,"soundset"),sounds[2].stock))continue;
            if(cls=="trigger_hurt"&&index==fallModel&&e.size()==4&&!strcmp(Value(e,"wait"),"1")&&
               !strcmp(Value(e,"dmg"),"50"))continue;
            return Fail(r,"Taspir brush differs from the stock mover or hazard");
        }
        if(!Vector(origin))return Fail(r,"Taspir entity requires a local origin");
        if(cls=="path_corner"){
            const char* name=Value(e,"targetname");
            const auto* corner=std::find_if(std::begin(corners),std::end(corners),[&](const Corner& c){return !strcmp(c.name,name);});
            if(corner==std::end(corners))return Fail(r,"Taspir lift requires all stock path corners");
            const size_t slot=corner-corners;
            if((cornerMask&(1u<<slot))||e.size()!=5u+bool(corner->wait)||
               Value(e,"speed")!=std::to_string(corner->speed)||strcmp(Value(e,"wait"),corner->wait?"3":"")||
               origin!=(std::string("-416 2656 ")+std::to_string(corner->height))||
               strcmp(Value(e,"target"),corners[(slot+1)%ARRAY_LEN(corners)].name))
                return Fail(r,"Taspir lift requires its stock cyclic path and timing");
            cornerMask|=1u<<slot;
        }else if(cls=="misc_model_static"){
            const auto* spec=std::find_if(std::begin(scenery),std::end(scenery),[&](const Scenery& s){return model==std::string(sceneryPrefix)+s.model;});
            if(spec==std::end(scenery)||strcmp(Value(e,"zoffset"),"32"))return Fail(r,"Taspir requires its stock scenery");
            for(const auto& f:e){
                if(OneOf(f.key,{"classname","model","origin","zoffset"}))continue;
                if(f.key=="angles"&&Vector(f.value.c_str()))continue;
                if(f.key=="modelscale"&&OneOf(f.value,{"1.5","2"}))continue;
                return Fail(r,"Taspir scenery placement is unsupported");
            }
            if(!sceneryCounts[spec-scenery]++)r.resources.push_back(model);
        }else if(bspSection::Position(cls.c_str())){
            const auto* angle=Value(e,"angle");
            const bool intermission=cls=="info_player_intermission";
            if(e.size()!=2u+bool(*angle)+intermission||(*angle&&!Number(angle,-360,360))||
               (intermission&&strcmp(Value(e,"target"),"t287")))
                return Fail(r,"Taspir start has unsupported fields");
            if(bspSection::Start(cls.c_str()))++r.entryPoints;
        }else if(cls=="info_null"||cls=="target_position"){
            if(e.size()!=3||!*Value(e,"targetname"))return Fail(r,"Taspir marker requires a local name");
        }else{
            const auto* spec=std::find_if(std::begin(pickups),std::end(pickups),[&](const rift::Pickup& p){return cls==p.name;});
            if(spec==std::end(pickups)||e.size()!=2)return Fail(r,"Portable Taspir adapter is pending for "+cls);
            if(!pickupCounts[spec-pickups]++)r.pickupClasses.push_back(cls);
        }
    }
    if(brushMask!=(1u<<modelCount)-2||cornerMask!=(1u<<ARRAY_LEN(corners))-1||r.entryPoints!=entryCount)
        return Fail(r,"Taspir needs every stock mover, corner and start");
    for(size_t i=0;i<ARRAY_LEN(pickups);++i)if(pickupCounts[i]!=pickups[i].count)
        return Fail(r,"Taspir pickup counts differ from the stock arena");
    for(size_t i=0;i<ARRAY_LEN(scenery);++i)if(sceneryCounts[i]!=scenery[i].count)
        return Fail(r,"Taspir scenery counts differ from the stock arena");
    for(const auto& sound:sounds)for(const char* file:sound.files)r.resources.push_back(file);
    r.ownedHelpers=int(ARRAY_LEN(platforms));
    r.allocationBudget=entityCount+helperCount;
    r.rejectedEntity=0;
    return true;
}
