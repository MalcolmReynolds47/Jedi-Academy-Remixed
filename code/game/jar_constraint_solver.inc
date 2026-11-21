// Solver-only native/mixed joint graph. No duplicate collision geometry or AI.
// Included in jar::physics after the shared coordinate/quaternion helpers.
Vec AngularVelocity(Vec before,Vec after,float dt){
 if(!finite(before,100000)||!finite(after,100000)||!std::isfinite(dt)||dt<=0)return {};
 auto inverse=Q(before);inverse.v=b3Neg(inverse.v);auto delta=b3NormalizeQuat(b3MulQuat(Q(after),inverse));if(delta.s<0){delta.s=-delta.s;delta.v=b3Neg(delta.v);}
 const float length=b3Length(delta.v);return length>.000001f?V(b3MulSV(2*std::atan2(length,delta.s)/(length*dt),delta.v)):Vec{};
}
Vec AdvanceAngles(Vec angles,Vec angular,float dt){
 if(!finite(angles,100000)||!finite(angular,1000)||!std::isfinite(dt))return finite(angles,100000)?angles:Vec{};
 const auto velocity=V(angular);const float speed=b3Length(velocity);return speed>.000001f?Angles(b3MulQuat(Axis(b3MulSV(1/speed,velocity),speed*dt),Q(angles))):angles;
}
// World-space angular impulse from r cross impulse and rotated box inertia.
Vec AngularImpulse(Vec angles,Vec half,float mass,Vec impulse,Vec lever){
 if(!finite(impulse,1e9f)||!finite(lever,131073))return {};
 return AngularResponse(angles,half,mass,V(b3Cross(V(lever),V(impulse))));
}
Vec AngularResponse(Vec angles,Vec half,float mass,Vec torque){
 if(!finite(angles,100000)||!finite(half,8193)||half.x<.25f||half.y<.25f||half.z<.25f||
    !std::isfinite(mass)||mass<1||mass>100000||!finite(torque,2e14f))return {};
 const auto q=Q(angles);const auto local=b3InvRotateVector(q,V(torque));
 const b3Vec3 spin={3*local.x/(mass*(half.y*half.y+half.z*half.z)),3*local.y/(mass*(half.x*half.x+half.z*half.z)),3*local.z/(mass*(half.x*half.x+half.y*half.y))};
 return V(b3RotateVector(q,spin));
}
struct ConstraintSolver::Impl {std::vector<ConstraintBody> result;};
ConstraintSolver::ConstraintSolver():impl(new Impl){}
ConstraintSolver::~ConstraintSolver()=default;
void ConstraintSolver::Clear(){impl->result.clear();}
// Solve each 5/6-row velocity constraint as one block. Separately solving
// rotation and translation leaves a large residual for small bodies on long
// lever arms. This mass/inertia projection adds no positional spring energy.
struct VelocityProjection {
 struct Contact {size_t body;double row[6]{},weighted[6]{};};
 uint32_t first=0,second=0;
 int count=0;
 bool unilateral=false;
 double gap=0,rows[6][12]{},weighted[6][12]{},inverse[6][6]{};
 std::vector<Contact> contacts;
};
static void JointWeight(const ConstraintBody& body,const double* row,double* weighted){
 if(!body.movable)return;
 for(int k=0;k<3;++k)weighted[k]=row[k]/body.mass;
 // Inertia uses game-unit distances, matching r cross linear impulse.
 const auto rotation=Q(body.pose.angles);
 const b3Vec3 xyz[3]={{1,0,0},{0,1,0},{0,0,1}};
 const double h[3]={body.half.x,body.half.y,body.half.z};
 for(int axis=0;axis<3;++axis){
  const auto rotated=b3RotateVector(rotation,xyz[axis]);const double basis[3]={rotated.x,rotated.y,rotated.z};
  double torque=0;for(int k=0;k<3;++k)torque+=basis[k]*row[3+k];
  torque*=3/(body.mass*(h[(axis+1)%3]*h[(axis+1)%3]+h[(axis+2)%3]*h[(axis+2)%3]));
  for(int k=0;k<3;++k)weighted[3+k]+=basis[k]*torque;
 }
}
static VelocityProjection VelocityBlock(const ConstraintBody&a,const ConstraintBody&b,const constraint::Joint&j,
 const std::vector<ConstraintBody>& bodies,const std::map<uint32_t,size_t>& indices,const ConstraintRoute* route){
 VelocityProjection projection;projection.first=j.first;projection.second=j.second;
 auto&rows=projection.rows;auto&weighted=projection.weighted;double matrix[6][12]{};
 const bool rope=j.kind==constraint::Rope;
 const int count=rope?1:j.kind==constraint::Weld?6:5;
 Vec pointA,pointB,unused;if(!FrameTransform(a.pose,j.a,pointA,unused)||!FrameTransform(b.pose,j.b,pointB,unused))return projection;
 const auto ra=b3Sub(V(pointA),V(a.pose.position)),rb=b3Sub(V(pointB),V(b.pose.position));
 const b3Vec3 xyz[3]={{1,0,0},{0,1,0},{0,0,1}};
 auto put=[](double*row,int at,b3Vec3 v){row[at]=v.x;row[at+1]=v.y;row[at+2]=v.z;};
 if(rope){
  b3Vec3 points[JAR_ROPE_PATH_BENDS+2],segments[JAR_ROPE_PATH_BENDS+1];
  points[0]=V(pointA);points[j.path.count+1]=V(pointB);
  for(int i=0;i<j.path.count;++i){
   const auto& point=j.path.bend[i];points[i+1]={point[0],point[1],point[2]};
   if(route&&(*route)[i].key){
    const auto& bend=(*route)[i];const auto& body=bodies[indices.at(bend.key)];
    points[i+1]=b3Add(V(body.pose.position),b3RotateVector(Q(body.pose.angles),V(bend.local)));
   }
  }
  double length=0;
  for(int i=0;i<=j.path.count;++i){
   const auto span=b3Sub(points[i+1],points[i]);const float distance=b3Length(span);length+=distance;
   segments[i]=distance>.0001f?b3MulSV(1/distance,span):b3Vec3{};
  }
  projection.unilateral=true;
  projection.gap=length-j.length;
  const auto dirA=b3Neg(segments[0]),dirB=segments[j.path.count];
  put(rows[0],0,dirA);put(rows[0],3,b3Cross(ra,dirA));
  put(rows[0],6,dirB);put(rows[0],9,b3Cross(rb,dirB));
  // The derivative at each bend is the difference of its two segment
  // directions. Add bends on the same prop before computing effective mass.
  if(route)for(int i=0;i<j.path.count;++i)if((*route)[i].key){
   const size_t index=indices.at((*route)[i].key);
   auto found=std::find_if(projection.contacts.begin(),projection.contacts.end(),[&](const auto& contact){return contact.body==index;});
   if(found==projection.contacts.end()){
    projection.contacts.push_back({index});found=projection.contacts.end()-1;
   }
   const auto direction=b3Sub(segments[i],segments[i+1]);
   const auto lever=b3Sub(points[i+1],V(bodies[index].pose.position));
   const auto moment=b3Cross(lever,direction);
   const double add[6]={direction.x,direction.y,direction.z,moment.x,moment.y,moment.z};
   for(int k=0;k<6;++k)found->row[k]+=add[k];
  }
 }else{
  for(int n=0;n<3;++n){put(rows[n],0,b3Neg(xyz[n]));put(rows[n],3,b3Neg(b3Cross(ra,xyz[n])));put(rows[n],6,xyz[n]);put(rows[n],9,b3Cross(rb,xyz[n]));}
  const auto frameQ=b3MulQuat(Q(a.pose.angles),b3Quat{{j.a.rotation[0],j.a.rotation[1],j.a.rotation[2]},j.a.rotation[3]});
  for(int n=3;n<count;++n){const auto axis=j.kind==constraint::Weld?xyz[n-3]:b3RotateVector(frameQ,xyz[n-3]);put(rows[n],3,b3Neg(axis));put(rows[n],9,axis);}
 }
 for(int n=0;n<count;++n){
  JointWeight(a,rows[n],weighted[n]);JointWeight(b,rows[n]+6,weighted[n]+6);
 }
 for(auto& contact:projection.contacts){
  JointWeight(bodies[contact.body],contact.row,contact.weighted);
  for(int k=0;k<6;++k)matrix[0][0]+=contact.row[k]*contact.weighted[k];
 }
 for(int i=0;i<count;++i){matrix[i][count+i]=1;for(int n=0;n<count;++n)for(int k=0;k<12;++k)matrix[i][n]+=rows[i][k]*weighted[n][k];}
 for(int i=0;i<count;++i){int pivot=i;for(int n=i+1;n<count;++n)if(std::fabs(matrix[n][i])>std::fabs(matrix[pivot][i]))pivot=n;if(std::fabs(matrix[pivot][i])<1e-14)return projection;
  for(int k=i;k<2*count;++k)std::swap(matrix[i][k],matrix[pivot][k]);const double divisor=matrix[i][i];for(int k=i;k<2*count;++k)matrix[i][k]/=divisor;
  for(int n=0;n<count;++n)if(n!=i){const double factor=matrix[n][i];for(int k=i;k<2*count;++k)matrix[n][k]-=factor*matrix[i][k];}
 }
 for(int i=0;i<count;++i)for(int n=0;n<count;++n)projection.inverse[i][n]=matrix[i][count+n];
 projection.count=count;return projection;
}
// Project the entire graph together. Pairwise iteration converges too slowly
// for chains, especially when a small body's anchor has a long lever arm.
// Matrix-free conjugate gradients with 5/6-row block preconditioning keeps
// storage linear and also handles redundant links without a dense inverse.
static bool ProjectGraph(const std::vector<VelocityProjection>&blocks,const std::map<uint32_t,size_t>&indices,size_t bodies,const std::vector<double>&error,std::vector<double>&change,std::vector<double>&lambda){
 const size_t count=error.size();lambda.assign(count,0);std::vector<double>residual(count),z(count),direction(count),product(count),motion(bodies*6),diagonal(count);
 std::vector<std::pair<size_t,size_t>> offsets;offsets.reserve(blocks.size());for(const auto&p:blocks)offsets.emplace_back(indices.at(p.first)*6,indices.at(p.second)*6);
 size_t row=0;
 for(const auto& p:blocks){
  for(int i=0;i<p.count;++i)for(int k=0;k<12;++k)diagonal[row+i]+=p.rows[i][k]*p.weighted[i][k];
  for(const auto& contact:p.contacts)for(int k=0;k<6;++k)diagonal[row]+=contact.row[k]*contact.weighted[k];
  row+=p.count;
 }
 auto scatter=[&](const std::vector<double>& x){
  std::fill(motion.begin(),motion.end(),0);size_t at=0,block=0;
  for(const auto& p:blocks){
   const auto offset=offsets[block++];const size_t first=offset.first,second=offset.second;
   for(int i=0;i<p.count;++i)for(int k=0;k<6;++k){motion[first+k]+=p.weighted[i][k]*x[at+i];motion[second+k]+=p.weighted[i][6+k]*x[at+i];}
   for(const auto& contact:p.contacts)for(int k=0;k<6;++k)motion[contact.body*6+k]+=contact.weighted[k]*x[at];
   at+=p.count;
  }
 };
 auto multiply=[&](const std::vector<double>& x,std::vector<double>& out){
  scatter(x);size_t at=0,block=0;
  for(const auto& p:blocks){
   const auto offset=offsets[block++];const size_t first=offset.first,second=offset.second;
   for(int i=0;i<p.count;++i){out[at+i]=1e-14*diagonal[at+i]*x[at+i];for(int k=0;k<6;++k)out[at+i]+=p.rows[i][k]*motion[first+k]+p.rows[i][6+k]*motion[second+k];}
   for(const auto& contact:p.contacts)for(int k=0;k<6;++k)out[at]+=contact.row[k]*motion[contact.body*6+k];
   at+=p.count;
  }
 };
 auto precondition=[&](){size_t at=0;for(const auto&p:blocks){for(int i=0;i<p.count;++i){z[at+i]=0;for(int n=0;n<p.count;++n)z[at+i]+=p.inverse[i][n]*residual[at+n];}at+=p.count;}};
 auto dot=[](const std::vector<double>&a,const std::vector<double>&b){double value=0;for(size_t i=0;i<a.size();++i)value+=a[i]*b[i];return value;};
 for(size_t i=0;i<count;++i)residual[i]=-error[i];precondition();direction=z;double rz=dot(residual,z);const double initial=rz;
 if(!std::isfinite(rz)||rz<0)return false;
 for(size_t iteration=0;iteration<std::max(size_t(16),count*4)&&rz>std::max(1e-18,initial*1e-14);++iteration){
  multiply(direction,product);const double divisor=dot(direction,product);if(!std::isfinite(divisor)||divisor<=0)return false;const double step=rz/divisor;
  for(size_t i=0;i<count;++i){lambda[i]+=step*direction[i];residual[i]-=step*product[i];}precondition();const double next=dot(residual,z);if(!std::isfinite(next)||next<0)return false;
  for(size_t i=0;i<count;++i)direction[i]=z[i]+(next/rz)*direction[i];rz=next;
 }
 scatter(lambda);change=motion;for(double value:change)if(!std::isfinite(value))return false;return true;
}
static bool ProjectActive(std::vector<VelocityProjection>& blocks,const std::map<uint32_t,size_t>& indices,size_t bodies,std::vector<double>& error,std::vector<double>& change){
 std::vector<double> impulses;
 for(;;){
  if(!ProjectGraph(blocks,indices,bodies,error,change,impulses))return false;
  size_t row=0,rejected=blocks.size(),rejectedRow=0;
  double largest=0;
  for(size_t i=0;i<blocks.size();++i){
   // Positive impulse would compress a cable. Release that row and resolve.
   if(blocks[i].unilateral&&impulses[row]>largest){largest=impulses[row];rejected=i;rejectedRow=row;}
   row+=blocks[i].count;
  }
  if(rejected==blocks.size())return true;
  blocks.erase(blocks.begin()+rejected);
  error.erase(error.begin()+rejectedRow);
 }
}
static double JointSpeed(const double* row,const Pose& pose){
 return row[0]*pose.linear.x+row[1]*pose.linear.y+row[2]*pose.linear.z+
        row[3]*pose.angular.x+row[4]*pose.angular.y+row[5]*pose.angular.z;
}
bool ConstraintSolver::Solve(std::vector<ConstraintBody>&bodies,const std::vector<constraint::Joint>&joints,float dt,bool multiplayer,
 const std::vector<ConstraintRoute>* routes){
 const size_t maximumBodies=constraint::limit*(routes?2+JAR_ROPE_PATH_BENDS:2);
 if(!std::isfinite(dt)||dt<=0||dt>(multiplayer?.101f:.051f)||bodies.size()>maximumBodies||joints.size()>constraint::limit||
    (routes&&(!multiplayer||routes->size()!=joints.size())))return false;
 std::map<uint32_t,size_t> indices;
 for(size_t i=0;i<bodies.size();++i){const auto&b=bodies[i];if(!b.key||!indices.emplace(b.key,i).second||!valid(b.pose)||!finite(b.half,8193)||b.half.x<.25f||b.half.y<.25f||b.half.z<.25f||!std::isfinite(b.mass)||b.mass<=0||(multiplayer?!std::isfinite(1.f/b.mass):b.mass<1||b.mass>100000))return false;}
 std::map<uint32_t,bool> ids;
 for(size_t i=0;i<joints.size();++i){
  const auto&j=joints[i];
  if(!constraint::Valid(j,multiplayer?131072.f:constraint::ropeMaxLength)||j.kind>(multiplayer?constraint::Rope:constraint::Hinge)||!indices.count(j.first)||!indices.count(j.second)||!ids.emplace(j.id,true).second)return false;
  if(routes)for(int b=0;b<j.path.count;++b){
   const auto& bend=(*routes)[i][b];
   if(bend.key&&(!indices.count(bend.key)||bend.key==j.first||bend.key==j.second||!finite(bend.local,131072)))return false;
  }
 }
 if(joints.empty()){Clear();return true;}
 auto&result=impl->result;result=bodies;
 // Coupled position/rotation projection uses the same mass Jacobian as the
 // velocity solve. A weld on a long lever arm must solve all six freedoms
 // together; alternating rotation and translation leaves visible sag.
 std::vector<double> change,error;std::vector<VelocityProjection> projections;
 for(int pass=0;pass<32;++pass){bool moved=false;error.clear();projections.clear();
  for(size_t index=0;index<joints.size();++index){
   const auto&j=joints[index];const auto&a=result[indices[j.first]],&b=result[indices[j.second]];
   if(!a.movable&&!b.movable&&j.kind!=constraint::Rope)continue;
   auto p=VelocityBlock(a,b,j,result,indices,routes?&(*routes)[index]:nullptr);
   if(p.unilateral){
    if(p.count&&p.gap>.001){moved=true;error.push_back(p.gap);projections.push_back(std::move(p));}
    continue;
   }
   if(!p.count)return false;
   Vec pa,pb,axis;FrameTransform(a.pose,j.a,pa,axis);FrameTransform(b.pose,j.b,pb,axis);
   const auto qa=b3NormalizeQuat(b3MulQuat(Q(a.pose.angles),b3Quat{{j.a.rotation[0],j.a.rotation[1],j.a.rotation[2]},j.a.rotation[3]}));
   const auto qb=b3NormalizeQuat(b3MulQuat(Q(b.pose.angles),b3Quat{{j.b.rotation[0],j.b.rotation[1],j.b.rotation[2]},j.b.rotation[3]}));
   auto inverse=qa;inverse.v=b3Neg(inverse.v);
   auto delta=j.kind==constraint::Weld?b3NormalizeQuat(b3MulQuat(qb,inverse)):b3ComputeQuatBetweenUnitVectors(b3RotateVector(qa,{0,0,1}),b3RotateVector(qb,{0,0,1}));
   if(delta.s<0){delta.s=-delta.s;delta.v=b3Neg(delta.v);}const float magnitude=b3Length(delta.v);
   const auto angular=magnitude>.000001f?b3MulSV(2*std::atan2(magnitude,delta.s)/magnitude,delta.v):b3Vec3{};
   double jointError[6]={pb.x-pa.x,pb.y-pa.y,pb.z-pa.z,0,0,0};
   for(int n=3;n<p.count;++n)jointError[n]=p.rows[n][9]*angular.x+p.rows[n][10]*angular.y+p.rows[n][11]*angular.z;
   const double distance=jointError[0]*jointError[0]+jointError[1]*jointError[1]+jointError[2]*jointError[2];if(distance>=.000001||b3LengthSquared(angular)>=.0000000001f)moved=true;
   error.insert(error.end(),jointError,jointError+p.count);projections.push_back(std::move(p));
  }if(!moved)break;
  if(!ProjectActive(projections,indices,result.size(),error,change))return false;
  auto value=[&](size_t at){return Vec{float(change[at]),float(change[at+1]),float(change[at+2])};};float rotation=0;
  for(size_t i=0;i<result.size();++i)rotation=std::max(rotation,b3Length(V(value(i*6+3))));const float fraction=rotation>.35f?.35f/rotation:1.f;
  for(size_t i=0;i<result.size();++i){auto&body=result[i];if(!body.movable)continue;const size_t at=i*6;body.pose.position.x+=float(change[at])*fraction;body.pose.position.y+=float(change[at+1])*fraction;body.pose.position.z+=float(change[at+2])*fraction;body.pose.angles=AdvanceAngles(body.pose.angles,value(at+3),fraction);if(!valid(body.pose))return false;}
 }
 for(int pass=0;pass<(multiplayer?8:1);++pass){
  projections.clear();error.clear();
  bool needsProjection=false;
  for(size_t index=0;index<joints.size();++index){
   const auto&j=joints[index];const auto&a=result[indices[j.first]],&b=result[indices[j.second]];
   if(!a.movable&&!b.movable&&j.kind!=constraint::Rope)continue;
   auto p=VelocityBlock(a,b,j,result,indices,routes?&(*routes)[index]:nullptr);
   if(!p.count){if(p.unilateral)continue;return false;}
   double residual[6]{};
   for(int i=0;i<p.count;++i)residual[i]=JointSpeed(p.rows[i],a.pose)+JointSpeed(p.rows[i]+6,b.pose);
   for(const auto& contact:p.contacts)residual[0]+=JointSpeed(contact.row,result[contact.body].pose);
   if(p.unilateral){
    residual[0]-=std::max(0.0,-p.gap)/dt;
    if(residual[0]<=.001)continue;
   }
   for(int i=0;i<p.count;++i)needsProjection|=std::fabs(residual[i])>.001;
   error.insert(error.end(),residual,residual+p.count);projections.push_back(std::move(p));
  }
  if(multiplayer&&!needsProjection)break;
  if(!ProjectActive(projections,indices,result.size(),error,change))return false;
  for(size_t i=0;i<result.size();++i){auto&body=result[i];if(!body.movable)continue;const size_t at=i*6;body.pose.linear.x+=float(change[at]);body.pose.linear.y+=float(change[at+1]);body.pose.linear.z+=float(change[at+2]);body.pose.angular.x+=float(change[at+3]);body.pose.angular.y+=float(change[at+4]);body.pose.angular.z+=float(change[at+5]);}
 }
 for(const auto&b:result)if(!valid(b.pose))return false;
 bodies=result;return true;
}
