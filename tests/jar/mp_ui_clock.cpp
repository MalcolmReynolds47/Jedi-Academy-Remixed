// Execute native MP surface completion with captured drawing callbacks.
#include "rd-vanilla/tr_local.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
void QDECL Com_Error(int,const char*,...){std::abort();}
refimport_t ri{};backEndState_t backEnd{};trGlobals_t tr{};shaderCommands_t tess{};
int skyboxportal,drawskyboxportal;
static cvar_t motion{},debugSort{},drawfog{},showtris{},shownormals{};
cvar_t *r_debugSort=&debugSort,*r_drawfog=&drawfog,*r_showtris=&showtris,*r_shownormals=&shownormals;
static shader_t material{},shadow{};static int draws,shadows;static bool frozen;
static cvar_t* Get(const char*n,const char*v,uint32_t flags,const char*){assert(!strcmp(n,"jar_uiMotion")&&!strcmp(v,"1")&&flags==CVAR_ARCHIVE);return &motion;}
static void Draw(){++draws;assert(backEnd.refdef.floatTime==(frozen?0:7.5f)&&backEnd.refdef.time==(frozen?0:7500)&&tess.shaderTime==(frozen?0:6.25f));}
void RB_ShadowTessEnd(){++shadows;Draw();}
void RB_StageIteratorSky(){Draw();}
static void DrawTris(shaderCommands_t*){Draw();}static void DrawNormals(shaderCommands_t*){Draw();}
#include "../../codemp/rd-vanilla/tr_jar_end_surface.inc"
static void Reset(){backEnd.refdef.floatTime=7.5f;backEnd.refdef.time=7500;tess.shaderTime=6.25f;tess.numIndexes=3;tess.numVertexes=3;tess.numPasses=1;tess.shader=&material;tess.currentStageIteratorFunc=Draw;}
static void Run(){RB_EndSurface();assert(backEnd.refdef.floatTime==7.5f&&backEnd.refdef.time==7500&&tess.shaderTime==6.25f);}
int main(){
 ri.Cvar_Get=Get;tr.shadowShader=&shadow;
 for(int twoD=0;twoD<=1;++twoD)for(int enabled=0;enabled<=1;++enabled){Reset();backEnd.projection2D=(qboolean)twoD;motion.integer=enabled;frozen=twoD&&!enabled;int before=draws;Run();assert(draws==before+1&&!tess.numIndexes);}
 backEnd.projection2D=qtrue;motion.integer=0;frozen=true;
 Reset();tess.numIndexes=0;int before=draws;Run();assert(draws==before);
 Reset();tess.shader=&shadow;Run();assert(shadows==1);
 Reset();debugSort.integer=1;material.sort=2;before=draws;Run();assert(draws==before);debugSort.integer=0;
 Reset();skyboxportal=1;tess.currentStageIteratorFunc=RB_StageIteratorSky;before=draws;Run();assert(draws==before);skyboxportal=0;
 Reset();showtris.integer=1;shownormals.integer=1;before=draws;Run();assert(draws==before+3);showtris.integer=shownormals.integer=0;
 // SP's classic renderer uses the same scope without a separate shader clock.
 float seconds=4.5f;int millis=4500;{JAR_UIClock a(true,seconds,millis);assert(seconds==0&&millis==0);{JAR_UIClock b(false,seconds,millis);assert(seconds==0&&millis==0);}assert(seconds==0&&millis==0);}assert(seconds==4.5f&&millis==4500);
 puts("MP UI clock: native draw, world/normal preservation, shader clock, empty/debug/sky/shadow returns and scoped restoration passed.");
}
