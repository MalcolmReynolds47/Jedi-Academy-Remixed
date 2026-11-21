// Portable music properties use the installed retail path catalog. GPL-2.0-only.
#ifndef JAR_MUSIC_H
#define JAR_MUSIC_H
#include "jar_music_tracks.h"
#include <cstring>
#include <string>
namespace jar { namespace music {
constexpr auto& tracks=jarMusicTracks;
constexpr unsigned trackCount=sizeof(tracks)/sizeof(tracks[0]);
constexpr unsigned textLimit=127;
constexpr const char* defaultTrack="music/hoth2/hoth2_explore.mp3";
inline bool Class(const char*name){return name&&!std::strcmp(name,"target_play_music");}
inline int Index(const std::string&path){for(unsigned i=0;i<trackCount;++i)if(path==tracks[i])return int(i);return -1;}
inline bool Canonical(const char*input,std::string&out){
 if(!input||!*input)return false;std::string text;
 for(const char*p=input;*p;++p){if(text.size()==textLimit)return false;char c=*p;if(c>='A'&&c<='Z')c+=32;text+=c;}
 const auto space=text.find(' ');if(space==std::string::npos){if(Index(text)<0)return false;}
 else if(Index(text.substr(0,space))<0||Index(text.substr(space+1))<0)return false;
 out=std::move(text);return true;
}
inline bool Valid(const std::string&text){std::string normalized;return text.size()==std::strlen(text.c_str())&&Canonical(text.c_str(),normalized);}
inline const char* Cycle(const char*text,int direction){std::string s;if(!Canonical(text,s))return defaultTrack;const auto at=Index(s.substr(0,s.find(' ')));return tracks[(at+int(trackCount)+(direction<0?-1:1))%trackCount];}
} }
#endif
