// Original solver-independent Sandbox constraint records. GPL-2.0-only.
#ifndef JAR_CONSTRAINT_H
#define JAR_CONSTRAINT_H
#include <cmath>
#include <cstdint>
#include "../../shared/qcommon/jar_rope_path.h"
namespace jar { namespace constraint {
constexpr unsigned limit=128;
constexpr float ropeMinLength=.01f,ropeMaxLength=8192.f;
constexpr float thrusterMinForce=1.f,thrusterMaxForce=100000.f;
// Owned IDs occupy the low half. Native endpoints encode a world entity slot;
// freeing/reusing a native slot removes its attachments before it can be reused.
constexpr uint32_t nativeBit=0x80000000u;
inline bool Native(uint32_t id){return (id&nativeBit)!=0;}
constexpr uint32_t NativeId(uint32_t entity){return nativeBit|entity;}
inline uint32_t NativeEntity(uint32_t id){return id&~nativeBit;}
enum Kind : uint32_t { Weld=1,Hinge=2,Rope=3,Thruster=4 };
struct Frame {float position[3]{};float rotation[4]{0,0,0,1};}; // xyz, quaternion xyzw
struct Joint {uint32_t id=0,kind=0,first=0,second=0;Frame a,b;float length=0,force=0;uint32_t enabled=1;jarRopePath_t path{};}; // Stable ownership IDs, never pointers.
inline void ReverseAxis(Frame& frame){
 // A half-turn around the frame's local X reverses its exhaust (local Z).
 auto& q=frame.rotation;
 const float x=q[0],y=q[1];
 q[0]=q[3];q[1]=q[2];q[2]=-y;q[3]=-x;
}
inline bool Valid(const Frame&f){
 float norm=0;for(float v:f.position)if(!std::isfinite(v)||std::fabs(v)>65535)return false;
 for(float v:f.rotation){if(!std::isfinite(v))return false;norm+=v*v;}
 return std::fabs(norm-1)<.001f;
}
inline bool Valid(const Joint&j,float maximumLength=ropeMaxLength){
 if(!JAR_RopePathValid(&j.path)||(j.kind!=Rope&&j.path.count))return false;
 for(int i=j.path.count;i<JAR_ROPE_PATH_BENDS;++i)for(float v:j.path.bend[i])if(v!=0)return false;
 return j.id && (j.kind>=Weld&&j.kind<=Thruster) && j.first && j.second && (j.kind==Thruster?j.first==j.second:j.first!=j.second) && Valid(j.a)&&Valid(j.b) && std::isfinite(j.length) && j.length>=0 && j.length<=maximumLength && std::isfinite(j.force) && j.force>=0 && j.force<=thrusterMaxForce && j.enabled<=1 && (j.kind!=Rope || j.length>=ropeMinLength) && (j.kind!=Thruster || j.force>=thrusterMinForce);}
} }
#endif
