// Optional rigid props. Included inside the workshop namespace. GPL-2.0-only.
jar::physics::World physicsWorld;
jar::physics::Pose physicsPose(const gentity_t&e){jar::physics::Pose p;p.position=pv(e.currentOrigin);p.angles=pv(e.currentAngles);p.linear=pv(e.s.pos.trDelta);p.angular=pv(e.s.apos.trDelta);return p;}
bool physicsRequested(){return gi.cvar("jar_box3d","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART)->integer==1;}
void physicsInvalidate(unsigned entity){physicsWorld.Forget(entity);if(entity<ENTITYNUM_WORLD && owned(entity)){
 VectorClear(g_entities[entity].s.pos.trDelta);VectorClear(g_entities[entity].s.apos.trDelta);}}
// An overlapping actor can leave the prop before it becomes solid again.
// Empty corners of the search bounds do not require that grace period.
int physicsFrozenContents(const gentity_t& prop){
 gentity_t*nearby[256];vec3_t low,high;for(int a=0;a<3;++a){low[a]=prop.currentOrigin[a]+prop.mins[a];high[a]=prop.currentOrigin[a]+prop.maxs[a];}
 const int count=gi.EntitiesInBox(low,high,nearby,256);
 for(int i=0;i<count;++i){const auto&actor=*nearby[i];
  if(!actor.inuse||!actor.linked||!actor.client||!(actor.contents&CONTENTS_BODY))continue;
  bool overlaps=true;for(int a=0;a<3;++a)if(actor.absmax[a]<=prop.currentOrigin[a]+prop.mins[a] || actor.absmin[a]>=prop.currentOrigin[a]+prop.maxs[a])overlaps=false;
  if(!overlaps)continue;
  if(const auto* hull=propTraceHull(prop,prop.currentAngles)){
   vec3_t relative;
   VectorSubtract(actor.currentOrigin,prop.currentOrigin,relative);
   if(!hull->Trace(pv(relative),pv(relative),pv(actor.mins),pv(actor.maxs)).startSolid)continue;
  }
  return CONTENTS_CORPSE;
 }
 return CONTENTS_BODY;
}
void physicsStop(bool freeze){
 jointFirst=0;
 if(freeze){releaseHeld(true);held=noEntity;if(activeTool)toolRelease=true;}
 if(freeze)for(unsigned i=0;i<state.ownedCount;++i){auto&o=state.owned[i];if(!prop(o.catalog)||!matches(o)||!physicsEligible(*prop(o.catalog),g_entities[o.entity].s.frame))continue;
  auto&e=g_entities[o.entity];o.frozen=1;VectorClear(e.s.pos.trDelta);VectorClear(e.s.apos.trDelta);e.contents=physicsFrozenContents(e);gi.linkentity(&e);
 }
 const bool was=physicsWorld.Active();physicsWorld.Stop();
 if(was&&gi.JAR_Log)gi.JAR_Log(1,"physics","stop","frozen=%d bodies=0 proxies=0",freeze);
}
qboolean physicsPolygon(void*context,int count,const float*points,const float*normal){
 auto&triangles=*static_cast<std::vector<jar::physics::Vec>*>(context);
 if(count<3 || triangles.size()+unsigned(count-2)*3>jar::physics::triangleLimit*3)return qfalse;
 for(int i=1;i<count-1;++i){const auto*a=points,*b=points+i*3,*c=points+(i+1)*3;vec3_t ab,ac,cross;
  VectorSubtract(b,a,ab);VectorSubtract(c,a,ac);CrossProduct(ab,ac,cross);
  if(VectorLengthSquared(cross)<.0001f)continue;
  triangles.push_back(pv(a));triangles.push_back(pv(DotProduct(cross,normal)>0?b:c));triangles.push_back(pv(DotProduct(cross,normal)>0?c:b));
 }return qtrue;
}
bool physicsEnsure(bool on){
 if(!on||!physicsRequested()){if(physicsWorld.Active())physicsStop(on);return false;}
 if(physicsWorld.Active())return true;
 std::vector<jar::physics::Vec> triangles;const int began=gi.Milliseconds();
 const int polygons=gi.JAR_CollisionPolygons?gi.JAR_CollisionPolygons(0,CONTENTS_SOLID,&triangles,physicsPolygon):-1;
 const unsigned cores=std::thread::hardware_concurrency();
 const unsigned workerBudget=cores>1?cores-1:1;
 if(polygons<=0 || !physicsWorld.Start(triangles,workerBudget)){
  physicsStop(true);gi.cvar_set("jar_box3d","0");message("Box3D unavailable for this collision map. Props frozen; terrain and oversized geometry require native mode.");return false;
 }
 if(gi.JAR_Log)gi.JAR_Log(1,"physics","start","polygons=%d triangles=%u setup_ms=%d",polygons,unsigned(triangles.size()/3),gi.Milliseconds()-began);
 return true;
}
void physicsFailure(const char*reason){physicsStop(true);gi.cvar_set("jar_box3d","0");message(reason);}
bool physicsBrushProxy(unsigned entity,const gentity_t&e){
 if(e.s.modelindex<=0)return false;
 if(!physicsWorld.HasProxyMesh(e.s.modelindex)){
  std::vector<jar::physics::Vec> triangles;
  if(!gi.JAR_CollisionPolygons || gi.JAR_CollisionPolygons(e.s.modelindex,CONTENTS_SOLID|CONTENTS_BODY,&triangles,physicsPolygon)<=0 ||
     !physicsWorld.AddProxyMesh(e.s.modelindex,triangles))return false;
 }
 jar::physics::Pose p;p.position=pv(e.currentOrigin);p.angles=pv(e.currentAngles);
 return physicsWorld.MeshProxy(entity,entityGeneration[entity]+1,e.s.modelindex,p);
}
void physicsActors(gentity_t&,const vec3_t,const vec3_t,const jar::physics::Pose&,float,bool frozen);
void physicsImpactSounds();
bool attachmentSyncBoxJoints();
void pushPropContacts(gentity_t&,const vec3_t,bool);
bool teleportPropContacts(gentity_t&,const vec3_t,bool,bool,const float* previousAngles=nullptr);
struct MountActorArea {vec3_t low,high;};

void addMountActorArea(const gentity_t& prop,const std::vector<jar::physics::MountedHull>& mounts,
 std::vector<MountActorArea>& areas){
 if(mounts.empty())return;
 float reach=0;
 for(const auto& mount:mounts){
  const vec3_t center={mount.center.x,mount.center.y,mount.center.z};
  const vec3_t half={mount.half.x,mount.half.y,mount.half.z};
  reach=std::max(reach,VectorLength(center)+VectorLength(half));
 }
 // Reserve the full 50 ms frame travel permitted by the rigid bridge.
 reach+=256;
 MountActorArea area;
 for(int axis=0;axis<3;++axis){
  area.low[axis]=prop.currentOrigin[axis]-reach;
  area.high[axis]=prop.currentOrigin[axis]+reach;
 }
 areas.push_back(area);
}

bool inMountActorArea(const gentity_t& actor,const std::vector<MountActorArea>& areas){
 return std::any_of(areas.begin(),areas.end(),[&](const MountActorArea& area){
  for(int axis=0;axis<3;++axis)if(actor.absmax[axis]<area.low[axis]||actor.absmin[axis]>area.high[axis])return false;
  return true;
 });
}

void physicsFrame(float dt){
 if(!physicsWorld.Active())return;
 bool any=false;vec3_t areaMin={65536,65536,65536},areaMax={-65536,-65536,-65536};
 std::vector<jar::physics::MountedHull> mounts;
 std::vector<MountActorArea> mountAreas;
 std::bitset<MAX_GENTITIES> rigidProps;
 for(unsigned i=0;i<state.ownedCount;++i){const auto&o=state.owned[i];const auto*p=prop(o.catalog);if(!p||!p->rigid||!matches(o))continue;auto&e=g_entities[o.entity];PhysicsBounds bounds;
  if(!physicalBounds(*p,bounds,e.s.frame))continue;
  const bool frozen=o.frozen!=0&&held!=o.entity;
  G_SandboxChargesHulls(e,mounts);
  if(!physicsWorld.Prop(o.entity,o.id,bounds.half,bounds.center,physicsPose(e),frozen,&mounts,bounds.geometry)){
   physicsFailure("Box3D body could not be qualified within its limits. Props frozen; native mode restored.");return;}
  if(!physicsWorld.Configure(o.entity,float(o.physics.mass),o.physics.noGravity||held==o.entity?0.f:1.f)){
   physicsFailure("Box3D prop settings could not be applied. Props frozen; native mode restored.");return;}
  rigidProps.set(o.entity);
  if(!frozen)addMountActorArea(e,mounts,mountAreas);
  e.contents=physicsFrozenContents(e);
  e.mass=o.physics.mass?float(o.physics.mass):std::max(1.f,physicsWorld.Mass(o.entity));
  jar::physics::Vec low,high;if(!physicsWorld.Bounds(o.entity,low,high)){physicsFailure("Box3D search bounds unavailable. Props frozen.");return;}
  vec3_t mins,maxs;nativeVector(low,mins);nativeVector(high,maxs);
  for(int a=0;a<3;++a){areaMin[a]=std::min(areaMin[a],e.currentOrigin[a]+mins[a]-256);areaMax[a]=std::max(areaMax[a],e.currentOrigin[a]+maxs[a]+256);}any=true;
 }
 if(!attachmentSyncBoxJoints()){
  physicsFailure("Box3D constraint could not be restored. Props frozen; saved links retained for inspection or Unlink.");return;
 }
 // Establish links before dragging so a hinged member keeps angular freedom.
 if(held!=noEntity&&physgunGrip.active)
  physicsWorld.DragAt(held,physgunAimPoint(holdDistance),physgunGrip.localPoint,physgunGrip.Facing(physgunViewYaw));
 physicsWorld.BeginProxies();unsigned actors=0,vehicles=0,movers=0;
 std::vector<unsigned> mountActors;
 if(any)for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];
  if(!e.inuse||!e.linked||!(e.contents&(CONTENTS_SOLID|CONTENTS_BODY))||i>=ENTITYNUM_WORLD)continue;
  // Mounted riders are represented by their vehicle's native collision hull.
  if(e.client && e.client->NPC_class!=CLASS_VEHICLE && (e.s.m_iVehicleNum || (e.owner&&e.owner->inuse&&e.owner->m_pVehicle)))continue;
  if(e.client&&!G_JAR_WorkshopControls(&e)){
   if(e.health>0&&!e.m_pVehicle&&e.absmax[0]-e.absmin[0]>=.5f&&e.absmax[1]-e.absmin[1]>=.5f&&
      e.absmax[2]-e.absmin[2]>=.5f&&inMountActorArea(e,mountAreas)){
    mountActors.push_back(i);
    physicsWorld.KeepProxy(i,entityGeneration[i]+1,0,true);
   }
   continue;
  }
  if(rigidProps.test(i))continue;
  bool near=true;for(int a=0;a<3;++a)if(e.absmax[a]<areaMin[a]||e.absmin[a]>areaMax[a])near=false;if(!near)continue;
  if(e.bmodel){
   if(!physicsBrushProxy(i,e)){physicsFailure("Box3D brush geometry unavailable or over budget. Props frozen; native mode restored.");return;}
   ++movers;continue;
  }
  jar::physics::Pose p;vec3_t center,half;for(int a=0;a<3;++a){half[a]=(e.absmax[a]-e.absmin[a])*.5f;center[a]=(e.absmin[a]+e.absmax[a])*.5f;}
  // Native broad-phase AABBs include one unit of padding; preserve that boundary.
  p.position=pv(center);
  if(half[0]<.25f||half[1]<.25f||half[2]<.25f)continue;
  if(!physicsWorld.Proxy(i,entityGeneration[i]+1,pv(half),{},p)){
   physicsFailure("Box3D native collision-proxy budget exceeded. Props frozen; no native actor controller changed.");return;}
  if(e.m_pVehicle)++vehicles;else if(e.client)++actors;else ++movers;
 }
 physicsWorld.EndProxies();
 for(const auto slot:mountActors){
  const auto& actor=g_entities[slot];
  const auto lo=pv(actor.absmin),hi=pv(actor.absmax);
  const jar::physics::Vec center={(hi.x+lo.x)*.5f,(hi.y+lo.y)*.5f,(hi.z+lo.z)*.5f};
  const jar::physics::Vec half={(hi.x-lo.x)*.5f,(hi.y-lo.y)*.5f,(hi.z-lo.z)*.5f};
  jar::physics::Pose pose;pose.position=center;
  if(!physicsWorld.Proxy(slot,entityGeneration[slot]+1,half,{},pose,true)){
   physicsFailure("Box3D mounted-charge actor budget exceeded. Props frozen; native mode restored.");return;}
  ++actors;
 }
 physicsWorld.Step(dt,g_gravity?g_gravity->value:800);
 if(gi.JAR_Log)for(const auto&contact:physicsWorld.Contacts()){const auto&e=g_entities[contact.native];gi.JAR_Log(1,"physics","native_contact","prop=%u native=%u kind=%s classname=%s",contact.prop,contact.native,e.m_pVehicle?"vehicle":e.client?"actor":"mover",e.classname?e.classname:"");}
 for(unsigned i=0;i<state.ownedCount;){const auto o=state.owned[i];if(!rigidProps.test(o.entity)||!matches(o)){++i;continue;}auto&e=g_entities[o.entity];jar::physics::Pose p;jar::physics::Vec lo,hi;
  const bool poseValid=physicsWorld.Read(o.entity,p);
  const bool boundsValid=physicsWorld.Bounds(o.entity,lo,hi);
  vec3_t pos,mins,maxs;nativeVector(p.position,pos);
  if(boundsValid){nativeVector(lo,mins);nativeVector(hi,maxs);}
  else{VectorCopy(e.mins,mins);VectorCopy(e.maxs,maxs);}
  if(removeOutsideMap(o,pos,mins,maxs))continue;
  if(!poseValid||!boundsValid){
   physicsFailure("Box3D produced an invalid prop state. Props frozen; native mode restored.");return;}
  vec3_t oldOrigin,oldAngles;VectorCopy(e.currentOrigin,oldOrigin);VectorCopy(e.currentAngles,oldAngles);
  vec3_t angles;nativeVector(p.angles,angles);G_SetOrigin(&e,pos);G_SetAngles(&e,angles);
  e.s.pos.trType=e.s.apos.trType=TR_INTERPOLATE;nativeVector(p.linear,e.s.pos.trDelta);nativeVector(p.angular,e.s.apos.trDelta);
  VectorCopy(mins,e.mins);VectorCopy(maxs,e.maxs);
  if(!teleportPropContacts(e,oldOrigin,true,false,oldAngles)){
   physicsActors(e,oldOrigin,oldAngles,p,dt,o.frozen!=0);e.contents=physicsFrozenContents(e);gi.linkentity(&e);
   pushPropContacts(e,oldOrigin,true);
  }
  ++i;
 }
 physicsImpactSounds();
 if(gi.JAR_Log && gi.JAR_LogLevel && gi.JAR_LogLevel()>=2){
  const auto stats=physicsWorld.Statistics();
  gi.JAR_Log(2,"physics","frame","level_ms=%d props=%u proxies=%u awake=%u actors=%u vehicles=%u movers=%u steps=%u step_ms=%.5f max_step_ms=%.5f workers=%u",level.time,stats.props,stats.proxies,stats.awake,actors,vehicles,movers,stats.steps,stats.stepMs,stats.maxStepMs,stats.workers);
 }
}
void physicsReport(){const auto s=physicsWorld.Statistics();
 if(gi.JAR_Log){gi.JAR_Log(0,"physics","status","requested=%d active=%d props=%u proxies=%u awake=%u triangles=%u joints=%u saved_joints=%u steps=%u step_ms=%.5f max_step_ms=%.5f workers=%u",physicsRequested(),physicsWorld.Active(),s.props,s.proxies,s.awake,s.triangles,s.joints,state.jointCount,s.steps,s.stepMs,s.maxStepMs,s.workers);
  for(unsigned i=0;i<state.ownedCount;++i){const auto&o=state.owned[i];jar::physics::Pose p;if(physicsWorld.Read(o.entity,p))gi.JAR_Log(0,"physics","prop","entity=%u id=%u catalog=%u frozen=%u xyz=%.3f,%.3f,%.3f angles=%.3f,%.3f,%.3f velocity=%.3f,%.3f,%.3f angular=%.3f,%.3f,%.3f awake=%d",o.entity,o.id,o.catalog,o.frozen,p.position.x,p.position.y,p.position.z,p.angles.x,p.angles.y,p.angles.z,p.linear.x,p.linear.y,p.linear.z,p.angular.x,p.angular.y,p.angular.z,p.awake);}
 }
}

void physicsSavedMotion(const char*event){
 if(!physicsRequested()||!gi.JAR_Log)return;
 for(unsigned i=0;i<state.ownedCount;++i){const auto&o=state.owned[i];if(!prop(o.catalog)||!matches(o)||!physicsEligible(*prop(o.catalog),g_entities[o.entity].s.frame))continue;
  const auto p=physicsPose(g_entities[o.entity]);gi.JAR_Log(1,"physics",event,"entity=%u id=%u catalog=%u frozen=%u xyz=%.6f,%.6f,%.6f angles=%.6f,%.6f,%.6f velocity=%.6f,%.6f,%.6f angular=%.6f,%.6f,%.6f",o.entity,o.id,o.catalog,o.frozen,p.position.x,p.position.y,p.position.z,p.angles.x,p.angles.y,p.angles.z,p.linear.x,p.linear.y,p.linear.z,p.angular.x,p.angular.y,p.angular.z);
 }
}
