// Map-local ambient soundscape selection and fading. GPL-2.0-only.
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr unsigned clipLimit=16, profileLimit=16, zoneLimit=64;
constexpr int scanInterval=200, fadeDuration=3000, maxManifest=16384;

struct Clip { std::string path; int pan=0; sfxHandle_t sound=0; };
using Levels=std::array<float,clipLimit>;
struct Zone { vec3_t origin{}; float radius=0; int profile=-1; };
struct Soundscape {
 std::vector<Clip> clips;
 std::vector<Levels> profiles;
 std::vector<Zone> zones;
};

Soundscape scene;
Levels previous{};
int current=-1, fadeStart=0, lastFrame=0, nextScan=0;

bool SafeName(const std::string& name){
 return !name.empty()&&std::all_of(name.begin(),name.end(),[](unsigned char ch){
  return (ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')||ch=='_';
 });
}

bool ReadSoundscape(std::istream& in,const std::string& map,Soundscape& candidate){
 std::string magic;
 unsigned version,count;
 if(!(in>>magic>>version>>count)||magic!="JAR_SOUNDSCAPE"||version!=1||count==0||count>clipLimit)return false;
 const std::string prefix="sound/jg/"+map+"/";
 candidate.clips.resize(count);
 for(size_t i=0;i<candidate.clips.size();++i){
  auto& clip=candidate.clips[i];
  if(!(in>>clip.pan>>clip.path)||clip.pan < -1||clip.pan > 1||
     clip.path.size()>=MAX_QPATH||clip.path.size()<=prefix.size()+4||
     clip.path.compare(0,prefix.size(),prefix)||clip.path.compare(clip.path.size()-4,4,".wav")||
     !SafeName(clip.path.substr(prefix.size(),clip.path.size()-prefix.size()-4)))return false;
  for(size_t j=0;j<i;++j)if(candidate.clips[j].path==clip.path)return false;
 }
 if(!(in>>count)||count==0||count>profileLimit)return false;
 candidate.profiles.resize(count);
 for(auto& profile:candidate.profiles){
  unsigned layers;
  if(!(in>>layers)||layers==0||layers>candidate.clips.size())return false;
  for(unsigned i=0;i<layers;++i){
   unsigned clip,gain;
   if(!(in>>clip>>gain)||clip>=candidate.clips.size()||!gain||gain>255||profile[clip])return false;
   profile[clip]=float(gain);
  }
 }
 if(!(in>>count)||count==0||count>zoneLimit)return false;
 candidate.zones.resize(count);
 for(auto& zone:candidate.zones){
  if(!(in>>zone.origin[0]>>zone.origin[1]>>zone.origin[2]>>zone.radius>>zone.profile)||
     zone.profile<0||unsigned(zone.profile)>=candidate.profiles.size()||
     !std::isfinite(zone.radius)||(zone.radius!=-1&&(zone.radius<=0||zone.radius>WORLD_SIZE)))return false;
  for(float coord:zone.origin)if(!std::isfinite(coord)||std::fabs(coord)>MAX_WORLD_COORD)return false;
 }
 in>>std::ws;
 return in.eof();
}

Levels Mix(int time){
 if(current<0)return {};
 const float t=std::min(1.f,float(time-fadeStart)/fadeDuration);
 const auto& target=scene.profiles[current];
 Levels result{};
 for(size_t i=0;i<scene.clips.size();++i)result[i]=previous[i]+t*(target[i]-previous[i]);
 return result;
}

int VisibleProfile(const vec3_t ear,int player){
 float nearest=std::numeric_limits<float>::infinity();
 int choice=current;
 for(const auto& zone:scene.zones){
  const float distance=DistanceSquared(ear,zone.origin);
  if(distance>=nearest||(zone.radius>=0&&distance>=zone.radius*zone.radius))continue;
  trace_t hit{};
  TraceSoundscape(&hit,zone.origin,ear,player,CONTENTS_SOLID|CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA);
  if(hit.startsolid||hit.allsolid||hit.fraction<1)continue;
  nearest=distance;
  choice=zone.profile;
 }
 return choice;
}
}

void CL_JAR_ResetSoundscape(){
 scene={};
 CL_JAR_RewindSoundscape();
}

void CL_JAR_RewindSoundscape(){
 previous={};
 current=-1;
 fadeStart=lastFrame=nextScan=0;
}

void CL_JAR_LoadSoundscape(const char* map){
 CL_JAR_ResetSoundscape();
 if(!map||!SafeName(map))return;
 const std::string name="maps/"+std::string(map)+".jarsound";
 fileHandle_t file=0;
 const long size=FS_FOpenFileRead(name.c_str(),&file,qfalse);
 if(!file)return;
 std::string data;
 if(size>0&&size<=maxManifest){
  data.resize(size_t(size));
  if(FS_Read(&data[0],int(size),file)!=size)data.clear();
 }
 FS_FCloseFile(file);

 Soundscape candidate;
 std::istringstream input(data);
 input.imbue(std::locale::classic());
 if(!ReadSoundscape(input,map,candidate)){
  Com_Printf(S_COLOR_YELLOW "Invalid map soundscape %s\n",name.c_str());
  return;
 }
 for(auto& clip:candidate.clips){
  clip.sound=S_RegisterSound(clip.path.c_str());
  if(!clip.sound){
   Com_Printf(S_COLOR_YELLOW "Missing map soundscape clip %s\n",clip.path.c_str());
   return;
  }
 }
 scene=std::move(candidate);
}

void CL_JAR_UpdateSoundscape(int time,const vec3_t ear,int player){
 if(scene.zones.empty())return;
 if(time<lastFrame){
  current=-1;
  previous={};
  nextScan=time;
 }
 lastFrame=time;
 if(time>=nextScan){
  const int choice=VisibleProfile(ear,player);
  if(choice!=current){
   previous=Mix(time);
   fadeStart=time;
   current=choice;
  }
  nextScan=time+scanInterval;
 }
 const auto levels=Mix(time);
 for(size_t i=0;i<scene.clips.size();++i){
  const auto gain=static_cast<unsigned char>(std::lround(levels[i]));
  if(gain)S_AddSoundscapeLoop(scene.clips[i].sound,gain,scene.clips[i].pan);
 }
}
