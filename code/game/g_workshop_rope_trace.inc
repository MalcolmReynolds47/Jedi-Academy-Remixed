// Native collision for rope endpoint and route checks. GPL-2.0-only.
// Unlink both endpoints once for the whole bounded query, then restore exactly
// their linked state. This matches native joint projection's peer exclusions.
struct RopeTraceScope {
 gentity_t* a;gentity_t* b;bool linkA,linkB;
 RopeTraceScope(gentity_t&first,gentity_t&second):a(&first),b(&second),
  linkA(first.s.number<ENTITYNUM_WORLD&&first.linked),linkB(second.s.number<ENTITYNUM_WORLD&&second.linked){
  if(linkA)gi.unlinkentity(a);if(linkB)gi.unlinkentity(b);
 }
 ~RopeTraceScope(){if(linkA)gi.linkentity(a);if(linkB)gi.linkentity(b);}
};
float ropeWorldEndClearance(vec3_t point,const vec3_t axis){
 const float offset=JAR_ROPE_PATH_CLEARANCE*(std::fabs(axis[0])+std::fabs(axis[1])+std::fabs(axis[2]))+.25f;
 VectorMA(point,offset,axis,point);
 return offset;
}
void ropeTrace(const float*a,const float*b,float radius,jarRopePathHit_t*out,void*data){
 trace_t tr{};const vec3_t lo={-radius,-radius,-radius},hi={radius,radius,radius};
 gi.trace(&tr,a,lo,hi,b,ENTITYNUM_NONE,JAR_ROPE_PATH_CONTENTS,(EG2_Collision)0,0);
 out->fraction=tr.fraction;out->solid=tr.startsolid||tr.allsolid;
 VectorCopy(tr.endpos,out->point);VectorCopy(tr.plane.normal,out->normal);
 if((out->solid||tr.fraction<1)&&tr.entityNum>=0&&tr.entityNum<ENTITYNUM_WORLD){
  const auto& e=g_entities[tr.entityNum];
  if(data&&(e.contents&CONTENTS_BODY)&&!(e.contents&MASK_SOLID))*static_cast<bool*>(data)=true;
  if(e.inuse&&e.linked&&!e.client&&!e.bmodel&&(e.contents&CONTENTS_BODY)){
   out->prop=tr.entityNum+1;out->generation=entityGeneration[tr.entityNum];
  }
 }
}
