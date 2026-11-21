// Native and mixed Weld/Hinge graphs. Native controllers retain simulation;
// this pass projects constraints after their motion and sweeps each correction.
#include "g_bsp_section.h"
jar::physics::ConstraintSolver nativeJointSolver;
std::map<uint32_t,bool> nativeJointKeys;
struct JointPoseHistory {unsigned generation;int time;jar::physics::Pose pose;};
std::map<uint32_t,JointPoseHistory> nativeJointHistory;
constexpr auto nativeJointWorld=jar::constraint::NativeId(ENTITYNUM_WORLD);
void nativeJointsReset(){nativeJointSolver.Clear();nativeJointKeys.clear();nativeJointHistory.clear();}
void attachmentClassifyJoints(){
 nativeJointKeys.clear();
 for(unsigned i=0;i<state.jointCount;++i){const auto&j=state.joints[i];if(j.kind>jar::constraint::Hinge)continue;auto*a=attachmentEntity(j.first),*b=attachmentEntity(j.second);
  if(a&&b&&((j.first!=nativeJointWorld&&!attachmentBox(*a))||(j.second!=nativeJointWorld&&!attachmentBox(*b)))){
   if(j.first!=nativeJointWorld)nativeJointKeys[j.first]=true;
   if(j.second!=nativeJointWorld)nativeJointKeys[j.second]=true;
  }
 }
 for(unsigned pass=0;pass<state.jointCount;++pass){bool changed=false;
  for(unsigned i=0;i<state.jointCount;++i){const auto&j=state.joints[i];if(j.kind>jar::constraint::Hinge||j.first==nativeJointWorld||j.second==nativeJointWorld||(!nativeJointKeys.count(j.first)&&!nativeJointKeys.count(j.second)))continue;
   changed|=nativeJointKeys.emplace(j.first,true).second;changed|=nativeJointKeys.emplace(j.second,true).second;}
  if(!changed)break;
 }
}
bool attachmentNativeJoint(const jar::constraint::Joint&j){return j.kind<=jar::constraint::Hinge&&(nativeJointKeys.count(j.first)||nativeJointKeys.count(j.second));}
void attachmentAdvanceProp(gentity_t&e,const Owned& owner,const Prop& model,float dt){
 if(owner.frozen||dt<=0)return;
 vec3_t spin;VectorCopy(e.s.apos.trDelta,spin);float speed=VectorLength(spin);if(speed<.0001f)return;
 jar::physics::Pose rigidPose;
 if(RigidCatalog(owner.catalog)&&physicsWorld.Read(e.s.number,rigidPose))return;
 PhysicsBounds bounds;
 const bool hasBounds=attachmentPropSpinBounds(e,owner,model,bounds);
 const auto center=hasBounds?bounds.center:jar::physics::Vec{};
 vec3_t pivot,offset;
 attachmentPropCenterOffset(center,e.currentAngles,offset);
 VectorAdd(e.currentOrigin,offset,pivot);
 const float radius=hasBounds?std::sqrt(bounds.half.x*bounds.half.x+bounds.half.y*bounds.half.y+
  bounds.half.z*bounds.half.z):RadiusFromBounds(e.mins,e.maxs);
 const float limit=std::min(8.f,2000.f/std::max(1.f,radius));
 if(speed>limit)VectorScale(spin,limit/speed,spin);
 const int steps=std::max(1,int(std::ceil(dt*250)));const float step=dt/steps;
 std::vector<SandboxChargeMove> charges,lastChargePose;
 for(int n=0;n<steps;++n){
  vec3_t angles,origin,lo,hi;
  nativeVector(jar::physics::AdvanceAngles(pv(e.currentAngles),pv(spin),step),angles);
  attachmentPropCenterOffset(center,angles,offset);
  VectorSubtract(pivot,offset,origin);
  if(!propHull(model,angles,lo,hi,e.s.frame)){VectorClear(spin);break;}
  charges.clear();
  if(!G_SandboxChargesPlanPose(e,origin,angles,charges,ENTITYNUM_WORLD)){VectorClear(spin);break;}
  {
   SandboxChargesUnlinked own(charges);
   if(!propBodyTransformClear(e,model,e.s.frame,origin,angles,true,lo,hi,pivot,center)){VectorClear(spin);break;}
  }
  if(!mountedChargeSweepClear(e,angles,charges,true,pivot)){VectorClear(spin);break;}
  vec3_t move;VectorSubtract(origin,e.currentOrigin,move);
  VectorAdd(e.s.pos.trBase,move,e.s.pos.trBase);VectorCopy(origin,e.currentOrigin);
  G_SetAngles(&e,angles);VectorCopy(lo,e.mins);VectorCopy(hi,e.maxs);setupMoved(e);gi.linkentity(&e);
  if(!charges.empty()){G_SandboxChargesPlace(charges);lastChargePose.swap(charges);}
 }
 if(!lastChargePose.empty())G_SandboxChargesRefresh(lastChargePose);
 VectorCopy(spin,e.s.apos.trDelta);
}
void attachmentNativeJoints(float dt){
 attachmentClassifyJoints();if(nativeJointKeys.empty()){nativeJointsReset();return;}
 std::vector<jar::constraint::Joint> joints;std::vector<jar::physics::ConstraintBody> bodies;
 for(unsigned i=0;i<state.jointCount;++i){
  const auto& joint=state.joints[i];
  if(!attachmentNativeJoint(joint))continue;
  const auto* a=attachmentEntity(joint.first);const auto* b=attachmentEntity(joint.second);
  if(a&&b&&!G_SandboxUsableDormant(*a)&&!G_SandboxUsableDormant(*b))joints.push_back(joint);
 }
 if(joints.empty()){nativeJointHistory.clear();return;}
 if(std::any_of(joints.begin(),joints.end(),[](const auto& j){return j.first==nativeJointWorld||j.second==nativeJointWorld;})){
  jar::physics::ConstraintBody world;
  world.key=nativeJointWorld;world.half={8,8,8};world.mass=80;world.movable=false;
  bodies.push_back(world);
 }
 for(const auto&entry:nativeJointKeys){auto*e=attachmentEntity(entry.first);if(!e)return;
  if(G_SandboxUsableDormant(*e))continue;
  jar::physics::ConstraintBody b;b.key=entry.first;b.movable=attachmentMovable(*e);b.mass=attachmentMass(*e);attachmentPose(*e,b.pose);
  if(attachmentBox(*e))physicsWorld.Read(e->s.number,b.pose);
  else {vec3_t velocity;attachmentVelocity(*e,velocity);b.pose.linear=pv(velocity);
   const auto*o=owned(e->s.number);if(o&&prop(o->catalog))b.pose.angular=pv(e->s.apos.trDelta);
   else if(!e->client){vec3_t spin;attachmentSpin(*e,spin);b.pose.angular=pv(spin);}
   auto previous=nativeJointHistory.find(b.key);
   if(previous!=nativeJointHistory.end()&&previous->second.generation==entityGeneration[e->s.number]&&previous->second.time<level.time&&level.time-previous->second.time<=100){
    const float elapsed=(level.time-previous->second.time)*.001f;
    if(e->client)b.pose.angular=jar::physics::AngularVelocity(previous->second.pose.angles,b.pose.angles,elapsed);
    if(!b.movable&&e->s.number!=ENTITYNUM_WORLD){const auto p=previous->second.pose.position;b.pose.linear={(b.pose.position.x-p.x)/elapsed,(b.pose.position.y-p.y)/elapsed,(b.pose.position.z-p.z)/elapsed};b.pose.angular=jar::physics::AngularVelocity(previous->second.pose.angles,b.pose.angles,elapsed);}
   }else if(!b.movable){b.pose.linear={};b.pose.angular={};}
  }
  b.half={std::max(.25f,std::min(8192.f,(e->maxs[0]-e->mins[0])*.5f)),std::max(.25f,std::min(8192.f,(e->maxs[1]-e->mins[1])*.5f)),std::max(.25f,std::min(8192.f,(e->maxs[2]-e->mins[2])*.5f))};
  bodies.push_back(b);
 }
 if(!nativeJointSolver.Solve(bodies,joints,dt)){message("Constraint state exceeds supported motion bounds; links retained for Unlink.");nativeJointHistory.clear();return;}
 for(const auto&body:bodies){auto*e=attachmentEntity(body.key);if(!e||!body.movable)continue;auto pose=body.pose;
  vec3_t target,angles,delta;nativeVector(pose.position,target);nativeVector(pose.angles,angles);VectorSubtract(target,e->currentOrigin,delta);const float length=VectorLength(delta);if(length>64)VectorMA(e->currentOrigin,64/length,delta,target);
  vec3_t lo,hi;VectorCopy(e->mins,lo);VectorCopy(e->maxs,hi);
  const auto* owner=owned(e->s.number);
  const auto* model=owner?prop(owner->catalog):nullptr;
  if(model&&!propHull(*model,angles,lo,hi,e->s.frame))continue;
  if(jar::bspSection::Portable(*e))jar::bspSection::Hull(e->mins,e->maxs,angles,lo,hi);
  // Connected pairs do not collide during constraint projection. Unrelated
  // scenery, actors and nonadjacent assembly members still block the sweep.
  std::vector<gentity_t*> unlinked;
  std::vector<SandboxChargeMove> peerCharges;
  for(const auto&j:joints){uint32_t other=j.first==body.key?j.second:j.second==body.key?j.first:0;if(!other)continue;auto*peer=attachmentEntity(other);if(peer&&peer->s.number<ENTITYNUM_WORLD&&peer->linked){
   G_SandboxChargesPlanPose(*peer,peer->currentOrigin,peer->currentAngles,peerCharges,ENTITYNUM_WORLD);
   gi.unlinkentity(peer);unlinked.push_back(peer);
  }}
  trace_t tr{};
  std::vector<SandboxChargeMove> charges;
  bool clear;
  {
   SandboxMountedChargeCollision mountedCollision;
   SandboxChargesUnlinked hidden(peerCharges);
   if(model){
    const bool planned=G_SandboxChargesPlanPose(*e,target,angles,charges,ENTITYNUM_WORLD);
    vec3_t low,high;
    const auto angular=jar::physics::AngularVelocity(pv(e->currentAngles),pose.angles,1);
    vec3_t spin;nativeVector(angular,spin);
    const float sag=RadiusFromBounds(e->mins,e->maxs)*(1-std::cos(VectorLength(spin)*.5f));
    for(int axis=0;axis<3;++axis){
     low[axis]=std::min(lo[axis],e->mins[axis])-sag;
     high[axis]=std::max(hi[axis],e->maxs[axis])+sag;
    }
    {
     SandboxChargesUnlinked own(charges);
     clear=planned&&propBodyTransformClear(*e,*model,e->s.frame,target,angles,true,low,high);
    }
    if(clear)clear=mountedChargeSweepClear(*e,angles,charges,true);
    tr.fraction=1;VectorCopy(target,tr.endpos);
   }else {
    gi.trace(&tr,e->currentOrigin,lo,hi,target,e->s.number,MASK_SOLID|CONTENTS_BODY,G2_NOCOLLIDE,0);
    clear=!tr.startsolid&&!tr.allsolid&&mountedChargePoseClear(*e,tr.endpos,angles,charges,true);
   }
  }
  for(auto*peer:unlinked)gi.linkentity(peer);
  if(!clear)continue;
  pose.position=pv(tr.endpos);if(tr.fraction<1){vec3_t v;nativeVector(pose.linear,v);const float speed=DotProduct(v,tr.plane.normal);if(speed<0)VectorMA(v,-speed,tr.plane.normal,v);pose.linear=pv(v);}
  if(attachmentBox(*e)&&!physicsWorld.CorrectPose(e->s.number,pose))continue;
  G_SetOrigin(e,tr.endpos);G_SetAngles(e,angles);if(e->client){VectorCopy(tr.endpos,e->client->ps.origin);SetClientViewAngle(e,angles);}
  if(model){VectorCopy(lo,e->mins);VectorCopy(hi,e->maxs);}
  vec3_t velocity;nativeVector(pose.linear,velocity);attachmentSetVelocity(*e,velocity);
  if(attachmentBox(*e)){e->s.pos.trType=e->s.apos.trType=TR_INTERPOLATE;nativeVector(pose.angular,e->s.apos.trDelta);}
  else if(owned(e->s.number)&&prop(owned(e->s.number)->catalog)){e->s.apos.trType=TR_INTERPOLATE;nativeVector(pose.angular,e->s.apos.trDelta);}
  else if(!e->client){vec3_t zero={0,0,0};G_JAR_MoverSpin(*e,zero);const auto next=jar::physics::AdvanceAngles(pose.angles,pose.angular,dt);const float values[3]={next.x,next.y,next.z};for(int axis=0;axis<3;++axis)e->s.apos.trDelta[axis]=AngleSubtract(values[axis],angles[axis])/dt;e->s.apos.trType=TR_LINEAR;e->s.apos.trTime=level.time;}
  setupMoved(*e);gi.linkentity(e);placeEditedCharges(charges);
 }
 std::map<uint32_t,JointPoseHistory> history;
 for(const auto&body:bodies)if(auto*e=attachmentEntity(body.key)){jar::physics::Pose p;attachmentPose(*e,p);history.emplace(body.key,JointPoseHistory{entityGeneration[e->s.number],level.time,p});}
 nativeJointHistory=std::move(history);
}
