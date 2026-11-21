#include "rd-vanilla/tr_local.h"
#include "qcommon/jar_bolt.h"
#include <cassert>
#include <vector>
#include <cstdio>
static_assert(offsetof(backEndData_t, commands) % alignof(void*) == 0);
trGlobals_t tr{};
#include "../../codemp/rd-vanilla/tr_jar_sort.inc"
#include "../../codemp/rd-vanilla/tr_jar_ghoul_pool.inc"
int main(){
 std::vector<drawSurf_t> surfaces(MAX_DRAWSURFS);tr.refdef.drawSurfs=surfaces.data();shader_t shader{};shader.sortedIndex=MAX_SHADERS-1;tr.sortedShaders[shader.sortedIndex]=&shader;surfaceType_t surface=SF_ENTITY;
 for(int ent:{0,2047,8191,16381,32766,REFENTITYNUM_WORLD}){tr.shiftedEntityNum=ent<<QSORT_REFENTITYNUM_SHIFT;int index=tr.refdef.numDrawSurfs;R_AddDrawSurf(&surface,&shader,31,3);
  int actual,fog,light;shader_t*actualShader;R_DecomposeSort(surfaces[index].sort,&actual,&actualShader,&fog,&light);assert(actual==ent&&actualShader==&shader&&fog==31&&light==3);
 }
 std::vector<drawSurf_t> shuffled(4096);for(size_t i=0;i<shuffled.size();++i)shuffled[i].sort=(uint64_t(i%2)<<63)|(uint64_t(4095-i)<<QSORT_SHADERNUM_SHIFT)|(uint64_t(i)<<7);
 R_RadixSort(shuffled.data(),int(shuffled.size()));for(size_t i=1;i<shuffled.size();++i)assert(shuffled[i-1].sort<=shuffled[i].sort);
 tr.refdef.numDrawSurfs=MAX_DRAWSURFS;auto first=surfaces[0].sort;R_AddDrawSurf(&surface,&shader,0,0);assert(tr.refdef.numDrawSurfs==MAX_DRAWSURFS&&surfaces[0].sort==first);
 for(int i=0;i<JAR_MP_RENDER_SURFACES;++i){auto*p=AllocRS();assert(p==&jarMPDrawSurfaces[i]&&p->ident==SF_MDX);p->boneCache=reinterpret_cast<CBoneCache*>(uintptr_t(i+1));}
 assert(!AllocRS());for(int i=0;i<JAR_MP_RENDER_SURFACES;++i)assert(jarMPDrawSurfaces[i].boneCache==reinterpret_cast<CBoneCache*>(uintptr_t(i+1)));
 R_ResetGhoul2Surfaces();assert(AllocRS()==&jarMPDrawSurfaces[0]&&!jarMPDrawSurfaces[0].boneCache);
 for(int e:{0,1023,8191,8192,15000,16381})for(int m:{0,255})for(int b:{0,1023}){int packed=-1;assert(JAR_MP_PackEntityBolt(&packed,e,m,b));assert(packed!=-1);assert(((uint32_t(packed)>>ENTITY_SHIFT)&ENTITY_AND)==e&&((uint32_t(packed)>>MODEL_SHIFT)&MODEL_AND)==m&&(uint32_t(packed)&BOLT_AND)==b);}
 for(int bad=0;bad<6;++bad){int packed=42;assert(!JAR_MP_PackEntityBolt(&packed,bad==0?-1:bad==1?16382:15000,bad==2?-1:bad==3?256:0,bad==4?-1:bad==5?1024:0)&&packed==42);}
 puts("MP renderer: high entity sort keys, all 64 sort bits, 65536 stable Ghoul2 surfaces, exhaustion and 14-bit attachment identities passed.");
}
