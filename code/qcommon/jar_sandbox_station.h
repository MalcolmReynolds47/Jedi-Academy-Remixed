// Editable single-player ammo and shield stations. GPL-2.0-only.
#pragma once
#include <cstring>

namespace jar { namespace sandboxStation {
enum Kind { None, Ammo, Shield };
inline Kind Class(const char* name){
 if(!name)return None;
 if(!std::strcmp(name,"misc_ammo_floor_unit")||!std::strcmp(name,"misc_model_ammo_power_converter"))return Ammo;
 if(!std::strcmp(name,"misc_shield_floor_unit")||!std::strcmp(name,"misc_model_shield_power_converter"))return Shield;
 return None;
}
constexpr int maxCapacity=10000,minRecharge=100,maxRecharge=60000;
constexpr int useSoundDuration=500,maxUseCooldown=1000;
struct Settings {
 int capacity=0,charge=0,recharge=0;
 bool unlimited=false,enabled=true;
};
inline bool Valid(const Settings& s){
 return s.capacity>0&&s.capacity<=maxCapacity&&s.charge>=0&&s.charge<=s.capacity&&
  (s.recharge==0||(s.recharge>=minRecharge&&s.recharge<=maxRecharge));
}
inline bool Recharging(const Settings& s){
 return s.enabled&&!s.unlimited&&s.recharge>0&&s.charge<s.capacity;
}
inline bool SameProperties(const Settings& a,const Settings& b){
 return a.capacity==b.capacity&&a.recharge==b.recharge&&a.unlimited==b.unlimited&&a.enabled==b.enabled;
}
} }
