// Bounded literal editor values; command transport is hexadecimal. GPL-2.0-only.
#ifndef JAR_SCRIPT_PARMS_H
#define JAR_SCRIPT_PARMS_H
#include <string>
#include <cstring>
namespace jar { namespace parms {
constexpr unsigned count=16,limit=63;
inline bool Valid(const std::string&value){
 if(value.size()>limit)return false;
 for(unsigned char c:value)if(c<32||c>126)return false;
 return true;
}
inline bool Read(const char*value,std::string&out){
 if(!value){out.clear();return true;}
 unsigned n=0;while(n<=limit&&value[n])++n;
 if(n>limit)return false;out.assign(value,n);return Valid(out);
}
} }
#endif
