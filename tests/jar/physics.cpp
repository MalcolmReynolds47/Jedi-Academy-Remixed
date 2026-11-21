// Exercise the production bridge against its pinned Box3D solver.
#undef NDEBUG
#include "game/jar_physics.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <cstdio>
using namespace jar::physics;
std::vector<Vec> floorMesh(){return {{-1024,-1024,0},{1024,-1024,0},{1024,1024,0},{-1024,-1024,0},{1024,1024,0},{-1024,1024,0}};}
void jointTests();
void overhaulTests();
void rotationTests();
void mountedHullTests();
static void workerTests(){
 World serial,adaptive;
 const auto floor=floorMesh();
 assert(serial.Start(floor)&&adaptive.Start(floor,8));
 assert(adaptive.Statistics().workers==1);
 const Vec half{6,6,6};
 // Sleeping bodies don't justify worker overhead. Releasing them ramps up
 // the same running world, then freezing them returns it to one worker.
 for(unsigned i=0;i<512;++i){
  Pose pose;pose.position={float(i%32)*24-384,float(i/32)*24-192,30+float(i%9)};
  for(auto* world:{&serial,&adaptive})assert(world->Prop(i+1,i+1,half,{},pose,true));
 }
 adaptive.Step(1.f/120,800);serial.Step(1.f/120,800);
 assert(adaptive.Statistics().workers==1);
 for(unsigned i=1;i<=512;++i){
  for(auto* world:{&serial,&adaptive}){
   Pose pose;assert(world->Read(i,pose));
   assert(world->Prop(i,i,half,{},pose,false));
  }
 }
 for(int i=0;i<90;++i){
  serial.Step(1.f/120,800);adaptive.Step(1.f/120,800);
  if(i==0)assert(adaptive.Statistics().workers==8);
 }
 for(unsigned i=1;i<=512;++i){
  Pose expected,actual;
  assert(serial.Read(i,expected)&&adaptive.Read(i,actual));
  assert(expected.awake==actual.awake);
  const Vec deltas[]={
   {expected.position.x-actual.position.x,expected.position.y-actual.position.y,expected.position.z-actual.position.z},
   {expected.linear.x-actual.linear.x,expected.linear.y-actual.linear.y,expected.linear.z-actual.linear.z},
   {expected.angles.x-actual.angles.x,expected.angles.y-actual.angles.y,expected.angles.z-actual.angles.z},
   {expected.angular.x-actual.angular.x,expected.angular.y-actual.angular.y,expected.angular.z-actual.angular.z}
  };
  for(const auto delta:deltas)assert(delta.x==0&&delta.y==0&&delta.z==0);
  assert(adaptive.Prop(i,i,half,{},actual,true));
 }
 adaptive.Step(1.f/120,800);
 assert(adaptive.Statistics().workers==1&&adaptive.Statistics().awake==0);
 adaptive.Stop();assert(adaptive.Statistics().workers==0);
 assert(adaptive.Start(floor,2)&&adaptive.Statistics().workers==1);
}
static void gripTests(){
 PhysgunGrip grip;
 assert(grip.Begin({0,0,300},{},{18,0,300},0));
 assert(std::fabs(grip.localPoint.x-18)<.0001f);
 const auto yaw=grip.Facing(90);
 assert(std::fabs(yaw.y-90)<.0001f&&yaw.x==0);
 const auto pivot=Vec{18,0,300};
 const auto origin=grip.Origin(pivot,yaw);
 assert(std::fabs(origin.x-18)<.001f&&std::fabs(origin.y+18)<.001f);
 const auto raised=grip.Origin({18,0,340},yaw);
 assert(std::fabs(raised.z-340)<.001f&&yaw.x==0);

 World world;assert(world.Start(floorMesh()));
 Pose pose;pose.position={0,0,300};
 assert(world.Prop(1,1,{10,12,15},{3,2,1},pose,false));
 for(int i=0;i<120;++i){
  assert(world.DragAt(1,pivot,grip.localPoint,yaw));
  world.Step(1.f/120,800);
  assert(world.Read(1,pose));
  const auto held=grip.Point(pose.position,pose.angles);
  assert(std::fabs(held.x-pivot.x)<.9f&&std::fabs(held.y-pivot.y)<.9f&&std::fabs(held.z-pivot.z)<.9f);
 }
 assert(std::fabs(pose.angles.y-90)<.1f);
 grip.Turn(0,30);
 const auto turned=grip.Facing(90);
 assert(std::fabs(turned.x-30)<.01f&&std::fabs(turned.y-90)<.01f);
 // A turn around an off-center point must stay pinned even at a 20 Hz
 // game frame rate, with six physics substeps between input updates.
 for(int i=0;i<20;++i){
  assert(world.DragAt(1,pivot,grip.localPoint,turned));
  world.Step(.05f,800);
  assert(world.Read(1,pose));
  const auto contact=grip.Point(pose.position,pose.angles);
  assert(std::fabs(contact.x-pivot.x)<.08f&&std::fabs(contact.y-pivot.y)<.08f&&std::fabs(contact.z-pivot.z)<.08f);
 }
 assert(world.Read(1,pose)&&std::fabs(pose.angles.x-30)<.1f);
 const auto held=grip.Point(pose.position,pose.angles);
 assert(std::fabs(held.z-pivot.z)<.05f);
 assert(world.DragAt(1,{48,0,300},grip.localPoint,turned));
 world.Step(1.f/120,800);
 world.Release(1);
 assert(world.Read(1,pose)&&pose.linear.x>50);

 // Pick up a side already facing the camera, then turn and raise the
 // sight line. The same side faces the heading and its underside is visible.
 assert(grip.Begin({100,0,300},{0,90,0},{90,7,305},0));
 const Vec liftedPivot={0,65,360};
 const auto liftedFacing=grip.Facing(90);
 assert(liftedFacing.x==0&&liftedFacing.y==180&&liftedFacing.z==0);
 const auto liftedOrigin=grip.Origin(liftedPivot,liftedFacing);
 const auto liftedContact=grip.Point(liftedOrigin,liftedFacing);
 assert(std::fabs(liftedOrigin.z-355)<.001f&&std::fabs(liftedContact.y-65)<.001f);
 grip.Turn(0,30);
 const auto cameraPitch=grip.Facing(90);
 assert(std::fabs(cameraPitch.x)<.001f&&std::fabs(cameraPitch.z-30)<.001f);

 // At a steep elevation the camera-up axis tilts toward its heading.
 // A quarter turn about it keeps that axis fixed and rotates the model's
 // forward axis out of the camera's vertical plane.
 assert(grip.Begin({0,0,300},{},{18,0,300},0));
 grip.Turn(90,0,90,0);
 const auto tiltedYaw=grip.Facing(0);
 assert(std::fabs(tiltedYaw.z-90)<.01f&&std::fabs(tiltedYaw.y)<.01f);
 const auto tiltedOrigin=grip.Origin(pivot,tiltedYaw);
 const auto tiltedContact=grip.Point(tiltedOrigin,tiltedYaw);
 assert(std::fabs(tiltedContact.x-pivot.x)<.001f&&std::fabs(tiltedContact.z-pivot.z)<.001f);
 assert(grip.Begin({0,0,300},{},{18,0,300},0));
 grip.Turn(0,90,0,90);
 const auto rolledPitch=grip.Facing(0);
 assert(std::fabs(rolledPitch.y-90)<.01f&&std::fabs(rolledPitch.x)<.01f);
}
int main(){
 jointTests();overhaulTests();rotationTests();mountedHullTests();gripTests();workerTests();
 World configurable;assert(configurable.Start(floorMesh()));Pose setting;setting.position={0,0,300};
 assert(configurable.Prop(1,1,{8,8,8},{},setting,false));const float naturalMass=configurable.Mass(1);
 assert(configurable.Configure(1,200,0)&&fabsf(configurable.Mass(1)-200)<.001f);
 for(int i=0;i<120;++i)configurable.Step(1.f/120,800);assert(configurable.Read(1,setting)&&fabsf(setting.position.z-300)<.01f);
 assert(configurable.Impulse(1,{2000,0,0},setting.position));assert(configurable.Read(1,setting)&&fabsf(setting.linear.x-10)<.01f);
 assert(configurable.Configure(1,400,1)&&fabsf(configurable.Mass(1)-400)<.001f);
 configurable.Step(1.f/120,800);assert(configurable.Read(1,setting)&&setting.linear.z<0);
 assert(configurable.Configure(1,0,1)&&fabsf(configurable.Mass(1)-naturalMass)<.001f);
 assert(!configurable.Configure(1,-1,1)&&!configurable.Configure(1,200,-1)&&!configurable.Configure(2,200,1));
 World w;assert(!w.Active());assert(!w.Start({}));assert(w.Start(floorMesh()));
 Pose p;p.position={0,0,96};assert(w.Prop(10,1,{10,10,10},{0,0,0},p,false));
 for(int i=0;i<240;++i)w.Step(1.f/60,800);
 assert(w.Read(10,p));assert(p.position.z>9.5f&&p.position.z<10.5f);assert(std::fabs(p.linear.z)<1);
 const Pose saved=p;
 // A held prop stays dynamic: dragging into the floor must not teleport through it.
 for(int i=0;i<120;++i){assert(w.Drag(10,{0,0,-50}));w.Step(1.f/60,800);}
 assert(w.Read(10,p));assert(p.position.z>9.0f);
 for(int i=0;i<120;++i){assert(w.Drag(10,{0,0,100}));w.Step(1.f/60,800);}
 assert(w.Read(10,p));assert(std::fabs(p.position.z-100)<.1f);
 assert(w.Prop(10,1,{10,10,10},{0,0,0},p,true));p.position.z=64;p.angles={20,30,40};assert(w.Prop(10,1,{10,10,10},{0,0,0},p,true));
 for(int i=0;i<30;++i)w.Step(1.f/60,800);Pose frozen;assert(w.Read(10,frozen));assert(std::fabs(frozen.position.z-64)<.01f);
 std::fprintf(stderr,"frozen angles %.7f %.7f %.7f\n",frozen.angles.x,frozen.angles.y,frozen.angles.z);assert(std::fabs(frozen.angles.x-20)<.01f&&std::fabs(frozen.angles.y-30)<.01f&&std::fabs(frozen.angles.z-40)<.01f);
 w.Forget(10);assert(!w.Read(10,p));assert(w.Prop(10,2,{10,10,10},{0,0,0},saved,false));
 Pose proxy;proxy.position={-60,0,32};w.BeginProxies();assert(w.Proxy(1,1,{16,16,32},{0,0,0},proxy));w.EndProxies();
 bool touched=false;
 for(int i=0;i<120;++i){proxy.position.x+=1;w.BeginProxies();assert(w.Proxy(1,1,{16,16,32},{0,0,0},proxy));w.EndProxies();w.Step(1.f/60,800);for(const auto&contact:w.Contacts())if(contact.prop==10&&contact.native==1)touched=true;}
 assert(touched);assert(w.Read(10,p));assert(p.position.x>70);assert(proxy.position.x==60); // native controller unchanged
 assert(w.Statistics().proxies==1);w.BeginProxies();w.EndProxies();assert(w.Statistics().proxies==0);
 World restored;assert(restored.Start(floorMesh()));assert(restored.Prop(10,2,{10,10,10},{0,0,0},p,false));Pose after;assert(restored.Read(10,after));
 assert(std::fabs(after.position.x-p.position.x)<.01f&&std::fabs(after.linear.x-p.linear.x)<.01f);
 // Native saves store origin velocity. An offset, rotating body must restore
 // both that velocity and its center-of-mass trajectory without adding spin drift.
 World spin,spinRestored;assert(spin.Start(floorMesh())&&spinRestored.Start(floorMesh()));
 Pose spinning;spinning.position={120,40,300};spinning.angles={20,30,40};spinning.linear={13,-27,41};spinning.angular={.7f,-.4f,1.2f};
 assert(spin.Prop(20,1,{12,9,17},{3,-4,8},spinning,false));Pose snapshot;assert(spin.Read(20,snapshot));
 assert(std::fabs(snapshot.linear.x-spinning.linear.x)<.001f&&std::fabs(snapshot.linear.y-spinning.linear.y)<.001f&&std::fabs(snapshot.linear.z-spinning.linear.z)<.001f);
 for(int i=0;i<7;++i)spin.Step(1.f/60,800);assert(spin.Read(20,snapshot));
 assert(spinRestored.Prop(20,1,{12,9,17},{3,-4,8},snapshot,false));Pose resumed;assert(spinRestored.Read(20,resumed));
 assert(std::fabs(resumed.linear.x-snapshot.linear.x)<.001f&&std::fabs(resumed.linear.y-snapshot.linear.y)<.001f&&std::fabs(resumed.linear.z-snapshot.linear.z)<.001f);
 for(int i=0;i<10;++i){spin.Step(1.f/60,800);spinRestored.Step(1.f/60,800);}assert(spin.Read(20,snapshot)&&spinRestored.Read(20,resumed));
 assert(std::fabs(snapshot.position.x-resumed.position.x)<.01f&&std::fabs(snapshot.position.y-resumed.position.y)<.01f&&std::fabs(snapshot.position.z-resumed.position.z)<.01f);
 assert(spin.Correct(20,snapshot.position,{30,40,50}));assert(spin.Read(20,snapshot));
 assert(std::fabs(snapshot.linear.x-30)<.001f&&std::fabs(snapshot.linear.y-40)<.001f&&std::fabs(snapshot.linear.z-50)<.001f);
 Pose invalid=p;invalid.position.x=std::numeric_limits<float>::quiet_NaN();assert(!w.Prop(11,3,{10,10,10},{0,0,0},invalid,false));
 w.Stop();assert(!w.Active()&&w.Statistics().props==0);assert(!w.Read(10,p));assert(w.Start(floorMesh()));
 for(unsigned i=0;i<propLimit;++i){Pose q;q.position={float(i%16)*32-240,float(i/16)*32-350,40};assert(w.Prop(i+1,i+1,{10,10,10},{0,0,0},q,true));}
 assert(!w.Prop(propLimit+100,propLimit+100,{10,10,10},{0,0,0},saved,true));w.Stop();assert(w.Statistics().props==0);
 std::puts("PASS: real mesh contact, rotated freeze, native proxy push, saved motion, identities, teardown and capacity");
}

