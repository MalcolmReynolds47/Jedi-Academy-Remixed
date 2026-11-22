// Scene storage owns the geometry and camera pose for the submitted panel.
template<class ValidateView>
static void R_SubmitPortablePortal(const jar::portal::View*view,ValidateView valid){
 if(!tr.registered||!view||!valid(*view))return;
 const bool capacity=backEndData->numJarPortals<jar::portal::frameLimit;
 const qhandle_t fallback=RE_RegisterShader("gfx/colors/black");
 qhandle_t shader=fallback;
 if(capacity){
  if(view->mirror)shader=RE_RegisterShader(JAR_PORTAL_PANEL_SHADER);
  else {
   // Register every monitor shader before draw surfaces are sorted.
   for(const char* material:jar::portal::monitorMaterials)RE_RegisterShader(material);
   shader=RE_RegisterShader(JAR_PORTAL_CAMERA_SHADER);
  }
 }
 polyVert_t verts[4]{};float points[4][3];jar::portal::Corners(*view,points);
 // FBO textures have their origin at the bottom left.
 for(int i=0;i<4;++i){VectorCopy(points[i],verts[i].xyz);verts[i].st[0]=i>=2?1:0;verts[i].st[1]=(i==1||i==2)?1:0;memset(verts[i].modulate,255,4);}
 const int before=r_numpolys;RE_AddPolyToScene(shader,4,verts,1);
 if(r_numpolys!=before+1||!capacity)return;
 auto&poly=backEndData->polys[before];auto&stored=backEndData->jarPortals[backEndData->numJarPortals++];stored=*view;poly.jarPortal=&stored;poly.jarFallback=fallback;
}
