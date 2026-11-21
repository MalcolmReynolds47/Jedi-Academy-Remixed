#include "tr_portal_monitor_feeds.h"

static const jar::portal::View* R_PortablePanel(const surfaceType_t* surface){
 return surface&&*surface==SF_POLY?reinterpret_cast<const srfPoly_t*>(surface)->jarPortal:nullptr;
}

// Fixed cameras render offscreen and do not claim the mirror view.
static void R_AddPortableMonitors(drawSurf_t* drawSurfs, int numDrawSurfs){
 PortableMonitorCandidates<drawSurf_t> candidates;
 for(int i=0;i<numDrawSurfs;++i){
  auto& draw=drawSurfs[i];
  const auto* panel=R_PortablePanel(draw.surface);
  if(!panel||panel->mirror)continue;
  auto& poly=*const_cast<srfPoly_t*>(reinterpret_cast<const srfPoly_t*>(draw.surface));
  poly.jarMonitorShader=0;
  candidates.Add(draw,*panel);
 }
 PortableMonitorFeeds feeds;
 candidates.VisitLargestFirst([&](drawSurf_t& draw){
  auto& poly=*const_cast<srfPoly_t*>(reinterpret_cast<const srfPoly_t*>(draw.surface));
  poly.jarMonitorShader=feeds.Select(*poly.jarPortal,[&](int slot){
   return R_MirrorViewBySurface(&draw,REFENTITYNUM_WORLD,slot);
  }).shader;
 });
}

static const surfaceType_t* R_SelectPortal(drawSurf_t* drawSurfs, int numDrawSurfs){
 for(int i=0;i<numDrawSurfs;++i){
  auto& draw=drawSurfs[i];
  const auto* panel=R_PortablePanel(draw.surface);
  if(panel&&panel->mirror&&R_MirrorViewBySurface(&draw,REFENTITYNUM_WORLD))return draw.surface;
 }
 for(int i=0;i<numDrawSurfs;++i){
  auto& draw=drawSurfs[i];
  shader_t* shader;
  int entityNum,fogNum,dlighted;
  R_DecomposeSort(draw.sort,&entityNum,&shader,&fogNum,&dlighted);
  if(shader->sort>SS_PORTAL)break;
  if(shader->sort==SS_BAD)Com_Error(ERR_DROP,"Shader '%s' with sort == SS_BAD",shader->name);
  if(!R_PortablePanel(draw.surface)&&R_MirrorViewBySurface(&draw,entityNum))return draw.surface;
 }
 return nullptr;
}

// Classic portal fallback keeps sort fields and unrelated surfaces intact.
static void R_PortablePortalShaders(drawSurf_t*drawSurfs,int numDrawSurfs,const surfaceType_t*chosenPortal,bool mainView){
 bool resort=false;
 for(int i=0;i<numDrawSurfs;++i){auto&draw=drawSurfs[i];if(!draw.surface||*draw.surface!=SF_POLY||draw.surface==chosenPortal)continue;
  const auto*poly=reinterpret_cast<const srfPoly_t*>(draw.surface);if(!poly->jarPortal)continue;
  const auto*opaque=R_GetShaderByHandle(mainView&&poly->jarMonitorShader?poly->jarMonitorShader:poly->jarFallback);
  const uint64_t mask=((uint64_t(1)<<SHADERNUM_BITS)-1)<<QSORT_SHADERNUM_SHIFT;
  draw.sort=(draw.sort&~mask)|(uint64_t(opaque->sortedIndex)<<QSORT_SHADERNUM_SHIFT);resort=true;
 }
 if(resort)R_RadixSort(drawSurfs,numDrawSurfs);

}
