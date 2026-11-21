// Frame-owned Ghoul2 submissions survive every view and glow/shadow pass.
// GPL-2.0-only. Exhaustion drops new surfaces; existing pointers never wrap.
#define JAR_MP_RENDER_SURFACES 65536
static CRenderableSurface jarMPDrawSurfaces[JAR_MP_RENDER_SURFACES];
static unsigned jarMPNextSurface;
CRenderableSurface* AllocRS(void){
 if(jarMPNextSurface==JAR_MP_RENDER_SURFACES)return nullptr;
 CRenderableSurface*surface=&jarMPDrawSurfaces[jarMPNextSurface++];
#ifdef _G2_GORE
 surface->Init();
#else
 surface->boneCache=nullptr;surface->surfaceData=nullptr;
#endif
 return surface;
}
void R_ResetGhoul2Surfaces(void){jarMPNextSurface=0;}
