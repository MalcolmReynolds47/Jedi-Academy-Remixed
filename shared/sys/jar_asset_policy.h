// Isolate the known JAPro base overrides only while JA:R MP is selected.
// No files are removed; other mods and custom maps retain native lookup rules.
#pragma once
#include <cstring>
namespace jar { namespace assets {
inline bool Same(const char*a,const char*b){if(!a||!b)return false;while(*a&&*b){char x=*a++,y=*b++;if(x>='A'&&x<='Z')x+='a'-'A';if(y>='A'&&y<='Z')y+='a'-'A';if(x!=y)return false;}return *a==*b;}
inline bool SkipBasePak(const char*game,const char*overrideGame,const char*directory,const char*file){
 if(!Same(game,"JediAcademyRemixedMP")||!Same(directory,"base"))return false;
 if(overrideGame&&*overrideGame&&!Same(overrideGame,"JediAcademyRemixedMP"))return false;
 return Same(file,"japro-assets.pk3")||Same(file,"japro-win-x86.pk3");
}
}}