void mountedHullTests(){
 World world(1);
 assert(world.Start(floorMesh()));
 Pose prop;prop.position={0,0,100};
 const Vec half={8,8,8};
 assert(world.Prop(1,1,half,{},prop,false));
 const float mass=world.Mass(1);
 std::vector<MountedHull> mounts={{2,1,{3,3,3},{0,0,-18}}};
 assert(world.Prop(1,1,half,{},prop,false,&mounts)&&world.Mass(1)==mass);
 for(int i=0;i<180;++i){
  assert(world.Drag(1,{0,0,-20}));
  world.Step(1.f/120,800);
 }
 assert(world.Read(1,prop)&&prop.position.z>20&&prop.position.z<22);
 assert(world.Prop(1,1,half,{},prop,false)); // The expired charge stops participating.
 for(int i=0;i<120;++i){
  assert(world.Drag(1,{0,0,-20}));
  world.Step(1.f/120,800);
 }
 assert(world.Read(1,prop)&&prop.position.z>7&&prop.position.z<9);

 world.Forget(1);prop={};prop.position={0,0,100};
 Pose other;other.position={40,0,100};
 mounts={{2,1,{3,3,3},{18,0,0}}};
 assert(world.Prop(5,1,{4,16,16},{},other,true));
 assert(world.Prop(1,2,half,{},prop,false,&mounts));
 for(int i=0;i<120;++i){
  assert(world.Drag(1,{30,0,100}));world.Step(1.f/120,0);
 }
 assert(world.Read(1,prop)&&prop.position.x>13&&prop.position.x<17);
 world.Forget(5);

 world.Forget(1);prop={};prop.position={0,0,100};
 Pose actor;actor.position={0,24,100};
 Pose obstacle;obstacle.position={200,0,100};
 assert(world.Proxy(3,1,{12,4,14},{},actor,true));
 assert(world.Proxy(4,1,{5,5,5},{},obstacle)); // The charge proxy has its own budget.
 assert(world.Prop(1,2,half,{},prop,false));
 for(int i=0;i<120;++i){
  assert(world.Drag(1,actor.position));world.Step(1.f/120,0);
 }
 assert(world.Read(1,prop)&&std::fabs(prop.position.y-24)<.1f); // Core hull bypasses charge-only proxies.
 world.Forget(1);prop={};prop.position={0,0,100};
 mounts={{2,2,{3,3,3},{24,0,0}}};
 assert(world.Prop(1,3,half,{},prop,false,&mounts)&&world.Rotate(1,90,0));
 bool contact=false;
 for(int i=0;i<120;++i){
  assert(world.Drag(1,{0,0,100}));world.Step(1.f/120,0);
  for(const auto& hit:world.Contacts())if(hit.prop==1&&hit.native==3)contact=true;
 }
 assert(contact&&world.Read(1,prop)&&prop.angles.y>25&&prop.angles.y<80);
 assert(world.Mass(1)==mass);
 world.BeginProxies();world.KeepProxy(3,1,0,true);world.EndProxies();
 assert(world.Statistics().proxies==1);
 // A recycled charge slot can replace geometry without replacing the support.
 mounts={{2,3,{3,3,3},{0,0,20}}};
 assert(world.Prop(1,3,half,{},prop,false,&mounts));
 for(int i=0;i<120;++i){
  assert(world.Drag(1,{0,0,100}));world.Step(1.f/120,0);
 }
 assert(world.Read(1,prop)&&std::fabs(prop.angles.y-90)<.1f&&world.Mass(1)==mass);
 world.BeginProxies();world.EndProxies();
 assert(world.Statistics().proxies==0);
 std::puts("PASS: mounted solver contact, isolated actor proxy, rotation, mass and removal");
}

