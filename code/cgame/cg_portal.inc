// Explicit camera panels use the normal renderer portal budget and clipping.
static bool CG_PortablePortal(centity_t*cent){
 if(!cent->gent||!G_PortalSurface(*cent->gent))return false;
 jar::portal::View view;if(G_PortalView(*cent->gent,view)){cgi_R_AddPortalToScene(&view);return true;}
 // Disabled or disconnected panels remain selectable and visibly dark.
 if(!jar::portal::Valid(view))return true;
 polyVert_t verts[4]{};float points[4][3];jar::portal::Corners(view,points);
 for(int i=0;i<4;++i){VectorCopy(points[i],verts[i].xyz);memset(verts[i].modulate,255,4);}
 cgi_R_AddPolyToScene(cgi_R_RegisterShader("gfx/colors/black"),4,verts);return true;
}
