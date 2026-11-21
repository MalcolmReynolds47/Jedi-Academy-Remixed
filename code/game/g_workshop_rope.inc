// Authoritative rope paths. Included after the native attachment helpers.
// GPL-2.0-only.
#include "../../shared/qcommon/jar_rope_path_mesh.h"
struct RopeState {
 uint32_t first=0,second=0;
 uint64_t scene=0;
 bool valid=false,history=false;
 vec3_t a{},b{};
 jar::physics::Pose poseA,poseB;
 jarRopePathResult_t result{};
 struct {
  uint64_t scene=0;
  float length=0;
  vec3_t low{},high{};
  int count=0;
  jarRopeVertex_t vertices[JAR_ROPE_VERTICES];
 } mesh;
};
std::map<uint32_t,RopeState> ropeStates;
int ropeBudgetTime=-1,ropeTraceBudget=4096,ropeMessageTime=0,ropeMessageStatus=-1;
unsigned ropeCursor=0;
void ropeReset(){ropeStates.clear();ropeBudgetTime=-1;ropeTraceBudget=4096;ropeMessageTime=0;ropeMessageStatus=-1;ropeCursor=0;}

#include "g_workshop_rope_trace.inc"
uint64_t ropeScene(const vec3_t low,const vec3_t high,int first,int second){
 gentity_t* nearby[JAR_ROPE_PATH_NEARBY];
 const int count=gi.EntitiesInBox(low,high,nearby,JAR_ROPE_PATH_NEARBY);
 if(count>=JAR_ROPE_PATH_NEARBY)return 0;
 uint64_t hash=1;
 for(int i=0;i<count;++i){const auto& e=*nearby[i];const int n=e.s.number;
  if(n==first||n==second||n<0||n>=ENTITYNUM_WORLD||!e.inuse||!e.linked||!(e.contents&JAR_ROPE_PATH_CONTENTS))continue;
  const bool prop=e.classname&&!strcmp(e.classname,"jar_sandbox_prop");
  hash^=JAR_RopeSceneEntity(n,entityGeneration[n],e.contents,e.s.modelindex,prop?e.s.frame:0,e.currentOrigin,
                          e.bmodel||prop?e.currentAngles:vec3_origin,e.mins,e.maxs);
 }
 return hash;
}
uint64_t ropePathScene(const jarRopePath_t& path,const vec3_t a,const vec3_t b,int first,int second){
 vec3_t low,high;JAR_RopePathBounds(&path,a,b,low,high);
 return ropeScene(low,high,first,second);
}
bool ropeEndpoints(const jar::constraint::Joint&j,const gentity_t&a,const gentity_t&b,vec3_t pa,vec3_t pb,float&caps){
 vec3_t axisA,axisB;caps=0;
 if(!attachmentPoint(a,j.a,pa,axisA)||!attachmentPoint(b,j.b,pb,axisB))return false;
 // The world cannot be excluded from collision. Move its trace endpoint out
 // along the clicked surface normal and account for that short end section.
 for(int i=0;i<2;++i){const auto&e=i?b:a;float*p=i?pb:pa;const float*axis=i?axisB:axisA;
  if(e.s.number!=ENTITYNUM_WORLD)continue;
  caps+=ropeWorldEndClearance(p,axis);
 }
 return JAR_RopePathPoint(pa)&&JAR_RopePathPoint(pb);
}
jar::physics::Pose ropePose(const gentity_t&e){jar::physics::Pose pose;attachmentPose(e,pose);return pose;}
bool ropeAdmission(jar::constraint::Joint&j,float*length=nullptr){
 if(j.kind!=jar::constraint::Rope)return true;
 auto*a=attachmentEntity(j.first),*b=attachmentEntity(j.second);vec3_t pa,pb;float caps;
 if(!a||!b)return false;
 if(!length&&(G_SandboxUsableDormant(*a)||G_SandboxUsableDormant(*b)))return true;
 if(!ropeEndpoints(j,*a,*b,pa,pb,caps))return false;
 RopeTraceScope scope(*a,*b);
 auto result=JAR_RopePathSolve(&j.path,pa,pb,j.length,ropeTrace,nullptr,JAR_ROPE_PATH_TRACE_LIMIT);
 if(result.status!=JAR_ROPE_PATH_OK)return false;
 if(length)*length=result.length+caps;
 return true;
}
bool resizeRope(jar::constraint::Joint& joint,float length){
 auto candidate=joint;
 candidate.length=length;
 if(candidate.kind!=jar::constraint::Rope||!jar::constraint::Valid(candidate)||!ropeAdmission(candidate))return false;
 joint=candidate;
 const auto found=ropeStates.find(joint.id);
 if(found!=ropeStates.end()){
  found->second.valid=false;
  found->second.scene=0;
  found->second.mesh.scene=0;
 }
 // The next rope update revalidates the route before a constraint can pull.
 physicsWorld.ForgetJoint(joint.id);
 return true;
}
void ropeStop(gentity_t&e){
 if(!attachmentMovable(e))return;
 vec3_t zero={0,0,0};
 if(attachmentBox(e)){jar::physics::Pose p;if(physicsWorld.Read(e.s.number,p)){p.linear=p.angular={};physicsWorld.CorrectPose(e.s.number,p);}}
 attachmentSetVelocity(e,zero);if(!e.client)VectorClear(e.s.apos.trDelta);
}
void ropeRestore(gentity_t&e,const jar::physics::Pose&old){
 if(e.s.number>=ENTITYNUM_WORLD||(!attachmentMovable(e)&&!nativeControls.count(e.s.number)))return;
 vec3_t target,angles,lo,hi,sweepLo,sweepHi;nativeVector(old.position,target);nativeVector(old.angles,angles);
 VectorCopy(e.mins,lo);VectorCopy(e.maxs,hi);
 if(const auto*o=owned(e.s.number))if(const auto*p=prop(o->catalog))if(!propHull(*p,angles,lo,hi,e.s.frame))return;
 const bool rotated=!VectorCompare(angles,e.currentAngles);
 const float radius=std::max(RadiusFromBounds(lo,hi),RadiusFromBounds(e.mins,e.maxs));
 for(int k=0;k<3;++k){sweepLo[k]=rotated?-radius:std::min(lo[k],e.mins[k]);sweepHi[k]=rotated?radius:std::max(hi[k],e.maxs[k]);}
 trace_t tr{};{BSPSectionMotionScope sectionMotion(e);if(!sectionMotion)return;
  gi.trace(&tr,e.currentOrigin,sweepLo,sweepHi,target,e.s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);}
 if(tr.startsolid||tr.allsolid||tr.fraction<1)return;
 std::vector<SandboxChargeMove> charges;
 if(!mountedChargePoseClear(e,target,angles,charges,true))return;
 auto pose=old;pose.linear=pose.angular={};if(attachmentBox(e)&&!physicsWorld.CorrectPose(e.s.number,pose))return;
 G_SetOrigin(&e,target);G_SetAngles(&e,angles);VectorCopy(lo,e.mins);VectorCopy(hi,e.maxs);
 if(e.client){VectorCopy(target,e.client->ps.origin);SetClientViewAngle(&e,angles);}
 vec3_t zero={0,0,0};attachmentSetVelocity(e,zero);if(!e.client)VectorClear(e.s.apos.trDelta);
 if(attachmentBox(e))e.s.pos.trType=e.s.apos.trType=TR_INTERPOLATE;
 setupMoved(e);gi.linkentity(&e);placeEditedCharges(charges);
}
bool ropeUpdate(jar::constraint::Joint&j){
 auto*a=attachmentEntity(j.first),*b=attachmentEntity(j.second);if(!a||!b)return false;
 if(G_SandboxUsableDormant(*a)||G_SandboxUsableDormant(*b)){ropeStates.erase(j.id);return false;}
 auto&cache=ropeStates[j.id];
 if(cache.first!=j.first||cache.second!=j.second){cache={};cache.first=j.first;cache.second=j.second;}
 vec3_t pa,pb;float caps;if(!ropeEndpoints(j,*a,*b,pa,pb,caps)){cache.valid=false;return false;}
 if(cache.history&&cache.scene&&VectorCompare(pa,cache.a)&&VectorCompare(pb,cache.b)&&
    cache.scene==ropePathScene(j.path,pa,pb,a->s.number,b->s.number)){
  cache.valid=true;cache.poseA=ropePose(*a);cache.poseB=ropePose(*b);return true;
 }
 if(ropeBudgetTime!=level.time){ropeBudgetTime=level.time;ropeTraceBudget=4096;}
 jarRopePathResult_t result{};result.status=JAR_ROPE_PATH_BUDGET;
 bool bodyContact=false;
 if(ropeTraceBudget>0){
  RopeTraceScope scope(*a,*b);
  result=JAR_RopePathSolve(&j.path,pa,pb,j.length,ropeTrace,&bodyContact,std::min(JAR_ROPE_PATH_TRACE_LIMIT,ropeTraceBudget));
  ropeTraceBudget-=result.traces;
 }
 if(result.status==JAR_ROPE_PATH_OK){
  cache.valid=cache.history=true;cache.scene=ropePathScene(j.path,pa,pb,a->s.number,b->s.number);VectorCopy(pa,cache.a);VectorCopy(pb,cache.b);
  cache.mesh.scene=0;
  cache.poseA=ropePose(*a);cache.poseB=ropePose(*b);if(j.path.count)physicsWorld.ForgetJoint(j.id);cache.result=result;cache.result.length+=caps;return true;
 }
 // Failed geometry never drives a straight constraint through an obstacle.
 physicsWorld.ForgetJoint(j.id);
 cache.valid=false;
 // A body can cross an anchor between frames. Suspend tension until a route
 // clears instead of teleporting or stopping either attached object.
 if(bodyContact)return false;
 if(cache.history){ropeRestore(*a,cache.poseA);ropeRestore(*b,cache.poseB);}
 ropeStop(*a);ropeStop(*b);
 if(level.time>=ropeMessageTime||ropeMessageStatus!=int(result.status)){message(result.status==JAR_ROPE_PATH_BUDGET?
  "Rope collision work is busy; movement held while links are checked.":
  "Rope route is obstructed. Move the obstacle or Unlink this rope to release it.");ropeMessageTime=level.time+2000;ropeMessageStatus=int(result.status);}
 return false;
}
void attachmentPrepareRopes(){
 std::map<uint32_t,bool> live;for(unsigned i=0;i<state.jointCount;++i)if(state.joints[i].kind==jar::constraint::Rope)live[state.joints[i].id]=true;
 for(auto it=ropeStates.begin();it!=ropeStates.end();)if(!live.count(it->first))it=ropeStates.erase(it);else ++it;
 if(live.empty())return;
 const unsigned start=ropeCursor++%state.jointCount;
 for(unsigned offset=0;offset<state.jointCount;++offset){auto&j=state.joints[(start+offset)%state.jointCount];if(j.kind==jar::constraint::Rope)ropeUpdate(j);}
}
bool ropeRigid(const jar::constraint::Joint&j){
 auto found=ropeStates.find(j.id);return !j.path.count&&found!=ropeStates.end()&&found->second.valid;
}
gentity_t* ropeContactProp(const jarRopeContact_t& contact){
 const int n=contact.prop-1;
 if(n<0||n>=ENTITYNUM_WORLD||entityGeneration[n]!=contact.generation||held==unsigned(n))return nullptr;
 auto& e=g_entities[n];const auto* o=owned(n);
 if(!e.inuse||!e.linked||e.client||!o||!RigidCatalog(o->catalog)||!matches(*o)||!attachmentMovable(e))return nullptr;
 return &e;
}
// Endpoints and bends use one net impulse and torque per body. Multiple bends
// on one prop can partly cancel, so apply the same sum used to compute tension.
struct RopeLoads {
 jarRopeLoad_t bodies[JAR_ROPE_PATH_BENDS+2];
 int count=0;

