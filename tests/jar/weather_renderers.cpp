// Production renderer adapters at the image/particle allocation boundary.
#include "qcommon/jar_weather_zones.h"
#include "test_support.h"
#include <vector>
#include <deque>
#include <cstddef>
using vec3_t=float[3];
#define VectorCopy(a,b) do{for(int v=0;v<3;++v)(b)[v]=(a)[v];}while(0)
#define VectorAdd(a,b,c) do{for(int v=0;v<3;++v)(c)[v]=(a)[v]+(b)[v];}while(0)
namespace classic {
static jar::weather::Frame jarWeather;
struct CParticleCloud {int allocations=0,resets=0,mBlendMode=0,mFilterMode=0,count=0,vertices=0;float mGravity=0,mWidth=0,mHeight=0,mColor[4]{},mFade=0;bool mWaterParticles=false,mOrientWithVelocity=false;struct{float mMin,mMax;}mMass{};struct{float mMins[3],mMaxs[3];}mSpawnRange{};std::string texture;
 void Reset(){++resets;count=0;}void Initialize(int n,const char*s,int v){++allocations;count=n;texture=s;vertices=v;}
};
#include "rd-vanilla/tr_jar_weather.inc"
}
namespace rend2 {
enum {IMGTYPE_COLORALPHA=1,IMGFLAG_CLAMPTOEDGE=2,IMGFLAG_SRGB=4};struct weatherSystem_t;static struct{bool hdrLighting=true;weatherSystem_t*weatherSystem=nullptr;}tr;
static std::string texture;static int flags=0,images=0;
void*R_FindImageFile(const char*s,int type,int f){CHECK(type==IMGTYPE_COLORALPHA);texture=s;flags=f;++images;return reinterpret_cast<void*>(1);}
struct weatherObject_t{bool active=false;int particleCount=0;float gravity=0,fadeDistance=0,velocityOrientationScale=0,size[2]{},color[4]{};void*drawImage=nullptr;void*vbo=nullptr;};
struct weatherSystem_t{weatherObject_t weatherSlots[5],sandboxSlots[3];jar::weather::Frame sandbox;jar::weather::Zones sandboxZones;float constWindDirection[3]{};int activeWindObjects=0;struct{float currentVelocity[3]{};}windSlots[10];};
#include "../../shared/rd-rend2/tr_jar_weather.inc"
enum {UNIFORM_JAR_WEATHER_COUNT,UNIFORM_JAR_WEATHER_ZONES};
struct UniformDataWriter{int count=-1;std::vector<float> data;void SetUniformInt(int,int n){count=n;}void SetUniformVec4(int,const float*p,size_t n){CHECK(n<=160);data.assign(p,p+4*n);}};
#define REND2_SP
#include "../../shared/rd-rend2/tr_weather_uniforms.inc"
#undef REND2_SP
}
namespace buffers {
using byte=unsigned char;constexpr int CHUNK_COUNT=9,HALF_CHUNK_EXTENDS=1000;
enum {VBO_USAGE_XFB,ATTR_INDEX_POSITION,ATTR_INDEX_COLOR,GL_FLOAT};
struct rainVertex_t{float position[3],velocity[3];};
struct VBO_t{std::vector<rainVertex_t> data;bool initialized;};
struct attribute{int index,numComponents,type;size_t offset,stride;void*vbo;};
struct weatherObject_t{VBO_t*lastVBO,*vbo;unsigned vboLastUpdateFrame;attribute attribsTemplate[2];};
struct World{struct{float bounds[2][3]={{-100,-100,-100},{100,100,100}};}bmodels[1];}world;
struct{World*world=&buffers::world;}tr;
float Q_flrand(float lo,float hi){return (lo+hi)*.5f;}
static std::deque<VBO_t> allocations;
VBO_t*R_CreateVBO(byte*data,size_t bytes,int,const char*){CHECK(bytes%sizeof(rainVertex_t)==0);allocations.emplace_back();auto&b=allocations.back();b.initialized=data!=nullptr;b.data.resize(bytes/sizeof(rainVertex_t));if(data)memcpy(b.data.data(),data,bytes);return &b;}
#include "../../shared/rd-rend2/tr_weather_particles.inc"
}
int main(){return RunTests("native Classic and Rend2 weather adapters preserve authored slots and bound allocation",[]{
 jar::weather::Frame baseline;float direction[3]={1,0,0},velocity[3]={1000,0,0},query[3];jar::weather::ClassicWindBase(baseline,direction,velocity,query);CHECK(query[0]==1&&query[1]==0);baseline.wind[1]=500;jar::weather::ClassicWindBase(baseline,direction,velocity,query);CHECK(query[0]==1000&&query[1]==500);baseline={};jar::weather::ClassicWindBase(baseline,direction,velocity,query);CHECK(query[0]==1&&query[1]==0);
 buffers::weatherObject_t gpu{};buffers::GenerateRainModel(gpu,jar::weather::particleLimit,true);CHECK(buffers::allocations.size()==2&&gpu.lastVBO->initialized&&gpu.vbo->initialized&&gpu.lastVBO->data.size()==36000);
 for(const auto&b:buffers::allocations)for(const auto&v:b.data)for(int a=0;a<3;++a)CHECK(std::isfinite(v.position[a])&&v.velocity[a]==0);
 buffers::weatherObject_t authored{};buffers::GenerateRainModel(authored,100);CHECK(!authored.lastVBO->initialized&&authored.vbo->initialized&&authored.vbo->data.size()==900);
 jar::weather::Frame f;f.layers[0].count=1000;f.layers[1].count=1000;f.layers[2].count=200;
 classic::JAR_WeatherApply(f);CHECK(classic::jarWeatherClouds[0].vertices==3&&classic::jarWeatherClouds[0].mHeight==80&&classic::jarWeatherClouds[0].mWaterParticles);CHECK(classic::jarWeatherClouds[1].texture=="gfx/effects/snowflake1.tga");
 rend2::weatherSystem_t ws;ws.weatherSlots[0].particleCount=123;ws.weatherSlots[0].active=true;ws.constWindDirection[0]=.25f;ws.sandboxSlots[0].vbo=reinterpret_cast<void*>(2);
 rend2::JAR_WeatherApply(ws,f);CHECK(ws.weatherSlots[0].particleCount==123&&ws.sandboxSlots[0].particleCount==1000&&ws.sandboxSlots[0].size[1]==14&&ws.sandboxSlots[0].vbo==reinterpret_cast<void*>(2)&&rend2::flags==6);
 rend2::tr.weatherSystem=&ws;ws.sandboxZones.count=32;for(int i=0;i<32;++i)for(int a=0;a<20;++a)ws.sandboxZones.values[i].v[a]=float(i*20+a);rend2::UniformDataWriter uniforms;rend2::JAR_WeatherUniforms(uniforms);CHECK(uniforms.count==32&&uniforms.data.size()==640&&uniforms.data.back()==639);ws.sandboxZones={};rend2::UniformDataWriter emptyUniforms;rend2::JAR_WeatherUniforms(emptyUniforms);CHECK(emptyUniforms.count==0&&emptyUniforms.data.empty());
 const auto images=rend2::images;f.wind[0]=2000;for(int i=0;i<20;++i){f.wind[1]=float(i);classic::JAR_WeatherApply(f);rend2::JAR_WeatherApply(ws,f);}CHECK(rend2::images==images&&classic::jarWeatherClouds[0].allocations==1);
 vec3_t wind;rend2::JAR_WeatherWind(ws,wind);CHECK(wind[0]==2.25f&&std::fabs(wind[1]-.019f)<.00001f);
 f.layers[2].mode=1;classic::JAR_WeatherApply(f);rend2::JAR_WeatherApply(ws,f);CHECK(classic::jarWeatherClouds[2].mBlendMode==0&&classic::jarWeatherClouds[0].allocations==1&&ws.sandboxSlots[2].color[0]==.9f);
 f={};classic::JAR_WeatherApply(f);rend2::JAR_WeatherApply(ws,f);CHECK(!ws.sandboxSlots[0].active&&ws.weatherSlots[0].active&&ws.weatherSlots[0].particleCount==123&&classic::jarWeatherClouds[0].count==0);rend2::JAR_WeatherWind(ws,wind);CHECK(wind[0]==.25f);
 classic::JAR_WeatherReset();CHECK(!jar::weather::Active(classic::jarWeather));
 });}
