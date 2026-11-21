#include "game/g_local.h"
#include "game/jar_physics.h"
#include "qcommon/qfiles.h"
#include "../../shared/qcommon/jar_md3_surface.h"
#include "rd-common/mdx_format.h"
#include <array>
#include "test_support.h"
#include <map>
#include <cmath>
#include <limits>

game_import_t gi{};
struct PosePool:IGhoul2InfoArray {
 std::map<int,std::vector<CGhoul2Info>> values;int next=1;
 int New()override{values[next]={};return next++;}void Delete(int n)override{CHECK(values.erase(n)==1);}
 bool IsValid(int n)const override{return values.count(n)!=0;}
 std::vector<CGhoul2Info>&Get(int n)override{return values.at(n);}
 const std::vector<CGhoul2Info>&Get(int n)const override{return values.at(n);}
} pool;
IGhoul2InfoArray &_TheGhoul2InfoArray(){return pool;}
static std::array<byte,2048> gla{};static bool failRoot=false;
static mdxaHeader_t* Header(){return reinterpret_cast<mdxaHeader_t*>(gla.data());}
struct Prop {unsigned id;const char*model;bool rigid=true;};
bool propValid(const Prop&){return true;}
#include "game/g_workshop_bounds.inc"
static int samples=0,cleans=0;static bool fail=false;static vec3_t modelLow={-2,-3,-4},modelHigh={6,9,12};
int main(){return RunTests("decorative placement, rotated hulls, pose caching and solver size eligibility",[]{
 auto*h=Header();h->ident=MDXA_IDENT;h->numFrames=10;h->numBones=2;h->ofsEnd=int(gla.size());
 auto*offsets=reinterpret_cast<int*>(h+1);
 for(int i=0;i<2;++i){offsets[i]=8+i*int(sizeof(mdxaSkel_t));auto*b=reinterpret_cast<mdxaSkel_t*>(reinterpret_cast<byte*>(h+1)+offsets[i]);b->parent=-1;Q_strncpyz(b->name,i?"second":"root",sizeof(b->name));}
 gi.G2API_InitGhoul2Model=[](CGhoul2Info_v&g,const char*,int,qhandle_t,qhandle_t,int,int){g.resize(1);g[0].aHeader=Header();return 0;};
 gi.G2API_GetGLAName=[](CGhoul2Info*)->char*{return const_cast<char*>("shape");};
 gi.G2API_SetBoneAnim=[](CGhoul2Info*g,const char*name,int start,int end,int flags,float speed,int,float set,int blend){
  CHECK(start>=0&&end==start+1&&end<=Header()->numFrames&&speed==0&&set==-1&&blend==0&&flags==(BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_NO_LERP));
  const int i=!strcmp(name,"second");if(failRoot&&i)return qfalse;g->mBlist.resize(2);g->mBlist[i].startFrame=start;g->mBlist[i].endFrame=end;return qtrue;
 };
 gi.G2API_CleanGhoul2Models=[](CGhoul2Info_v&g){++cleans;g.clear();};
 gi.G2API_JAR_ModelBounds=[](CGhoul2Info_v&g,int,vec3_t low,vec3_t high){++samples;CHECK(g[0].mBlist.size()==2&&g[0].mBlist[0].startFrame==g[0].mBlist[1].startFrame);VectorCopy(modelLow,low);VectorCopy(modelHigh,high);high[0]+=g[0].mBlist[0].startFrame*2;return fail?qfalse:qtrue;};
 Prop prop{1,"shape.glm"};vec3_t lo,hi,angle={0,90,0};CHECK(propHull(prop,vec3_origin,lo,hi));CHECK(lo[0]==-2&&hi[1]==9&&lo[2]==-4&&hi[2]==12);
 CHECK(propHull(prop,angle,lo,hi));CHECK(std::fabs(lo[0]+9)<.001f&&std::fabs(hi[0]-3)<.001f&&std::fabs(lo[1]+2)<.001f&&std::fabs(hi[1]-6)<.001f);
 CHECK(physicsEligible(prop)&&samples==1&&cleans==1);PhysicsBounds bounds;CHECK(physicalBounds(prop,bounds)&&bounds.center.x==2&&bounds.half.x==4);
 // The native hull and solver must use identical local-center/angle conventions.
 jar::physics::World world;CHECK(world.Start({{-1000,-1000,-100},{1000,-1000,-100},{1000,1000,-100},{-1000,-1000,-100},{1000,1000,-100},{-1000,1000,-100}}));
 jar::physics::Pose pose;pose.angles={25,65,30};CHECK(world.Prop(2,1,bounds.half,bounds.center,pose,false));
 CHECK(std::fabs(world.Mass(2)-5.625f)<.01f);jar::physics::Vec solverLow,solverHigh;CHECK(world.Bounds(2,solverLow,solverHigh));
 nativeVector(pose.angles,angle);CHECK(propHull(prop,angle,lo,hi));vec3_t expectedLow,expectedHigh;nativeVector(solverLow,expectedLow);nativeVector(solverHigh,expectedHigh);
 for(int i=0;i<3;++i){CHECK(std::fabs(lo[i]-expectedLow[i])<.01f);CHECK(std::fabs(hi[i]-expectedHigh[i])<.01f);}world.Stop();
 // Oversized but bounded models retain truthful native collision geometry.
 prop.id=2;modelHigh[0]=1200;CHECK(modelBounds(prop,bounds)&&!physicsEligible(prop));CHECK(propHull(prop,vec3_origin,lo,hi)&&hi[0]==1200);
 // Empty, invalid, nonfinite and changed models cannot produce a cached hull.
 prop.id=3;fail=true;CHECK(!modelBounds(prop,bounds));const int failedSamples=samples;fail=false;CHECK(!modelBounds(prop,bounds)&&samples==failedSamples);
 physicsBounds.erase({3,0});modelLow[0]=1300;CHECK(!modelBounds(prop,bounds));physicsBounds.erase({3,0});modelLow[0]=0;modelHigh[0]=std::numeric_limits<float>::infinity();CHECK(!modelBounds(prop,bounds));
 physicsBounds.erase({3,0});modelHigh[0]=0;modelLow[1]=modelHigh[1]=0;modelLow[2]=modelHigh[2]=0;CHECK(modelBounds(prop,bounds)&&bounds.half.x==.25f&&bounds.half.y==.25f);
 physicsBounds.clear();CHECK(cleans==samples);modelLow[0]=-2;modelHigh[0]=6;modelLow[1]=-3;modelHigh[1]=9;modelLow[2]=-4;modelHigh[2]=12;
 CHECK(propPoseFrames(prop)==10);CHECK(propHull(prop,vec3_origin,lo,hi,4)&&hi[0]==14);CHECK(propHull(prop,vec3_origin,lo,hi,0)&&hi[0]==6);
 const int sampled=samples;CHECK(propHull(prop,vec3_origin,lo,hi,4)&&samples==sampled);CHECK(!propHull(prop,vec3_origin,lo,hi,-1)&&!propHull(prop,vec3_origin,lo,hi,10));
 // Multiple root trees and failure on the second root roll back the first.
 CGhoul2Info_v instance;CHECK(gi.G2API_InitGhoul2Model(instance,"shape.glm",0,0,0,0,0)==0);CHECK(skeletonPose(instance,3));failRoot=true;CHECK(!skeletonPose(instance,4)&&instance[0].mBlist[0].startFrame==3&&instance[0].mBlist[1].startFrame==3);failRoot=false;
 offsets[1]=h->ofsEnd;int frames;std::vector<std::string> roots;CHECK(!skeletonPoseInfo(instance,frames,roots));offsets[1]=8+sizeof(mdxaSkel_t);gi.G2API_CleanGhoul2Models(instance);CHECK(pool.values.empty());
 // Real MD3 frame layout: selected frame bounds, truncated data and huge offsets.
 std::array<byte,sizeof(md3Header_t)+3*sizeof(md3Frame_t)> md3{};auto*mh=reinterpret_cast<md3Header_t*>(md3.data());mh->ident=MD3_IDENT;mh->version=MD3_VERSION;mh->numFrames=3;mh->ofsFrames=sizeof(*mh);mh->ofsEnd=int(md3.size());auto*mf=reinterpret_cast<md3Frame_t*>(mh+1);
 for(int i=0;i<3;++i){VectorSet(mf[i].bounds[0],-2,-3,-4);VectorSet(mf[i].bounds[1],6+float(i)*2,9,12);}
 const md3Frame_t*selected=nullptr;CHECK(md3PoseFrame(md3.data(),int(md3.size()),2,selected,frames)&&frames==3&&selected->bounds[1][0]==10);
 CHECK(!md3PoseFrame(md3.data(),int(md3.size())-1,2,selected,frames));mh->ofsFrames=INT_MAX;CHECK(!md3PoseFrame(md3.data(),int(md3.size()),0,selected,frames));mh->ofsFrames=sizeof(*mh);mh->numFrames=65537;CHECK(!md3PoseFrame(md3.data(),int(md3.size()),0,selected,frames));
 // A diagonal prism is far narrower than its rotated local bounding box.
 auto diagonal=std::make_shared<jar::physics::PropGeometry>();
 for(float height:{-8.f,8.f})for(float extent:{-64.f,64.f})for(float width:{-2.f,2.f})
  diagonal->Add({extent+width,extent-width,height});
 CHECK(diagonal->RigidHull()&&diagonal->RigidHull()==diagonal->RigidHull());
 CHECK(world.Start({{-1000,-1000,-100},{1000,-1000,-100},{1000,1000,-100},{-1000,-1000,-100},{1000,1000,-100},{-1000,1000,-100}}));
 pose.angles={0,-45,0};
 CHECK(world.Prop(3,3,{66,66,8},{},pose,false,nullptr,diagonal));
 CHECK(world.Prop(4,4,{66,66,8},{},pose,false));
 CHECK(world.Mass(3)>0&&world.Mass(3)<world.Mass(4)*.1f);
 CHECK(world.Bounds(3,solverLow,solverHigh)&&solverHigh.y<3&&solverLow.y> -3&&solverHigh.x>90);
 jar::physics::NativePropHull native;
 CHECK(native.Build(*diagonal,{}));
 auto hit=native.Trace({45,-45,-20},{45,-45,20},{-2,-2,-2},{2,2,2});
 CHECK(hit.fraction==1&&!hit.startSolid); // The old outer box blocked this corner.
 // The tip at (10,3) never wins a signed-axis or diagonal vertex sample.
 // Its visible overhang still belongs inside the intersection of the planes.
 jar::physics::PropGeometry overhang;
 const jar::physics::Vec profile[]={{10,0,0},{8,8,0},{0,10,0},{-10,0,0},{0,-10,0},{10,3,0}};
 for(const auto& p:profile)for(float z:{-2.f,2.f})overhang.Add({p.x,p.y,z});
 jar::physics::NativePropHull overhangHull;
 CHECK(overhangHull.Build(overhang,{}));
 const auto overhangHit=overhangHull.Trace({9.9f,3,6},{9.9f,3,-6},{},{});
 CHECK(!overhangHit.startSolid&&std::fabs(6-12*overhangHit.fraction-2.125f)<.001f);
 const auto clearCorner=overhangHull.Trace({9.9f,7,6},{9.9f,7,-6},{},{});
 CHECK(clearCorner.fraction==1&&!clearCorner.startSolid);
 // The visible edge from (10,3) to (8,8) is tighter than any support diagonal.
 CHECK(overhangHull.Trace({9.6f,5,6},{9.6f,5,-6},{},{}).fraction==1);
 jar::physics::PropGeometry denseOverhang;
 for(const auto& p:profile)for(float z:{-2.f,2.f})denseOverhang.Add({p.x,p.y,z});
 for(int i=0;i<JarSweepHull::modelPoints;++i)denseOverhang.Add({i*.125f,0,0});
 jar::physics::NativePropHull supportHull;
 CHECK(supportHull.Build(denseOverhang,{}));
 CHECK(supportHull.Trace({9.6f,5,6},{9.6f,5,-6},{},{}).fraction<1);
 hit=native.Trace({0,0,0},{0,0,0},{},{});
 CHECK(hit.startSolid&&hit.allSolid&&hit.fraction==0&&hit.depth>2.8f&&hit.depth<2.9f);
 hit=native.Trace({0,0,0},{0,0,40},{},{});
 CHECK(hit.startSolid&&!hit.allSolid&&hit.fraction==1&&hit.depth==0);
 CHECK(native.Build(*diagonal,{0,-45,0}));
 hit=native.Trace({60,0,40},{60,0,-40},{-2,-2,-24},{2,2,40});
 CHECK(!hit.startSolid&&hit.fraction>.097f&&hit.fraction<.099f&&hit.normal.z>.999f);
 CHECK(std::fabs(hit.planeDistance-8)<.001f&&std::fabs(hit.depth-72)<.001f);
 CHECK(native.Build(*diagonal,{0,45,0}));
 hit=native.Trace({60,0,40},{60,0,-40},{-2,-2,-24},{2,2,40});
 CHECK(!hit.startSolid&&hit.fraction==1);
 auto flat=std::make_shared<jar::physics::PropGeometry>();
 for(float x:{-8.f,8.f})for(float y:{-8.f,8.f})flat->Add({x,y,0});
 CHECK(!flat->RigidHull());
 CHECK(!native.Build(*flat,{})&&native.Trace({},{},{},{}).fraction==1);
 CHECK(world.Prop(5,5,{8,8,.25f},{},pose,false,nullptr,flat)&&world.Mass(5)>0);
 world.Stop();
 // Read the selected MD3 frame and ignore a vertex absent from the triangles.
 struct Mesh {
  md3Header_t header;md3Frame_t frames[2];md3Surface_t surface;
  md3Triangle_t triangle;md3XyzNormal_t vertices[2][4];
 } mesh{};
 mesh.header.ident=MD3_IDENT;mesh.header.version=MD3_VERSION;mesh.header.numFrames=2;
 mesh.header.ofsEnd=sizeof(mesh);mesh.header.numSurfaces=1;mesh.header.ofsSurfaces=offsetof(Mesh,surface);
 mesh.surface.ident=MD3_IDENT;mesh.surface.numFrames=2;mesh.surface.numVerts=4;mesh.surface.numTriangles=1;
 mesh.surface.ofsTriangles=offsetof(Mesh,triangle)-offsetof(Mesh,surface);
 mesh.surface.ofsXyzNormals=offsetof(Mesh,vertices)-offsetof(Mesh,surface);
 mesh.surface.ofsEnd=sizeof(mesh)-offsetof(Mesh,surface);
 mesh.triangle.indexes[1]=1;mesh.triangle.indexes[2]=2;
 mesh.vertices[0][0].xyz[0]=64;mesh.vertices[1][0].xyz[0]=128;mesh.vertices[1][3].xyz[0]=32767;
 for(auto& vertices:mesh.vertices){vertices[1].xyz[1]=128;vertices[2].xyz[2]=128;}
 jar::physics::PropGeometry frameZero,frameOne;
 CHECK(md3PoseGeometry(mesh.header,0,2,frameZero)&&md3PoseGeometry(mesh.header,1,2,frameOne));
 const jar::physics::Vec axes[]={{1,0,0},{0,1,0},{0,0,1}};
 frameZero.Bounds(axes,solverLow,solverHigh);CHECK(solverHigh.x>1&&solverHigh.x<1.01f);
 frameOne.Bounds(axes,solverLow,solverHigh);CHECK(solverHigh.x>2&&solverHigh.x<2.01f);
 // The same mesh can have a visible slope, an empty corner and a closer
 // occluder. Preserve the ray endpoint when the triangle is unavailable.
 const vec3_t origin={10,20,30},rotated={0,90,0},start={9.5f,10,30.5f},end={9.5f,30,30.5f};
 vec3_t normal{};
 float fraction=1;
 CHECK(jar::md3::Pick(&mesh,sizeof(mesh),1,origin,rotated,start,end,fraction,normal));
 CHECK(std::fabs(fraction-.55f)<.0001f&&normal[1]<-.57f);
 fraction=.5f;
 CHECK(jar::md3::Pick(&mesh,sizeof(mesh),1,origin,rotated,start,end,fraction,normal)&&fraction==.5f);
 const vec3_t emptyStart={8.5f,10,31.5f},emptyEnd={8.5f,30,31.5f};
 fraction=1;
 CHECK(jar::md3::Pick(&mesh,sizeof(mesh),1,origin,rotated,emptyStart,emptyEnd,fraction,normal)&&fraction==1);
 mesh.triangle.indexes[2]=4;jar::physics::PropGeometry invalid;
 CHECK(!md3PoseGeometry(mesh.header,1,2,invalid));
 CHECK(!jar::md3::Pick(&mesh,sizeof(mesh),1,origin,rotated,start,end,fraction,normal)&&fraction==1);
 // Changing local bounds reconstructs a real Box3D shape and its saved joint.
 CHECK(world.Start({{-1000,-1000,-100},{1000,-1000,-100},{1000,1000,-100},{-1000,-1000,-100},{1000,1000,-100},{-1000,1000,-100}}));
 jar::physics::Pose first,second;first.position={0,0,50};second.position={40,0,50};first.linear={1,2,3};
 CHECK(physicalBounds(prop,bounds,0));CHECK(world.Prop(1,1,bounds.half,bounds.center,first,false));CHECK(world.Prop(2,2,{8,8,8},{},second,true));
 jar::constraint::Joint joint;joint.id=1;joint.first=1;joint.second=2;joint.kind=jar::constraint::Weld;CHECK(world.JointFrames(1,2,{20,0,50},{0,0,1},joint.a,joint.b));CHECK(world.Joint(joint,1,2));
 CHECK(physicalBounds(prop,bounds,4));CHECK(world.Prop(1,1,bounds.half,bounds.center,first,false));CHECK(world.Joint(joint,1,2));CHECK(world.Statistics().joints==1);
 jar::physics::Pose after;CHECK(world.Read(1,after)&&after.linear.x==1&&after.linear.y==2&&after.linear.z==3);CHECK(world.Bounds(1,solverLow,solverHigh)&&std::fabs(solverHigh.x-14)<.01f);world.Stop();
 });}
