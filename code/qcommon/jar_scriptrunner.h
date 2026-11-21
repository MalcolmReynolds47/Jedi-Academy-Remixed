// Portable native ICARUS runner properties. GPL-2.0-only.
#ifndef JAR_SCRIPTRUNNER_H
#define JAR_SCRIPTRUNNER_H
#include "jar_script_catalog.h"
#include <cmath>
#include <cstring>
#include <string>
namespace jar { namespace scriptRunner {
constexpr unsigned textLimit=127;
constexpr const char* defaultScript="common/dooropenedelsewhere";
struct Settings {std::string script;float delay=0,wait=1;int count=-1;bool activator=true;};
inline bool Class(const char*name){return name&&!std::strcmp(name,"target_scriptrunner");}
inline bool Canonical(const char*input,std::string&out){
 if(!input||!*input)return false;std::string text;
 for(const char*p=input;*p;++p){if(text.size()>=textLimit)return false;char c=*p;if(c>='A'&&c<='Z')c+=32;text+=c;}
 if(text.compare(0,8,"scripts/")==0)text.erase(0,8);
 if(text.size()>4&&text.compare(text.size()-4,4,".ibi")==0)text.resize(text.size()-4);
 if(text.empty()||text.front()=='/'||text.back()=='/'||text.find("//")!=std::string::npos)return false;
 for(char c:text)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='/'))return false;
 out=std::move(text);return true;
}
inline bool Valid(const Settings&s){std::string path;return s.script.size()==std::strlen(s.script.c_str())&&Canonical(s.script.c_str(),path)&&std::isfinite(s.delay)&&s.delay>=0&&s.delay<=3600&&std::isfinite(s.wait)&&s.wait>=0&&s.wait<=3600&&s.count>=-1&&s.count<=100000;}
inline const char*Cycle(const char*input,int direction){std::string path;if(!Canonical(input,path))return defaultScript;for(unsigned i=0;i<scriptCount;++i)if(path==scripts[i])return scripts[(i+scriptCount+(direction<0?-1:1))%scriptCount];return defaultScript;}
} }
#endif
