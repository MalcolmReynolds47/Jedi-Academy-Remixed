// The exact classic BSP registration path; file access and GPU loading mocked.
#include "qcommon/q_shared.h"
#include "test_support.h"
#include "../../shared/qcommon/jar_yavin_sky.h"
#include <map>
#include <string>
#include <vector>
enum{MOD_BAD,MOD_BRUSH};struct model_t{int index=0,type=MOD_BRUSH;};
struct world_t{char name[MAX_QPATH]{};};
struct{int numBSPModels=0;world_t bspModels[MAX_SUB_BSP];model_t*models[512]{};}tr;
struct modelHash_t{modelHash_t*next=nullptr;int handle=0;char name[MAX_QPATH]{};};
constexpr int FILE_HASH_SIZE=1;static modelHash_t*mhHashTable[1]{};
static std::vector<modelHash_t*>hashes;static std::vector<model_t*>models;static int readCount=0,loadCount=0,flushCount=0;
static long generateHashValue(const char*,int){return 0;}
void RE_InsertModelIntoHash(const char*name,model_t*model){auto*h=new modelHash_t;Q_strncpyz(h->name,name,MAX_QPATH);h->handle=model->index;h->next=mhHashTable[0];mhHashTable[0]=h;hashes.push_back(h);}
void R_IssuePendingRenderCommands(){++flushCount;}
void RE_LoadWorldMap_Actual(const char*name,world_t&world,int index){++loadCount;Q_strncpyz(world.name,name,MAX_QPATH);
 for(int n=0;n<3;++n){auto*m=new model_t;m->index=int(models.size())+1;models.push_back(m);tr.models[m->index]=m;char key[MAX_QPATH];Com_sprintf(key,sizeof(key),"*%d-%d",index,n);RE_InsertModelIntoHash(key,m);}}
static int Read(const char*name,void**out){CHECK(!out);++readCount;return strstr(name,"missing")?-1:500;}
struct{int(*FS_ReadFile)(const char*,void**)=Read;}ri;
#include "../../code/rd-common/tr_bsp_register.inc"
#include "../../code/rd-common/tr_bsp_far.h"
void reset(){for(auto*p:hashes)delete p;hashes.clear();for(auto*p:models)delete p;models.clear();mhHashTable[0]=nullptr;tr={};}

static void ClassicBSPFarDepth(){
 const vec3_t bounds[2]={{-100,-200,-50},{100,200,50}};
 const vec3_t identity[3]={{1,0,0},{0,1,0},{0,0,1}};
 const vec3_t rotated[3]={{0,1,0},{-1,0,0},{0,0,1}};
 const vec3_t eye={0,0,0},forward={1,0,0};
 const vec3_t ahead={2800,0,0},behind={-2800,0,0},offside={2800,5000,0};
 cplane_t sides[4]={};
 VectorSet(sides[0].normal,1,1,0);
 VectorSet(sides[1].normal,1,-1,0);
 VectorSet(sides[2].normal,1,0,1);
 VectorSet(sides[3].normal,1,0,-1);
 constexpr float hostFar=2048;
 const float roomFar=R_BSPFarDepth(bounds,ahead,rotated,eye,forward,sides);
 CHECK(roomFar>hostFar&&roomFar==3001);
 CHECK(R_BSPFarDepth(bounds,ahead,identity,eye,forward,sides)==2901);
 CHECK(R_BSPFarDepth(bounds,behind,identity,eye,forward,sides)==0);
 CHECK(R_BSPFarDepth(bounds,offside,identity,eye,forward,sides)==0);
 CHECK(R_BSPFarDepth(bounds,offside,identity,eye,forward,nullptr)==2901);
 CHECK(R_BSPFarDepth(bounds,behind,identity,eye,forward,nullptr)==0);
}

