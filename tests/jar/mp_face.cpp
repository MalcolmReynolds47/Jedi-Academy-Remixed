// Actual renderer controller and engine bridge with native MP skeletal types.
#define NOMINMAX
#include "rd-vanilla/tr_local.h"
#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <vector>
#include <cstring>
struct Pool:IGhoul2InfoArray{
 int next=0;std::map<int,std::vector<CGhoul2Info>> items;
 int New()override{items[++next]={};return next;}void Delete(int h)override{items.erase(h);}
 bool IsValid(int h)const override{return items.count(h)!=0;}
 std::vector<CGhoul2Info>&Get(int h)override{return items.at(h);}const std::vector<CGhoul2Info>&Get(int h)const override{return items.at(h);}
};
static Pool pool;IGhoul2InfoArray&TheGhoul2InfoArray(){return pool;}
static const char*names[]={"face","jaw","lblip2","rblip2","ltlip2","rtlip2","ceyebrow"};
static int writes,creates;static float energy=1;
float S_JAR_VoiceEnergy(int entity){assert(entity==23);return energy;}
int G2API_GetBoneIndex(CGhoul2Info*m,const char*name){
 int bone=-1;for(int n=0;n<7;++n)if(!strcmp(names[n],name))bone=n;
 if(bone<0)return -1;for(int n=0;n<(int)m->mBlist.size();++n)if(m->mBlist[n].boneNumber==bone)return n;return -1;
}
qboolean G2API_SetBoneAnglesMatrix(CGhoul2Info*m,const char*name,const mdxaBone_t&matrix,const int flags,qhandle_t*,int,int){
 assert(flags==0&&G2API_GetBoneIndex(m,name)<0);for(int i=0;i<3;++i)for(int j=0;j<4;++j)assert(matrix.matrix[i][j]==(i==j?1:0));
 int bone=-1;for(int n=0;n<7;++n)if(!strcmp(names[n],name))bone=n;if(bone<0||!m->aHeader)return qfalse;
 boneInfo_t b{};b.boneNumber=bone;m->mBlist.push_back(b);++creates;return qtrue;
}
qboolean G2API_SetBoneAnglesMatrixIndex(CGhoul2Info*m,const int index,const mdxaBone_t&matrix,const int flags,qhandle_t*,int,int){++writes;auto&b=m->mBlist.at(index);b.flags=(b.flags&~BONE_ANGLES_TOTAL)|flags;b.matrix=b.newMatrix=matrix;return qtrue;}
qboolean G2API_StopBoneAnglesIndex(CGhoul2Info*m,const int index){m->mBlist.at(index).flags&=~BONE_ANGLES_TOTAL;return qtrue;}
#include "../../codemp/rd-common/jar_face.inc"
static refexport_t exports;static refexport_t*re=&exports;
#include "../../codemp/client/cl_jar_face.inc"
int main(){
 static_assert(!(BONE_ANGLES_JAR_FACE&(BONE_NEED_TRANSFORM|BONE_ANGLES_TOTAL|BONE_ANIM_TOTAL|BONE_ANGLES_RAGDOLL|BONE_ANGLES_IK)),"ownership marker must not overlap native flags");
 exports.G2API_JAR_ApplyFace=G2API_JAR_ApplyFace;
 std::vector<byte> rig(sizeof(mdxaHeader_t)+7*sizeof(int)+7*sizeof(mdxaSkel_t));auto*h=(mdxaHeader_t*)rig.data();h->numBones=7;h->ofsEnd=(int)rig.size();auto*offsets=(int*)(h+1);
 for(int n=0;n<7;++n){offsets[n]=7*sizeof(int)+n*sizeof(mdxaSkel_t);auto*b=(mdxaSkel_t*)((byte*)offsets+offsets[n]);strcpy(b->name,names[n]);b->parent=n?0:-1;b->BasePoseMat.matrix[0][3]=n==2||n==4?.6f:-.5f;b->BasePoseMat.matrix[1][3]=n==1?-.5f:-3.8f;b->BasePoseMat.matrix[2][3]=n==1?60.8f:60;}
 CGhoul2Info_v g;g.resize(1);auto&m=g[0];m.aHeader=h;m.currentAnimModelSize=(int)rig.size();m.mModel=7;
 auto apply=[&](int time,int preset=1,qboolean active=qtrue,float sensitivity=1){CL_JAR_ApplyFace(&g,23,time,preset,sensitivity,active);};
 apply(1000,0);assert(!writes&&!creates);apply(1000);apply(1050);assert(writes>0&&creates==6);int jaw=G2API_GetBoneIndex(&m,"jaw"),brow=G2API_GetBoneIndex(&m,"ceyebrow");float expressive=m.mBlist[jaw].matrix.matrix[2][1];
 mdxaBone_t same=m.mBlist[jaw].matrix;apply(1050);assert(!memcmp(&same,&m.mBlist[jaw].matrix,sizeof(same)));apply(1100,3);assert(m.mBlist[jaw].matrix.matrix[2][1]>expressive);
 m.mBlist[brow].flags=BONE_NEED_TRANSFORM|BONE_ANGLES_POSTMULT;m.mBlist[brow].matrix.matrix[2][3]=42;apply(1150);assert(m.mBlist[brow].matrix.matrix[2][3]==42);
 apply(1200,0);assert(!(m.mBlist[jaw].flags&(BONE_ANGLES_TOTAL|BONE_ANGLES_JAR_FACE))&&m.mBlist[brow].flags==(BONE_NEED_TRANSFORM|BONE_ANGLES_POSTMULT));
 apply(1250);apply(1300);apply(1300,1,qfalse);assert(!(m.mBlist[jaw].flags&BONE_ANGLES_JAR_FACE));
 apply(1350);m.mBlist[jaw].flags=BONE_ANGLES_JAR_FACE|BONE_ANGLES_IK|BONE_ANGLES_REPLACE;apply(1350,0);assert(m.mBlist[jaw].flags==(BONE_ANGLES_IK|BONE_ANGLES_REPLACE));m.mBlist[jaw].flags=0;
 apply(1400);m.mFlags|=0x10;apply(1450);assert(!(m.mBlist[jaw].flags&BONE_ANGLES_JAR_FACE));m.mFlags=0;
 apply(1500);apply(1550);CL_JAR_ResetFaces();apply(1600);assert(jarMPFaceStates[23].amount==0);apply(1650);assert(jarMPFaceStates[23].amount>0);
 apply(100);assert(jarMPFaceStates[23].amount==0);apply(150);energy=0;for(int t=200;t<=1500;t+=25)apply(t);assert(!(m.mBlist[jaw].flags&BONE_ANGLES_JAR_FACE));energy=1;
 // Model replacement, missing rigs and corrupt offsets cannot reuse state.
 m.mModel=8;apply(1550);assert(jarMPFaceStates[23].amount==0);apply(1600);m.aHeader=nullptr;apply(1650);assert(!(m.mBlist[jaw].flags&BONE_ANGLES_JAR_FACE));m.aHeader=h;
 int old=offsets[1];offsets[1]=h->ofsEnd;apply(1700);assert(!(m.mBlist[jaw].flags&BONE_ANGLES_JAR_FACE));offsets[1]=old;
 CGhoul2Info_v empty;CL_JAR_ApplyFace(&empty,23,1750,1,1,qtrue);CL_JAR_ApplyFace(nullptr,23,1750,1,1,qtrue);CL_JAR_ApplyFace(&g,-1,1750,1,1,qtrue);CL_JAR_ApplyFace(&g,MAX_GENTITIES,1750,1,1,qtrue);
 puts("MP facial controller: shared deformation, native bone admission, repeated-frame stability, Stock/death/ragdoll cleanup, independent overrides, epoch/model/time reset and missing rigs passed.");
}
