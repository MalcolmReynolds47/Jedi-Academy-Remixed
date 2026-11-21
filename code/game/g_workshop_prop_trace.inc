// Native trace geometry is cached at the current and rendered rotations.
// The two poses can alternate between server and client traces. GPL-2.0-only.
struct WorkshopPropTracePose {
 vec3_t angles{};
 std::unique_ptr<jar::physics::NativePropHull> hull;
 bool valid=false;
};
struct WorkshopPropTraceCache {
 unsigned catalog=0;
 int frame=-1;
 std::shared_ptr<const jar::physics::PropGeometry> geometry;
 WorkshopPropTracePose current,rendered;
};
std::map<unsigned,WorkshopPropTraceCache> propTraceCache;

bool propTraceBroadphase(const gentity_t& entity,const vec3_t origin,const vec3_t angles,
 const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs){
 const bool atCurrentAngles=VectorCompare(angles,entity.currentAngles)!=qfalse;
 float radius=0;
 if(!atCurrentAngles){
  vec3_t extreme;
  for(int axis=0;axis<3;++axis)extreme[axis]=std::max(std::fabs(entity.mins[axis]),std::fabs(entity.maxs[axis]));
  radius=VectorLength(extreme);
 }
 for(int axis=0;axis<3;++axis){
  const float low=origin[axis]+(atCurrentAngles?entity.mins[axis]:-radius)-1;
  const float high=origin[axis]+(atCurrentAngles?entity.maxs[axis]:radius)+1;
  if(std::max(start[axis],end[axis])+maxs[axis]<low||
     std::min(start[axis],end[axis])+mins[axis]>high)return false;
 }
 return true;
}

const jar::physics::NativePropHull* propTraceHull(const gentity_t& entity,const vec3_t angles){
 auto found=propTraceCache.find(entity.s.number);
 if(found==propTraceCache.end()){
  const auto* owner=owned(entity.s.number);
  if(!owner||!prop(owner->catalog))return nullptr;
  found=propTraceCache.try_emplace(entity.s.number).first;
  found->second.catalog=owner->catalog;
 }
 auto& cache=found->second;
 if(cache.frame!=entity.s.frame){
  PhysicsBounds bounds;
  const auto* definition=prop(cache.catalog);
  cache.geometry=definition&&modelBounds(*definition,bounds,entity.s.frame)?std::move(bounds.geometry):nullptr;
  cache.current.valid=cache.rendered.valid=false;
  cache.current.hull.reset();cache.rendered.hull.reset();
  cache.frame=entity.s.frame;
 }
 if(!cache.geometry)return nullptr;
 auto& pose=VectorCompare(angles,entity.currentAngles)?cache.current:cache.rendered;
 if(!pose.valid||!VectorCompare(pose.angles,angles)){
  if(!pose.hull)pose.hull=std::make_unique<jar::physics::NativePropHull>();
  if(!pose.hull->Build(*cache.geometry,pv(angles)))pose.hull.reset();
  VectorCopy(angles,pose.angles);pose.valid=true;
 }
 return pose.hull.get();
}

const JarSweepHull* propMovingHull(const gentity_t& entity){
 const auto* hull=propTraceHull(entity,entity.currentAngles);
 return hull?&hull->Sweep():nullptr;
}

std::shared_ptr<const jar::physics::PropGeometry> propTransformGeometry(const Prop& model,int frame){
 PhysicsBounds bounds;
 return modelBounds(model,bounds,frame)?std::move(bounds.geometry):nullptr;
}

bool propSceneryHull(const Prop& model,int frame,const vec3_t angles,JarSweepHull& shape){
 if(!gi.JAR_TraceSceneryHull)return false;
 const auto geometry=propTransformGeometry(model,frame);
 return geometry&&jar::physics::BuildPropSweep(*geometry,pv(angles),shape);
}
