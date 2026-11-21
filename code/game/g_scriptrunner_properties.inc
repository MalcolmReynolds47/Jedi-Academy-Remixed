// Native callbacks, extracted for headless lifetime/timer tests. GPL-2.0-only.
#include "g_target_scriptrunner.h"
#include <algorithm>
#include <climits>
bool G_ScriptRunnerAvailable(const char*input){
 std::string path;if(!jar::scriptRunner::Canonical(input,path))return false;
 path="scripts/"+path+".ibi";fileHandle_t file=0;
 const int size=gi.FS_FOpenFile(path.c_str(),&file,FS_READ);if(!file)return false;
 unsigned char header[8]{};const int read=size>=8?gi.FS_Read(header,sizeof(header),file):0;gi.FS_FCloseFile(file);
 static const unsigned char expected[]={ 'I','B','I',0,0xc3,0xf5,0xc8,0x3f }; // Native IBI 1.57 header.
 return size>8&&read==sizeof(header)&&!memcmp(header,expected,sizeof(header));
}
bool G_ScriptRunnerRead(const gentity_t&e,jar::scriptRunner::Settings&out){
 if(!jar::scriptRunner::Class(e.classname))return false;jar::scriptRunner::Settings s;
 if(!jar::scriptRunner::Canonical(e.behaviorSet[BSET_USE],s.script))return false;
 s.delay=e.delay/1000.f;s.wait=e.wait/1000.f;s.count=e.count;s.activator=(e.spawnflags&1)!=0;
 if(!jar::scriptRunner::Valid(s))return false;out=std::move(s);return true;
}
bool G_ScriptRunnerSet(gentity_t&e,const jar::scriptRunner::Settings&s){
 if(!e.inuse||!jar::scriptRunner::Class(e.classname)||!jar::scriptRunner::Valid(s)||!G_ScriptRunnerAvailable(s.script.c_str()))return false;
 std::string path;jar::scriptRunner::Canonical(s.script.c_str(),path);char*replacement=G_NewString(path.c_str());
 if(e.behaviorSet[BSET_USE]&&gi.bIsFromZone(e.behaviorSet[BSET_USE],TAG_G_ALLOC))gi.Free(e.behaviorSet[BSET_USE]);
 e.behaviorSet[BSET_USE]=replacement;e.delay=int(std::lround(s.delay*1000));e.wait=std::round(s.wait*1000);e.count=s.count;
 e.spawnflags=s.activator?e.spawnflags|1:e.spawnflags&~1;
 // Editing rearms properties, never executes or transfers an old pending use.
 e.activator=nullptr;e.nextthink=0;e.e_ThinkFunc=thinkF_NULL;e.e_UseFunc=s.count?useF_target_scriptrunner_use:useF_NULL;
 return true;
}