void rotationTests(){
 World w;assert(w.Start(floorMesh()));Pose a,b;a.position={0,0,300};b.position={64,0,300};
 assert(w.Prop(10,1,{8,9,10},{},a,false));
 assert(w.Rotate(10,90,30));
 for(int i=0;i<120;++i){w.Drag(10,a.position);w.Step(1.f/120,800);}
 assert(w.Read(10,a));assert(std::fabs(a.angles.y-90)<.1f&&std::fabs(a.angles.x-30)<.1f);
 w.EndRotate(10);w.Drag(10,{60,0,300});w.Step(1.f/120,800);w.Release(10);assert(w.Read(10,a));assert(a.linear.x>10); // fling survives rotation
 assert(!w.Rotate(10,std::numeric_limits<float>::quiet_NaN(),0));
 assert(!w.Rotate(10,181,0));assert(!w.Rotate(99,10,0));
 // Rotation must retain, and obey, existing attachments instead of teleporting.
 w.Stop();assert(w.Start(floorMesh()));a={};a.position={0,0,300};b={};b.position={64,0,300};
 assert(w.Prop(10,1,{8,8,8},{},a,true));assert(w.Prop(20,2,{8,8,8},{},b,true));
 jar::constraint::Joint weld;weld.id=1;weld.kind=jar::constraint::Weld;weld.first=1;weld.second=2;
 assert(w.JointFrames(10,20,{32,0,300},{0,0,1},weld.a,weld.b)&&w.Joint(weld,10,20));
 assert(w.Prop(20,2,{8,8,8},{},b,false));assert(w.Rotate(20,90,0));
 for(int i=0;i<120;++i){w.Drag(20,b.position);w.Step(1.f/120,800);}
 assert(w.Read(20,b));assert(w.Statistics().joints==1&&std::fabs(b.angles.y)<2&&std::fabs(b.position.x-64)<1);
 // A free hinge permits rotation around its own axis.
 w.ForgetJoint(1);w.Release(20);b={};b.position={64,0,300};assert(w.Prop(20,2,{8,8,8},{},b,true));
 auto hinge=weld;hinge.id=2;hinge.kind=jar::constraint::Hinge;assert(w.JointFrames(10,20,b.position,{0,0,1},hinge.a,hinge.b)&&w.Joint(hinge,10,20));
 assert(w.Prop(20,2,{8,8,8},{},b,false));assert(w.Rotate(20,90,0));
 for(int i=0;i<120;++i){w.Drag(20,{64,0,300});w.Step(1.f/120,800);}assert(w.Read(20,b));assert(std::fabs(b.angles.y-90)<2&&w.Statistics().joints==1);
}

