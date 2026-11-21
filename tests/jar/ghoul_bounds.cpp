// Production posed-vertex traversal against deterministic bone-cache results.
#include "rd-vanilla/tr_local.h"
#include "ghoul2/G2.h"
#include "test_support.h"
#include <array>

static mdxaBone_t bones[2];static mdxmSurface_t* surfaces[3];
struct EmptyPool:IGhoul2InfoArray {
 std::vector<CGhoul2Info> entries;
 int New() override{throw std::runtime_error("Unexpected Ghoul2 allocation");}
 void Delete(int) override{throw std::runtime_error("Unexpected Ghoul2 deletion");}
 bool IsValid(int handle)const override{CHECK(handle==0);return false;}
 std::vector<CGhoul2Info>& Get(int)override{return entries;}
 const std::vector<CGhoul2Info>& Get(int)const override{return entries;}
} pool;
IGhoul2InfoArray &_TheGhoul2InfoArray(){return pool;}
int G2API_GetTime(int time){return time;}
qboolean G2_SetupModelPointers(CGhoul2Info_v&){return qfalse;}
void G2_ConstructGhoulSkeleton(CGhoul2Info_v&,int,bool,const vec3_t){throw std::runtime_error("Unexpected skeleton construction");}
const mdxaBone_t& EvalBoneCache(int i,CBoneCache*){CHECK(i>=0&&i<2);return bones[i];}
void* G2_FindSurface(const model_s*,int i,int lod){CHECK(lod==0&&i>=0&&i<3);return surfaces[i];}
const surfaceInfo_t* G2_FindOverrideSurface(int i,const surfaceInfo_v& list){for(const auto&v:list)if(v.surface==i)return &v;return nullptr;}
#include "rd-common/jar_ghoul_bounds.inc"

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
 std::vector<float> positions;
 CHECK(JAR_GhoulSurfaceBounds(g,low,high,&positions,[](void* context,const float* point){
  auto& result=*static_cast<std::vector<float>*>(context);result.push_back(point[0]);
 }));
 CHECK(positions.size()==2&&positions[0]==8&&positions[1]==15);
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
 CHECK(JAR_GhoulSurfaceBounds(g,low,high));g.mBoneCache=nullptr;
 });}
