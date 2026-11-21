#include "cgame/cg_local.h"
#include "../../shared/qcommon/jar_galak_surfaces.h"
#include "test_support.h"
#include <map>

cgameImport_t imports{};cgameImport_t*trap=&imports;cg_t cg{};cgs_t cgs{};
static int draws,particles,sparks,debris;
static bool blocked;
static std::map<std::string,int> surfaces;
static refEntity_t beam;
const char*bgToggleableSurfaces[BG_NUM_TOGGLEABLE_SURFACES]={"vehicle_body",nullptr};
const int bgToggleableSurfaceDebris[BG_NUM_TOGGLEABLE_SURFACES]={1};
static void CG_CreateSurfaceDebris(centity_t*,int,int,qboolean){++debris;}
#define TURN_OFF 0x100
#define TURN_ON 0
static void UpdateSurfaces(centity_t*cent){
#include "cgame/cg_npc_surfaces.inc"
}
void CG_Trace(trace_t*t,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask){
 CHECK(pass==120&&mask==MASK_SHOT&&start[0]==100&&end[0]==1124);
 *t={};t->startsolid=blocked?qtrue:qfalse;t->fraction=.5f;VectorSet(t->endpos,612,200,300);t->plane.normal[0]=-1;
}
void BG_GiveMeVectorFromMatrix(mdxaBone_t*m,int mode,vec3_t v){for(int i=0;i<3;++i)v[i]=mode==ORIGIN?m->matrix[i][3]:-m->matrix[i][1];}
#include "cgame/cg_jar_galak.inc"
void QDECL Error(int,const char*,...){throw std::runtime_error("Com_Error");}
void QDECL Print(const char*,...){}
void(*Com_Error)(int,const char*,...)=Error;
void(*Com_Printf)(const char*,...)=Print;
int QDECL Com_sprintf(char*b,int size,const char*f,...){va_list a;va_start(a,f);int n=vsnprintf(b,size,f,a);va_end(a);return n;}
int main(){return RunTests("MP Galak client surface deltas and following beam",[]{
 centity_t cent{};cent.ghoul2=(void*)1;cent.currentState.number=120;cent.currentState.NPC_class=CLASS_GALAKMECH;
 trap->G2API_AddBolt=[](void*,int,const char*n){CHECK(!strcmp(n,"*flashc"));return 4;};
 trap->G2API_GetBoltMatrix=[](void*,int,int b,mdxaBone_t*m,const vec3_t,const vec3_t,int,qhandle_t*,vec3_t){CHECK(b==4);memset(m,0,sizeof(*m));m->matrix[0][3]=100;m->matrix[1][3]=200;m->matrix[2][3]=300;m->matrix[0][1]=-1;return qtrue;};
 trap->G2API_SetSurfaceOnOff=[](void*,const char*n,int f){surfaces[n]=f;return qtrue;};
 trap->R_RegisterShader=[](const char*n){CHECK(!strcmp(n,"gfx/misc/blueLine"));return 7;};
 trap->R_AddRefEntityToScene=[](const refEntity_t*e){beam=*e;++draws;};
 trap->FX_RegisterEffect=[](const char*n){return strstr(n,"warmup")?1:strstr(n,"follow")?2:3;};
 trap->FX_PlayBoltedEffectID=[](int id,vec3_t org,void*,int b,int e,int,int,qboolean relative){CHECK((id==1||id==2)&&org[0]==100&&b==4&&e==120&&relative);++particles;return qtrue;};
 trap->FX_PlayEffectID=[](int id,vec3_t org,vec3_t,int,int,qboolean){CHECK(id==3&&org[0]==612);++sparks;};
 cent.currentState.surfacesOn=(1<<14)|JAR_GALAK_CHARGING;cent.currentState.surfacesOff=1<<7;
 UpdateSurfaces(&cent);CHECK(!surfaces["torso_shield_off"]&&surfaces["torso_antenna"]==0x100&&!debris);
 cg.time=1000;CG_JAR_GalakEffects(&cent);CHECK(particles==1&&!draws&&!sparks);
 cent.currentState.surfacesOn=(1<<14)|JAR_GALAK_FIRING;
 cg.time=1100;CG_JAR_GalakEffects(&cent);CHECK(particles==2&&draws==1&&sparks==1&&beam.oldorigin[0]==612&&beam.radius==8&&beam.customShader==7);
 cg.time=1110;CG_JAR_GalakEffects(&cent);CHECK(particles==2&&draws==2&&sparks==1);
 blocked=true;cg.time=1200;CG_JAR_GalakEffects(&cent);CHECK(draws==2&&sparks==1);blocked=false;
 cent.currentState.surfacesOn=0;cent.currentState.surfacesOff|=1<<14;UpdateSurfaces(&cent);CHECK(surfaces["torso_shield_off"]==0x100);
 CG_JAR_GalakEffects(&cent);CHECK(draws==2);cent.currentState.surfacesOn=JAR_GALAK_FIRING;cent.currentState.eFlags=EF_DEAD;CG_JAR_GalakEffects(&cent);CHECK(draws==2);
 // A freshly observed entity immediately uses its current server phase.
 cent.currentState.eFlags=0;cg.time=2000;CG_JAR_GalakEffects(&cent);CHECK(draws==3);
 cent={};cent.ghoul2=(void*)1;cent.currentState.NPC_class=CLASS_VEHICLE;cent.currentState.surfacesOff=1;UpdateSurfaces(&cent);CHECK(debris==1&&surfaces["vehicle_body"]==0x100);
 CG_JAR_GalakEffects(&cent);CHECK(draws==3);
 });}
