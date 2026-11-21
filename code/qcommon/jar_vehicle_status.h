// Single-player Sandbox vehicle resources. GPL-2.0-only.
#pragma once
#include "jar_context.h"
#include <array>

namespace jar { namespace vehicleStatus {
constexpr int ammoSlots=4;
constexpr int fields=2+ammoSlots;
constexpr int maximum=100000;
constexpr int FieldMinimum(int field){return field==0?1:0;}

struct Values {
 std::array<int,fields> value{};
};

inline bool Valid(const Values& resources){
 for(int i=0;i<fields;++i)if(resources.value[i]<FieldMinimum(i))return false;
 return true;
}

inline bool Field(const char* text,int field,int limit,int& value){
 float parsed=0;
 if(!context::Number(text,parsed,float(FieldMinimum(field)),float(limit))||parsed!=std::floor(parsed))return false;
 value=int(parsed);
 return true;
}
} }
