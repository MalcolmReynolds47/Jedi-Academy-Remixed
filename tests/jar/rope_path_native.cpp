// Actual SP/MP CM_TraceThroughBrush; brush data and map traversal are fixtures.
#include "qcommon/cm_local.h"
#include "../../shared/qcommon/jar_rope_path.h"
#include "../../shared/qcommon/jar_rope_path_mesh.h"
#include "test_support.h"
#include <array>
#include <cmath>
#include <vector>
#include <type_traits>
#ifndef JAR_MP
#include "game/jar_physics.h"
#include "../../shared/qcommon/jar_sweep_hull.h"
#include "qcommon/cm_public.h"
#endif
clipMap_t cmg;
int c_brush_traces,c_traces,c_patch_traces;
cvar_t* cm_noCurves;
void CM_StoreLeafs(leafList_t*,int) { CHECK(false); }
void CM_BoxLeafnums_r(leafList_t*,int) { CHECK(false); }
cmodel_t* CM_ClipHandleToModel(clipHandle_t handle,clipMap_t** map) {
#ifndef JAR_MP
    if (map && cmg.cmodels && handle>=0 && handle<cmg.numSubModels) {
        *map=&cmg;
        return cmg.cmodels+handle;
    }
#endif
    CHECK(false); return nullptr;
}
qboolean CM_PositionTestInPatchCollide(traceWork_t*,const patchCollide_s*) { CHECK(false); return qfalse; }
#ifdef JAR_MP
void CM_TraceThroughBrush(traceWork_t*,trace_t&,cbrush_t*,bool);
void CM_TraceThroughPatchCollide(traceWork_t*,trace_t&,const patchCollide_s*) { CHECK(false); }
clipHandle_t CM_TempBoxModel(const vec3_t,const vec3_t,int) { CHECK(false); return 0; }
void CM_ModelBounds(clipHandle_t,vec3_t,vec3_t) { CHECK(false); }
void QDECL Com_Error(int,const char*,...) { CHECK(false); }
#else
void CM_TraceThroughBrush(traceWork_t*,cbrush_t*);
void CM_TraceThroughPatchCollide(traceWork_t*,const patchCollide_s*) { CHECK(false); }
#endif
struct Brush {
    cplane_t planes[6]{}; cbrushside_t sides[6]{}; cbrush_t brush{};
    Brush(float x,float y,float z,float hx,float hy,float hz) {
        const float center[3]={x,y,z},half[3]={hx,hy,hz};
        brush.numsides=6; brush.sides=sides; brush.contents=CONTENTS_SOLID;
        for (int k=0;k<3;++k) { brush.bounds[0][k]=center[k]-half[k]; brush.bounds[1][k]=center[k]+half[k]; }
        for (int i=0;i<6;++i) {
            int axis=i/2; float sign=i%2?-1.f:1.f;
            planes[i].normal[axis]=sign; planes[i].dist=half[axis]+sign*center[axis];
            planes[i].signbits=i%2?(1<<axis):0; planes[i].type=i%2?3:axis;
            sides[i].plane=&planes[i];
        }
    }
};
#ifndef JAR_MP
static void TestWorkshopPropPair() {
    using namespace jar::physics;
    PropGeometry bar;
    for(float x:{-40.f,40.f})for(float y:{-2.f,2.f})for(float z:{-4.f,4.f})bar.Add({x,y,z});
    NativePropHull target;
    CHECK(target.Build(bar,{0,45,0}));
    JarSweepHull moving{};
    CHECK(BuildPropSweep(bar,{0,45,0},moving));
    vec3_t low,high;
    JAR_SweepBounds(moving,low,high);
    const auto boxed=target.Trace({0,8,30},{0,8,-30},{low[0],low[1],low[2]},{high[0],high[1],high[2]});
    CHECK(boxed.fraction<1);
    const auto clear=target.TraceHull({0,8,30},{0,8,-30},moving);
    CHECK(clear.fraction==1&&!clear.startSolid);
    CHECK(!target.TraceHull({0,8,0},{0,8,0},moving).startSolid);

    const auto contact=target.TraceHull({0,2,30},{0,2,-30},moving);
    CHECK(std::fabs(30-60*contact.fraction-8.125f)<.001f);
    CHECK(contact.normal.z>.999f&&std::fabs(contact.planeDistance-4)<.001f);
    const auto reverse=target.TraceHull({0,2,-30},{0,2,30},moving);
    CHECK(std::fabs(reverse.fraction-contact.fraction)<.001f&&reverse.normal.z<-.999f);
    const auto trapped=target.TraceHull({0,2,0},{0,2,1},moving);
    CHECK(trapped.startSolid&&trapped.allSolid&&trapped.fraction==0);
    const auto escaping=target.TraceHull({0,2,0},{0,2,30},moving);
    CHECK(escaping.startSolid&&!escaping.allSolid&&escaping.fraction==1);

    // Both sets of face projections overlap here. Only an axis crossed
    // from an edge of each oblique prop separates their visible hulls.
    PropGeometry skew;
    for(float x:{-15.f,15.f})for(float y:{-3.f,3.f})for(float z:{-2.f,2.f})skew.Add({x,y,z});
    CHECK(target.Build(skew,{14,15,22})&&BuildPropSweep(skew,{53,67,34},moving));
    const Vec skewOffset={-7,-9,-2};
    JAR_SweepBounds(moving,low,high);
    CHECK(target.Trace(skewOffset,skewOffset,{low[0],low[1],low[2]},{high[0],high[1],high[2]}).startSolid);
    CHECK(!target.TraceHull(skewOffset,skewOffset,moving).startSolid);
    CHECK(target.TraceHull(skewOffset,{},moving).fraction<1);

    PropGeometry arm,smallProp;
    for(float x:{4.f,64.f})for(float y:{-.1f,.1f})for(float z:{-2.f,2.f})arm.Add({x,y,z});
    for(float x:{-.25f,.25f})for(float y:{-.25f,.25f})for(float z:{-1.f,1.f})smallProp.Add({x,y,z});
    CHECK(target.Build(smallProp,{0,31,0}));
    struct RotationCheck { const NativePropHull& fixed; Vec origin; };
    RotationCheck check{target,{-24,-24,0}};
    const auto rotationClear=[](void* data,const JarSweepHull& shape) {
        const auto& query=*static_cast<RotationCheck*>(data);
        const auto hit=query.fixed.TraceHull(query.origin,query.origin,shape);
        return !hit.startSolid&&hit.fraction==1;
    };
    for(float angle:{0.f,90.f}){
        CHECK(BuildPropSweep(arm,{0,angle,0},moving));
        CHECK(rotationClear(&check,moving));
    }
    CHECK(!TracePropRotation(arm,{},{0,90,0},&check,rotationClear));
    check.origin={-24,24,0};
    CHECK(TracePropRotation(arm,{},{0,90,0},&check,rotationClear));
}

