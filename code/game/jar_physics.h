// Original JA:R native/Box3D bridge. GPL-2.0-only.
#ifndef JAR_PHYSICS_H
#define JAR_PHYSICS_H
#include <array>
#include <cstdint>
#include <memory>
#include "../qcommon/jar_constraint.h"
#include "../../shared/qcommon/jar_sweep_hull.h"
#include <vector>
struct b3HullData;
namespace jar { namespace physics {
constexpr unsigned propLimit=4096,proxyLimit=256,triangleLimit=262144;
struct Vec { float x=0,y=0,z=0; };
// The hit point is local to the entity. Facing is relative to the camera's
// heading, and elevation moves the grip without changing the object's tilt.
struct PhysgunGrip {
 Vec localPoint,relativeAngles;
 bool active=false;
 bool Begin(Vec origin,Vec angles,Vec point,float viewYaw);
 Vec Point(Vec origin,Vec angles) const;
 Vec Origin(Vec point,Vec angles) const;
 Vec Facing(float viewYaw) const;
 void Turn(float yaw,float pitch,float viewPitch=0,float viewRoll=0);
};
// Model points and enclosing support planes in local coordinates.
class PropGeometry {
 std::array<float,27> supports{};
 std::vector<Vec> points;
 mutable std::shared_ptr<b3HullData> rigidHull;
 mutable bool rigidReady=false;
 bool sampled=false;
 float Support(Vec direction) const;
public:
 // The MP engine can retain the model point that established each maximum.
 using SupportPoints=std::array<Vec,27>;
 void Add(Vec point,SupportPoints* witnesses=nullptr);
 bool Empty() const { return !sampled; }
 // One point beyond the sweep limit identifies the support-plane path.
 const std::vector<Vec>& ModelPoints() const { return points; }
 void Bounds(const Vec (&localAxes)[3],Vec& low,Vec& high) const;
 const b3HullData* RigidHull() const;
};
struct PropTrace {
 float fraction=1,planeDistance=0,depth=0;
 Vec normal;
 bool startSolid=false,allSolid=false;
};
bool BuildPropSweep(const PropGeometry&,Vec angles,JarSweepHull&);
// Visits conservative chord envelopes along the shortest arc and a linear move.
bool TracePropRotation(const PropGeometry&,Vec before,Vec after,void* context,
 bool (*clear)(void*,const JarSweepHull&),Vec localPivot={},Vec translation={});
// Sweeps a native box or convex prop against a rotated, stationary prop.
// Query positions are relative to the stationary prop's model origin.
class NativePropHull {
 struct Axis {Vec normal;float low,high;};
 std::vector<Axis> axes;
 JarSweepHull sweep{};
public:
 bool Build(const PropGeometry&,Vec angles);
 PropTrace Trace(Vec start,Vec end,Vec mins,Vec maxs) const;
 // Signed vertical travel for the box to first reach the hull from below.
 bool BoxContactFromBelow(Vec origin,Vec mins,Vec maxs,float& height) const;
 PropTrace TraceHull(Vec start,Vec end,const JarSweepHull& moving) const;
 const JarSweepHull& Sweep() const { return sweep; }
};
struct Pose { Vec position,angles,linear,angular; bool awake=true; };
// Batch relocation requires the expected body type and preserves it.
struct PoseUpdate {uint32_t entity;Pose pose;bool frozen=false;};
struct MountedHull {uint32_t entity;uint64_t lifetime;Vec half,center;};
bool LocalFrame(const Pose&,Vec anchor,Vec axis,constraint::Frame&);
bool FrameTransform(const Pose&,const constraint::Frame&,Vec&point,Vec&axis);
Vec AngularVelocity(Vec before,Vec after,float seconds);
Vec AdvanceAngles(Vec angles,Vec angular,float seconds);
Vec AngularImpulse(Vec angles,Vec half,float mass,Vec impulse,Vec lever);
Vec AngularResponse(Vec angles,Vec half,float mass,Vec torque);
// Native movement already advanced each pose. Position projection and velocity
// solving are separate, so this contact-free graph never advances it twice.
// Native sweeps remain authoritative for world collision.
struct ConstraintBody {uint32_t key=0;Pose pose;Vec half{8,8,8};float mass=80;bool movable=true;};
struct ConstraintBend {uint32_t key=0;Vec local;};
using ConstraintRoute=std::array<ConstraintBend,JAR_ROPE_PATH_BENDS>;
class ConstraintSolver {
 struct Impl;std::unique_ptr<Impl> impl;
public:
 ConstraintSolver();~ConstraintSolver();void Clear();
 bool Solve(std::vector<ConstraintBody>&,const std::vector<constraint::Joint>&,float seconds,bool multiplayer=false,
            const std::vector<ConstraintRoute>* routes=nullptr);
};
struct Contact {uint32_t prop=0,native=0;};
struct Impact {uint32_t prop=0;float speed=0;Vec point;};
struct Stats { unsigned props=0,proxies=0,joints=0,awake=0,triangles=0,steps=0,proxyMeshes=0,proxyTriangles=0,workers=0; float stepMs=0,maxStepMs=0; };
// Public coordinates are native game units; angular velocity is radians/second.
// Native controllers supply proxy poses. The solver never writes back to them.
class World {
 struct Impl; std::unique_ptr<Impl> impl;
 const unsigned proxyCapacity;
 const bool rigidConstraints;
public:
 explicit World(unsigned proxyCapacity=proxyLimit,bool rigidConstraints=false); ~World();
 World(const World&)=delete; World& operator=(const World&)=delete;
 // A larger budget starts a worker pool. The active count follows awake bodies.
 bool Start(const std::vector<Vec>& triangles,unsigned maxWorkers=1);
 void Stop(); bool Active() const;
 bool Prop(uint32_t entity,uint32_t identity,Vec half,Vec center,const Pose&,bool frozen,
  const std::vector<MountedHull>* mounts=nullptr,
  const std::shared_ptr<const PropGeometry>& geometry={});
 void Forget(uint32_t entity);
 bool JointFrames(uint32_t first,uint32_t second,Vec anchor,Vec axis,constraint::Frame&a,constraint::Frame&b)const;
 // An identity transform with no collision shape and no prop slot.
 static constexpr uint32_t worldAnchor = UINT32_MAX;
 void BeginJoints();bool Joint(const constraint::Joint&record,uint32_t first,uint32_t second);void EndJoints();
 void ForgetJoint(uint32_t id);
 bool Drag(uint32_t entity,Vec target,bool constrained=false);
 bool DragAt(uint32_t entity,Vec point,Vec localPoint,Vec facing);
 bool Rotate(uint32_t entity,float yaw,float pitch);
 void EndRotate(uint32_t entity);
 void Release(uint32_t entity);
 bool Impulse(uint32_t entity,Vec impulse,Vec point);
 bool LimitedImpulse(uint32_t entity,Vec impulse,Vec point,float speed,float spin);
 bool LimitedWrench(uint32_t entity,Vec impulse,Vec torque,Vec origin,float speed,float spin);
 float ImpulseResponse(uint32_t entity,Vec direction,Vec torque,Vec origin) const;
 bool Correct(uint32_t entity,Vec position,Vec linear);
 bool CorrectPose(uint32_t entity,const Pose&);
 bool CorrectPoses(const std::vector<PoseUpdate>&);
 float Mass(uint32_t entity) const;
 bool Configure(uint32_t entity,float mass,float gravityScale,bool unrestrictedMass=false);
 bool JointPoints(const constraint::Joint&,uint32_t first,uint32_t second,Vec&a,Vec&b)const;
 bool Read(uint32_t entity,Pose&) const;
 bool Bounds(uint32_t entity,Vec& minimum,Vec& maximum) const;
 void BeginProxies();
 // Mark an existing matching body before pruning a changing candidate set.
 void KeepProxy(uint32_t entity,uint32_t identity,unsigned model,bool mountOnly=false);
 bool Proxy(uint32_t entity,uint32_t identity,Vec half,Vec center,const Pose&,bool mountOnly=false);
 // Map-local immutable geometry is shared across native brush instances.
 // EndProxies discards meshes with no remaining bodies; Stop clears the map.
 bool HasProxyMesh(unsigned model)const;
 bool AddProxyMesh(unsigned model,const std::vector<Vec>& triangles);
 bool MeshProxy(uint32_t entity,uint32_t identity,unsigned model,const Pose&);
 void EndProxies();
 void Step(float elapsedSeconds,float gravity);
 Stats Statistics() const;
 const std::vector<Contact>& Contacts() const;
 const std::vector<Impact>& Impacts() const;
};
} }
#endif
