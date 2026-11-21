// Execute production ownership, capture scheduling, native face construction,
// selection and draw sorting. GL drawing/convolution are explicit boundaries.
#include "tr_local.h"
#include "test_support.h"
#include <memory>
#include <vector>
#include <limits>

trGlobals_t tr;
refimport_t ri{};
glconfig_t glConfig{};
backEndData_t* backEndData;
int r_firstSceneDrawSurf,r_numdlights,r_firstSceneDlight,r_numentities,r_firstSceneEntity,r_numpolys,r_firstScenePoly,r_numpolyverts;
static cvar_t cube{}, anaglyph{};
cvar_t* r_cubeMapping=&cube;
cvar_t* r_anaglyphMode=&anaglyph;
static unsigned now=1000;
static int captures, filters, flushes, fences;
static std::vector<void*> allocations;
static image_t images[jar::reflection::limit];
static std::vector<refdef_t> faces;
static std::vector<stereoFrame_t> stereoCalls;
static bool allocateImage=true;
static int Milliseconds(){return (int)now;}
void* Hunk_Alloc(int bytes,ha_pref){void* p=std::calloc(1,bytes);allocations.push_back(p);return p;}
void R_IssuePendingRenderCommands(){++flushes;}
#ifdef REND2_SP
void R_PushDebugGroup(annotationLayer_t,const char*){}
#endif
void RE_BeginFrame(stereoFrame_t stereo){
    stereoCalls.push_back(stereo);
    const int before=captures;
    R_JAR_ReflectionFrame(stereo); // actual re-entrant integration hook
    CHECK(captures==before);
#ifdef REND2_SP
    backEndData->previousFrame=backEndData->currentFrame;
#endif
    backEndData->currentFrame=&backEndData->frames[backEndData->realFrameNumber%MAX_FRAMES];
#ifdef REND2_SP
    backEndData->currentFrame->numCachedModelUboOffsets=0;
    backEndData->currentFrame->numCachedGhoulUboOffsets=0;
#endif
}
void RE_BeginScene(const refdef_t* fd){
    faces.push_back(*fd);
    CHECK(!r_numentities&&!r_numpolys&&!r_numdlights);
    tr.refdef.time=fd->time; tr.refdef.lastTime=fd->time;
    tr.numCachedViewParms=1;
    tr.cachedViewParms[0]={};
    auto& view=tr.cachedViewParms[0];view.fovX=fd->fov_x;view.fovY=fd->fov_y;
    view.viewportWidth=fd->width;view.viewportHeight=fd->height;
    VectorCopy(fd->vieworg,view.ori.origin);
    memcpy(view.ori.axis,fd->viewaxis,sizeof(fd->viewaxis));
#ifdef REND2_SP
    for(int i=0;i<16;++i)backEndData->currentFrame->viewProjectionMatrix[i]=float(captures*16+i);
    backEndData->currentFrame->time=fd->time*0.001f;
#endif
}
void R_RenderView(viewParms_t* view){
    CHECK(view->flags&VPF_NOVIEWMODEL);
    CHECK(view->flags&VPF_NOCUBEMAPS);
    CHECK(view->targetFbo&&view->targetFboLayer==0);
    CHECK(view->viewportWidth==128&&view->viewportHeight==128);
    CHECK(view->fovX==90&&view->fovY==90);
    ++captures;
}
void RE_EndScene(){tr.numCachedViewParms=0;}
void R_NewFrameSync(){++fences;++backEndData->realFrameNumber;
#ifdef REND2_SP
    backEndData->currentFrame->currentScene=0;
#endif
}
void R_AddConvolveCubemapCmd(cubemap_t* map,int id){
    CHECK(id>=tr.jarProbes.nativeCount&&id<tr.numCubemaps);
    CHECK(captures%6==0);
    if(allocateImage)map->image=&images[id-tr.jarProbes.nativeCount];
    ++filters;
}
#include "tr_frame.inc"
#include "tr_cubemap_capture.inc"
#include "tr_jar_probes.inc"
#include "tr_cubemap_select.inc"