 void Add(const gentity_t& entity,const vec3_t point,const vec3_t pull){
  if(!attachmentMovable(entity))return;
  jarRopeContact_t contact{};
  contact.prop=entity.s.number+1;
  VectorCopy(pull,contact.pull);
  JAR_RopeAddLoad(bodies,&count,&contact,point,entity.currentOrigin,attachmentMass(entity));
 }

 float Response() const {
  float result=0;
  for(int i=0;i<count;++i){
   const auto& load=bodies[i];const auto& e=g_entities[load.prop-1];
   if(attachmentBox(e)){
    result+=physicsWorld.ImpulseResponse(e.s.number,pv(load.linear),pv(load.torque),pv(e.currentOrigin));
   }else{
    result+=VectorLengthSquared(load.linear)/load.mass;
    const auto* o=owned(e.s.number);
    if(o&&prop(o->catalog)){
     vec3_t angular;
     nativeVector(jar::physics::AngularResponse(pv(e.currentAngles),attachmentHalf(e),load.mass,pv(load.torque)),angular);
     result+=DotProduct(load.torque,angular);
    }
   }
  }
  return result;
 }

 void Apply(float tension) const {
  if(!(tension>0)||!std::isfinite(tension))return;
  for(int i=0;i<count;++i){
   const auto& load=bodies[i];auto& e=g_entities[load.prop-1];
   vec3_t impulse,torque;
   VectorScale(load.linear,tension,impulse);VectorScale(load.torque,tension,torque);
   if(attachmentBox(e)){
    physicsWorld.LimitedWrench(e.s.number,pv(impulse),pv(torque),pv(e.currentOrigin),2000,8);
   }else{
    if(VectorLengthSquared(impulse)>0)attachmentImpulse(e,impulse,e.currentOrigin);
    const auto* o=owned(e.s.number);
    if(o&&prop(o->catalog)){
     vec3_t angular;
     nativeVector(jar::physics::AngularResponse(pv(e.currentAngles),attachmentHalf(e),load.mass,pv(torque)),angular);
     VectorAdd(e.s.apos.trDelta,angular,e.s.apos.trDelta);
     const float spin=VectorLength(e.s.apos.trDelta);
     if(spin>8)VectorScale(e.s.apos.trDelta,8/spin,e.s.apos.trDelta);
    }
   }
  }
 }
};
struct RopeMeshTraceBounds { vec3_t low,high; };
void ropeMeshTrace(const float* a,const float* b,float radius,jarRopePathHit_t* hit,void* data){
 auto& bounds=*static_cast<RopeMeshTraceBounds*>(data);
 // Record attempted sag as well as the clipped mesh. Moving a floor away
 // must invalidate the mesh even if the accepted spans never touched it.
 for(int k=0;k<3;++k){
  bounds.low[k]=std::min(bounds.low[k],std::min(a[k],b[k])-radius);
  bounds.high[k]=std::max(bounds.high[k],std::max(a[k],b[k])+radius);
 }
 ropeTrace(a,b,radius,hit,nullptr);
}
int ropeMesh(const jar::constraint::Joint&j,const jarRopeVertex_t*& vertices){
 auto found=ropeStates.find(j.id);if(found==ropeStates.end()||!found->second.valid)return 0;
 auto*a=attachmentEntity(j.first),*b=attachmentEntity(j.second);vec3_t pa,pb;float caps;
 if(!a||!b||!ropeEndpoints(j,*a,*b,pa,pb,caps)||!VectorCompare(pa,found->second.a)||!VectorCompare(pb,found->second.b))return 0;
 auto& mesh=found->second.mesh;
 const float length=std::max(0.f,j.length-caps);
 if(!mesh.scene||mesh.length!=length||mesh.scene!=ropeScene(mesh.low,mesh.high,a->s.number,b->s.number)){
  RopeMeshTraceBounds bounds;
  JAR_RopePathBounds(&j.path,pa,pb,bounds.low,bounds.high);
  {
   RopeTraceScope scope(*a,*b);
   mesh.count=JAR_RopePathMesh(&j.path,pa,pb,length,mesh.vertices,ropeMeshTrace,&bounds,0);
  }
  VectorCopy(bounds.low,mesh.low);VectorCopy(bounds.high,mesh.high);
  mesh.length=length;
  mesh.scene=ropeScene(mesh.low,mesh.high,a->s.number,b->s.number);
 }
 vertices=mesh.vertices;
 return mesh.count;
}
