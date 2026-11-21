// Sky views use ordinary current entities without changing their portal flags.
// The marker itself is excluded, and invisible clusters do not submit entities.
#include "qcommon/jar_skyportal.h"
static void CG_AddPortableSkyEntities(jar::skyPortal::View sky){
 if(!sky.entities||!cg.snap)return;
 const int delta=cg.nextSnap?cg.nextSnap->serverTime-cg.snap->serverTime:0;
 cg.frameInterpolation=delta?float(cg.time-cg.snap->serverTime)/delta:0;
 VectorSet(cg.autoAngles,0,(cg.time&2047)*360/2048.0f,0);
 VectorSet(cg.autoAnglesFast,0,(cg.time&1023)*360/1024.0f,0);
 AnglesToAxis(cg.autoAngles,cg.autoAxis);AnglesToAxis(cg.autoAnglesFast,cg.autoAxisFast);
 auto add=[&](centity_t*cent){
  if(cent->currentState.number==sky.entity||!cent->gent)return;
  CG_CalcEntityLerpPositions(cent);
  if(cgi_R_inPVS(sky.origin,cent->lerpOrigin))CG_AddCEntity(cent);
 };
 for(int i=0;i<cg.snap->numEntities;++i)add(&cg_entities[cg.snap->entities[i].number]);
 for(int i=0;i<cg_numpermanents;++i)if(cg_permanents[i]->currentValid)add(cg_permanents[i]);
}