static void TestWorkshopHull() {
    const float diagonal=std::sqrt(.5f);
    JarSweepHull hull{};
    for (float z:{-4.f,4.f}) for (float length:{-40.f,40.f}) for (float width:{-2.f,2.f}) {
        VectorSet(hull.points[hull.pointCount],length+width,length-width,z);
        ++hull.pointCount;
    }
    const vec3_t axes[]={{diagonal,diagonal,0},{diagonal,-diagonal,0},{0,0,1}};
    for (const auto& axis:axes) {
        VectorCopy(axis,hull.faces[hull.faceCount]);++hull.faceCount;
        VectorNegate(axis,hull.faces[hull.faceCount]);++hull.faceCount;
        for (int edge=0;edge<4;++edge) {
            VectorCopy(axis,hull.edges[hull.edgeCount]);++hull.edgeCount;
        }
    }
    Brush emptyCorner(24,-24,0,2,2,2),contact(24,24,0,2,2,2),slope(0,0,0,100,100,2);
    cvar_t curves{};curves.integer=1;cm_noCurves=&curves;
    cmodel_t models[2]{};cLeaf_t leaves[2]{};cNode_t node{};
    cplane_t split{};VectorSet(split.normal,0,0,1);split.type=2;
    node.plane=&split;node.children[0]=-1;node.children[1]=-2;
    leaves[0].numLeafBrushes=leaves[1].numLeafBrushes=models[1].leaf.numLeafBrushes=1;
    int brushIndex=0;
    cmg.nodes=&node;cmg.numNodes=1;cmg.leafs=leaves;cmg.numLeafs=2;
    cmg.cmodels=models;cmg.numSubModels=2;cmg.leafbrushes=&brushIndex;cmg.numBrushes=1;
    cmg.brushes=&emptyCorner.brush;
    const vec3_t start={0,0,30},end={0,0,-30},mins={-42,-42,-4},maxs={42,42,4};
    trace_t trace{};
    CM_BoxTrace(&trace,start,end,mins,maxs,0,CONTENTS_SOLID);
    CHECK(trace.fraction<1);
    CM_HullTrace(&trace,start,end,0,CONTENTS_SOLID,vec3_origin,vec3_origin,hull);
    CHECK(trace.fraction==1&&!trace.startsolid&&VectorCompare(trace.endpos,end));
    cmg.brushes=&contact.brush;
    CM_HullTrace(&trace,start,end,0,CONTENTS_SOLID,vec3_origin,vec3_origin,hull);
    CHECK(std::fabs(trace.endpos[2]-6.125f)<.001f&&trace.plane.normal[2]>.999f);
    CM_HullTrace(&trace,vec3_origin,vec3_origin,1,CONTENTS_SOLID,vec3_origin,vec3_origin,hull);
    CHECK(trace.startsolid&&trace.allsolid&&trace.fraction==0);
    CM_HullTrace(&trace,vec3_origin,start,1,CONTENTS_SOLID,vec3_origin,vec3_origin,hull);
    CHECK(trace.startsolid&&!trace.allsolid&&trace.fraction==1);
    // Rotating a local pillar from the empty corner onto the occupied diagonal.
    cmg.brushes=&emptyCorner.brush;
    const vec3_t yaw={0,90,0},origin={0,0,10},above={0,0,40};
    CM_HullTrace(&trace,above,end,1,CONTENTS_SOLID,origin,yaw,hull);
    CHECK(std::fabs(trace.endpos[2]-16.125f)<.001f&&std::fabs(trace.plane.dist-12.f)<.001f);
    // A sloped brush plane uses the asymmetric model's real support extent.
    cplane_t extra{};VectorSet(extra.normal,diagonal,-diagonal,0);extra.type=3;extra.signbits=2;
    cbrushside_t sides[7]{};
    for (int i=0;i<6;++i)sides[i]=slope.sides[i];
    sides[6].plane=&extra;slope.brush.sides=sides;slope.brush.numsides=7;cmg.brushes=&slope.brush;
    const vec3_t slopeStart={12,0,30},slopeEnd={12,0,-30};
    CM_BoxTrace(&trace,slopeStart,slopeEnd,mins,maxs,0,CONTENTS_SOLID);
    CHECK(trace.fraction<1);
    CM_HullTrace(&trace,slopeStart,slopeEnd,0,CONTENTS_SOLID,vec3_origin,vec3_origin,hull);
    CHECK(trace.fraction==1&&!trace.startsolid);

    using namespace jar::physics;
    struct RotationProbe { int steps=0; };
    const auto clear=[](void* data,const JarSweepHull& shape){
        auto& probe=*static_cast<RotationProbe*>(data);++probe.steps;
        const vec3_t vertical={0,0,1};
        CHECK(JAR_SweepPadding(shape,vertical)<.0001f);
        trace_t hit{};
        CM_HullTrace(&hit,vec3_origin,vec3_origin,1,CONTENTS_SOLID,vec3_origin,vec3_origin,shape);
        return !hit.startsolid&&!hit.allsolid&&hit.fraction==1;
    };
    PropGeometry arm;
    for(float x:{4.f,64.f})for(float y:{-.1f,.1f})for(float z:{-2.f,2.f})arm.Add({x,y,z});
    Brush midpoint(24,24,0,1,1,1),tip(63.98f,0,0,.008f,.008f,1),floor(0,0,-6.1f,100,100,4);
    RotationProbe probe;
    cmg.brushes=&midpoint.brush;
    for(float angle:{0.f,90.f}){
        JarSweepHull endpoint{};CHECK(BuildPropSweep(arm,{0,angle,0},endpoint));
        CHECK(clear(&probe,endpoint));
    }
    probe.steps=0;
    CHECK(!TracePropRotation(arm,{},{0,90,0},&probe,clear)&&probe.steps>1&&probe.steps<18);
    Brush gripArc(-24,-24,0,1,1,1);
    cmg.brushes=&gripArc.brush;
    for(float angle:{0.f,90.f}){
        JarSweepHull endpoint{};CHECK(BuildPropSweep(arm,{0,angle,0},endpoint));
        for(int i=0;i<endpoint.pointCount;++i)endpoint.points[i][angle==0?0:1]-=64;
        CHECK(clear(&probe,endpoint));
    }
    CHECK(!TracePropRotation(arm,{},{0,90,0},&probe,clear,{64,0,0}));
    cmg.brushes=&tip.brush;
    for(float angle:{-2.5f,2.5f}){
        JarSweepHull endpoint{};CHECK(BuildPropSweep(arm,{0,angle,0},endpoint));
        CHECK(clear(&probe,endpoint));
    }
    CHECK(!TracePropRotation(arm,{0,-2.5f,0},{0,2.5f,0},&probe,clear));
    cmg.brushes=&floor.brush;
    CHECK(TracePropRotation(arm,{},{0,90,0},&probe,clear));
    Brush movingArc(64,24,0,1,1,1);
    cmg.brushes=&movingArc.brush;
    for(int end:{0,1}){
        JarSweepHull endpoint{};
        CHECK(BuildPropSweep(arm,{0,90.f*end,0},endpoint));
        for(int i=0;i<endpoint.pointCount;++i)endpoint.points[i][0]+=80.f*end;
        CHECK(clear(&probe,endpoint));
    }
    CHECK(!TracePropRotation(arm,{},{0,90,0},&probe,clear,{}, {80,0,0}));
    CHECK(TracePropRotation(arm,{},{0,90,0},&probe,clear));
    PropGeometry diagonalArm;
    for(int i=0;i<hull.pointCount;++i)diagonalArm.Add({hull.points[i][0],hull.points[i][1],hull.points[i][2]});
    cmg.brushes=&emptyCorner.brush;
    CHECK(TracePropRotation(diagonalArm,{},{0,10,0},&probe,clear));
    cmg.cmodels=nullptr;cmg.nodes=nullptr;cmg.leafs=nullptr;cmg.brushes=nullptr;
    cmg.leafbrushes=nullptr;cm_noCurves=nullptr;
}
#endif
struct Scene { std::vector<Brush*> brushes; int calls=0; float lastA[3]{},lastB[3]{}; jarRopePathHit_t lastHit{}; int prop=0; };
void NativeTrace(const float* a,const float* b,float radius,jarRopePathHit_t* hit,void* context) {
    auto& scene=*static_cast<Scene*>(context); ++scene.calls;
    traceWork_t work{}; VectorCopy(a,work.start); VectorCopy(b,work.end);
    for (int k=0;k<3;++k) {
        work.size[0][k]=-radius; work.size[1][k]=radius;
        work.bounds[0][k]=fminf(a[k],b[k])-radius;
        work.bounds[1][k]=fmaxf(a[k],b[k])+radius;
        for (int i=0;i<8;++i) work.offsets[i][k]=i&(1<<k)?radius:-radius;
    }
#ifdef JAR_MP
    trace_t result{}; result.fraction=1;
    for (auto* brush:scene.brushes) CM_TraceThroughBrush(&work,result,&brush->brush,false);
#else
    work.trace.fraction=1;
    for (auto* brush:scene.brushes) CM_TraceThroughBrush(&work,&brush->brush);
    const auto& result=work.trace;
#endif
    *hit={}; hit->fraction=result.fraction; hit->solid=result.startsolid||result.allsolid;
    if (result.fraction<1&&scene.brushes.size()==1) hit->prop=scene.prop;
    for (int k=0;k<3;++k) { hit->point[k]=a[k]+(b[k]-a[k])*result.fraction; hit->normal[k]=result.plane.normal[k]; }
    VectorCopy(a,scene.lastA); VectorCopy(b,scene.lastB); scene.lastHit=*hit;
}

