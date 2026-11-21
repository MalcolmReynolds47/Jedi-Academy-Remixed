// Native parameter storage shared by ICARUS and the property editor. GPL-2.0-only.
#ifndef G_SCRIPT_PARMS_H
#define G_SCRIPT_PARMS_H
#include "qcommon/jar_script_parms.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
namespace jar { namespace entityParms {
static_assert(MAX_PARMS==parms::count&&MAX_PARM_STRING_LENGTH==parms::limit+1,"Native parameter layout changed");
inline bool Literal(gentity_t&e,unsigned slot,const std::string&text){
 if(slot>=parms::count||!parms::Valid(text))return false;
 if(!e.parms){if(text.empty())return true;e.parms=static_cast<parms_t*>(G_Alloc(sizeof(parms_t)));memset(e.parms,0,sizeof(parms_t));}
 memcpy(e.parms->parm[slot],text.c_str(),text.size()+1);return true;
}
enum Result {Invalid,Assigned,Truncated};
inline Result Native(gentity_t&e,int slot,const char*text){
 if(slot<0||slot>=MAX_PARMS||!text)return Invalid;
 // Copy first: self-activation may supply a pointer to the destination slot.
 const std::string source(text);char value[MAX_PARM_STRING_LENGTH]{};
 float increment=0;
 if(source.size()>1&&(source[0]=='+'||source[0]=='-'))increment=static_cast<float>(std::atof(source.c_str()+1)*(source[0]=='-'?-1:1));
 Result result=Assigned;
 if(!std::isfinite(increment))return Invalid;
 if(increment){
  const double current=e.parms?std::atof(e.parms->parm[slot]):0;
  const float sum=static_cast<float>(current+increment);if(!std::isfinite(sum))return Invalid;
  Com_sprintf(value,sizeof(value),"%f",sum);
 }else{
  const auto length=std::min(source.size(),size_t(MAX_PARM_STRING_LENGTH-1));
  memcpy(value,source.data(),length);if(length<source.size())result=Truncated;
 }
 if(!e.parms){e.parms=static_cast<parms_t*>(G_Alloc(sizeof(parms_t)));memset(e.parms,0,sizeof(parms_t));}
 memcpy(e.parms->parm[slot],value,sizeof(value));return result;
}
} }
#endif