void jointTests(){
 using jar::constraint::Joint;
 // Both bodies begin frozen, including arbitrary rotations/offset centers.
 World w;assert(w.Start(floorMesh()));Pose a,b;a.position={0,0,300};a.angles={20,30,40};b.position={64,0,300};b.angles={-10,75,25};
 assert(w.Prop(10,1,{8,9,10},{2,3,4},a,true));assert(w.Prop(20,2,{8,9,10},{-2,1,5},b,true));
 Joint weld;weld.id=1;weld.kind=jar::constraint::Weld;weld.first=1;weld.second=2;
 assert(w.JointFrames(10,20,{32,0,300},{0,1,0},weld.a,weld.b));assert(w.Joint(weld,10,20));assert(w.Statistics().joints==1);
 assert(w.Prop(20,2,{8,9,10},{-2,1,5},b,false));
 for(int i=0;i<120;++i)w.Step(1.f/60,800);Pose read;assert(w.Read(20,read));
 std::fprintf(stderr,"weld x=%.6f z=%.6f yaw=%.6f\n",read.position.x,read.position.z,read.angles.y);
 // Box3D uses a compliant iterative constraint; require sub-unit drift under gravity.
 assert(std::fabs(read.position.x-64)<.5f&&std::fabs(read.position.z-300)<.5f&&std::fabs(read.angles.y-75)<.2f);
 // A body recreation must destroy runtime links; immutable records recreate them.
 w.Forget(20);assert(w.Statistics().joints==0);assert(w.Prop(20,2,{8,9,10},{-2,1,5},read,false));assert(w.Joint(weld,10,20));
 w.BeginJoints();assert(w.Joint(weld,10,20));w.EndJoints();assert(w.Statistics().joints==1);
 w.BeginJoints();w.EndJoints();assert(w.Statistics().joints==0);
 // A reused entity slot cannot inherit the prior ownership identity.
 assert(w.Prop(20,99,{8,9,10},{-2,1,5},read,false));assert(!w.Joint(weld,10,20));
 w.Stop();assert(!w.Statistics().joints);
 // A valid, slightly rounded saved quaternion is normalized at the solver boundary.
 assert(w.Start(floorMesh()));assert(w.Prop(10,1,{8,8,8},{},a,true));assert(w.Prop(20,2,{8,8,8},{},b,true));
 auto rounded=weld;rounded.a.rotation[0]*=1.0001f;rounded.a.rotation[1]*=1.0001f;rounded.a.rotation[2]*=1.0001f;rounded.a.rotation[3]*=1.0001f;
 assert(jar::constraint::Valid(rounded));assert(w.Joint(rounded,10,20));w.Stop();
 // A horizontal hinge swings under gravity while retaining its pivot radius.
 assert(w.Start(floorMesh()));a={};b={};a.position={0,0,300};b.position={64,0,300};
 assert(w.Prop(10,1,{8,8,8},{},a,true));assert(w.Prop(20,2,{8,8,8},{},b,true));
 Joint hinge;hinge.id=2;hinge.kind=jar::constraint::Hinge;hinge.first=1;hinge.second=2;
 assert(w.JointFrames(10,20,{0,0,300},{0,1,0},hinge.a,hinge.b));assert(w.Joint(hinge,10,20));
 assert(w.Prop(20,2,{8,8,8},{},b,false));for(int i=0;i<20;++i)w.Step(1.f/60,800);
 assert(w.Read(20,b));assert(b.position.z<280&&std::fabs(b.angles.x)>20);
 float radius=std::sqrt(b.position.x*b.position.x+(b.position.z-300)*(b.position.z-300));assert(std::fabs(radius-64)<.4f);
 // Restore the original local frames, not a newly captured rest pose.
 World resumed;assert(resumed.Start(floorMesh()));assert(resumed.Prop(10,1,{8,8,8},{},a,true));assert(resumed.Prop(20,2,{8,8,8},{},b,false));assert(resumed.Joint(hinge,10,20));
 for(int i=0;i<40;++i){w.Step(1.f/60,800);resumed.Step(1.f/60,800);}Pose other;assert(w.Read(20,b)&&resumed.Read(20,other));
 assert(std::fabs(b.position.x-other.position.x)<.5f&&std::fabs(b.position.z-other.position.z)<.5f);
 resumed.ForgetJoint(2);assert(!resumed.Statistics().joints);for(int i=0;i<120;++i)resumed.Step(1.f/60,800);assert(resumed.Read(20,other));assert(other.position.z<50);
 // Physgun dragging a linked member must retain angular freedom around the hinge.
 for(int i=0;i<180;++i){assert(w.Drag(20,{0,0,364}));w.Step(1.f/60,800);}assert(w.Read(20,other));
 std::fprintf(stderr,"hinge drag x=%.3f z=%.3f\n",other.position.x,other.position.z);
 assert(std::fabs(other.position.x)<5&&std::fabs(other.position.z-364)<5);
 // Invalid axes/frames never reach solver assertions.
 assert(!w.JointFrames(10,20,{0,0,300},{},hinge.a,hinge.b));auto invalid=hinge;invalid.a.rotation[3]=100;assert(!w.Joint(invalid,10,20));
 w.Stop();assert(w.Start(floorMesh()));a={};a.position={0,0,300};assert(w.Prop(1,1,{8,8,8},{},a,true));
 for(unsigned i=0;i<jar::constraint::limit+1;++i){assert(w.Prop(i+2,i+2,{8,8,8},{},a,true));Joint j;j.id=i+1;j.kind=jar::constraint::Weld;j.first=1;j.second=i+2;
  assert(w.Joint(j,1,i+2)==(i<jar::constraint::limit));}
 assert(w.Statistics().joints==128);w.Stop();assert(!w.Statistics().joints);
 // Both endpoint orders work and independent anchors share one static body.
 assert(w.Start(floorMesh()));
 const auto fixed=jar::constraint::NativeId(16382);
 a={};b={};a.position={-128,0,300};b.position={128,0,300};
 assert(w.Prop(10,1,{8,8,8},{},a,false));assert(w.Prop(20,2,{8,8,8},{},b,false));
 weld.first=1;weld.second=fixed;
 assert(LocalFrame(a,a.position,{0,0,1},weld.a)&&LocalFrame({},a.position,{0,0,1},weld.b));
 hinge.first=fixed;hinge.second=2;
 assert(LocalFrame({},{64,0,300},{0,1,0},hinge.a)&&LocalFrame(b,{64,0,300},{0,1,0},hinge.b));
 w.BeginJoints();
 assert(w.Joint(weld,10,World::worldAnchor)&&w.Joint(hinge,World::worldAnchor,20));
 w.EndJoints();
 for(int i=0;i<20;++i)w.Step(1.f/60,800);
 assert(w.Read(10,a)&&std::fabs(a.position.z-300)<.5f);
 assert(w.Read(20,b)&&b.position.z<280&&std::fabs(b.angles.x)>20);
 assert(w.Statistics().props==2&&w.Statistics().joints==2);
 w.Forget(10);assert(w.Statistics().joints==1);
 w.BeginJoints();w.EndJoints();assert(w.Statistics().joints==0);
 std::puts("PASS: rotated weld, frozen-to-dynamic hinge, pivot radius, restored swing, unlink, identity reuse, invalid frames and 128-joint cap");
}