static void TestThreePassages() {
    Brush floor(68,0,-20,500,500,20), ceiling(68,0,100,500,500,20);
    Brush left(68,-40,40,300,20,40), right(68,40,40,300,20,40);
    for (float side : {-1.f,1.f}) {
        Brush near(32,-4*side,40,3,16,40), middle(68,4*side,40,3,16,40), far(104,-4*side,40,3,16,40);
        Scene scene{{&floor,&ceiling,&left,&right,&near,&middle,&far}};
        for (bool reverse : {false,true}) for (float offset : {-2.f,-1.f,0.f,1.f,2.f}) {
            const float a[3]={reverse?136.f:0.f,offset,40}, b[3]={reverse?0.f:136.f,-offset,40};
            jarRopePath_t path{};
            const int calls=scene.calls;
            const auto result=JAR_RopePathSolve(&path,a,b,240,NativeTrace,&scene,JAR_ROPE_PATH_TRACE_LIMIT);
            CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==6);
            CHECK(result.traces==scene.calls-calls&&result.traces<=JAR_ROPE_PATH_TRACE_LIMIT);
            CHECK(path.bend[0][1]*side>12&&path.bend[2][1]*side<-12&&path.bend[5][1]*side>12);
            float curve[JAR_ROPE_SEGMENTS+1][3];
            jarRopeVertex_t mesh[JAR_ROPE_VERTICES];
            CHECK(JAR_RopePathCurve(&path,a,b,240,curve,NativeTrace,&scene)==JAR_ROPE_SEGMENTS);
            CHECK(JAR_RopePathMesh(&path,a,b,240,mesh,NativeTrace,&scene,0)==JAR_ROPE_VERTICES);
            for (int i=0;i<JAR_ROPE_SEGMENTS;++i) {
                jarRopePathHit_t hit;
                NativeTrace(curve[i],curve[i+1],JAR_ROPE_PATH_RADIUS,&hit,&scene);
                CHECK(!hit.solid&&hit.fraction==1);
            }
        }

        if (side<0) continue;
        // As in the Workshop, once a route is accepted a stationary rope can
        // reuse it. Allow the remaining cold routes to retry on later frames.
        std::array<jarRopePath_t,16> paths{};
        std::size_t accepted=0;
        int deferred=0;
        const float a[3]={0,2,40}, b[3]={136,-2,40};
        for (std::size_t frame=0; frame<4&&accepted<paths.size(); ++frame) {
            int remaining=4096;
            for (std::size_t step=0;step<paths.size();++step) {
                auto& path=paths[(frame+step)%paths.size()];
                if (path.count) continue;
                if (!remaining) { ++deferred; continue; }
                const int allowance=std::min(JAR_ROPE_PATH_TRACE_LIMIT,remaining);
                const auto result=JAR_RopePathSolve(&path,a,b,240,NativeTrace,&scene,allowance);
                CHECK(result.traces<=allowance);
                remaining-=result.traces;
                if (result.status==JAR_ROPE_PATH_OK) {
                    CHECK(path.count==6);
                    ++accepted;
                } else {
                    CHECK(allowance<JAR_ROPE_PATH_TRACE_LIMIT);
                    const jarRopePath_t empty{};
                    CHECK(!memcmp(&path,&empty,sizeof(path)));
                    ++deferred;
                }
            }
        }
        CHECK(accepted==paths.size()&&deferred>0);
    }
}

