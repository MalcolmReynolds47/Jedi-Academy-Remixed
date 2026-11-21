// Native authored CTF lift and trigger fields shared by stock section profiles.
// GPL-2.0-only.
template<size_t LiftCount,size_t PadCount,size_t FallCount>
inline bool CtfBrush(const Entity& e,const std::string& cls,int index,
    const sectionCTF::Lift (&lifts)[LiftCount],const sectionCTF::Pad (&pads)[PadCount],
    const int (&falls)[FallCount],const char* fallFlags,Report& r){
    if(cls=="func_door"){
        const auto* lift=std::find_if(std::begin(lifts),std::end(lifts),[&](const sectionCTF::Lift& row){return row.model==index;});
        if(lift==std::end(lifts))return Fail(r,"CTF lift has an unknown model");
        const auto speed=lift->speed==400?std::string{}:std::to_string(lift->speed);
        if(e.size()!=10u+!speed.empty()+lift->linear||strcmp(Value(e,"angle"),"-2")||Value(e,"lip")!=std::to_string(lift->lip)||
           strcmp(Value(e,"targetname"),lift->target)||strcmp(Value(e,"soundset"),"medium_platform")||Value(e,"speed")!=speed||
           strcmp(Value(e,"dmg"),"5000")||strcmp(Value(e,"wait"),"3")||strcmp(Value(e,"delay"),"0.2")||
           strcmp(Value(e,"spawnflags"),"5")||strcmp(Value(e,"linear"),lift->linear?"1":""))
            return Fail(r,"CTF lift differs from its stock travel or target");
        return true;
    }
    if(cls=="trigger_multiple"){
        const auto* pad=std::find_if(std::begin(pads),std::end(pads),[&](const sectionCTF::Pad& row){return row.model==index;});
        if(pad==std::end(pads))return Fail(r,"CTF lift trigger has an unknown model");
        const bool facing=pad->yaw>=0;
        if(e.size()!=(facing?7u:5u)||Value(e,"angle")!=(facing?std::to_string(pad->yaw):"")||
           strcmp(Value(e,"spawnflags"),facing?"2":"")||strcmp(Value(e,"target"),lifts[pad->lift].target)||
           Value(e,"wait")!=std::to_string(pad->wait)||strcmp(Value(e,"delay"),pad->delay==200?"0.2":"2"))
            return Fail(r,"CTF lift trigger differs from its stock direction, delay or link");
        return true;
    }
    return (cls=="trigger_hurt"&&std::find(std::begin(falls),std::end(falls),index)!=std::end(falls)&&
            e.size()==3u+bool(*fallFlags)&&!strcmp(Value(e,"spawnflags"),fallFlags)&&!strcmp(Value(e,"dmg"),"-1"))||
            Fail(r,"CTF section requires its stock fall hazards");
}
