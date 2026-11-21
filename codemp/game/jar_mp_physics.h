// C boundary around the existing JA:R/Box3D implementation. GPL-2.0-only.
#pragma once
#include "../../shared/qcommon/jar_rope_path.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {float position[3],angles[3],linear[3],angular[3];} jarMPPose_t;
typedef struct {float localPoint[3],relativeAngles[3];} jarMPGrip_t;
int JAR_GripBegin(jarMPGrip_t*,const float*origin,const float*angles,const float*point,float viewYaw);
void JAR_GripPoint(const jarMPGrip_t*,const float*origin,const float*angles,float*point);
void JAR_GripOrigin(const jarMPGrip_t*,const float*point,const float*angles,float*origin);
void JAR_GripTurn(jarMPGrip_t*,float yaw,float pitch,float viewYaw,float viewPitch,float viewRoll,float*facing);
enum { JAR_MP_RIGID_PROXY_LIMIT = 1024 };
void JAR_RigidReset(void);
int JAR_RigidPolygon(int count,const float*points,const float*normal);
int JAR_RigidStart(void);
int JAR_RigidActive(void);
int JAR_RigidBody(unsigned entity,unsigned identity,const float*half,const float*center,const jarMPPose_t*pose,int frozen,float mass,float gravity);
typedef int (*jarMPModelHullSource_t)(const char*,int,void*,void (*)(void*,const float*));
int JAR_RigidModelHull(const char* model,int frame,jarMPModelHullSource_t source,const float* angles,float* mins,float* maxs);
int JAR_RigidModelBody(unsigned entity,unsigned identity,const float* half,const float* center,
 const jarMPPose_t* pose,int frozen,float mass,float gravity,const char* model,int frame);
// Returns -1 if the model hull is unavailable, zero for no contact, or one for contact.
int JAR_RigidModelActor(const char* model,int frame,const float* angles,const float* before,const float* after,
 const float* actorOrigin,const float* mins,const float* maxs,float* normal,float* depth);
void JAR_RigidForget(unsigned entity);
int JAR_RigidRead(unsigned entity,jarMPPose_t*pose,float*mins,float*maxs);
int JAR_RigidOutOfBounds(unsigned entity);
int JAR_RigidCorrect(unsigned entity,const jarMPPose_t*pose);
typedef struct { unsigned entity; jarMPPose_t pose; int frozen; } jarMPRigidPoseUpdate_t;
int JAR_RigidCorrectPoses(const jarMPRigidPoseUpdate_t* updates,int count);
int JAR_RigidDragAt(unsigned entity,const float*point,const jarMPGrip_t*,const float*facing);
void JAR_RigidRemoveRope(unsigned id);
void JAR_RigidBeginRopes(void);
int JAR_RigidRope(unsigned id,int first,unsigned firstIdentity,int second,unsigned secondIdentity,const float*a,const float*b,float length);
void JAR_RigidEndRopes(void);
int JAR_RigidForce(unsigned entity,const float*impulse,const float*point);
float JAR_RigidImpulseResponse(unsigned entity,const float*direction,const float*torque,const float*origin);
int JAR_RigidPointVelocity(unsigned entity,const float*point,float*velocity);
void JAR_RigidRelease(unsigned entity,float mass,float gravity);
void JAR_RigidBeginProxies(void);
void JAR_RigidKeepProxy(unsigned entity,unsigned identity,unsigned model);
int JAR_RigidProxy(unsigned entity,unsigned identity,const float*half,const float*center,const jarMPPose_t*pose);
int JAR_RigidHasProxyMesh(unsigned model);
void JAR_RigidBeginGeometry(void);
int JAR_RigidAddProxyMesh(unsigned model);
int JAR_RigidMeshProxy(unsigned entity,unsigned identity,unsigned model,const jarMPPose_t*pose);
void JAR_RigidEndProxies(void);
void JAR_RigidStep(float seconds,float gravity);
int JAR_RigidImpact(int index,unsigned*entity,float*speed);
int JAR_RigidImpulse(unsigned entity,const float*impulse,const float*point);
int JAR_RigidSweepActor(const float*before,const float*after,const float*mins,const float*maxs,const float*actorMin,const float*actorMax,float*normal,float*depth);
unsigned JAR_RigidSteps(void);
unsigned JAR_RigidTriangles(void);
#ifdef __cplusplus
}
#endif

// Immutable joint frames are shared by the rigid and native graph solvers.
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 unsigned id,kind,first,second;
 float a[3],b[3],ra[4],rb[4],length;
 jarRopePath_t path;
 struct {unsigned key;float local[3];} bends[JAR_ROPE_PATH_BENDS];
} jarMPJoint_t;
typedef struct {unsigned key;jarMPPose_t pose;float half[3],mass;int movable;} jarMPJointBody_t;
int JAR_JointFrame(const jarMPPose_t*,const float*point,const float*axis,float*local,float*rotation);
int JAR_JointPoint(const jarMPPose_t*,const float*local,const float*rotation,float*point,float*axis);
int JAR_RigidJoint(const jarMPJoint_t*,int first,int second);
int JAR_NativeJoints(jarMPJointBody_t*,int bodies,const jarMPJoint_t*,int joints,float dt);
void JAR_NativeJointsClear(void);
void JAR_AngularVelocity(const float*before,const float*after,float dt,float*out);
void JAR_AdvanceAngles(const float*angles,const float*angular,float dt,float*out);
#ifdef __cplusplus
}
#endif