static void RotateRopeFixturePoint(float *p,float c,float s) {
    const float x=p[0], y=p[1];
    p[0]=x*c-y*s;
    p[1]=x*s+y*c;
}
static void RotateRopeFixtureBrush(Brush& brush,float c,float s) {
    for (auto& plane:brush.planes) {
        RotateRopeFixturePoint(plane.normal,c,s);
        plane.signbits=0;
        for (int i=0;i<3;++i) if (plane.normal[i]<0) plane.signbits|=1<<i;
        if (plane.normal[0]||plane.normal[1]) plane.type=3;
    }
    const float x0=brush.brush.bounds[0][0],x1=brush.brush.bounds[1][0],
                y0=brush.brush.bounds[0][1],y1=brush.brush.bounds[1][1];
    brush.brush.bounds[0][0]=brush.brush.bounds[0][1]=1e6f;
    brush.brush.bounds[1][0]=brush.brush.bounds[1][1]=-1e6f;
    for (float x:{x0,x1}) for (float y:{y0,y1}) {
        float p[3]={x,y,0};
        RotateRopeFixturePoint(p,c,s);
        for (int i=0;i<2;++i) {
            brush.brush.bounds[0][i]=std::min(brush.brush.bounds[0][i],p[i]);
            brush.brush.bounds[1][i]=std::max(brush.brush.bounds[1][i],p[i]);
        }
    }
}
static void TestFourPassages() {
    for (float angle : {0.f,.2f,.4f,.6f}) for (float spacing : {28.f,36.f,44.f}) for (float side : {-1.f,1.f}) {
        if (angle!=0&&spacing==28) continue;
        const float end=64+3*spacing;
        Brush floor(end*.5f,0,-20,500,500,20),ceiling(end*.5f,0,100,500,500,20);
        Brush left(end*.5f,-40,40,300,20,40),right(end*.5f,40,40,300,20,40);
        Brush first(32,-4*side,40,3,16,40),second(32+spacing,4*side,40,3,16,40),
              third(32+2*spacing,-4*side,40,3,16,40),fourth(32+3*spacing,4*side,40,3,16,40);
        Scene scene{{&floor,&ceiling,&left,&right,&first,&second,&third,&fourth}};
        const float c=cosf(angle),s=sinf(angle);
        for (auto* brush:scene.brushes) RotateRopeFixtureBrush(*brush,c,s);
        for (bool backwards : {false,true}) for (float offset : {-2.f,-1.f,0.f,1.f,2.f}) {
            float a[3]={backwards?end:0.f,offset,40},b[3]={backwards?0.f:end,-offset,40};
            RotateRopeFixturePoint(a,c,s); RotateRopeFixturePoint(b,c,s);
            jarRopePath_t witness{};
            witness.count=8;
            for (int i=0;i<8;++i) {
                const int point=backwards?7-i:i;
                witness.bend[i][0]=32+(point/2)*spacing+((point%2)?7.f:-7.f);
                witness.bend[i][1]=(point/2)%2?-16*side:16*side;
                witness.bend[i][2]=40;
                RotateRopeFixturePoint(witness.bend[i],c,s);
            }
            for (int i=0;i<=witness.count;++i) {
                jarRopePathHit_t hit;
                NativeTrace(JAR_RopePathNode(&witness,a,b,i),JAR_RopePathNode(&witness,a,b,i+1),
                            JAR_ROPE_PATH_CLEARANCE,&hit,&scene);
                CHECK(!hit.solid&&hit.fraction==1);
            }
            jarRopePath_t path{};
            const int firstSweep=scene.calls;
            const auto route=JAR_RopePathSolve(&path,a,b,300,NativeTrace,&scene,JAR_ROPE_PATH_TRACE_LIMIT);
            CHECK(route.status==JAR_ROPE_PATH_OK&&path.count==8);
            CHECK(route.traces==scene.calls-firstSweep&&route.traces<=JAR_ROPE_PATH_TRACE_LIMIT);
            const auto solved=path;
            CHECK(JAR_RopePathSolve(&path,a,b,300,NativeTrace,&scene,1).status==JAR_ROPE_PATH_BUDGET);
            CHECK(!memcmp(&path,&solved,sizeof(path)));
            float curve[JAR_ROPE_SEGMENTS+1][3];
            CHECK(JAR_RopePathCurve(&path,a,b,300,curve,NativeTrace,&scene)==JAR_ROPE_SEGMENTS);
            jarRopeVertex_t mesh[JAR_ROPE_VERTICES];
            CHECK(JAR_RopePathMesh(&path,a,b,300,mesh,NativeTrace,&scene,0)==JAR_ROPE_VERTICES);
            for (int i=0;i<JAR_ROPE_SEGMENTS;++i) {
                jarRopePathHit_t hit;
                NativeTrace(curve[i],curve[i+1],JAR_ROPE_PATH_RADIUS,&hit,&scene);
                CHECK(!hit.solid&&hit.fraction==1);
            }
        }
    }
}