static jar::reflection::Probe Probe(unsigned owner,float x=0,float radius=512){
    return {owner,1,{x,0,0},radius};
}
static void Frame(unsigned advance=0){now+=advance;R_JAR_ReflectionFrame(STEREO_CENTER);}
int main(){return RunTests("live reflection probes: native faces, lifecycle, selection and wide draw indices",[]{
    auto scene=std::make_unique<backEndData_t>();backEndData=scene.get();
    backEndData->currentFrame=&backEndData->frames[1];backEndData->realFrameNumber=2;
    auto world=std::make_unique<world_t>();
    ri.Milliseconds=Milliseconds;
#ifndef REND2_SP
    ri.Hunk_Alloc=Hunk_Alloc;
#endif
    auto p=Probe(1);
    CHECK(RE_SetReflectionProbes(&p,1)==jar::reflection::NoWorld);
    tr.registered=qtrue;tr.world=world.get();
    CHECK(RE_SetReflectionProbes(&p,1)==jar::reflection::Disabled);
    cube.integer=1;
    FBO_t fbo[7]{};for(int i=0;i<6;++i){fbo[i].width=128;fbo[i].height=128;tr.renderCubeFbo[i]=&fbo[i];}
    tr.filterCubeFbo=&fbo[6];
    cubemap_t native[128]{};image_t original{};
    for(int i=0;i<128;++i){native[i].origin[0]=float(i*1000);native[i].parallaxRadius=1000;native[i].image=&original;}
    tr.cubemaps=native;tr.numCubemaps=128;
    const auto nativeCopy=std::vector<cubemap_t>(native,native+128);
    CHECK(RE_SetReflectionProbes(nullptr,-1)==jar::reflection::Invalid);
    CHECK(RE_SetReflectionProbes(nullptr,1)==jar::reflection::Invalid);
    CHECK(RE_SetReflectionProbes(&p,65)==jar::reflection::Capacity);
#ifdef JAR_MP
    CHECK(JAR_ProbeRadiusValid(15.f)&&JAR_ProbeRadiusValid(32769.f));
    const float tooLarge=131073.f;
#else
    CHECK(!JAR_ProbeRadiusValid(15.f));
    const float tooLarge=32769.f;
#endif
    for(float bad:{-1.f,0.f,tooLarge,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        auto invalid=p;invalid.radius=bad;CHECK(RE_SetReflectionProbes(&invalid,1)==jar::reflection::Invalid);
    }
    auto invalid=p;invalid.origin[2]=65536;CHECK(RE_SetReflectionProbes(&invalid,1)==jar::reflection::Invalid);
    invalid=p;invalid.owner=0;CHECK(RE_SetReflectionProbes(&invalid,1)==jar::reflection::Invalid);
    jar::reflection::Probe duplicated[2]={p,p};CHECK(RE_SetReflectionProbes(duplicated,2)==jar::reflection::Invalid);
    CHECK(RE_SetReflectionProbes(&p,1)==0);p.origin[0]=100; // submission owns its copy
    r_numentities=1;Frame(200);CHECK(!tr.jarProbes.initialized&&captures==0);r_numentities=0;
    r_numpolys=1;Frame();CHECK(!tr.jarProbes.initialized);r_numpolys=0;
    r_numdlights=1;Frame();CHECK(!tr.jarProbes.initialized);r_numdlights=0;
    R_JAR_ReflectionFrame(STEREO_RIGHT);CHECK(!tr.jarProbes.initialized);
    Frame();CHECK(tr.numCubemaps==192&&tr.jarProbes.activeCount==1&&allocations.size()==1);
    CHECK(!memcmp(nativeCopy.data(),tr.cubemaps,128*sizeof(cubemap_t)));
    vec3_t zero{};CHECK(R_CubemapForPoint(zero)==1); // map fallback until all faces complete
    tr.refdef.time=23456;tr.refdef.lastTime=23400;
#ifdef REND2_SP
    matrix_t gameplayCamera;for(int i=0;i<16;++i)gameplayCamera[i]=float(100+i);
    memcpy(backEndData->currentFrame->viewProjectionMatrix,gameplayCamera,sizeof(gameplayCamera));
    backEndData->currentFrame->time=23.4f;
    backEndData->currentFrame->numCachedModelUboOffsets=12;
#endif
    Frame(149);CHECK(!captures);Frame(1);
    CHECK(captures==6&&filters==1&&fences==6&&tr.jarProbes.slots[0].ready);
    CHECK(tr.refdef.time==23456&&tr.refdef.lastTime==23400&&!tr.jarProbes.capturing);
    CHECK(backEndData->realFrameNumber==8);
#ifdef REND2_SP
    CHECK(!memcmp(backEndData->currentFrame->viewProjectionMatrix,gameplayCamera,sizeof(gameplayCamera)));
    CHECK(backEndData->currentFrame->time==23.4f);
    CHECK(backEndData->currentFrame->numCachedModelUboOffsets==0);
#endif
    CHECK(R_CubemapForPoint(zero)==129&&R_JAR_ProbeForPoint(zero)==129);
    const float directions[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for(int i=0;i<6;++i){CHECK(faces[i].time==23456);CHECK(faces[i].vieworg[0]==0);
        CHECK(!memcmp(faces[i].viewaxis[0],directions[i],sizeof(vec3_t)));
        for(int a=0;a<3;++a)for(int b=0;b<3;++b)CHECK(DotProduct(faces[i].viewaxis[a],faces[i].viewaxis[b])==(a==b?1.f:0.f));}
    const int unchangedFlushes=flushes;
    p.origin[0]=0;CHECK(RE_SetReflectionProbes(&p,1)==1);Frame(200);
    CHECK(captures==6&&flushes==unchangedFlushes);
    p=Probe(1);p.radius=16;CHECK(RE_SetReflectionProbes(&p,1)==1);Frame();CHECK(captures==6);
    vec3_t edge{16,0,0},outside{17,0,0};CHECK(R_JAR_ProbeForPoint(edge)==129&&R_JAR_ProbeForPoint(outside)==0);
    msurface_t surface{};surface.cubemapIndex=7;surface.cullinfo.type=CULLINFO_BOX;
    VectorSet(surface.cullinfo.bounds[0],-2,-2,-2);VectorSet(surface.cullinfo.bounds[1],2,2,2);
    CHECK(R_JAR_ProbeForSurface(&surface,nullptr,REFENTITYNUM_WORLD)==129);
    surface.cullinfo.type=CULLINFO_SPHERE;VectorSet(surface.cullinfo.localOrigin,0,2,0);
    trRefEntity_t mover{};VectorSet(mover.e.axis[0],0,1,0);VectorSet(mover.e.axis[1],-1,0,0);VectorSet(mover.e.axis[2],0,0,1);
    mover.e.origin[0]=18;CHECK(R_JAR_ProbeForSurface(&surface,&mover,1)==129);
    mover.e.origin[0]=19;CHECK(R_JAR_ProbeForSurface(&surface,&mover,1)==7);
    surface.cullinfo.type=0;CHECK(R_JAR_ProbeForSurface(&surface,nullptr,REFENTITYNUM_WORLD)==7);
    // Moving hides stale captures and waits for the final position to settle.
    p.origin[0]=200;CHECK(RE_SetReflectionProbes(&p,1)==0);Frame();CHECK(!R_JAR_ProbeForPoint(zero));
    p.origin[0]=300;CHECK(RE_SetReflectionProbes(&p,1)==0);Frame(100);Frame(149);CHECK(captures==6);
    Frame(1);CHECK(captures==12&&tr.cubemaps[128].origin[0]==300);
    ++p.revision;CHECK(RE_SetReflectionProbes(&p,1)==0);Frame();Frame(150);CHECK(captures==18);
    image_t* image=tr.cubemaps[128].image;
    CHECK(RE_SetReflectionProbes(nullptr,0)==0);Frame();CHECK(tr.jarProbes.activeCount==0&&R_CubemapForPoint(zero)==1);
    CHECK(R_JAR_ProbeForSurface(&surface,nullptr,REFENTITYNUM_WORLD)==7);
    CHECK(tr.cubemaps[128].image==image); // bounded reuse rather than per-delete image leaks
    p=Probe(99);CHECK(RE_SetReflectionProbes(&p,1)==0);Frame();CHECK(!R_JAR_ProbeForPoint(zero));Frame(150);
    CHECK(tr.cubemaps[128].image==image&&R_JAR_ProbeForPoint(zero)==129);
    // A full replacement and permutation retain identities and one-capture/frame budget.
    jar::reflection::Probe all[64];for(int i=0;i<64;++i)all[i]=Probe(100+i,float(i*100),32);
    CHECK(RE_SetReflectionProbes(all,64)==0);Frame();const int before=captures;
    for(int i=0;i<64;++i){Frame(150);CHECK(captures==before+6*(i+1));}
    CHECK(RE_SetReflectionProbes(all,64)==64);
    for(int i=0;i<32;++i)std::swap(all[i],all[63-i]);Frame();
    CHECK(RE_SetReflectionProbes(all,64)==64);Frame(200);CHECK(captures==before+64*6&&allocations.size()==1);
    CHECK(RE_SetReflectionProbes(duplicated,2)==jar::reflection::Invalid);Frame();CHECK(tr.jarProbes.activeCount==64);
    cube.integer=0;CHECK(RE_SetReflectionProbes(all,64)==jar::reflection::Disabled);Frame(200);CHECK(R_CubemapForPoint(zero)==0);cube.integer=1;
    // Unsigned time deltas cross the 32-bit millisecond rollover safely.
    now=0xfffffff0u;p=Probe(900);CHECK(RE_SetReflectionProbes(&p,1)==0);Frame();Frame(149);const int wrapBefore=captures;Frame(1);CHECK(captures==wrapBefore+6);
    // A missing GPU image never becomes selectable, and retry work remains bounded.
    p=Probe(901);CHECK(RE_SetReflectionProbes(&p,1)==0);Frame();tr.cubemaps[128].image=nullptr;allocateImage=false;Frame(150);
    CHECK(!R_JAR_ProbeForPoint(zero));const int failed=captures;Frame(149);CHECK(captures==failed);allocateImage=true;Frame(1);CHECK(captures==failed+6);
    // Native capture uses a valid left-eye frame when stereo or anaglyph is active.
    glConfig.stereoEnabled=qtrue;++p.revision;CHECK(RE_SetReflectionProbes(&p,1)==0);Frame();Frame(150);CHECK(stereoCalls.back()==STEREO_LEFT);glConfig.stereoEnabled=qfalse;
    anaglyph.integer=1;++p.revision;CHECK(RE_SetReflectionProbes(&p,1)==0);Frame();Frame(150);CHECK(stereoCalls.back()==STEREO_LEFT);anaglyph.integer=0;
#ifdef REND2_SP
    shader_t shader{};shader.sortedIndex=MAX_SHADERS-1;tr.sortedShaders[shader.sortedIndex]=&shader;
    for(int probe:{0,63,64,127,128,129,192,65535})for(int entity:{0,16383,REFENTITYNUM_WORLD}){
        int gotEntity,gotProbe,post;shader_t* gotShader;
        R_DecomposeSort(R_CreateSortKey(entity,shader.sortedIndex,probe,1),&gotEntity,&gotShader,&gotProbe,&post);
        CHECK(gotEntity==entity&&gotProbe==probe&&gotShader==&shader&&post==1);
    }
#else
    static_assert(QSORT_CUBEMAP_BITS==16,"MP draw sort must keep the full probe index");
#endif
    CHECK(!memcmp(nativeCopy.data(),tr.cubemaps,128*sizeof(cubemap_t)));
    tr={};CHECK(!tr.jarProbes.initialized&&!tr.jarProbes.activeCount&&!tr.jarProbes.pendingSet);
    CHECK(RE_SetReflectionProbes(&p,1)==jar::reflection::NoWorld);
    // A subsequent map with no authored probes starts clean. Capacity refusal
    // does not allocate or replace a previous pending authoritative set.
    tr.registered=qtrue;tr.world=world.get();
    for(int i=0;i<6;++i)tr.renderCubeFbo[i]=&fbo[i];tr.filterCubeFbo=&fbo[6];
    tr.numCubemaps=QSORT_CUBEMAP_MASK-jar::reflection::limit+1;
    CHECK(RE_SetReflectionProbes(&p,1)==jar::reflection::Capacity&&!tr.jarProbes.pendingSet);
    tr.numCubemaps=0;tr.cubemaps=nullptr;
    CHECK(RE_SetReflectionProbes(&p,1)==0);Frame();Frame(150);
    CHECK(tr.jarProbes.nativeCount==0&&tr.numCubemaps==64&&R_CubemapForPoint(zero)==1);
    CHECK(RE_SetReflectionProbes(nullptr,0)==0);Frame();CHECK(R_CubemapForPoint(zero)==0);
    for(void* allocation:allocations)std::free(allocation);
    std::puts("Reflection probes: six native face orientations, immutable map fallback, movement/refresh/removal/reuse, 64-owner capacity, quiet-period scheduling, stereo, wraparound and wide indices passed. GL drawing/convolution remain fixture boundaries.");
});}
