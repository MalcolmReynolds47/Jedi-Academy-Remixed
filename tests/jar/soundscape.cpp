#include "qcommon/qcommon.h"
#include "../../code/client/jar_soundscape.h"
#include "client/snd_public.h"
#include "../../shared/client/jar_soundscape_pan.h"
#ifdef JAR_MP
#include "qcommon/cm_public.h"
#endif

#include <array>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

namespace {
const char manifest[]=
 "JAR_SOUNDSCAPE 1 4\n"
 "0 sound/jg/fixture/out.wav\n"
 "-1 sound/jg/fixture/left.wav\n"
 "1 sound/jg/fixture/right.wav\n"
 "0 sound/jg/fixture/room.wav\n"
 "2\n"
 "3 0 100 1 50 2 50\n"
 "3 3 200 1 100 2 100\n"
 "2\n"
 "0 0 0 -1 0\n"
 "10 0 0 5 1\n";

std::string input=manifest;
bool blockOutside=false,blockRoom=false,failSound=false;
int registrations=0,closes=0,traces=0;
std::array<int,4> volume{};
std::vector<int> pans;

void Frame(int time,float x){
 volume.fill(0);
 pans.clear();
 const vec3_t ear={x,0,0};
 CL_JAR_UpdateSoundscape(time,ear,0);
}
}

long FS_FOpenFileRead(const char* name,fileHandle_t* handle,qboolean){
 assert(!strcmp(name,"maps/fixture.jarsound"));
 *handle=1;
 return long(input.size());
}
int FS_Read(void* dest,int length,fileHandle_t handle){
 assert(handle==1&&length==int(input.size()));
 memcpy(dest,input.data(),length);
 return length;
}
void FS_FCloseFile(fileHandle_t handle){assert(handle==1);++closes;}
sfxHandle_t S_RegisterSound(const char* path){
 assert(strstr(path,"sound/jg/fixture/")==path);
 return failSound?0:++registrations;
}
void S_AddSoundscapeLoop(sfxHandle_t sound,unsigned char gain,int pan){
 assert(sound>=1&&sound<=int(volume.size())&&!volume[sound-1]);
 volume[sound-1]=gain;
 pans.push_back(pan);
}
#ifdef JAR_MP
void QDECL Com_Printf(const char*, ...){}
void CM_BoxTrace(trace_t* hit,const vec3_t start,const vec3_t,const vec3_t mins,const vec3_t maxs,
                 clipHandle_t model,int mask,int capsule){
 assert(model==0&&!capsule&&VectorCompare(mins,vec3_origin)&&VectorCompare(maxs,vec3_origin));
#else
void SV_TraceBrushLine(trace_t* hit,const vec3_t start,const vec3_t,int player,int mask){
 assert(player==0);
#endif
 assert((mask&CONTENTS_SOLID)&&(mask&CONTENTS_WATER));
 ++traces;
 hit->fraction=(start[0]==0?blockOutside:blockRoom)?.5f:1.f;
}

int main(){
 CL_JAR_LoadSoundscape("fixture");
 assert(registrations==4&&closes==1);
 Frame(100,0);
 assert((volume==std::array<int,4>{0,0,0,0}));
 Frame(1600,0);
 assert((volume==std::array<int,4>{50,25,25,0}));
 assert((pans==std::vector<int>{0,-1,1}));
 Frame(3100,0);
 assert((volume==std::array<int,4>{100,50,50,0}));
 Frame(3300,5);
 assert(volume[3]==0); // Exact radius is outside the room.
 Frame(3500,10);
 Frame(5000,10);
 assert((volume==std::array<int,4>{50,75,75,100}));
 Frame(6500,10);
 assert((volume==std::array<int,4>{0,100,100,200}));
 blockOutside=blockRoom=true;
 Frame(6700,10);
 assert(volume[3]==200); // Keep the last scene through occluded passages.
 blockOutside=false;
 Frame(6900,10);
 Frame(8400,10);
 assert((volume==std::array<int,4>{50,75,75,100}));
 Frame(200,0);
 assert((volume==std::array<int,4>{0,0,0,0})); // Save/time rewind discards the fade.
 Frame(3200,0);
 assert(volume[0]==100);
 CL_JAR_RewindSoundscape();
 Frame(3400,0);
 assert(volume[0]==0&&registrations==4); // map_restart reuses the registered clips.
 Frame(6400,0);
 assert(volume[0]==100);

 using namespace jar::soundscape;
 int left=0,right=0;
 Levels(ClipPan(-1),81,false,left,right);
 assert(left==81&&right==0&&RelativeX(ClipPan(-1))==-1);
 Levels(ClipPan(1),81,false,left,right);
 assert(left==0&&right==81&&RelativeX(ClipPan(1))==1);
 Levels(ClipPan(0),81,false,left,right);
 assert(left==81&&right==81&&RelativeX(ClipPan(0))==0);
 Levels(ClipPan(1),81,true,left,right);
 assert(left==40&&right==40);

 CL_JAR_LoadSoundscape("../fixture");
 Frame(3500,0);
 assert((volume==std::array<int,4>{0,0,0,0})&&closes==1);
 input=manifest+std::string("extra");
 CL_JAR_LoadSoundscape("fixture");
 assert(closes==2&&registrations==4);
 Frame(3500,0);
 assert((volume==std::array<int,4>{0,0,0,0}));
 input.assign(17000,'x');
 CL_JAR_LoadSoundscape("fixture");
 assert(closes==3&&registrations==4);
 input=manifest;
 failSound=true;
 CL_JAR_LoadSoundscape("fixture");
 Frame(3500,0);
 assert((volume==std::array<int,4>{0,0,0,0})&&closes==4&&traces>0);
}