namespace mpRend2 {
enum { CULL_IN, CULL_OUT, VPF_DEPTHSHADOW = 1 };
enum { RF_JAR_SECTION_ARENA = 0x08000000, RF_JAR_SECTION_BACKDROP = 0x10000000 };
struct trRefEntity_t {
 struct { int hModel; vec3_t origin; int renderfx; } e;
 int needDlights;
};
struct msurface_t {};
struct bmodel_t { vec3_t bounds[2]; int worldIndex, firstSurface, numSurfaces; };
struct model_t { struct { bmodel_t *bmodel; } data; };
struct world_t { msurface_t surfaces[jar::yavinSky::surfaceCount + 1]; int surfacesViewCount[4]; };
static struct { struct { int flags; } viewParms; int viewCount; } tr{{0},17};
static bmodel_t brush{{},1,1,2};
static model_t model{{&brush}};
static world_t world{{},{0,17,0,0}};
static bool culled;
static int lightUpdates;
struct Submission { const msurface_t *surface; int entity; float x; int dlights; };
static std::vector<Submission> submissions;

static model_t *R_GetModelByHandle(int handle){CHECK(handle==5);return &model;}
static int R_CullLocalBox(const vec3_t *bounds){CHECK(bounds==brush.bounds);return culled?CULL_OUT:CULL_IN;}
static void R_DlightBmodel(bmodel_t *bmodel,trRefEntity_t *ent){CHECK(bmodel==&brush);ent->needDlights=ent->e.origin[0]>0;++lightUpdates;}
static world_t *R_GetWorld(int worldIndex){CHECK(worldIndex==1);return &world;}
static void R_AddWorldSurface(msurface_t *surface,const trRefEntity_t *ent,int entity,int dlights,int pshadows){
 CHECK(pshadows==0);
 submissions.push_back({surface,entity,ent->e.origin[0],dlights});
}
#include "../../codemp/rd-rend2/tr_jar_brush_surfaces.inc"

static void TransformedBrushCopies(){
 trRefEntity_t first{{5,{-20,0,0}},0},second{{5,{40,0,0}},0};
 R_AddBrushModelSurfaces(&first,19);
 R_AddBrushModelSurfaces(&second,23);
 CHECK(submissions.size()==4&&lightUpdates==2);
 for(int i=0;i<2;++i){
  CHECK(submissions[i].surface==&world.surfaces[1+i]&&submissions[i].entity==19);
  CHECK(submissions[i].x==-20&&!submissions[i].dlights);
  CHECK(submissions[2+i].surface==submissions[i].surface&&submissions[2+i].entity==23);
  CHECK(submissions[2+i].x==40&&submissions[2+i].dlights==1);
 }
 CHECK(world.surfacesViewCount[1]==tr.viewCount&&world.surfacesViewCount[2]==0);

 culled=true;
 R_AddBrushModelSurfaces(&second,23);
 CHECK(submissions.size()==4&&lightUpdates==2);
 culled=false;
 tr.viewParms.flags=VPF_DEPTHSHADOW;
 R_AddBrushModelSurfaces(&second,23);
 CHECK(submissions.size()==6&&lightUpdates==2);
}

static void SectionBackdropSubmission(int surfaces,int backdropCount,bool (*isBackdrop)(int)){
 brush.numSurfaces=surfaces;
 trRefEntity_t room{{5,{-20,0,0},RF_JAR_SECTION_ARENA},0};
 trRefEntity_t sky{{5,{0,0,0},RF_JAR_SECTION_BACKDROP},0};
 submissions.clear();
 R_AddBrushModelSurfaces(&room,19);
 CHECK(submissions.size()==surfaces-backdropCount);
 R_AddBrushModelSurfaces(&sky,23);
 CHECK(submissions.size()==surfaces);
 std::vector<bool> seen(surfaces);
 for(const auto& draw:submissions){
  const int index=int(draw.surface-world.surfaces)-brush.firstSurface;
  CHECK(index>=0&&index<surfaces&&!seen[index]);
  CHECK((draw.entity==23)==isBackdrop(index));
  seen[index]=true;
 }
}
}

int main(){return RunTests("BSP renderer registration, Classic far depth and independent MP Rend2 brush submissions",[]{
 const auto root=JAR_RegisterBSPModel("#room");CHECK(root&&loadCount==1&&readCount==1&&flushCount==1);
 CHECK(JAR_RegisterBSPModel("#ROOM:0")==root&&loadCount==1&&tr.numBSPModels==1);
 CHECK(JAR_RegisterBSPModel("#room:2")==root+2&&loadCount==1);
 CHECK(!JAR_RegisterBSPModel("#room:99")&&loadCount==1);
 CHECK(!JAR_RegisterBSPModel("#../bad")&&readCount==1);
 CHECK(!JAR_RegisterBSPModel("#missing")&&tr.numBSPModels==1&&loadCount==1);
 CHECK(JAR_RegisterBSPModel("#another:1")&&tr.numBSPModels==2);
 for(int n=2;n<MAX_SUB_BSP;++n){char name[MAX_QPATH];Com_sprintf(name,sizeof(name),"#section%d",n);CHECK(JAR_RegisterBSPModel(name));}
 CHECK(tr.numBSPModels==MAX_SUB_BSP&&!JAR_RegisterBSPModel("#full"));CHECK(JAR_RegisterBSPModel("#room")==root);
 reset();CHECK(JAR_RegisterBSPModel("#room")&&tr.numBSPModels==1);reset();
 ClassicBSPFarDepth();
 mpRend2::TransformedBrushCopies();
 mpRend2::SectionBackdropSubmission(jar::hothSky::surfaceCount,105,jar::hothSky::BackdropSurface);
 mpRend2::SectionBackdropSubmission(jar::yavinSky::surfaceCount,9,jar::yavinSky::BackdropSurface);
 });}
