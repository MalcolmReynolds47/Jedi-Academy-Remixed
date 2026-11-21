// Compile/link real fog shaders in an owned hidden WGL context. No game client.
#include "tr_local.h"
#include "test_support.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <windows.h>

template<class T>static T Proc(const char *name){auto p=wglGetProcAddress(name);CHECK(p);return reinterpret_cast<T>(p);}
struct Context {
 HWND window=nullptr;HDC dc=nullptr;HGLRC gl=nullptr;
 Context(){
  window=CreateWindowExA(0,"STATIC","JA:R shader qualification",0,0,0,32,32,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);CHECK(window);
  dc=GetDC(window);CHECK(dc);PIXELFORMATDESCRIPTOR pfd{};pfd.nSize=sizeof(pfd);pfd.nVersion=1;pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=24;pfd.cDepthBits=24;
  int pixel=ChoosePixelFormat(dc,&pfd);CHECK(pixel&&SetPixelFormat(dc,pixel,&pfd));gl=wglCreateContext(dc);CHECK(gl&&wglMakeCurrent(dc,gl));
 }
 ~Context(){wglMakeCurrent(nullptr,nullptr);if(gl)wglDeleteContext(gl);if(dc)ReleaseDC(window,dc);if(window)DestroyWindow(window);}
};
static std::string header(){
 std::ostringstream s;s<<"#version 330 core\n#define M_PI 3.14159265358979323846\n#define USE_ALPHA_TEST\n#define MAX_GPU_FOGS "<<MAX_GPU_FOGS<<"\n#define MAX_G2_BONES "<<MAX_G2_BONES<<"\n#define r_volumetricFogSamples 8\n#define r_FBufScale vec2(32.0,32.0)\n";
#define DEFINE_ENUM(n) s<<"#define " #n " "<<int(n)<<"\n"
 DEFINE_ENUM(DEFORM_WAVE);DEFINE_ENUM(DEFORM_BULGE);DEFINE_ENUM(DEFORM_BULGE_UNIFORM);DEFINE_ENUM(DEFORM_MOVE);DEFINE_ENUM(DEFORM_NORMALS);DEFINE_ENUM(DEFORM_PROJECTION_SHADOW);
 DEFINE_ENUM(TCGEN_LIGHTMAP);DEFINE_ENUM(TCGEN_LIGHTMAP1);DEFINE_ENUM(TCGEN_LIGHTMAP2);DEFINE_ENUM(TCGEN_LIGHTMAP3);DEFINE_ENUM(TCGEN_TEXTURE);DEFINE_ENUM(TCGEN_ENVIRONMENT_MAPPED);DEFINE_ENUM(TCGEN_ENVIRONMENT_MAPPED_SP);DEFINE_ENUM(TCGEN_ENVIRONMENT_MAPPED_SP_FP);DEFINE_ENUM(TCGEN_FOG);DEFINE_ENUM(TCGEN_VECTOR);
 DEFINE_ENUM(CGEN_LIGHTING_DIFFUSE);DEFINE_ENUM(CGEN_DISINTEGRATION_1);DEFINE_ENUM(CGEN_DISINTEGRATION_2);DEFINE_ENUM(AGEN_LIGHTING_SPECULAR);DEFINE_ENUM(AGEN_LIGHTING_SPECULAR_STATIC);DEFINE_ENUM(AGEN_PORTAL);
 DEFINE_ENUM(ALPHA_TEST_GT0);DEFINE_ENUM(ALPHA_TEST_LT128);DEFINE_ENUM(ALPHA_TEST_GE128);DEFINE_ENUM(ALPHA_TEST_GE192);DEFINE_ENUM(ALPHA_TEST_E255);
#undef DEFINE_ENUM
 s<<"#define WF_SIN "<<GF_SIN<<"\n#define WF_SQUARE "<<GF_SQUARE<<"\n#define WF_TRIANGLE "<<GF_TRIANGLE<<"\n#define WF_SAWTOOTH "<<GF_SAWTOOTH<<"\n#define WF_INVERSE_SAWTOOTH "<<GF_INVERSE_SAWTOOTH<<"\n";
 return s.str();
}
int main(int argc,char**argv){return RunTests("driver compilation/linking and std140 layout of Rend2 fog shader variants",[&]{
 CHECK(argc==2);Context context;printf("GL renderer: %s; version: %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
 auto createShader=Proc<PFNGLCREATESHADERPROC>("glCreateShader");auto shaderSource=Proc<PFNGLSHADERSOURCEPROC>("glShaderSource");auto compileShader=Proc<PFNGLCOMPILESHADERPROC>("glCompileShader");
 auto getShaderiv=Proc<PFNGLGETSHADERIVPROC>("glGetShaderiv");auto getShaderInfoLog=Proc<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");auto deleteShader=Proc<PFNGLDELETESHADERPROC>("glDeleteShader");
 auto createProgram=Proc<PFNGLCREATEPROGRAMPROC>("glCreateProgram");auto attachShader=Proc<PFNGLATTACHSHADERPROC>("glAttachShader");auto linkProgram=Proc<PFNGLLINKPROGRAMPROC>("glLinkProgram");
 auto getProgramiv=Proc<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");auto getProgramInfoLog=Proc<PFNGLGETPROGRAMINFOLOGPROC>("glGetProgramInfoLog");auto deleteProgram=Proc<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");
 auto getBlock=Proc<PFNGLGETUNIFORMBLOCKINDEXPROC>("glGetUniformBlockIndex");auto getBlockiv=Proc<PFNGLGETACTIVEUNIFORMBLOCKIVPROC>("glGetActiveUniformBlockiv");
 auto getIndices=Proc<PFNGLGETUNIFORMINDICESPROC>("glGetUniformIndices");auto getUniforms=Proc<PFNGLGETACTIVEUNIFORMSIVPROC>("glGetActiveUniformsiv");
 int programs=0;
 for(const char *name:{"fogpass","generic","surface_sprites"}){
  std::ifstream input(std::string(argv[1])+"/"+name+".glsl");CHECK(input.good());std::string source((std::istreambuf_iterator<char>(input)),{});
  const std::string vertexMarker="/*[Vertex]*/",fragmentMarker="/*[Fragment]*/";size_t split=source.find(fragmentMarker);CHECK(split!=std::string::npos);
  std::string stages[2]={source.substr(source.find(vertexMarker)+vertexMarker.size(),split-vertexMarker.size()),source.substr(split+fragmentMarker.size())};
  for(int variant=0;variant<8;++variant){
   std::string flags="#define USE_FOG\n";
   if(variant&1)flags+="#define USE_VOLUMETRIC_FOG\n";
   if(std::string(name)=="fogpass"){
    if(variant&2)flags+="#define USE_FALLBACK_GLOBAL_FOG\n";
    if(variant&4)flags+="#define USE_DEFORM_VERTEXES\n#define USE_SKELETAL_ANIMATION\n";
   }else if(std::string(name)=="generic"){
    if(variant&2)flags+="#define USE_TCGEN\n#define USE_TCMOD\n#define USE_RGBAGEN\n";
    if(variant&4)flags+="#define USE_DEFORM_VERTEXES\n#define USE_VERTEX_ANIMATION\n";
   }else{
    if(variant&2)flags+="#define FACE_CAMERA\n#define FX_SPRITE\n";
    if(variant&4)flags+="#define VELOCITY_PASS\n";
   }
   GLuint program=createProgram();
   for(int i=0;i<2;++i){GLuint shader=createShader(i==0?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);std::string text=header()+flags+stages[i];const char*p=text.c_str();shaderSource(shader,1,&p,nullptr);compileShader(shader);GLint ok=0;getShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){char log[8192];getShaderInfoLog(shader,sizeof(log),nullptr,log);throw std::runtime_error(std::string(name)+" variant "+std::to_string(variant)+" stage "+std::to_string(i)+": "+log);}
    attachShader(program,shader);deleteShader(shader);
   }
   linkProgram(program);GLint linked=0;getProgramiv(program,GL_LINK_STATUS,&linked);
   if(!linked){char log[8192];getProgramInfoLog(program,sizeof(log),nullptr,log);throw std::runtime_error(std::string(name)+": "+log);}
   GLuint block=getBlock(program,"Fogs");CHECK(block!=GL_INVALID_INDEX);GLint blockSize=0;getBlockiv(program,block,GL_UNIFORM_BLOCK_DATA_SIZE,&blockSize);CHECK(blockSize==sizeof(FogsBlock));
   if(variant&1){
    const char*names[]={"u_FogWorldToLightGrid","u_FogWorldToGlobalLightGrid","u_Fogs[0].plane"};GLuint indices[3];getIndices(program,3,names,indices);for(auto idx:indices)CHECK(idx!=GL_INVALID_INDEX);GLint offsets[3];getUniforms(program,3,indices,GL_UNIFORM_OFFSET,offsets);CHECK(offsets[0]==16&&offsets[1]==80&&offsets[2]==144);
   }
   deleteProgram(program);++programs;
  }
 }
 CHECK(programs==24);printf("Compiled and linked %d real shader variants; fog UBO layouts match C++. No scene drawn.\n",programs);
 });}
