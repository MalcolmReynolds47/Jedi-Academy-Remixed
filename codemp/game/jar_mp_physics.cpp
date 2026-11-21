// No game globals cross the C/C++ boundary. The server owns all policies.
#include "jar_mp_physics.h"
#include "../qcommon/jar_capacity.h"
#include "../../code/game/jar_physics.h"
#include "../../code/game/jar_actor_contact.h"
#include <cmath>
#include <map>
#include <string>
namespace {
using namespace jar::physics;
static_assert(JAR_MP_PROP_LIMIT == propLimit, "MP prop admission must match the shared solver");
World world(JAR_MP_RIGID_PROXY_LIMIT,true);ConstraintSolver nativeJoints;std::vector<Vec> triangles;
using ModelHullKey=std::pair<std::string,int>;
struct ModelHull {
 std::shared_ptr<const PropGeometry> geometry;
 std::unique_ptr<NativePropHull> rotated;
 Vec angles;
};
std::map<ModelHullKey,ModelHull> modelHulls;
ModelHull* FindModelHull(const char* model,int frame){
 if(!model)return nullptr;
 const auto found=modelHulls.find({model,frame});
 return found==modelHulls.end()?nullptr:&found->second;
}
constexpr auto mapAnchorIdentity=jar::constraint::NativeId(JAR_MP_ENTITY_SLOTS-2);
unsigned RigidEndpoint(int entity,uint32_t& identity){
 if(entity>=0)return static_cast<unsigned>(entity);
 identity=mapAnchorIdentity;
 return World::worldAnchor;
}
Vec V(const float*p){return {p[0],p[1],p[2]};}
void Put(Vec v,float*p){p[0]=v.x;p[1]=v.y;p[2]=v.z;}
Pose PoseOf(const jarMPPose_t&p){Pose out;out.position=V(p.position);out.angles=V(p.angles);out.linear=V(p.linear);out.angular=V(p.angular);return out;}
PhysgunGrip GripOf(const jarMPGrip_t& grip){return {V(grip.localPoint),V(grip.relativeAngles),true};}
}
int JAR_GripBegin(jarMPGrip_t*grip,const float*origin,const float*angles,const float*point,float viewYaw){
 PhysgunGrip hold;
 if(!hold.Begin(V(origin),V(angles),V(point),viewYaw))return 0;
 Put(hold.localPoint,grip->localPoint);Put(hold.relativeAngles,grip->relativeAngles);return 1;
}
void JAR_GripPoint(const jarMPGrip_t*grip,const float*origin,const float*angles,float*point){Put(GripOf(*grip).Point(V(origin),V(angles)),point);}
void JAR_GripOrigin(const jarMPGrip_t*grip,const float*point,const float*angles,float*origin){Put(GripOf(*grip).Origin(V(point),V(angles)),origin);}
void JAR_GripTurn(jarMPGrip_t*grip,float yaw,float pitch,float viewYaw,float viewPitch,float viewRoll,float*facing){
 auto hold=GripOf(*grip);
 if(yaw||pitch){
  hold.Turn(yaw,pitch,viewPitch,viewRoll);
  Put(hold.relativeAngles,grip->relativeAngles);
 }
 Put(hold.Facing(viewYaw),facing);
}
void JAR_RigidReset(){nativeJoints.Clear();world.Stop();triangles.clear();modelHulls.clear();}
int JAR_RigidPolygon(int count,const float*points,const float*normal){
 if(count<3||triangles.size()+size_t(count-2)*3>triangleLimit*3)return 0;
 for(int i=1;i<count-1;++i){const float*a=points,*b=points+i*3,*c=points+(i+1)*3;float u[3],v[3],cross[3];for(int j=0;j<3;++j){u[j]=b[j]-a[j];v[j]=c[j]-a[j];}
 cross[0]=u[1]*v[2]-u[2]*v[1];cross[1]=u[2]*v[0]-u[0]*v[2];cross[2]=u[0]*v[1]-u[1]*v[0];
 if(cross[0]*cross[0]+cross[1]*cross[1]+cross[2]*cross[2]<.0001f)continue;
 bool forward=cross[0]*normal[0]+cross[1]*normal[1]+cross[2]*normal[2]>0;triangles.push_back(V(a));triangles.push_back(V(forward?b:c));triangles.push_back(V(forward?c:b));}return 1;
}
int JAR_RigidStart(){bool ok=world.Start(triangles);triangles.clear();return ok;}
int JAR_RigidActive(){return world.Active();}
int JAR_RigidBody(unsigned n,unsigned id,const float*half,const float*center,const jarMPPose_t*p,int frozen,float mass,float gravity){return world.Prop(n,id,V(half),V(center),PoseOf(*p),frozen!=0)&&world.Configure(n,mass,gravity,true);}
int JAR_RigidModelHull(const char* model,int frame,jarMPModelHullSource_t source,const float* angles,float* mins,float* maxs){
 if(!model||!source)return 0;
 auto* hull=FindModelHull(model,frame);
 if(!hull){
  if(modelHulls.size()>=8192)modelHulls.erase(modelHulls.begin());
  auto geometry=std::make_shared<PropGeometry>();
  const auto emit=[](void* context,const float* point){static_cast<PropGeometry*>(context)->Add(V(point));};
  auto& cached=modelHulls[{model,frame}];
  if(source(model,frame,geometry.get(),emit)&&geometry->RigidHull())cached.geometry=std::move(geometry);
  hull=&cached;
 }
 if(!hull->geometry)return 0;
 if(angles&&mins&&maxs){
  JarSweepHull shape;
  if(!BuildPropSweep(*hull->geometry,V(angles),shape))return 0;
  JAR_SweepBounds(shape,mins,maxs);
 }
 return 1;
}
int JAR_RigidModelBody(unsigned n,unsigned id,const float* half,const float* center,const jarMPPose_t* p,
 int frozen,float mass,float gravity,const char* model,int frame){
 const auto* hull=FindModelHull(model,frame);
 if(!hull||!hull->geometry)return JAR_RigidBody(n,id,half,center,p,frozen,mass,gravity);
 return world.Prop(n,id,V(half),V(center),PoseOf(*p),frozen!=0,nullptr,hull->geometry)&&world.Configure(n,mass,gravity,true);
}
int JAR_RigidModelActor(const char* model,int frame,const float* angles,const float* before,const float* after,
 const float* actorOrigin,const float* mins,const float* maxs,float* normal,float* depth){
 auto* hull=FindModelHull(model,frame);
 if(!hull||!hull->geometry)return -1;
 const auto pose=V(angles);
 if(!hull->rotated||pose.x!=hull->angles.x||pose.y!=hull->angles.y||pose.z!=hull->angles.z){
  if(!hull->rotated)hull->rotated=std::make_unique<NativePropHull>();
  if(!hull->rotated->Build(*hull->geometry,pose)){hull->rotated.reset();return -1;}
  hull->angles=pose;
 }
 Vec start,end;
 start={actorOrigin[0]-before[0],actorOrigin[1]-before[1],actorOrigin[2]-before[2]};
 end={actorOrigin[0]-after[0],actorOrigin[1]-after[1],actorOrigin[2]-after[2]};
 const auto hit=hull->rotated->Trace(start,end,V(mins),V(maxs));
 if(hit.depth<=.001f)return 0;
 Put(hit.normal,normal);
 *depth=hit.depth;
 return 1;
}
void JAR_RigidForget(unsigned n){world.ForgetJoint(n+1);world.Forget(n);}
int JAR_RigidRead(unsigned n,jarMPPose_t*p,float*mins,float*maxs){Pose out;Vec low,high;if(!world.Read(n,out)||!world.Bounds(n,low,high))return 0;Put(out.position,p->position);Put(out.angles,p->angles);Put(out.linear,p->linear);Put(out.angular,p->angular);Put(low,mins);Put(high,maxs);return 1;}
int JAR_RigidOutOfBounds(unsigned n){
 Pose pose;
 world.Read(n,pose); // Read retains the sampled position when validation rejects the pose.
 const Vec p=pose.position;
 return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&
  (std::fabs(p.x)>=65536||std::fabs(p.y)>=65536||std::fabs(p.z)>=65536);
}
int JAR_RigidCorrect(unsigned n,const jarMPPose_t*p){return world.CorrectPose(n,PoseOf(*p));}
int JAR_RigidCorrectPoses(const jarMPRigidPoseUpdate_t* updates,int count){
 std::vector<PoseUpdate> poses;
 poses.reserve(count);
 for(int i=0;i<count;++i)poses.push_back({updates[i].entity,PoseOf(updates[i].pose),updates[i].frozen!=0});
 return world.CorrectPoses(poses);
}
int JAR_RigidDragAt(unsigned n,const float*p,const jarMPGrip_t*grip,const float*facing){return world.DragAt(n,V(p),V(grip->localPoint),V(facing));}
void JAR_RigidRemoveRope(unsigned id){world.ForgetJoint(id);}
void JAR_RigidBeginRopes(){world.BeginJoints();}
int JAR_RigidRope(unsigned id,int first,unsigned firstIdentity,int second,unsigned secondIdentity,const float*a,const float*b,float length){
 if(!id||first==second)return 0;
 jar::constraint::Joint j;j.id=id;j.kind=jar::constraint::Rope;j.length=length;
 j.first=firstIdentity;j.second=secondIdentity;
 const auto x=RigidEndpoint(first,j.first),y=RigidEndpoint(second,j.second);
 for(int i=0;i<3;++i){j.a.position[i]=a[i];j.b.position[i]=b[i];}
 return world.Joint(j,x,y);
}
void JAR_RigidEndRopes(){world.EndJoints();}
int JAR_RigidForce(unsigned n,const float*impulse,const float*point){return world.LimitedImpulse(n,V(impulse),V(point),1500,8);}
float JAR_RigidImpulseResponse(unsigned n,const float*direction,const float*torque,const float*origin){return world.ImpulseResponse(n,V(direction),V(torque),V(origin));}
int JAR_RigidPointVelocity(unsigned n,const float*point,float*velocity){Pose p;if(!world.Read(n,p))return 0;float offset[3]={point[0]-p.position.x,point[1]-p.position.y,point[2]-p.position.z};velocity[0]=p.linear.x+p.angular.y*offset[2]-p.angular.z*offset[1];velocity[1]=p.linear.y+p.angular.z*offset[0]-p.angular.x*offset[2];velocity[2]=p.linear.z+p.angular.x*offset[1]-p.angular.y*offset[0];return 1;}
void JAR_RigidRelease(unsigned n,float mass,float gravity){world.Release(n);world.Configure(n,mass,gravity,true);}
void JAR_RigidBeginProxies(){world.BeginProxies();}
void JAR_RigidKeepProxy(unsigned n,unsigned id,unsigned model){world.KeepProxy(n,id,model);}
int JAR_RigidProxy(unsigned n,unsigned id,const float*half,const float*center,const jarMPPose_t*p){return world.Proxy(n,id,V(half),V(center),PoseOf(*p));}
int JAR_RigidHasProxyMesh(unsigned model){return world.HasProxyMesh(model);}
void JAR_RigidBeginGeometry(){triangles.clear();}
int JAR_RigidAddProxyMesh(unsigned model){const bool ok=world.AddProxyMesh(model,triangles);triangles.clear();return ok;}
int JAR_RigidMeshProxy(unsigned n,unsigned id,unsigned model,const jarMPPose_t*p){return world.MeshProxy(n,id,model,PoseOf(*p));}
void JAR_RigidEndProxies(){world.EndProxies();}
void JAR_RigidStep(float dt,float gravity){world.Step(dt,gravity);}
int JAR_RigidImpact(int index,unsigned*n,float*speed){const auto&events=world.Impacts();if(index<0||unsigned(index)>=events.size())return 0;*n=events[index].prop;*speed=events[index].speed;return 1;}
int JAR_RigidImpulse(unsigned n,const float*impulse,const float*point){return world.Impulse(n,V(impulse),V(point));}
int JAR_RigidSweepActor(const float*before,const float*after,const float*mins,const float*maxs,const float*actorMin,const float*actorMax,float*normal,float*depth){// Native coordinates can round a resting boundary slightly inside the hull.
 // Bias only the sweep start by 0.002 units so the incoming face wins over
 // minimum-overlap side ejection on the next server frame.
 float start[3];float length=0;for(int i=0;i<3;++i){start[i]=after[i]-before[i];length+=start[i]*start[i];}length=std::sqrt(length);
 for(int i=0;i<3;++i)start[i]=before[i]-(length>.001f?start[i]*.002f/length:0);
 ActorContact hit;if(!SweepActor(start,after,mins,maxs,actorMin,actorMax,hit))return 0;for(int i=0;i<3;++i)normal[i]=hit.normal[i];*depth=hit.depth;return 1;}
