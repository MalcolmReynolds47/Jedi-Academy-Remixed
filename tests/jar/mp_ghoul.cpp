// Production posed-vertex traversal against deterministic bone-cache results.
#include "rd-vanilla/tr_local.h"
#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"
#include "test_support.h"
#include <array>

static mdxaBone_t bones[2];static mdxmSurface_t* surfaces[3];
struct EmptyPool:IGhoul2InfoArray {
 std::vector<CGhoul2Info> entries;
 int New() override{entries.clear();return 1;}
 void Delete(int) override{entries.clear();}
 bool IsValid(int handle)const override{return handle==1;}
 std::vector<CGhoul2Info>& Get(int)override{return entries;}
 const std::vector<CGhoul2Info>& Get(int)const override{return entries;}
} pool;
IGhoul2InfoArray &TheGhoul2InfoArray(){return pool;}
int G2API_GetTime(int time){return time;}
qboolean G2_SetupModelPointers(CGhoul2Info_v&g){return g.size()==1?qtrue:qfalse;}
static int boneResets;
void RemoveBoneCache(CBoneCache*){++boneResets;}
void G2_ConstructGhoulSkeleton(CGhoul2Info_v&g,int,bool,const vec3_t){g[0].mBoneCache=reinterpret_cast<CBoneCache*>(uintptr_t(1));}
const mdxaBone_t& EvalBoneCache(int i,CBoneCache*){CHECK(i>=0&&i<2);return bones[i];}
void* G2_FindSurface(void*,int i,int lod){CHECK(lod==0&&i>=0&&i<3);return surfaces[i];}
surfaceInfo_t* G2_FindOverrideSurface(int i,surfaceInfo_v& list){for(auto&v:list)if(v.surface==i)return &v;return nullptr;}
static int poseCalls,failRoot=-1;
qboolean G2API_SetBoneAnim(CGhoul2Info_v&g,int model,const char*name,int start,int end,int flags,float speed,int time,float setFrame,int blend){
 CHECK(model==0&&end==start+1&&flags==BONE_ANIM_OVERRIDE_FREEZE&&speed==1&&time==0&&setFrame==-1&&blend==0);
 const int root=!strcmp(name,"root_b")?1:0;++poseCalls;if(failRoot==root)return qfalse;boneInfo_t state{};state.boneNumber=root;state.startFrame=start;g[0].mBlist.push_back(state);return qtrue;
}
#include "rd-common/jar_ghoul_pose.inc"
void QDECL Com_Error(int,const char*,...){throw std::runtime_error("Com_Error");}
void QDECL Com_Printf(const char*,...){}
void QDECL Com_DPrintf(const char*,...){}
int QDECL Com_sprintf(char*b,int size,const char*f,...){va_list a;va_start(a,f);int n=vsnprintf(b,size,f,a);va_end(a);return n;}

