// Rend2 dynamic-poly portal traversal. GPL-2.0-only.
#include "tr_portal_monitor_feeds.h"

static qboolean R_AddPortablePortalView(const trRefdef_t*refdef){
 bool mirror=false;
 PortableMonitorCandidates<srfPoly_t> candidates;
 for(int i=0;i<refdef->numPolys;++i){
  auto& poly=refdef->polys[i];
  if(!poly.jarPortal)continue;
  if(!poly.jarPortal->mirror){
   poly.jarMonitorShader=0;
   poly.jarSelected=false;
   candidates.Add(poly,*poly.jarPortal);
   continue;
  }
  msurface_t surface{};
  surface.shader=R_GetShaderByHandle(poly.hShader);
  surface.data=reinterpret_cast<surfaceType_t*>(&poly);
  poly.jarSelected=!mirror&&R_MirrorViewBySurface(&surface,REFENTITYNUM_WORLD);
  if(!poly.jarSelected)continue;
  mirror=true;
  R_SetupViewParms(refdef);
 }
 PortableMonitorFeeds feeds;
 candidates.VisitLargestFirst([&](srfPoly_t& poly){
  msurface_t surface{};
  surface.shader=R_GetShaderByHandle(poly.hShader);
  surface.data=reinterpret_cast<surfaceType_t*>(&poly);
  const auto feed=feeds.Select(*poly.jarPortal,[&](int slot){
   return R_MirrorViewBySurface(&surface,REFENTITYNUM_WORLD,slot);
  });
  poly.jarMonitorShader=feed.shader;
  poly.jarSelected=feed.shader!=0;
  // Restore the player's clock and PVS for the next capture or authored mirror.
  if(feed.rendered)R_SetupViewParms(refdef);
 });
 return mirror?qtrue:qfalse;
}
