// Production native/mixed projection, with native movement already advanced.
#include "game/jar_physics.h"
#include "test_support.h"
#include <cmath>
#include <limits>
#include <chrono>
using namespace jar::physics;
static jar::constraint::Joint Link(unsigned kind,ConstraintBody&a,ConstraintBody&b,Vec anchor,Vec axis={0,0,1}){
 jar::constraint::Joint j;j.id=1;j.kind=kind;j.first=a.key;j.second=b.key;CHECK(LocalFrame(a.pose,anchor,axis,j.a)&&LocalFrame(b.pose,anchor,axis,j.b));return j;
}
static void Advance(std::vector<ConstraintBody>&b,float dt,bool gravity){for(auto&v:b){if(v.movable&&gravity)v.pose.linear.z-=800*dt;v.pose.position.x+=v.pose.linear.x*dt;v.pose.position.y+=v.pose.linear.y*dt;v.pose.position.z+=v.pose.linear.z*dt;v.pose.angles=AdvanceAngles(v.pose.angles,v.pose.angular,dt);}}
int main(){return RunTests("native joint graph, mass, frame rates, hinge freedom, moving anchors and invalid input",[]{
 // r cross impulse, rotated anisotropic inertia, mass and size scaling.
 auto torque=AngularImpulse({}, {1,2,3}, 12, {0,6,0}, {2,0,0});CHECK(std::fabs(torque.z-.6f)<.00001f);
 CHECK(AngularImpulse({}, {1,2,3}, 12, {0,6,0}, {}).z==0);
 CHECK(std::fabs(AngularImpulse({}, {1,2,3}, 24, {0,6,0}, {2,0,0}).z-.3f)<.00001f);
 CHECK(std::fabs(AngularImpulse({}, {2,4,6}, 12, {0,6,0}, {2,0,0}).z-.15f)<.00001f);
 torque=AngularImpulse({0,90,0},{1,2,3},12,{0,0,52},{0,1,0});CHECK(std::fabs(torque.x-1.3f)<.00001f&&std::fabs(torque.y)<.00001f);
 CHECK(AngularImpulse({}, {0,2,3},12,{0,6,0},{2,0,0}).z==0);
 for(float dt:{.05f,1.f/60,1.f/120}){
  ConstraintSolver solver;std::vector<ConstraintBody>b(2);b[0].key=1;b[1].key=2;b[0].movable=false;b[0].pose.position={0,0,200};b[1].pose.position={50,0,200};
  auto j=Link(jar::constraint::Weld,b[0],b[1],{0,0,200});
  for(int i=0;i<int(3/dt);++i){Advance(b,dt,true);CHECK(solver.Solve(b,{j},dt));}
  std::printf("dt %.6f welded xyz %.5f %.5f %.5f speed %.5f\n",dt,b[1].pose.position.x,b[1].pose.position.y,b[1].pose.position.z,b[1].pose.linear.z);
  CHECK(std::fabs(b[1].pose.position.x-50)<.1f&&std::fabs(b[1].pose.position.z-200)<.1f&&std::fabs(b[1].pose.linear.z)<.1f);
  // A moving native anchor translates the weld once per frame, never twice.
  b[0].pose.linear.x=60;const float start=b[1].pose.position.x;const int frames=int(2/dt);
  for(int i=0;i<frames;++i){Advance(b,dt,true);CHECK(solver.Solve(b,{j},dt));}
  CHECK(std::fabs((b[1].pose.position.x-start)-60*frames*dt)<.2f);
  b[0].pose.linear={};b[0].pose.angular={0,0,1};
  for(int i=0;i<int(1/dt);++i){Advance(b,dt,true);CHECK(solver.Solve(b,{j},dt));}
  Vec pivotA,pivotB,axis;CHECK(FrameTransform(b[0].pose,j.a,pivotA,axis)&&FrameTransform(b[1].pose,j.b,pivotB,axis));
  CHECK(std::fabs(pivotA.x-pivotB.x)<.1f&&std::fabs(pivotA.y-pivotB.y)<.1f&&std::fabs(pivotA.z-pivotB.z)<.1f);
  // Axis spin remains free on a hinge but is removed by a weld.
  solver.Clear();b[0].pose={};b[0].pose.position={0,0,200};b[1].pose=b[0].pose;j=Link(jar::constraint::Hinge,b[0],b[1],{0,0,200});b[1].pose.angular={0,0,1};
  for(int i=0;i<int(1/dt);++i){Advance(b,dt,false);CHECK(solver.Solve(b,{j},dt));}
  CHECK(std::fabs(b[1].pose.angles.y-57.29578f*int(1/dt)*dt)<.3f&&std::fabs(b[1].pose.angular.z-1)<.01f);
  b[1].pose=b[0].pose;j=Link(jar::constraint::Weld,b[0],b[1],{0,0,200});b[1].pose.angular={0,0,1};
  for(int i=0;i<int(1/dt);++i){Advance(b,dt,false);CHECK(solver.Solve(b,{j},dt));}CHECK(std::fabs(b[1].pose.angles.y)<.1f&&std::fabs(b[1].pose.angular.z)<.01f);
 }
 // Reciprocal impulses use mass, not a first/second master override.
 ConstraintSolver solver;std::vector<ConstraintBody>b(2);b[0].key=1;b[1].key=2;b[0].mass=80;b[1].mass=160;b[0].pose.position={0,0,200};b[1].pose.position={0,40,200};
 auto j=Link(jar::constraint::Weld,b[0],b[1],{0,20,200});b[0].pose.linear.y=-20;b[1].pose.linear.y=10;Advance(b,.05f,false);CHECK(solver.Solve(b,{j},.05f));
 CHECK(std::fabs(b[0].pose.linear.y*80+b[1].pose.linear.y*160)<.1f&&std::fabs(b[0].pose.linear.y)<.1f&&std::fabs(b[1].pose.linear.y)<.1f);
 auto bad=b;bad[0].mass=0;CHECK(!solver.Solve(bad,{j},.05f));bad=b;bad[0].pose.position.x=std::numeric_limits<float>::quiet_NaN();CHECK(!solver.Solve(bad,{j},.05f));
 auto broken=j;broken.second=3;CHECK(!solver.Solve(b,{broken},.05f));CHECK(!solver.Solve(b,{j},.2f));CHECK(solver.Solve(b,{},.05f));
 b[0].movable=false;b[0].pose={};b[1].pose={};b[1].pose.position={400,0,200};j=Link(jar::constraint::Weld,b[0],b[1],{});
 for(int i=0;i<60;++i){Advance(b,.05f,true);CHECK(solver.Solve(b,{j},.05f));}
 std::printf("Long-arm weld endpoint %.5f %.5f\n",b[1].pose.position.x,b[1].pose.position.z);
 CHECK(std::fabs(b[1].pose.position.x-400)<.1f&&std::fabs(b[1].pose.position.z-200)<.1f);
 std::vector<ConstraintBody>chain(6);std::vector<jar::constraint::Joint>chainLinks;
 for(unsigned i=0;i<chain.size();++i){chain[i].key=i+1;chain[i].pose.position={i*40.f,0,200};chain[i].movable=i!=0;if(i){auto link=Link(jar::constraint::Weld,chain[i-1],chain[i],chain[i-1].pose.position);link.id=i;chainLinks.push_back(link);}}
 for(int i=0;i<60;++i){Advance(chain,.05f,true);CHECK(solver.Solve(chain,chainLinks,.05f));}
 std::printf("Chain endpoint %.5f %.5f, vertical speed %.5f\n",chain.back().pose.position.x,chain.back().pose.position.z,chain.back().pose.linear.z);
 CHECK(std::fabs(chain.back().pose.position.x-200)<.1f&&std::fabs(chain.back().pose.position.z-200)<.1f&&std::fabs(chain.back().pose.linear.z)<.1f);
 // Closing a loop introduces redundant rows; it must not destabilize the graph.
 auto loop=Link(jar::constraint::Weld,chain.front(),chain.back(),chain.front().pose.position);loop.id=6;chainLinks.push_back(loop);
 for(int i=0;i<20;++i){Advance(chain,.05f,true);CHECK(solver.Solve(chain,chainLinks,.05f));}
 CHECK(std::fabs(chain.back().pose.position.z-200)<.1f&&std::fabs(chain.back().pose.linear.z)<.1f);
 // Exercise one connected assembly at the complete 128-link capacity.
 chain.assign(129,ConstraintBody{});chainLinks.clear();
 for(unsigned i=0;i<chain.size();++i){chain[i].key=i+1;chain[i].pose.position={i*20.f,0,200};chain[i].movable=i!=0;if(i){auto link=Link(jar::constraint::Weld,chain[i-1],chain[i],chain[i-1].pose.position);link.id=i;chainLinks.push_back(link);}}
 const auto chainStart=std::chrono::steady_clock::now();for(int i=0;i<5;++i){Advance(chain,.05f,true);CHECK(solver.Solve(chain,chainLinks,.05f));}
 std::printf("128-link chain: %.3f ms per headless graph solve, endpoint z %.5f speed %.5f\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-chainStart).count()/5,chain.back().pose.position.z,chain.back().pose.linear.z);
 CHECK(std::fabs(chain.back().pose.position.x-2560)<.1f&&std::fabs(chain.back().pose.position.z-200)<.1f&&std::fabs(chain.back().pose.linear.z)<.1f);
 // A flipped hinge axis converges without fixing the free twist angle.
 b[0].pose={};b[1].pose={};j=Link(jar::constraint::Hinge,b[0],b[1],{});b[1].pose.angles={0,0,180};CHECK(solver.Solve(b,{j},.05f));
 Vec hingePoint,hingeAxis;CHECK(FrameTransform(b[1].pose,j.b,hingePoint,hingeAxis));CHECK(hingeAxis.z>.999f);
 // Fill the actual graph capacity without native game/rendering services.
 std::vector<ConstraintBody>many;std::vector<jar::constraint::Joint>links;
 for(unsigned i=0;i<jar::constraint::limit;++i){ConstraintBody a,c;a.key=2*i+1;c.key=2*i+2;a.movable=false;a.pose.position={float(i*100),0,200};c.pose.position={float(i*100+40),0,200};auto link=Link(jar::constraint::Weld,a,c,a.pose.position);link.id=i+1;many.push_back(a);many.push_back(c);links.push_back(link);}
 const auto began=std::chrono::steady_clock::now();for(int i=0;i<20;++i){Advance(many,.05f,true);CHECK(solver.Solve(many,links,.05f));}
 const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();std::printf("128 independent joints, 256 endpoints: %.3f ms per headless graph solve\n",elapsed/20);
 for(unsigned i=0;i<jar::constraint::limit;++i)CHECK(std::fabs(many[i*2+1].pose.position.z-200)<.1f);
 many.push_back(many.back());CHECK(!solver.Solve(many,links,.05f));
 });}