int main(){return RunTests("posed Ghoul2 visible surfaces, weighted bones and bounded geometry",[]{
 CGhoul2Info_v empty;vec3_t emptyLow{},emptyHigh{};CHECK(!G2API_JAR_ModelBounds(empty,0,emptyLow,emptyHigh));
 std::array<byte,4096> data{};auto*h=reinterpret_cast<mdxmHeader_t*>(data.data());h->ofsEnd=int(data.size());h->numSurfaces=3;
 auto*offsets=reinterpret_cast<mdxmHierarchyOffsets_t*>(h+1);mdxmSurfHierarchy_t*info[3];mdxmVertex_t*verts[3];int cursor=sizeof(*h)+3*sizeof(int);
 for(int i=0;i<3;++i){offsets->offsets[i]=cursor-sizeof(*h);info[i]=reinterpret_cast<mdxmSurfHierarchy_t*>(data.data()+cursor);cursor+=sizeof(mdxmSurfHierarchy_t)+2*sizeof(int);}
 info[0]->numChildren=2;info[0]->childIndexes[0]=1;info[0]->childIndexes[1]=2;info[1]->flags=G2SURFACEFLAG_OFF;info[2]->flags=G2SURFACEFLAG_ISBOLT;
 for(int i=0;i<3;++i){auto*s=surfaces[i]=reinterpret_cast<mdxmSurface_t*>(data.data()+cursor);s->thisSurfaceIndex=i;s->numVerts=2;s->numBoneReferences=2;s->ofsBoneReferences=sizeof(*s);s->ofsVerts=sizeof(*s)+2*sizeof(int);
  auto*refs=reinterpret_cast<int*>(reinterpret_cast<byte*>(s)+s->ofsBoneReferences);refs[0]=0;refs[1]=1;
  verts[i]=reinterpret_cast<mdxmVertex_t*>(reinterpret_cast<byte*>(s)+s->ofsVerts);cursor+=s->ofsVerts+2*sizeof(mdxmVertex_t);
  VectorSet(verts[i][0].vertCoords,float(i?500:-2),-3,-4);VectorSet(verts[i][1].vertCoords,5,6,7);
 }
 for(int k=0;k<2;++k)for(int a=0;a<3;++a)bones[k].matrix[a][a]=1;
 bones[0].matrix[0][3]=10;bones[1].matrix[0][3]=20;
 model_t model{};model.mdxm=h;mdxaHeader_t anim{};anim.numBones=2;
 CGhoul2Info g;g.mValid=true;g.currentModel=&model;g.aHeader=&anim;g.mBoneCache=reinterpret_cast<CBoneCache*>(uintptr_t(1));
 vec3_t low{},high{};CHECK(JAR_GhoulSurfaceBounds(g,low,high));CHECK(low[0]==8&&high[0]==15&&low[2]==-4&&high[2]==7);
 // Two equal-weight influences produce a different pose than the bind vertices.
 auto&v=verts[0][1];v.uiNmWeightsAndBoneIndexes=(1u<<30)|(1u<<5);v.BoneWeightings[0]=255;
 CHECK(JAR_GhoulSurfaceBounds(g,low,high));float total=0;const float w=G2_GetVertBoneWeight(&v,0,total,2);CHECK(std::fabs(high[0]-(5+10*w+20*(1-w)))<.001f);
 surfaceInfo_t overrideSurface{};overrideSurface.surface=1;overrideSurface.offFlags=0;g.mSlist.push_back(overrideSurface);
 CHECK(JAR_GhoulSurfaceBounds(g,low,high)&&high[0]==510);g.mSlist.clear();
 // Hidden roots still traverse visible children unless descendants are disabled.
 info[0]->flags=G2SURFACEFLAG_OFF;info[1]->flags=0;CHECK(JAR_GhoulSurfaceBounds(g,low,high)&&low[0]==15&&high[0]==510);
 info[0]->flags|=G2SURFACEFLAG_NODESCENDANTS;VectorSet(low,1,2,3);CHECK(!JAR_GhoulSurfaceBounds(g,low,high)&&low[0]==1);
 info[0]->flags=0;info[1]->flags=G2SURFACEFLAG_OFF;
 // Failure is transactional: no partial hull or unbounded traversal escapes.
 verts[0][0].vertCoords[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!JAR_GhoulSurfaceBounds(g,low,high)&&low[0]==1);verts[0][0].vertCoords[0]=-2;
 auto saved=v.uiNmWeightsAndBoneIndexes;v.uiNmWeightsAndBoneIndexes=31;CHECK(!JAR_GhoulSurfaceBounds(g,low,high));v.uiNmWeightsAndBoneIndexes=saved;
 const int offset=surfaces[0]->ofsVerts;surfaces[0]->ofsVerts=h->ofsEnd;CHECK(!JAR_GhoulSurfaceBounds(g,low,high));surfaces[0]->ofsVerts=offset;
 info[0]->childIndexes[0]=0;CHECK(!JAR_GhoulSurfaceBounds(g,low,high));info[0]->childIndexes[0]=1;
 verts[0][0].vertCoords[0]=9000;CHECK(!JAR_GhoulSurfaceBounds(g,low,high));verts[0][0].vertCoords[0]=-2;
 CHECK(JAR_GhoulSurfaceBounds(g,low,high));
 // Two independent roots must both freeze; setter/geometry failures restore prior overrides.
 std::array<byte,1024> skeleton{};auto*ah=reinterpret_cast<mdxaHeader_t*>(skeleton.data());ah->ident=MDXA_IDENT;ah->numFrames=4;ah->numBones=2;ah->ofsEnd=skeleton.size();
 int*bo=reinterpret_cast<int*>(ah+1);mdxaSkel_t*rootBones[2];for(int i=0;i<2;++i){bo[i]=8+i*sizeof(mdxaSkel_t);rootBones[i]=reinterpret_cast<mdxaSkel_t*>(reinterpret_cast<byte*>(bo)+bo[i]);strcpy(rootBones[i]->name,i?"root_b":"root_a");rootBones[i]->parent=-1;}
 CGhoul2Info_v posed;g.aHeader=ah;posed.push_back(g);int frames=0;
 CHECK(G2API_JAR_PoseBounds(posed,3,&frames,low,high)&&frames==4&&poseCalls==2&&boneResets==1&&posed[0].mBlist.size()==2&&posed[0].mBlist[1].startFrame==3);
 const auto previous=posed[0].mBlist;failRoot=1;CHECK(!G2API_JAR_PoseBounds(posed,1,&frames,low,high)&&posed[0].mBlist.size()==previous.size()&&posed[0].mBlist[0].startFrame==3);failRoot=-1;
 int calls=poseCalls;CHECK(!G2API_JAR_PoseBounds(posed,4,&frames,low,high)&&frames==4&&poseCalls==calls);
 rootBones[1]->parent=99;CHECK(!G2API_JAR_PoseBounds(posed,0,&frames,low,high)&&poseCalls==calls);rootBones[1]->parent=-1;
 verts[0][0].vertCoords[0]=9000;CHECK(!G2API_JAR_PoseBounds(posed,1,&frames,low,high)&&posed[0].mBlist.size()==previous.size()&&posed[0].mBlist[0].startFrame==3);verts[0][0].vertCoords[0]=-2;
 g.mBoneCache=nullptr;posed[0].mBoneCache=nullptr;
 });}