void overhaulTests(){
 World w;assert(w.Start(floorMesh()));Pose small,large;small.position={-100,0,200};large.position={100,0,200};
 assert(w.Prop(1,1,{8,8,8},{},small,false));assert(w.Prop(2,2,{16,16,16},{},large,false));
 const float m=w.Mass(1);assert(m>1 && std::fabs(w.Mass(2)/m-8)<.001f);
 assert(w.Impulse(1,{800,0,0},small.position));assert(w.Impulse(2,{800,0,0},large.position));
 assert(w.Read(1,small)&&w.Read(2,large));assert(std::fabs(small.linear.x/large.linear.x-8)<.001f);
 // A quick sweep leaves velocity. Releasing re-enables gravity and rotation without zeroing that velocity.
 for(int i=0;i<12;++i){assert(w.Drag(1,{-80.f+i*8,0,200}));w.Step(1.f/120,800);}
 assert(w.Read(1,small));const auto released=small;assert(released.linear.x>20);w.Release(1);
 w.Step(1.f/120,800);assert(w.Read(1,small));assert(small.position.x>released.position.x);assert(small.linear.z<released.linear.z);
 World timing;assert(timing.Start(floorMesh()));for(int i=0;i<20;++i)timing.Step(.05f,800);assert(timing.Statistics().steps==120);
 // Real collision-hit events are used by rate-limited prop audio.
 World hits;assert(hits.Start(floorMesh()));Pose drop;drop.position={0,0,200};assert(hits.Prop(1,1,{8,8,8},{},drop,false));bool impact=false;
 for(int i=0;i<240;++i){hits.Step(1.f/120,800);for(const auto&e:hits.Impacts())if(e.prop==1&&e.speed>48)impact=true;}assert(impact);
 // Rope is slack below its maximum and arrests separation above it.
 World rope;assert(rope.Start(floorMesh()));Pose anchor,payload;anchor.position={0,0,400};payload.position={64,0,400};
 assert(rope.Prop(1,1,{8,8,8},{},anchor,true));assert(rope.Prop(2,2,{8,8,8},{},payload,false));
 jar::constraint::Joint link;link.id=1;link.kind=jar::constraint::Rope;link.first=1;link.second=2;link.length=96;
 assert(rope.Joint(link,1,2));for(int i=0;i<120;++i)rope.Step(1.f/120,800);
 assert(rope.Read(2,payload));float d=std::sqrt(payload.position.x*payload.position.x+(payload.position.z-400)*(payload.position.z-400));
 std::fprintf(stderr,"rope radius %.5f z %.5f\n",d,payload.position.z);assert(d<98 && payload.position.z<340);
 Vec a,b;assert(rope.JointPoints(link,1,2,a,b));assert(std::fabs(a.z-400)<.01f);
 // A saved thruster exerts finite force, responds to mass, toggles and removes cleanly.
 World thrust;assert(thrust.Start(floorMesh()));Pose body;body.position={0,0,300};assert(thrust.Prop(1,1,{8,8,8},{},body,false));
 jar::constraint::Joint engine;engine.id=1;engine.kind=jar::constraint::Thruster;engine.first=engine.second=1;engine.force=500;
 assert(thrust.Joint(engine,1,1));for(int i=0;i<12;++i)thrust.Step(1.f/120,0);assert(thrust.Read(1,body));assert(body.linear.z< -1); // outward +Z mount now pushes into the surface.
 engine.enabled=0;assert(thrust.Joint(engine,1,1));const float velocity=body.linear.z;for(int i=0;i<12;++i)thrust.Step(1.f/120,0);
 assert(thrust.Read(1,body));assert(std::fabs(body.linear.z-velocity)<.01f);thrust.Forget(1);assert(thrust.Statistics().joints==0);
 std::puts("PASS: mass-scaled impulses, fling/gravity restoration, 120 Hz, impact audio events, slack rope and switchable thruster");
}
