// Decorative model poses and bounds, shared by placement and physics.
// Cache qualified geometry by catalog and frame, not by model alone. GPL-2.0-only.
struct PhysicsBounds {
 jar::physics::Vec half,center;
 std::shared_ptr<const jar::physics::PropGeometry> geometry;
};
std::map<std::pair<unsigned,int>,PhysicsBounds> physicsBounds;
std::map<unsigned,int> modelFrameCounts;
jar::physics::Vec pv(const float*v){return {v[0],v[1],v[2]};}
void nativeVector(jar::physics::Vec v,float*out){out[0]=v.x;out[1]=v.y;out[2]=v.z;}
// Loaded GLA headers are host-endian. Validate offsets before inspecting roots;
// every root must be overridden so disconnected skeleton trees share the pose.
bool skeletonPoseInfo(CGhoul2Info_v&g,int&frames,std::vector<std::string>&roots){
 frames=0;roots.clear();if(g.size()!=1||!gi.G2API_GetGLAName||!gi.G2API_GetGLAName(&g[0]))return false;
 const auto*h=g[0].aHeader;
 if(!h||h->ident!=MDXA_IDENT||h->numFrames<1||h->numFrames>65536||h->numBones<1||h->numBones>512||h->ofsEnd<int(sizeof(*h))+h->numBones*4)return false;
 const auto*base=reinterpret_cast<const char*>(h)+sizeof(*h);const auto*offsets=reinterpret_cast<const int*>(base);
 for(int i=0;i<h->numBones;++i){const int offset=offsets[i];
  if(offset<h->numBones*4||offset>h->ofsEnd-int(sizeof(*h))-int(offsetof(mdxaSkel_t,children)))return false;
  const auto*bone=reinterpret_cast<const mdxaSkel_t*>(base+offset);
  if(bone->parent< -1||bone->parent>=h->numBones||!memchr(bone->name,0,sizeof(bone->name))||!bone->name[0])return false;
  if(bone->parent==-1)roots.emplace_back(bone->name);
 }
 if(roots.empty())return false;frames=h->numFrames;return true;
}
bool skeletonPose(CGhoul2Info_v&g,int frame){
 int frames;std::vector<std::string> roots;if(!skeletonPoseInfo(g,frames,roots)||frame<0||frame>=frames||!gi.G2API_SetBoneAnim)return false;
 const auto previous=g[0].mBlist;
 for(const auto&root:roots)if(!gi.G2API_SetBoneAnim(&g[0],root.c_str(),frame,frame+1,BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_NO_LERP,0,0,-1,0)){
  g[0].mBlist=previous;g[0].mSkelFrameNum=0;return false;
 }
 // setFrame must be -1: the native setter divides by animSpeed otherwise.
 return true;
}
bool md3PoseFrame(const void*bytes,int length,int frame,const md3Frame_t*&out,int&frames){
 out=nullptr;frames=0;if(!bytes||length<int(sizeof(md3Header_t))||frame<0)return false;
 const auto*h=static_cast<const md3Header_t*>(bytes);const int offset=LittleLong(h->ofsFrames),count=LittleLong(h->numFrames),end=LittleLong(h->ofsEnd);
 if(LittleLong(h->ident)!=MD3_IDENT||LittleLong(h->version)!=MD3_VERSION||count<1||count>65536||frame>=count||end>length||offset<int(sizeof(*h))||offset>end||count>(end-offset)/int(sizeof(md3Frame_t)))return false;
 frames=count;out=reinterpret_cast<const md3Frame_t*>(static_cast<const char*>(bytes)+offset)+frame;return true;
}
bool md3PoseGeometry(const md3Header_t& header,int frame,int frames,jar::physics::PropGeometry& geometry){
 const bool valid=jar::md3::VisitSurfaces(header,frame,frames,[&](const auto* vertices,const auto* faces,int triangles){
  std::array<bool,MD3_MAX_VERTS> seen{};
  for(int t=0;t<triangles;++t)for(const int rawIndex:faces[t].indexes){
   const int index=LittleLong(rawIndex);
   if(seen[index])continue;
   seen[index]=true;
   vec3_t point;
   jar::md3::Vertex(vertices[index],point);
   geometry.Add(pv(point));
  }
 });
 return valid&&!geometry.Empty();
}
void sampleModelVertex(void* context,const float* point){
 static_cast<jar::physics::PropGeometry*>(context)->Add(pv(point));
}
bool modelBounds(const Prop&p,PhysicsBounds&out,int frame=0){
 if(frame<0||frame>65535)return false;const auto key=std::make_pair(p.id,frame);
 const auto cached=physicsBounds.find(key);if(cached!=physicsBounds.end()){out=cached->second;return out.half.x>=.25f;}
 // Keep default failures cached, plus room for every live prop's selected pose.
 if(physicsBounds.size()>=8192){auto old=std::find_if(physicsBounds.begin(),physicsBounds.end(),[](const decltype(physicsBounds)::value_type&v){return v.first.second!=0;});if(old!=physicsBounds.end())physicsBounds.erase(old);}
 // Failures are map-local too: old saves can contain empty helper models.
 // Never reinitialize their skeleton or reread a changed asset every frame.
 physicsBounds.emplace(key,PhysicsBounds{});
 if(!propValid(p))return false;vec3_t lo{},hi{};bool ok=false;
 auto geometry=std::make_shared<jar::physics::PropGeometry>();
 if(!Q_stricmp(COM_GetExtension(p.model),"glm")){
  CGhoul2Info_v probe;
  if((gi.G2API_JAR_ModelGeometry||gi.G2API_JAR_ModelBounds) &&
     gi.G2API_InitGhoul2Model(probe,p.model,0,NULL_HANDLE,NULL_HANDLE,0,0)>=0){
   int frames;std::vector<std::string> roots;
   if(skeletonPoseInfo(probe,frames,roots)){
    modelFrameCounts[p.id]=frames;
    if(skeletonPose(probe,frame)){
     ok=gi.G2API_JAR_ModelGeometry?
      gi.G2API_JAR_ModelGeometry(probe,0,lo,hi,geometry.get(),sampleModelVertex)!=qfalse:
      gi.G2API_JAR_ModelBounds(probe,0,lo,hi)!=qfalse;
    }
   }
  }
  gi.G2API_CleanGhoul2Models(probe);
 }else{
  void*bytes=nullptr;const int length=gi.FS_ReadFile(p.model,&bytes);
  const md3Frame_t*f=nullptr;int frames;
   if(md3PoseFrame(bytes,length,frame,f,frames)){modelFrameCounts[p.id]=frames;ok=true;
    for(int a=0;a<3;++a){lo[a]=LittleFloat(f->bounds[0][a]);hi[a]=LittleFloat(f->bounds[1][a]);}
    if(md3PoseGeometry(*static_cast<const md3Header_t*>(bytes),frame,frames,*geometry)){
     const jar::physics::Vec axes[]={{1,0,0},{0,1,0},{0,0,1}};
     jar::physics::Vec low,high;
     geometry->Bounds(axes,low,high);
     nativeVector(low,lo);nativeVector(high,hi);
    }else geometry.reset();
   }
  if(bytes)gi.FS_FreeFile(bytes);
 }
 if(!ok)return false;float half[3],center[3];
 for(int a=0;a<3;++a){if(!std::isfinite(lo[a])||!std::isfinite(hi[a])||lo[a]>hi[a]||std::fabs(lo[a])>8192||std::fabs(hi[a])>8192)return false;
  half[a]=std::max(.25f,(hi[a]-lo[a])*.5f);center[a]=(hi[a]+lo[a])*.5f;}
 out.half=pv(half);out.center=pv(center);
 out.geometry=geometry&&!geometry->Empty()?std::move(geometry):nullptr;
 physicsBounds[key]=out;return true;
}
int propPoseFrames(const Prop&p){if(!modelFrameCounts.count(p.id)){PhysicsBounds bounds;modelBounds(p,bounds);modelFrameCounts.emplace(p.id,0);}return modelFrameCounts[p.id];}
bool physicalBounds(const Prop&p,PhysicsBounds&out,int frame=0){
 if(!modelBounds(p,out,frame))return false;vec3_t half,center;nativeVector(out.half,half);nativeVector(out.center,center);
 for(int a=0;a<3;++a)if(half[a]>512||std::fabs(center[a])>512)return false;return true;
}
bool physicsEligible(const Prop&p,int frame=0){PhysicsBounds bounds;return p.rigid&&physicalBounds(p,bounds,frame);}
bool propHull(const Prop&p,const vec3_t angles,vec3_t mins,vec3_t maxs,int frame=0){
 PhysicsBounds bounds;if(!modelBounds(p,bounds,frame))return false;
 vec3_t axis[3];AnglesToAxis(angles,axis);
 if(bounds.geometry){
  const jar::physics::Vec axes[]={{axis[0][0],axis[1][0],axis[2][0]},
   {axis[0][1],axis[1][1],axis[2][1]},{axis[0][2],axis[1][2],axis[2][2]}};
  jar::physics::Vec low,high;bounds.geometry->Bounds(axes,low,high);
  nativeVector(low,mins);nativeVector(high,maxs);return true;
 }
 vec3_t half,center;nativeVector(bounds.half,half);nativeVector(bounds.center,center);
 for(int a=0;a<3;++a){float middle=0,radius=0;for(int k=0;k<3;++k){middle+=axis[k][a]*center[k];radius+=std::fabs(axis[k][a])*half[k];}mins[a]=middle-radius;maxs[a]=middle+radius;}
 return true;
}
