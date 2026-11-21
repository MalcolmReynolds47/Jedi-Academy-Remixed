// The production Box3D bridge with real concave meshes; no solver mocks.
#undef NDEBUG
#include "game/jar_physics.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace jar::physics;
std::vector<Vec> floorMesh(){return {{-4096,-4096,-256},{4096,-4096,-256},{4096,4096,-256},{-4096,-4096,-256},{4096,4096,-256},{-4096,4096,-256}};}
// An open room, with a floor and two walls. Its bounding box contains air.
std::vector<Vec> room(){return {
 {-64,-64,0},{64,-64,0},{64,64,0},{-64,-64,0},{64,64,0},{-64,64,0},
 {64,-64,0},{64,-64,128},{64,64,128},{64,-64,0},{64,64,128},{64,64,0},
 {-64,-64,0},{-64,64,128},{-64,-64,128},{-64,-64,0},{-64,64,0},{-64,64,128}};}
void tick(World&w,Pose&brush,unsigned id=1,unsigned model=7){w.BeginProxies();assert(w.MeshProxy(90,id,model,brush));w.EndProxies();w.Step(1.f/120,800);}
int main(){
 World w;assert(w.Start(floorMesh()));assert(w.AddProxyMesh(7,room()));assert(!w.AddProxyMesh(7,room()));
 Pose brush,p;p.position={0,0,64};assert(w.Prop(1,1,{8,8,8},{},p,false));
 bool contact=false;for(int i=0;i<240;++i){tick(w,brush);for(const auto&c:w.Contacts())if(c.prop==1&&c.native==90)contact=true;}
 assert(contact&&w.Read(1,p));std::fprintf(stderr,"Room rest: %.6f %.6f %.6f\n",p.position.x,p.position.y,p.position.z);assert(std::fabs(p.position.z-8)<.3f&&std::fabs(p.position.x)<.5f&&std::fabs(p.position.y)<.5f);
 // The moving native floor carries the prop; no box fills its interior.
 for(int i=0;i<120;++i){brush.position.z+=.25f;tick(w,brush);}assert(w.Read(1,p));assert(std::fabs(p.position.z-38)<.5f);
 // A fast moving surface imparts velocity. The launched prop can leave the
 // open room, but must land on either the room floor or the lower world floor.
 brush.position.z+=10;tick(w,brush);assert(w.Read(1,p)&&p.linear.z>0);
 for(int i=0;i<600;++i)tick(w,brush);assert(w.Read(1,p));std::fprintf(stderr,"Fast floor rest: %.4f %.4f %.4f\n",p.position.x,p.position.y,p.position.z);assert(std::fabs(p.position.z-48)<.5f||std::fabs(p.position.z+248)<.5f);
 // Reused slot identity teleports to the new native entity, not a swept old pose.
 brush.position={512,256,20};brush.angles={0,90,0};tick(w,brush,2);
 p={};p.position={512,256,84};w.Forget(1);assert(w.Prop(1,2,{8,8,8},{},p,false));
 for(int i=0;i<240;++i)tick(w,brush,2);assert(w.Read(1,p)&&std::fabs(p.position.z-28)<.2f);
 // Rotation puts the x=64 wall at y=320; movement into it is arrested.
 assert(w.Impulse(1,{0,5000,0},p.position));for(int i=0;i<120;++i)tick(w,brush,2);
 assert(w.Read(1,p)&&p.position.y<313&&p.position.y>285);
 // A second instance shares one geometry allocation, but not its transform.
 w.BeginProxies();assert(w.MeshProxy(90,2,7,brush));Pose another;another.position={-500,0,0};assert(w.MeshProxy(91,1,7,another));w.EndProxies();
 assert(w.Statistics().proxyMeshes==1&&w.Statistics().proxyTriangles==6&&w.Statistics().proxies==2);
 // Slot reuse between box and mesh proxies must replace the old shape.
 w.BeginProxies();assert(w.Proxy(90,3,{8,8,8},{},{}));w.EndProxies();assert(!w.HasProxyMesh(7)&&w.Statistics().proxyTriangles==0);
 assert(w.AddProxyMesh(8,room()));w.BeginProxies();assert(w.MeshProxy(90,4,8,{}));w.EndProxies();assert(w.Statistics().proxies==1);
 w.BeginProxies();w.EndProxies();assert(w.Statistics().proxies==0&&w.Statistics().proxyMeshes==0);
 // Failed uploads do not consume capacity or publish usable geometry.
 assert(!w.AddProxyMesh(0,room())&&!w.AddProxyMesh(1,{})&&!w.AddProxyMesh(1,{{0,0,0}}));
 auto bad=room();bad[0].x=std::numeric_limits<float>::quiet_NaN();assert(!w.AddProxyMesh(1,bad));assert(!w.HasProxyMesh(1));
 assert(!w.MeshProxy(90,1,999,{}));assert(w.AddProxyMesh(1,room()));Pose invalid;invalid.angles.y=std::numeric_limits<float>::infinity();assert(!w.MeshProxy(90,1,1,invalid));
 w.Stop();assert(!w.Active()&&!w.HasProxyMesh(1)&&w.Statistics().proxyTriangles==0);
 // Independent aggregate mesh/body budgets and teardown, without stepping load.
 assert(w.Start(floorMesh()));for(unsigned n=1;n<=proxyLimit;++n){assert(w.AddProxyMesh(n,room()));assert(w.MeshProxy(n,n,n,{}));}
 assert(!w.AddProxyMesh(proxyLimit+1,room())&&!w.MeshProxy(proxyLimit+1,1,1,{}));w.Stop();
 assert(w.Start(floorMesh()));assert(w.AddProxyMesh(1,room()));std::vector<Vec> oversized((triangleLimit-5)*3);assert(!w.AddProxyMesh(2,oversized));assert(w.Statistics().proxyTriangles==6);w.Stop();
 std::puts("PASS: hollow brush interiors, translated/rotated collision, native contact, moving support, shared meshes, reuse, budgets and teardown");
}
