// Portable script reference and navigation-goal properties. GPL-2.0-only.
#ifndef JAR_REFERENCE_H
#define JAR_REFERENCE_H
#include <string>
#include <cstring>
namespace jar { namespace reference {
struct Settings {std::string name,owner;int radius=16;bool enabled=true;};
inline bool Class(const char*s){return s&&(!std::strcmp(s,"ref_tag")||!std::strcmp(s,"waypoint_navgoal"));}
inline bool Identifier(const std::string&s,bool empty=false){
 if(s.empty())return empty;if(s.size()>31)return false;
 for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;
 return true;
}
inline std::string Name(std::string s){for(char&c:s)if(c>='A'&&c<='Z')c+=32;return s;}
inline bool Valid(const Settings&s){return Identifier(s.name)&&Identifier(s.owner,true)&&Name(s.owner)!="__world__"&&s.radius>=0&&s.radius<=1024;}
} }
#endif