int main() { return RunTests("rope path through native brush sweeps, surface epsilon, blocked starts and moving anchors",[]{
    std::remove_pointer<decltype(cmg.shaders)>::type shader{}; cmg.shaders=&shader;
#ifndef JAR_MP
    TestWorkshopHull();
    TestWorkshopPropPair();
#endif
    TestThreePassages();
    TestFourPassages();
    Brush pillar(50,0,40,10,12,38),floor(50,0,-20,500,500,20),ceiling(50,0,100,500,500,20);
    Scene scene{{&pillar,&floor,&ceiling}}; float a[3]={0,0,40},b[3]={100,0,40}; jarRopePath_t path{};
    for (int step=0;step<64;++step) {
        a[1]=sinf(step*.1f)*4; b[1]=cosf(step*.1f)*4;
        int before=scene.calls; auto result=JAR_RopePathSolve(&path,a,b,180,NativeTrace,&scene,512);
        CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==2&&result.length>100);
        CHECK(result.traces==scene.calls-before&&result.traces<=512);
        CHECK(result.pullA[1]*result.pullB[1]>0);
        float curve[JAR_ROPE_SEGMENTS+1][3]; jarRopeVertex_t mesh[JAR_ROPE_VERTICES];
        before=scene.calls;
        int curveCount=JAR_RopePathCurve(&path,a,b,180,curve,NativeTrace,&scene);
        if (curveCount!=JAR_ROPE_SEGMENTS) std::fprintf(stderr,"curve failed at frame %d after %d traces; last sweep %.7f,%.7f,%.7f -> %.7f,%.7f,%.7f; fraction %.9f solid %d\n",step,scene.calls-before,scene.lastA[0],scene.lastA[1],scene.lastA[2],scene.lastB[0],scene.lastB[1],scene.lastB[2],scene.lastHit.fraction,scene.lastHit.solid);
        CHECK(curveCount==JAR_ROPE_SEGMENTS);
        CHECK(scene.calls-before<=JAR_ROPE_SEGMENTS*2+JAR_ROPE_PATH_BENDS+2);
        CHECK(JAR_RopePathMesh(&path,a,b,180,mesh,NativeTrace,&scene,0)==JAR_ROPE_VERTICES);
        for (int i=0;i<JAR_ROPE_SEGMENTS;++i) {
            jarRopePathHit_t hit; NativeTrace(curve[i],curve[i+1],JAR_ROPE_PATH_RADIUS,&hit,&scene);
            CHECK(!hit.solid&&hit.fraction==1);
        }
        for (int i=0;i<=path.count;++i) {
            jarRopePathHit_t hit;
            NativeTrace(JAR_RopePathNode(&path,a,b,i),JAR_RopePathNode(&path,a,b,i+1),JAR_ROPE_PATH_CLEARANCE,&hit,&scene);
            CHECK(!hit.solid&&hit.fraction==1);
        }
    }
    a[1]=b[1]=40; auto result=JAR_RopePathSolve(&path,a,b,180,NativeTrace,&scene,512);
    CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==0);
    a[0]=50; a[1]=0; jarRopePath_t before=path;
    result=JAR_RopePathSolve(&path,a,b,180,NativeTrace,&scene,512);
    CHECK(result.status==JAR_ROPE_PATH_BLOCKED&&!memcmp(&path,&before,sizeof(path)));
    Brush wallLeft(50,-50,40,300,20,40),wallRight(50,50,40,300,20,40);
    Brush nearBaffle(32,-12,40,3,18,40),farBaffle(68,12,40,3,18,40);
    Scene corridor{{&floor,&ceiling,&wallLeft,&wallRight,&nearBaffle,&farBaffle}};
    VectorSet(a,0,0,40);VectorSet(b,100,0,40);
    for (int step=0;step<32;++step) {
        a[1]=sinf(step*.1f)*2;b[1]=-a[1];
        const int calls=corridor.calls;
        result=JAR_RopePathSolve(&path,a,b,180,NativeTrace,&corridor,512);
        CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==2&&result.length<135);
        CHECK(path.bend[0][1]>8&&path.bend[1][1]<-8&&result.pullA[1]*result.pullB[1]<0);
        CHECK(result.traces==corridor.calls-calls&&result.traces<=512);
        if (step) CHECK(result.traces<128);
        float curve[JAR_ROPE_SEGMENTS+1][3];
        CHECK(JAR_RopePathCurve(&path,a,b,180,curve,NativeTrace,&corridor)==JAR_ROPE_SEGMENTS);
        for (int i=0;i<JAR_ROPE_SEGMENTS;++i) {
            jarRopePathHit_t hit;NativeTrace(curve[i],curve[i+1],JAR_ROPE_PATH_RADIUS,&hit,&corridor);
            CHECK(!hit.solid&&hit.fraction==1);
        }
    }
    Brush tightLeft(50,-40,40,300,20,40),tightRight(50,40,40,300,20,40);
    Brush tightNear(32,-4,40,3,16,40),tightFar(68,4,40,3,16,40);
    Scene tight{{&floor,&ceiling,&tightLeft,&tightRight,&tightNear,&tightFar}};
    path={};VectorSet(a,0,0,40);VectorSet(b,100,0,40);
    result=JAR_RopePathSolve(&path,a,b,180,NativeTrace,&tight,512);
    CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==4&&result.length<135&&result.traces<=512);
    for (int i=0;i<=path.count;++i) {
        jarRopePathHit_t hit;
        NativeTrace(JAR_RopePathNode(&path,a,b,i),JAR_RopePathNode(&path,a,b,i+1),
            JAR_ROPE_PATH_CLEARANCE,&hit,&tight);
        CHECK(!hit.solid&&hit.fraction==1);
    }
    float curve[JAR_ROPE_SEGMENTS+1][3];
    CHECK(JAR_RopePathCurve(&path,a,b,180,curve,NativeTrace,&tight)==JAR_ROPE_SEGMENTS);
    for (int i=0;i<JAR_ROPE_SEGMENTS;++i) {
        jarRopePathHit_t hit;NativeTrace(curve[i],curve[i+1],JAR_ROPE_PATH_RADIUS,&hit,&tight);
        CHECK(!hit.solid&&hit.fraction==1);
    }
    // Moving retained routes should all get a turn under the frame trace cap.
    auto paths=std::array<jarRopePath_t,32>{};
    paths.fill(path);
    for (int step=0;step<16;++step) {
        int remaining=4096;
        for (std::size_t i=0;i<paths.size();++i) {
            const auto index=(i+step)%paths.size();
            const float displacement=sinf(index*.03f+step*.02f)*.2f;
            VectorSet(a,0,displacement,40);VectorSet(b,100,-displacement,40);
            CHECK(remaining>0);
            result=JAR_RopePathSolve(&paths[index],a,b,180,NativeTrace,&tight,std::min(512,remaining));
            CHECK(result.status==JAR_ROPE_PATH_OK&&paths[index].count==4&&result.traces<=remaining);
            remaining-=result.traces;
        }
    }
    // Stock crate dimensions place a shallow bend just outside two planes.
    // The native sweep epsilon must still allow both corners to transfer load.
    Brush crate(9,-141,31.984375f,16.8f,10.921875f,10.92f);
    Scene shallow{{&crate}}; shallow.prop=44;
    const float left[3]={-68,-150,32},right[3]={68,-150,32};
    path={};result=JAR_RopePathSolve(&path,left,right,136,NativeTrace,&shallow,512);
    CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==2&&result.contactCount==2);
    CHECK(result.contacts[0].prop==44&&result.contacts[1].prop==44);
    for(int k=0;k<3;++k)CHECK(fabsf(result.pullA[k]+result.pullB[k]+result.contacts[0].pull[k]+result.contacts[1].pull[k])<.001f);
}); }
