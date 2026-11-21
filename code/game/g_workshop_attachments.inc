#include "../../shared/qcommon/jar_thruster.h"
#include "g_door_camera.h"
#include "g_crystal_crate.h"
#include "g_mover_motion.h"
#include "g_sandbox_usable.h"
#include "g_sandbox_angular.h"
#include "g_sandbox_charges.h"
#include "g_workshop_mounted_motion.inc"
// Saved Rope/Thruster attachments for native controllers and mixed assemblies.
// Native actors keep their movement/AI controllers. GPL-2.0-only.
jar::constraint::Frame attachmentPending;
uint32_t attachmentKey(const gentity_t&e){const auto*o=owned(e.s.number);return o&&matches(*o)?o->id:jar::constraint::NativeId(e.s.number);}
gentity_t*attachmentEntity(uint32_t key){
 unsigned entity=ENTITYNUM_NONE;
 if(jar::constraint::Native(key))entity=jar::constraint::NativeEntity(key);
 else {const auto*o=OwnedId(state,key);if(!o||!matches(*o))return nullptr;entity=o->entity;}
 if(entity==ENTITYNUM_WORLD)return &g_entities[entity];
 return entity<ENTITYNUM_WORLD&&editable(g_entities[entity])?&g_entities[entity]:nullptr;
}
bool attachmentBox(const gentity_t&e){const auto*o=owned(e.s.number);jar::physics::Pose pose;return o&&RigidCatalog(o->catalog)&&physicsWorld.Read(e.s.number,pose);}
bool attachmentNativeJoint(const jar::constraint::Joint&);
void attachmentClassifyJoints();
void attachmentNativeJoints(float);
bool ropeRigid(const jar::constraint::Joint&);
bool attachmentBoxJoint(const jar::constraint::Joint&j){
 if(attachmentNativeJoint(j)||(j.kind==jar::constraint::Rope&&!ropeRigid(j)))return false;
 const auto* a=attachmentEntity(j.first);
 const auto* b=attachmentEntity(j.second);
 const auto rigid=[&](const gentity_t* entity){
  return entity&&(attachmentBox(*entity)||(entity->s.number==ENTITYNUM_WORLD&&j.kind!=jar::constraint::Thruster));
 };
 return rigid(a)&&rigid(b);
}
bool attachmentSyncBoxJoints(){
 attachmentClassifyJoints();
 physicsWorld.BeginJoints();
 for(unsigned i=0;i<state.jointCount;++i){
  const auto& joint=state.joints[i];
  if(!attachmentBoxJoint(joint))continue;
  const auto slot=[](uint32_t key){
   const int entity=attachmentEntity(key)->s.number;
   return entity==ENTITYNUM_WORLD?jar::physics::World::worldAnchor:static_cast<uint32_t>(entity);
  };
  if(!physicsWorld.Joint(joint,slot(joint.first),slot(joint.second)))return false;
 }
 physicsWorld.EndJoints();
 return true;
}
bool attachmentPose(const gentity_t&e,jar::physics::Pose&p){
 if(e.s.number==ENTITYNUM_WORLD){p={};return true;}
 if(attachmentBox(e)&&physicsWorld.Read(e.s.number,p)){p.linear=p.angular={};return true;}
 p={};p.position=pv(e.currentOrigin);p.angles=pv(e.currentAngles);return true;
}
bool attachmentPoint(const gentity_t&e,const jar::constraint::Frame&f,vec3_t point,vec3_t direction){
 jar::physics::Pose p;attachmentPose(e,p);jar::physics::Vec a,b;
 if(!jar::physics::FrameTransform(p,f,a,b))return false;nativeVector(a,point);nativeVector(b,direction);return true;
}
bool attachmentFrameAt(const gentity_t&e,const vec3_t point,const vec3_t direction,jar::constraint::Frame&f){
 jar::physics::Pose p;attachmentPose(e,p);return jar::physics::LocalFrame(p,pv(point),pv(direction),f);
}
bool attachmentMovable(const gentity_t&e){
 if(e.s.number>=ENTITYNUM_WORLD||occupied(e)||nativeControls.count(e.s.number)||G_SandboxUsableDormant(e))return false;
 if(e.client)return e.health>0&&e.next_roff_time<=level.time&&e.client->ps.saberLockTime<=level.time&&(e.s.number!=0||usable());
 if(const auto*o=owned(e.s.number))if(prop(o->catalog))return !o->frozen||held==unsigned(e.s.number);
 if(e.bmodel||e.s.eType==ET_MOVER)return false; // Native path movers provide anchors.
 return e.s.eType==ET_ITEM||e.s.eType==ET_MISSILE||e.e_ThinkFunc==thinkF_NULL||e.e_ThinkFunc==thinkF_G_RunObject;
}
float attachmentMass(const gentity_t&e){
 const auto* o=owned(e.s.number);
 if(o&&o->physics.mass)return float(o->physics.mass);
 if(attachmentBox(e)){const float mass=physicsWorld.Mass(e.s.number);if(mass>0)return mass;}
 if(e.client)return std::max(40.f,std::min(2000.f,e.mass>0?float(e.mass):80.f));
 if(o&&prop(o->catalog)){
  PhysicsBounds bounds;
  if(modelBounds(*prop(o->catalog),bounds,e.s.frame))
   return std::max(1.f,std::min(float(maxPropMass),bounds.half.x*bounds.half.y*bounds.half.z*960.f/(32*32*32)));
 }else if(e.mass>0)return std::max(1.f,std::min(100000.f,float(e.mass)));
 float volume=1;for(int a=0;a<3;++a)volume*=std::max(1.f,e.maxs[a]-e.mins[a]);return std::max(1.f,std::min(100000.f,volume*120.f/(32*32*32)));
}
void attachmentSpin(const gentity_t&e,vec3_t spin){
 VectorClear(spin);if(e.client)return;
 const auto*o=owned(e.s.number);if(o&&prop(o->catalog)){VectorCopy(e.s.apos.trDelta,spin);return;}
 if(G_SandboxAngular(e))G_SandboxAngularVelocity(e,spin);
 else {vec3_t rate,next;EvaluateTrajectoryDelta(&e.s.apos,level.time,rate);VectorMA(e.currentAngles,.001f,rate,next);nativeVector(jar::physics::AngularVelocity(pv(e.currentAngles),pv(next),.001f),spin);}
 JarMoverMotion motion;if(G_JAR_MoverMotionRead(e,motion))VectorAdd(spin,motion.angular,spin);
}
jar::physics::Vec attachmentHalf(const gentity_t&e){return {std::max(.25f,std::min(8192.f,(e.maxs[0]-e.mins[0])*.5f)),std::max(.25f,std::min(8192.f,(e.maxs[1]-e.mins[1])*.5f)),std::max(.25f,std::min(8192.f,(e.maxs[2]-e.mins[2])*.5f))};}
void attachmentPropCenterOffset(jar::physics::Vec center,const vec3_t angles,vec3_t offset){
 vec3_t axes[3];AnglesToAxis(angles,axes);
 VectorScale(axes[0],center.x,offset);
 VectorMA(offset,center.y,axes[1],offset);
 VectorMA(offset,center.z,axes[2],offset);
}
bool attachmentPropSpinBounds(const gentity_t& entity,const Owned& owner,const Prop& model,PhysicsBounds& bounds){
 // Native link projection measures linear velocity at the entity origin.
 return !Constrained(state,owner.id)&&modelBounds(model,bounds,entity.s.frame);
}
void attachmentTorque(gentity_t&e,const vec3_t force,const vec3_t point,float dt){
 if(e.client||e.s.number>=ENTITYNUM_WORLD||occupied(e)||nativeControls.count(e.s.number)||(e.svFlags&SVF_INACTIVE))return;
 const auto* owner=owned(e.s.number);
 const auto* model=owner?prop(owner->catalog):nullptr;
 if(model&&!attachmentMovable(e))return;
 auto half=attachmentHalf(e);
 vec3_t lever,acceleration;
 VectorSubtract(point,e.currentOrigin,lever);
 PhysicsBounds bounds;
 if(model&&attachmentPropSpinBounds(e,*owner,*model,bounds)){
  vec3_t offset;attachmentPropCenterOffset(bounds.center,e.currentAngles,offset);
  VectorSubtract(lever,offset,lever);
  half=bounds.half;
 }
 nativeVector(jar::physics::AngularImpulse(pv(e.currentAngles),half,attachmentMass(e),pv(force),pv(lever)),acceleration);
 if(model){
  VectorMA(e.s.apos.trDelta,dt,acceleration,e.s.apos.trDelta);
  const float speed=VectorLength(e.s.apos.trDelta);
  if(speed>8)VectorScale(e.s.apos.trDelta,8/speed,e.s.apos.trDelta);
 }else G_JAR_MoverTorque(e,acceleration);
}
void attachmentVelocity(const gentity_t&e,vec3_t velocity,const float*point=nullptr){
 jar::physics::Pose p;if(attachmentBox(e)&&physicsWorld.Read(e.s.number,p)){nativeVector(p.linear,velocity);
  if(point){vec3_t angular,offset,spin;nativeVector(p.angular,angular);const vec3_t origin={p.position.x,p.position.y,p.position.z};VectorSubtract(point,origin,offset);CrossProduct(angular,offset,spin);VectorAdd(velocity,spin,velocity);}return;}
 if(e.client){VectorCopy(e.client->ps.velocity,velocity);return;}
 const auto*o=owned(e.s.number);if(o&&prop(o->catalog))VectorCopy(e.s.pos.trDelta,velocity);
 else EvaluateTrajectoryDelta(&e.s.pos,level.time,velocity);
 JarMoverMotion motion;if(G_JAR_MoverMotionRead(e,motion))VectorAdd(velocity,motion.velocity,velocity);
 if(point){vec3_t angular,offset,spin;attachmentSpin(e,angular);VectorSubtract(point,e.currentOrigin,offset);CrossProduct(angular,offset,spin);VectorAdd(velocity,spin,velocity);}
}
void attachmentSetVelocity(gentity_t&e,const vec3_t velocity){
 if(e.client){VectorCopy(velocity,e.client->ps.velocity);if(velocity[2]>0)e.client->ps.groundEntityNum=ENTITYNUM_NONE;return;}
 VectorCopy(velocity,e.s.pos.trDelta);
 if(const auto*o=owned(e.s.number))if(prop(o->catalog))return;
 if(e.mass<=0)e.mass=std::max(1,int(attachmentMass(e)));
 const bool missile=e.s.eType==ET_MISSILE;
 VectorCopy(e.currentOrigin,e.s.pos.trBase);e.s.pos.trTime=level.time;
 if(!missile)e.s.pos.trType=TR_GRAVITY;
 if(e.s.eType!=ET_ITEM&&!missile){e.e_ThinkFunc=thinkF_G_RunObject;e.nextthink=level.time+FRAMETIME;e.clipmask|=MASK_SOLID|CONTENTS_BODY;}
}
void attachmentImpulse(gentity_t&e,const vec3_t impulse,const vec3_t point){
 if(!attachmentMovable(e))return;
 const float mass=attachmentMass(e);vec3_t velocity,next;attachmentVelocity(e,velocity);VectorMA(velocity,1.f/mass,impulse,next);
 for(float v:next)if(!std::isfinite(v))return;
 const float speed=VectorLength(next);if(speed>2000)VectorScale(next,2000.f/speed,next);
 if(attachmentBox(e)){vec3_t limited;VectorSubtract(next,velocity,limited);VectorScale(limited,mass,limited);physicsWorld.Impulse(e.s.number,pv(limited),pv(point));}
 else attachmentSetVelocity(e,next);
}
void attachmentMove(gentity_t&e,const vec3_t delta){
 if(!attachmentMovable(e))return;
 vec3_t destination;VectorAdd(e.currentOrigin,delta,destination);
 trace_t tr{};
 std::vector<SandboxChargeMove> charges;bool chargeContact=false;
 if(!G_SandboxPropTrace(e,destination,tr,charges,chargeContact,propMovingHull(e)))return;
 vec3_t velocity;attachmentVelocity(e,velocity);
 if(attachmentBox(e)&&!physicsWorld.Correct(e.s.number,pv(tr.endpos),pv(velocity)))return;
 const auto trajectory=e.s.pos.trType;G_SetOrigin(&e,tr.endpos);e.s.pos.trType=trajectory;if(e.client)VectorCopy(tr.endpos,e.client->ps.origin);
 if(!attachmentBox(e))attachmentSetVelocity(e,velocity);else {e.s.pos.trType=TR_INTERPOLATE;VectorCopy(velocity,e.s.pos.trDelta);}
 setupMoved(e);gi.linkentity(&e);placeEditedCharges(charges);
}
#include "g_workshop_rope.inc"
void attachmentTick(float dt){
 if(!enabled()){G_JAR_MoverMotionFrame(0,nullptr);return;}if(dt<=0)return;
 // Advance forced native reference frames first so ropes and joints see
 // their new anchor positions in this same frame.
 for(unsigned i=0;i<state.jointCount;++i){const auto&j=state.joints[i];if(j.kind!=jar::constraint::Thruster||!j.enabled)continue;auto*a=attachmentEntity(j.first);
  if(!a||attachmentBox(*a)||G_SandboxUsableDormant(*a))continue;vec3_t point,axis,acceleration;if(!attachmentPoint(*a,j.a,point,axis))continue;
  if(a->m_pVehicle){
   VectorScale(axis,JAR_THRUSTER_SIGN*j.force*32.f/attachmentMass(*a),acceleration);
   G_WorkshopVehicleThrust(*a,acceleration,dt);
   continue;
  }
  if(G_JAR_MoverMotionEligible(*a)||G_JAR_MoverTranslationActive(*a)){VectorScale(axis,JAR_THRUSTER_SIGN*j.force*32.f/attachmentMass(*a),acceleration);G_JAR_MoverForce(*a,acceleration);}
  else {vec3_t impulse;VectorScale(axis,JAR_THRUSTER_SIGN*j.force*32.f*dt,impulse);attachmentImpulse(*a,impulse,point);}
  vec3_t force;VectorScale(axis,JAR_THRUSTER_SIGN*j.force*32.f,force);attachmentTorque(*a,force,point,dt);
 }
 G_JAR_MoverMotionFrame(dt,[](const gentity_t&e){return !occupied(e)&&!nativeControls.count(e.s.number)&&!(e.svFlags&SVF_INACTIVE);},[](gentity_t&e){setupMoved(e);gi.linkentity(&e);});
 attachmentPrepareRopes();
 for(unsigned i=0;i<state.jointCount;){const auto&j=state.joints[i];auto*a=attachmentEntity(j.first),*b=attachmentEntity(j.second);
  if(!a||!b||(a->client&&a->health<=0)||(b->client&&b->health<=0)){removeJoint(j.id);continue;}++i;
  if(G_SandboxUsableDormant(*a)||G_SandboxUsableDormant(*b))continue;
  if(j.kind<jar::constraint::Rope||attachmentBoxJoint(j))continue;
  vec3_t pa,pb,axis,unused;if(!attachmentPoint(*a,j.a,pa,axis)||!attachmentPoint(*b,j.b,pb,unused))continue;
  if(j.kind==jar::constraint::Thruster)continue;
  auto route=ropeStates.find(j.id);if(route==ropeStates.end()||!route->second.valid)continue;
  const auto&path=route->second.result;const float length=path.length;if(length<.001f)continue;
  const float wa=attachmentMovable(*a)?1.f/attachmentMass(*a):0,wb=attachmentMovable(*b)?1.f/attachmentMass(*b):0,total=wa+wb;
  vec3_t va,vb;attachmentVelocity(*a,va,pa);attachmentVelocity(*b,vb,pb);
  float outward=-DotProduct(va,path.pullA)-DotProduct(vb,path.pullB);
  const float error=length-j.length;
  if(error<=0&&error+outward*dt<=0&&!path.contactCount)continue;
  RopeLoads loads;
  loads.Add(*a,pa,path.pullA);loads.Add(*b,pb,path.pullB);
  bool movableBend=false;
  for(int contactIndex=0;contactIndex<path.contactCount;++contactIndex){
   const auto& contact=path.contacts[contactIndex];
   auto* e=ropeContactProp(contact);
   if(!e)continue;
   movableBend=true;
   vec3_t point,velocity;
   JAR_RopeContactPoint(&contact,e->currentOrigin,e->mins,e->maxs,point);
   attachmentVelocity(*e,velocity,point);
   outward-=DotProduct(velocity,contact.pull);
   loads.Add(*e,point,contact.pull);
  }
  const float response=loads.Response();
  // A compliant bend lets fixed anchors pull a prop clear without projecting
  // that obstacle through the rope. Static bends keep swept endpoint recovery.
  if(movableBend){loads.Apply(JAR_RopeTensionImpulse(error,outward,response,dt));continue;}
  if(total<=0)continue;
  // Unilateral rope: slack has no force and never pushes endpoints apart.
  // Projection already removes stretch; adding an error/dt velocity bias would
  // inject energy and make a hanging actor bounce. Only arrest separation.
  if(response>0&&(error>0||error+outward*dt>0))
   loads.Apply((outward+std::min(0.f,error)/dt)/response);
  if(error>0){const float correction=std::min(64.f,error);vec3_t delta;VectorScale(path.pullA,correction*wa/total,delta);attachmentMove(*a,delta);VectorScale(path.pullB,correction*wb/total,delta);attachmentMove(*b,delta);}
 }
 attachmentNativeJoints(dt);attachmentPrepareRopes();
}
#include "g_workshop_native_joints.inc"
void attachmentRemember(unsigned id){Edit edit{};edit.joint=id;rememberAction(std::move(edit));}
void attachmentRope(gentity_t&e,const vec3_t anchor,const vec3_t axis){
 const auto key=attachmentKey(e);jar::constraint::Frame frame;
 if(!attachmentFrameAt(e,anchor,axis,frame)){message("Rope anchor is outside supported bounds.");return;}
 if(jointFirst&&jointPendingMode!=jar::constraint::Rope)jointFirst=0;
 if(!jointFirst){jointFirst=key;jointPendingMode=jar::constraint::Rope;attachmentPending=frame;message("First rope anchor set. Select another entity or world surface; right click cancels.");return;}
 auto*a=attachmentEntity(jointFirst);const auto first=jointFirst;jointFirst=0;
 if(!a||first==key){message("Choose two different entities or one entity and the world.");return;}
 vec3_t point,direction;if(!attachmentPoint(*a,attachmentPending,point,direction))return;
 jar::constraint::Joint j;j.kind=jar::constraint::Rope;j.first=first;j.second=key;j.a=attachmentPending;j.b=frame;
 j.length=std::max(4.f,Distance(point,anchor))*std::max(1.f,std::min(3.f,gi.cvar("jar_ropeSlack","1.1",CVAR_ARCHIVE)->value));
 float routeLength;if(!ropeAdmission(j,&routeLength)){message("Rope could not find a clear route around scenery. Move its anchors and try again.");return;}
 j.length=std::max(j.length,routeLength*std::max(1.f,std::min(3.f,gi.cvar("jar_ropeSlack","1.1",CVAR_ARCHIVE)->value)));
 const auto id=AddJoint(state,j);if(!id){message("Rope pair already linked, too long, or attachment capacity reached.");return;}
 attachmentRemember(id);message("Rope attached. Released entities can move within its length; native path movers and world surfaces act as anchors.");
}
void attachmentThruster(gentity_t&e,bool alternate,const vec3_t anchor,const vec3_t axis){
 const auto key=attachmentKey(e);
 if(auto* thruster=ThrusterFor(state,key)){
  if(alternate){removeJoint(thruster->id);message("Thruster removed.");}
  else {thruster->enabled=!thruster->enabled;message(thruster->enabled?"Thruster on.":"Thruster off.");}
  return;
 }
 if(alternate)return;
 // Script and path controllers use additive translation rather than being
 // converted into generic physics objects. Frozen/occupied controls wait.
 if(e.s.number>=ENTITYNUM_WORLD){message("World geometry cannot receive a thruster; choose a movable entity.");return;}
 jar::constraint::Joint j;j.kind=jar::constraint::Thruster;j.first=j.second=key;
 const bool reverse=gi.cvar("jar_thrusterReverse","0",CVAR_ARCHIVE)->integer!=0;
 vec3_t exhaust;VectorScale(axis,reverse?-1.f:1.f,exhaust);
 // Save the signed local exhaust direction in the existing mount frame.
 if(!attachmentFrameAt(e,anchor,exhaust,j.a))return;j.b=j.a;
 j.force=std::max(100.f,std::min(100000.f,gi.cvar("jar_thrusterForce","5000",CVAR_ARCHIVE)->value));
 const auto id=AddJoint(state,j);if(!id){message("Thruster capacity reached.");return;}
 attachmentRemember(id);if(auto*owner=owned(e.s.number))if(prop(owner->catalog))owner->frozen=0;
 message(reverse?"Thruster mounted. Force pushes away from the surface.":"Thruster mounted. Force pushes into the surface.");
}
