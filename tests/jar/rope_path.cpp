#include "../../shared/qcommon/jar_rope_path.h"
#include "../../shared/qcommon/jar_rope_path_mesh.h"
#include "test_support.h"
#include <array>
#include <cmath>
#include <vector>
#include <limits>

extern "C" jarRopePathResult_t JAR_TestCRopePath(jarRopePath_t*,const float*,const float*,float,jarRopePathTrace_t,void*,int);
namespace {
using Vec=std::array<float,3>;
struct Plane { Vec normal; float distance; };
using Brush=std::vector<Plane>;
struct Scene { std::vector<Brush> brushes; int calls=0; };
float Dot(const Vec& a,const float* b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Brush Box(Vec center,Vec half,float angle=0) {
    float c=cosf(angle),s=sinf(angle); Vec axis[3]={{c,s,0},{-s,c,0},{0,0,1}}; Brush brush;
    for (int a=0;a<3;++a) for (float sign:{-1.f,1.f}) {
        Vec n; for (int k=0;k<3;++k) n[k]=axis[a][k]*sign;
        brush.push_back({n,Dot(n,center.data())+half[a]});
    }
    return brush;
}
void Trace(const float* a,const float* b,float radius,jarRopePathHit_t* hit,void* context) {
    auto& scene=*static_cast<Scene*>(context); ++scene.calls;
    *hit={}; hit->fraction=1; for (int k=0;k<3;++k) hit->point[k]=b[k];
    for (const auto& brush:scene.brushes) {
        bool inside=true,miss=false; float enter=0,exit=1; Vec normal{};
        for (const auto& plane:brush) {
            float expanded=plane.distance+radius*(fabsf(plane.normal[0])+fabsf(plane.normal[1])+fabsf(plane.normal[2]));
            float x=Dot(plane.normal,a)-expanded,y=Dot(plane.normal,b)-expanded;
            if (x>0) inside=false;
            if (x>0&&y>0) { miss=true; break; }
            if (x<=0&&y<=0) continue;
            float t=x/(x-y);
            if (x>y) { if (t>=enter) { enter=t; normal=plane.normal; } }
            else exit=fminf(exit,t);
        }
        if (miss||enter>exit) continue;
        if (inside) { hit->solid=1; hit->fraction=0; for (int k=0;k<3;++k) hit->point[k]=a[k]; return; }
        if (enter<hit->fraction) {
            hit->fraction=enter; for (int k=0;k<3;++k) { hit->point[k]=a[k]+(b[k]-a[k])*enter; hit->normal[k]=normal[k]; }
        }
    }
}
bool Same(const jarRopePath_t& a,const jarRopePath_t& b) { return !memcmp(&a,&b,sizeof(a)); }
void Safe(const jarRopePath_t& path,const Vec& a,const Vec& b,Scene& scene) {
    CHECK(JAR_RopePathValid(&path));
    for (int i=0;i<=path.count;++i) {
        const float* x=JAR_RopePathNode(&path,a.data(),b.data(),i);
        const float* y=JAR_RopePathNode(&path,a.data(),b.data(),i+1);
        jarRopePathHit_t hit; Trace(x,y,JAR_ROPE_PATH_CLEARANCE,&hit,&scene); CHECK(!hit.solid&&hit.fraction==1);
        // Independent point/plane oracle, including the interiors of spans.
        for (int step=0;step<=128;++step) {
            Vec p; for (int k=0;k<3;++k) p[k]=x[k]+(y[k]-x[k])*float(step)/128;
            for (const auto& brush:scene.brushes) {
                bool outside=false; for (const auto& plane:brush) {
                    float expanded=plane.distance+JAR_ROPE_PATH_RADIUS*(fabsf(plane.normal[0])+fabsf(plane.normal[1])+fabsf(plane.normal[2]));
                    outside|=Dot(plane.normal,p.data())>expanded-.001f;
                }
                CHECK(outside);
            }
        }
    }
}
void MeshSafe(const jarRopePath_t& path,const Vec& a,const Vec& b,Scene& scene,float length) {
    float curve[JAR_ROPE_SEGMENTS+1][3]; jarRopeVertex_t mesh[JAR_ROPE_VERTICES];
    int before=scene.calls;
    CHECK(JAR_RopePathCurve(&path,a.data(),b.data(),length,curve,Trace,&scene)==JAR_ROPE_SEGMENTS);
    CHECK(scene.calls-before<=JAR_ROPE_SEGMENTS*2+JAR_ROPE_PATH_BENDS+2);
    // All authoritative corners occur exactly in the curve, including close
    // bends which must not be lost through even-distance curve resampling.
    for (int bend=0;bend<path.count;++bend) {
        bool found=false;
        for (const auto& point:curve) found|=JAR_RopePathDistance(path.bend[bend],point)<.0001f;
        CHECK(found);
    }
    for (int i=0;i<JAR_ROPE_SEGMENTS;++i) {
        jarRopePathHit_t hit; Trace(curve[i],curve[i+1],JAR_ROPE_PATH_RADIUS,&hit,&scene); CHECK(!hit.solid&&hit.fraction==1);
    }
    CHECK(JAR_RopePathMesh(&path,a.data(),b.data(),length,mesh,Trace,&scene,0)==JAR_ROPE_VERTICES);
    for (const auto& v:mesh) {
        for (float x:v.xyz) CHECK(std::isfinite(x)); CHECK(v.color[3]==255);
        for (const auto& brush:scene.brushes) {
            bool outside=false;
            for (const auto& plane:brush) {
                outside|=Dot(plane.normal,v.xyz)>plane.distance-.001f;
            }
            CHECK(outside);
        }
    }
    // Adjacent tube segments share their complete edge, with no cracks at bends.
    for (int i=0;i<JAR_ROPE_SEGMENTS-1;++i) for (int face=0;face<4;++face) {
        CHECK(!memcmp(mesh[(i*4+face)*4+2].xyz,mesh[((i+1)*4+face)*4+1].xyz,sizeof(Vec)));
        CHECK(!memcmp(mesh[(i*4+face)*4+3].xyz,mesh[((i+1)*4+face)*4].xyz,sizeof(Vec)));
    }
}
jarRopePathResult_t Solve(jarRopePath_t& path,const Vec& a,const Vec& b,Scene& scene,int budget=512,float length=180) {
    int before=scene.calls; auto result=JAR_RopePathSolve(&path,a.data(),b.data(),length,Trace,&scene,budget);
    CHECK(result.traces==scene.calls-before&&result.traces<=budget);
    if (result.status==JAR_ROPE_PATH_OK) Safe(path,a,b,scene);
    else CHECK(result.length==0&&result.pullA[0]==0&&result.pullB[0]==0);
    return result;
}
}
int main() { return RunTests("shared rope collision paths, retained bends, unwrapping, endpoint forces and bounded failure",[]{
    Vec a={0,0,40},b={100,0,40}; Scene scene; jarRopePath_t path{};
    auto result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==0&&result.length==100);
    CHECK(result.traces==2&&result.pullA[0]==1&&result.pullB[0]==-1);
    auto coincident=Solve(path,a,a,scene); CHECK(coincident.status==JAR_ROPE_PATH_OK&&coincident.length==0);
    // A low ceiling and a floor rule out routes above/below a tall pillar.
    scene.brushes={Box({50,0,40},{10,12,38}),Box({50,0,-20},{500,500,20}),Box({50,0,100},{500,500,20})};
    result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==2&&result.length>100);
    CHECK(fabsf(result.pullA[1])>.2f&&result.pullA[1]*result.pullB[1]>0);
    MeshSafe(path,a,b,scene,180);
    const float side=path.bend[0][1];
    // Moving anchors retain their side of the pillar and pull toward the first
    // and last bends, not through the straight chord that crosses the pillar.
    for (int step=0;step<24;++step) {
        a[1]=sinf(step*.2f)*3; b[1]=cosf(step*.2f)*3;
        result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==2&&path.bend[0][1]*side>0);
    }
    // Contract for coupling: endpoint pull is the negative derivative of total
    // path length while world pivots stay fixed. This exchanges tension with the
    // scenery; endpoint forces need not be opposite across a bend.
    for (int end=0;end<2;++end) for (int axis=0;axis<3;++axis) {
        Vec x=a,y=b; (end?y:x)[axis]+=.01f; jarRopePathResult_t moved{};
        JAR_RopePathMeasure(&path,x.data(),y.data(),&moved);
        CHECK(fabsf((moved.length-result.length)/.01f+(end?result.pullB[axis]:result.pullA[axis]))<.004f);
    }
    // Endpoint movement can unwrap the path without destroying/recreating it.
    a[1]=b[1]=60; result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==0);
    a={0,0,40}; b={100,0,40};
    // Scenery removal releases retained bends immediately.
    result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==2);
    scene.brushes.clear(); result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==0);
    // A visible floor support, sloped/rotated faces and vertical endpoints.
    scene.brushes={Box({50,0,30},{10,500,30}),Box({50,0,-20},{500,500,20})};
    result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==2&&path.bend[0][2]>61);
    MeshSafe(path,a,b,scene,180);
    // Multiple obstructions can share a route; all spans must clear all brushes.
    path={}; scene.brushes={Box({30,0,40},{6,12,20}),Box({65,2,40},{8,18,20})};
    result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count>0);
    MeshSafe(path,a,b,scene,180);
    // Two solid baffles span floor to ceiling and leave openings on opposite
    // sides. Uniform sideways offsets cannot connect both ends.
    scene.brushes={Box({50,0,-20},{300,300,20}),Box({50,0,100},{300,300,20}),
        Box({50,-50,40},{300,20,40}),Box({50,50,40},{300,20,40}),
        Box({32,-12,40},{3,18,40}),Box({68,12,40},{3,18,40})};
    path={};
    result=Solve(path,a,b,scene);
    CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==2&&result.length<131);
    CHECK(path.bend[0][1]>8&&path.bend[1][1]<-8&&result.pullA[1]*result.pullB[1]<0);
    MeshSafe(path,a,b,scene,180);
    jarRopePath_t oppositeC{},reverse{};
    const auto cResult=JAR_TestCRopePath(&oppositeC,a.data(),b.data(),180,Trace,&scene,512);
    CHECK(cResult.status==result.status&&cResult.traces==result.traces&&Same(oppositeC,path));
    CHECK(Solve(reverse,b,a,scene).status==JAR_ROPE_PATH_OK&&reverse.count==2);
    CHECK(reverse.bend[0][1]<-8&&reverse.bend[1][1]>8);
    const auto retained=path;
    auto failed=Solve(path,a,b,scene,1);
    CHECK(failed.status==JAR_ROPE_PATH_BUDGET&&Same(path,retained));
    scene.brushes.resize(4);
    CHECK(Solve(path,a,b,scene).status==JAR_ROPE_PATH_OK&&path.count==0);
    // Narrow openings require crossing the two faces before turning.
    scene.brushes[2]=Box({50,-40,40},{300,20,40});
    scene.brushes[3]=Box({50,40,40},{300,20,40});
    scene.brushes.push_back(Box({32,-4,40},{3,16,40}));
    scene.brushes.push_back(Box({68,4,40},{3,16,40}));
    result=Solve(path,a,b,scene);
    CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==4&&result.length<135);
    CHECK(path.bend[0][1]>12&&path.bend[1][1]>12&&path.bend[2][1]<-12&&path.bend[3][1]<-12);
    MeshSafe(path,a,b,scene,180);
    jarRopePath_t narrowC{};
    const auto narrowResult=JAR_TestCRopePath(&narrowC,a.data(),b.data(),180,Trace,&scene,512);
    CHECK(narrowResult.status==result.status&&narrowResult.traces==result.traces&&Same(narrowC,path));
    const auto narrowPath=path;
    CHECK(Solve(path,a,b,scene,8).status==JAR_ROPE_PATH_BUDGET&&Same(path,narrowPath));
    for (int angle=0;angle<24;++angle) {
        path={}; scene.brushes={Box({50,0,40},{12,8,20},angle*.13f)};
        result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count>0);
        MeshSafe(path,a,b,scene,180);
        // Header is compiled as real C as well as C++; same geometry and budgets.
        jarRopePath_t cPath{}; int before=scene.calls;
        auto c=JAR_TestCRopePath(&cPath,a.data(),b.data(),180,Trace,&scene,512);
        CHECK(c.status==result.status&&c.traces==scene.calls-before&&c.length==result.length&&Same(cPath,path));
    }
    path={}; a={0,0,0}; b={0,0,100}; scene.brushes={Box({0,0,50},{12,12,10})};
    result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_OK&&path.count==2);
    MeshSafe(path,a,b,scene,180);
    // Bounded failures cannot replace an existing physical path with a chord.
    jarRopePath_t before=path; int count=scene.calls;
    result=Solve(path,a,b,scene,1); CHECK(result.status==JAR_ROPE_PATH_BUDGET&&result.traces==1&&Same(path,before));
    scene.brushes={Box({0,0,0},{20,20,20})}; result=Solve(path,a,b,scene);
    CHECK(result.status==JAR_ROPE_PATH_BLOCKED&&Same(path,before));
    path={}; a={0,0,40}; b={100,0,40};
    scene.brushes={Box({50,0,40},{10,10000,10000})}; before=path;
    result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_BLOCKED&&Same(path,before));
    // Save/network inputs and native-trace failures are separately rejected.
    path.count=9; before=path; count=scene.calls; result=Solve(path,a,b,scene);
    CHECK(result.status==JAR_ROPE_PATH_INVALID&&scene.calls==count&&Same(path,before));
    path={}; path.count=1; path.bend[0][1]=std::numeric_limits<float>::quiet_NaN(); before=path;
    result=Solve(path,a,b,scene); CHECK(result.status==JAR_ROPE_PATH_INVALID&&Same(path,before));
    path={}; before=path;
    auto badTrace=[](const float*,const float*,float,jarRopePathHit_t* h,void*){h->fraction=std::numeric_limits<float>::quiet_NaN();};
    result=JAR_RopePathSolve(&path,a.data(),b.data(),180,badTrace,nullptr,512);
    CHECK(result.status==JAR_ROPE_PATH_INVALID&&Same(path,before)&&result.traces==1);
    auto badPoint=[](const float*,const float* e,float,jarRopePathHit_t* h,void*){h->fraction=1; for(int k=0;k<3;++k)h->point[k]=e[k]; h->point[0]+=1;};
    result=JAR_RopePathSolve(&path,a.data(),b.data(),180,badPoint,nullptr,512);
    CHECK(result.status==JAR_ROPE_PATH_INVALID&&Same(path,before));
    result=JAR_RopePathSolve(&path,a.data(),b.data(),INFINITY,Trace,&scene,512);
    CHECK(result.status==JAR_ROPE_PATH_INVALID&&result.traces==0);
    result=JAR_RopePathSolve(&path,a.data(),b.data(),180,Trace,&scene,513);
    CHECK(result.status==JAR_ROPE_PATH_INVALID&&result.traces==0);
    // An invalid/unresolved authoritative span must not get a decorative mesh.
    float curve[JAR_ROPE_SEGMENTS+1][3]; jarRopeVertex_t mesh[JAR_ROPE_VERTICES];
    CHECK(!JAR_RopePathMesh(&path,a.data(),b.data(),180,mesh,Trace,&scene,0));
    CHECK(JAR_RopePathMesh(&path,a.data(),b.data(),180,mesh,Trace,&scene,1)==JAR_ROPE_VERTICES);
    scene.brushes.clear(); a[0]=131073;
    CHECK(!JAR_RopePathCurve(&path,a.data(),b.data(),180,curve,Trace,&scene));
    a={0,0,0}; b={100,0,0};
    CHECK(!JAR_RopePathCurve(&path,a.data(),b.data(),INFINITY,curve,Trace,&scene));
    CHECK(JAR_RopePathCurve(&path,a.data(),a.data(),0,curve,Trace,&scene)==0);
    CHECK(JAR_RopePathMesh(&path,a.data(),a.data(),40,mesh,Trace,&scene,0)==JAR_ROPE_VERTICES);
    // A clear chord can have heavily clipped sag. Drawn spans and their full
    // cross-sections still stay above the floor, even between trace samples.
    a[2]=b[2]=8; scene.brushes={Box({50,0,-20},{500,500,20})};
    MeshSafe(path,a,b,scene,300);
    // Render a long MP snapshot near the storage limit. The slack allocated
    // across its two legs must sum to the same length as the server cable.
    scene.brushes.clear();a={-30000,0,30000};b={30000,0,30000};
    path={};path.count=1;path.bend[0][1]=6000;
    constexpr float cable=120000;
    CHECK(JAR_RopePathCurveMode(&path,a.data(),b.data(),cable,curve,Trace,&scene,1)==JAR_ROPE_SEGMENTS);
    double drawn=0;
    for(int i=0;i<JAR_ROPE_SEGMENTS;++i)drawn+=JAR_RopePathDistance(curve[i],curve[i+1]);
    CHECK(drawn>=cable-.05&&drawn<=cable+.03);
}); }
