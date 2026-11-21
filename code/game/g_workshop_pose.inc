// Owned decorative models only: native actors and map controllers retain their
// animation state machines. Pose frames use the existing saved entityState.
int propPoseFrameCount(const gentity_t&e){
 const auto*o=owned(e.s.number);if(!o||!matches(*o))return 0;
 const auto*p=prop(o->catalog);return p?propPoseFrames(*p):0;
}
bool setPropPose(gentity_t&e,int frame){
 const int count=propPoseFrameCount(e);if(frame<0||frame>=count)return false;
 const auto*p=prop(owned(e.s.number)->catalog);vec3_t lo,hi;
 if(!propHull(*p,e.currentAngles,lo,hi,frame))return false;
 std::vector<SandboxChargeMove> charges;
 if(!propTransformClear(e,*p,frame,e.currentOrigin,e.currentAngles,charges,false))return false;
 if(!Q_stricmp(COM_GetExtension(p->model),"glm")&&!skeletonPose(e.ghoul2,frame))return false;
 if(held==unsigned(e.s.number))releaseHeld();
 jar::physics::Pose live;
 if(physicsWorld.Read(e.s.number,live)){
  nativeVector(live.linear,e.s.pos.trDelta);
  nativeVector(live.angular,e.s.apos.trDelta);
 }
 e.s.frame=frame;VectorCopy(lo,e.mins);VectorCopy(hi,e.maxs);
 // Rebuild the body's shape on the next physics frame. Saved entity-local
 // joint anchors and motion survive; the old solver joints are recreated.
 physicsWorld.Forget(e.s.number);e.contents=physicsFrozenContents(e);gi.linkentity(&e);
 G_SandboxChargesRefresh(charges);return true;
}
void restorePropPoses(){
 for(unsigned i=0;i<state.ownedCount;++i){const auto&o=state.owned[i];const auto*p=prop(o.catalog);if(!p)continue;
  auto&e=g_entities[o.entity];vec3_t lo,hi;
  if(!matches(o)||!propHull(*p,e.currentAngles,lo,hi,e.s.frame)||
     (!Q_stricmp(COM_GetExtension(p->model),"glm")&&!skeletonPose(e.ghoul2,e.s.frame))){gi.Error(ERR_DROP,"Invalid saved decorative model pose");return;}
  VectorCopy(lo,e.mins);VectorCopy(hi,e.maxs);e.contents=physicsFrozenContents(e);gi.linkentity(&e);
 }
}