unsigned JAR_RigidSteps(){return world.Statistics().steps;}
unsigned JAR_RigidTriangles(){return world.Statistics().triangles;}

namespace {
jar::constraint::Frame FrameOf(const float*p,const float*q){jar::constraint::Frame f;for(int a=0;a<3;++a)f.position[a]=p[a];for(int a=0;a<4;++a)f.rotation[a]=q[a];return f;}
jar::constraint::Joint JointOf(const jarMPJoint_t&r){jar::constraint::Joint j;j.id=r.id;j.kind=r.kind;j.first=r.first;j.second=r.second;j.a=FrameOf(r.a,r.ra);j.b=FrameOf(r.b,r.rb);j.length=r.length;j.path=r.path;return j;}
void PutPose(const Pose&p,jarMPPose_t*out){Put(p.position,out->position);Put(p.angles,out->angles);Put(p.linear,out->linear);Put(p.angular,out->angular);}
}
int JAR_JointFrame(const jarMPPose_t*p,const float*point,const float*axis,float*local,float*rotation){jar::constraint::Frame f;if(!LocalFrame(PoseOf(*p),V(point),V(axis),f))return 0;for(int a=0;a<3;++a)local[a]=f.position[a];for(int a=0;a<4;++a)rotation[a]=f.rotation[a];return 1;}
int JAR_JointPoint(const jarMPPose_t*p,const float*local,const float*rotation,float*point,float*axis){Vec x,y;if(!FrameTransform(PoseOf(*p),FrameOf(local,rotation),x,y))return 0;Put(x,point);Put(y,axis);return 1;}
int JAR_RigidJoint(const jarMPJoint_t*r,int first,int second){
 if(!r||r->kind<1||r->kind>2||first==second)return 0;
 auto j=JointOf(*r);
 const auto x=RigidEndpoint(first,j.first),y=RigidEndpoint(second,j.second);
 return world.Joint(j,x,y);
}
int JAR_NativeJoints(jarMPJointBody_t*body,int count,const jarMPJoint_t*links,int linkCount,float dt){
 if(count<0||count>JAR_MP_ATTACHMENT_LIMIT*(2+JAR_ROPE_PATH_BENDS)||linkCount<0||linkCount>JAR_MP_ATTACHMENT_LIMIT)return 0;
 std::vector<ConstraintBody>bodies;std::vector<jar::constraint::Joint>joints;bodies.reserve(count);joints.reserve(linkCount);
 for(int i=0;i<count;++i){const auto&b=body[i];ConstraintBody x;x.key=b.key;x.pose=PoseOf(b.pose);x.half=V(b.half);x.mass=b.mass;x.movable=b.movable!=0;bodies.push_back(x);}
 std::vector<ConstraintRoute> routes(linkCount);
 for(int i=0;i<linkCount;++i){
  if(links[i].kind<1||links[i].kind>3)return 0;
  joints.push_back(JointOf(links[i]));
  for(int b=0;b<JAR_ROPE_PATH_BENDS;++b)routes[i][b]={links[i].bends[b].key,V(links[i].bends[b].local)};
 }
 if(!nativeJoints.Solve(bodies,joints,dt,true,&routes))return 0;
 for(int i=0;i<count;++i)PutPose(bodies[i].pose,&body[i].pose);return 1;
}
void JAR_NativeJointsClear(){nativeJoints.Clear();}
void JAR_AngularVelocity(const float*a,const float*b,float dt,float*out){Put(AngularVelocity(V(a),V(b),dt),out);}
void JAR_AdvanceAngles(const float*a,const float*b,float dt,float*out){Put(AdvanceAngles(V(a),V(b),dt),out);}
