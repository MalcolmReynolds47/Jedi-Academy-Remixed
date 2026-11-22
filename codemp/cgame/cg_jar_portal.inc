// Submit the authoritative portal pose and its separately linked camera view.
static void PortalDraw(const centity_t* cent) {
 const entityState_t* state=&cent->currentState;
 jarPortablePortalView_t view={0};
 view.width=state->speed;
 view.height=state->eventParm;
 if(!JAR_MP_PortalSizeValid(view.width,view.height))return;
 VectorCopy(cent->lerpOrigin,view.origin);
 AnglesToAxis(cent->lerpAngles,view.axis);
 if(state->time==JAR_PORTAL_VIEW_CAMERA||state->time==JAR_PORTAL_VIEW_MIRROR){
  view.mirror=state->time==JAR_PORTAL_VIEW_MIRROR;
  VectorCopy(view.mirror?cent->lerpOrigin:state->origin,view.cameraOrigin);
  AnglesToAxis(view.mirror?cent->lerpAngles:state->angles,view.cameraAxis);
  if(trap->R_AddPortalToScene)trap->R_AddPortalToScene(&view);
  return;
 }
 const qhandle_t shader=trap->R_RegisterShader("gfx/colors/black");
 polyVert_t verts[4]={0};
 const int x[]={-1,-1,1,1}, y[]={-1,1,1,-1};
 for(int i=0;i<4;++i){
  VectorMA(view.origin,x[i]*view.width*.5f,view.axis[1],verts[i].xyz);
  VectorMA(verts[i].xyz,y[i]*view.height*.5f,view.axis[2],verts[i].xyz);
  memset(verts[i].modulate,255,4);
 }
 if(shader)trap->R_AddPolysToScene(shader,4,verts,1);
}
