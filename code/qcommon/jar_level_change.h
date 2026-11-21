// Portable map transitions. Names become native console command arguments.
// GPL-2.0-only.
#ifndef JAR_LEVEL_CHANGE_H
#define JAR_LEVEL_CHANGE_H
#include <string>
#include <cstring>
namespace jar { namespace levelChange {
constexpr unsigned mapLimit=54,spawnLimit=63;
struct Settings {std::string map,spawn;bool hub=false;};
inline bool Class(const char*s){return s&&!std::strcmp(s,"target_level_change");}
inline bool Word(unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-';}
inline bool Valid(const Settings&s){
 if(s.map.empty()||s.map.size()>mapLimit||s.spawn.size()>spawnLimit)return false;
 bool component=false;
 for(unsigned char c:s.map){if(c=='/'){if(!component)return false;component=false;}else{if(!Word(c))return false;component=true;}}
 if(!component)return false;
 for(unsigned char c:s.spawn)if(!Word(c))return false;
 return true;
}
} }
#endif
