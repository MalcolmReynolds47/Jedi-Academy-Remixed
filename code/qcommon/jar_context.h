// Shared input validation for the in-game entity editor. GPL-2.0-only.
#ifndef JAR_CONTEXT_H
#define JAR_CONTEXT_H
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include "../../shared/qcommon/jar_start_marker.h"
namespace jar { namespace context {
inline bool LabelClass(const char*name){return name&&(JAR_StartCard(name)||!std::strcmp(name,"target_location")||!std::strcmp(name,"trigger_location")||!std::strcmp(name,"target_print")||!std::strcmp(name,"item_datapad")||!std::strcmp(name,"misc_teleporter_dest"));}
inline bool Label(const char*text){
 if(!text||!*text)return false;unsigned length=0;bool visible=false;
 for(const unsigned char*p=reinterpret_cast<const unsigned char*>(text);*p;++p){
  if(++length>63||*p<32||*p>126||*p=='"'||*p=='\\'||*p==';')return false;visible|=*p!=' ';
 }return visible;
}
inline bool Number(const char*text,float&out,float minimum,float maximum){
 if(!text||!*text)return false;
 // Restrict the command alphabet as well as the numeric range. UI values never
 // become executable console text, even if a cvar is edited externally.
 for(const char*p=text;*p;++p)if((*p<'0'||*p>'9')&&*p!='-'&&*p!='+'&&*p!='.')return false;
 char*end=nullptr;const float value=std::strtof(text,&end);
 if(!end||*end||!std::isfinite(value)||value<minimum||value>maximum)return false;
 out=value;return true;
}
inline bool Token(const char*text,uint32_t&out){
 if(!text||!*text)return false;uint64_t value=0;
 for(;*text;++text){if(*text<'0'||*text>'9')return false;value=value*10+(*text-'0');if(value>UINT32_MAX)return false;}
 out=uint32_t(value);return out!=0;
}
inline bool RotateGesture(bool overlay,bool physgun,uint32_t token,int buttons,int attack,int use,int alternate){
 return !overlay&&physgun&&token&&(buttons&(attack|use|alternate))==(attack|use);
}
inline int PhysgunUseButtons(int buttons,int use,bool held,bool& consumed){
 if(!(buttons&use)){consumed=false;return buttons;}
 consumed=consumed||held;
 return consumed?buttons&~use:buttons;
}
} }
#endif
