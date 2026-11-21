// Native Jedi Academy player carry data used by map transitions and autosaves.
// GPL-2.0-only. Include after the single-player game headers.
#pragma once
#include <array>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

namespace jar { namespace playerTransfer {
struct Saber {
    char name[MAX_QPATH]{};
    std::array<int,MAX_BLADES> active{},color{};
};

struct State {
    int health=0,armor=0,weapons=0,items=0,weapon=0,weaponState=0,battery=0;
    float view[3]{};
    int forceKnown=0,force=0,forceMax=0,regenRate=0,regenAmount=0;
    Saber sabers[2];
    int styles=0,style=0,lockEnemy=0,lockTime=0;
};

class Reader {
    const char* pos;
    static bool Space(char c) { return std::isspace(static_cast<unsigned char>(c))!=0; }
    void SkipSpace() { while(Space(*pos))++pos; }
    bool TokenEnded(const char* end) const { return end!=pos&&(!*end||Space(*end)); }

public:
    explicit Reader(const char* text):pos(text?text:""){}

    bool Done() { SkipSpace();return !*pos; }

    bool Integer(int& result) {
        SkipSpace();
        char* end=nullptr;
        errno=0;
        const long value=std::strtol(pos,&end,10);
        if(!TokenEnded(end)||errno==ERANGE||value<INT_MIN||value>INT_MAX)return false;
        result=int(value);
        pos=end;
        return true;
    }

    bool Float(float& result) {
        SkipSpace();
        char* end=nullptr;
        const float value=std::strtof(pos,&end);
        if(!TokenEnded(end)||!std::isfinite(value))return false;
        result=value;
        pos=end;
        return true;
    }

    bool Name(char (&out)[MAX_QPATH]) {
        SkipSpace();
        const char* end=pos;
        while(*end&&!Space(*end))++end;
        const auto length=end-pos;
        if(length<=0||length>=MAX_QPATH)return false;
        std::memcpy(out,pos,size_t(length));
        out[length]='\0';
        pos=end;
        if(!Q_stricmp(out,"(null)"))out[0]='\0';
        return true;
    }
};

inline bool Decode(const char* text,State& out) {
    Reader reader(text);
    State saved;
    for(auto* value:{&saved.health,&saved.armor,&saved.weapons,&saved.items,&saved.weapon,
                    &saved.weaponState,&saved.battery})
        if(!reader.Integer(*value))return false;
    for(float& value:saved.view)if(!reader.Float(value))return false;
    for(auto* value:{&saved.forceKnown,&saved.force,&saved.forceMax,&saved.regenRate,&saved.regenAmount})
        if(!reader.Integer(*value))return false;
    for(auto& saber:saved.sabers){
        if(!reader.Name(saber.name))return false;
        for(int& on:saber.active)if(!reader.Integer(on)||(on!=0&&on!=1))return false;
        for(int& color:saber.color)if(!reader.Integer(color)||color<SABER_RED||color>SABER_PURPLE)return false;
    }
    for(auto* value:{&saved.styles,&saved.style,&saved.lockEnemy,&saved.lockTime})
        if(!reader.Integer(*value))return false;
    if(!reader.Done()||saved.weapon<WP_NONE||saved.weapon>=WP_NUM_WEAPONS||
       saved.weaponState<WEAPON_READY||saved.weaponState>WEAPON_IDLE||
       saved.style<SS_NONE||saved.style>=SS_NUM_SABER_STYLES||
       saved.lockEnemy<-1||saved.lockEnemy>ENTITYNUM_NONE)return false;
    out=saved;
    return true;
}

inline std::string Encode(const playerState_t& ps) {
    std::string result;
    const auto append=[&](auto value){
        if(!result.empty())result+=' ';
        result+=std::to_string(value);
    };
    for(int value:{ps.stats[STAT_HEALTH],ps.stats[STAT_ARMOR],ps.stats[STAT_WEAPONS],ps.stats[STAT_ITEMS],
                   ps.weapon,ps.weaponstate,ps.batteryCharge})append(value);
    for(float angle:ps.viewangles)append(angle);
    for(int value:{ps.forcePowersKnown,ps.forcePower,ps.forcePowerMax,ps.forcePowerRegenRate,ps.forcePowerRegenAmount})
        append(value);
    for(const auto& saber:ps.saber){
        result+=' ';
        result+=(saber.name&&*saber.name)?saber.name:"(null)";
        for(const auto& blade:saber.blade)append(blade.active);
        for(const auto& blade:saber.blade)append(blade.color);
    }
    for(int value:{ps.saberStylesKnown,ps.saberAnimLevel,ps.saberLockEnemy,ps.saberLockTime})append(value);
    return result;
}

template<size_t N,typename Valid>
bool ReadValues(const char* text,std::array<int,N>& out,Valid valid) {
    Reader reader(text);
    std::array<int,N> values{};
    if(reader.Done())return false;
    for(size_t i=0;!reader.Done();++i){
        if(i>=N||!reader.Integer(values[i])||!valid(i,values[i]))return false;
    }
    out=values;
    return true;
}

inline bool ReadAmmo(const char* text,std::array<int,AMMO_MAX>& out) {
    return ReadValues(text,out,[](size_t,int value){return value>=-1;});
}

inline bool ReadForceRanks(const char* text,std::array<int,NUM_FORCE_POWERS>& out) {
    return ReadValues(text,out,[](size_t power,int rank){
        // Match the native Force console limits, including Mind Trick and
        // the separately indexed Saber Offense styles.
        const int maximum=power==FP_SABER_OFFENSE?SS_NUM_SABER_STYLES-1:power==FP_TELEPATHY?4:3;
        return rank>=0&&rank<=maximum;
    });
}
} }
