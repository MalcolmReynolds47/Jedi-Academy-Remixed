// Stock cloud geometry/shaders with a portable, removable control marker.
// GPL-2.0-only.
#include "g_cloud.h"
void cloudlayer_use(gentity_t*self,gentity_t*,gentity_t*){
 if(self&&G_CloudClass(*self)&&!(self->svFlags&SVF_INACTIVE))self->spawnflags^=jar::cloud::offFlag;
}
/*QUAKED fx_cloudlayer (1 0.3 0.5) (-8 -8 -8) (8 8 8) TUBE ALT STARTOFF
Stock scrolling cloud disk, or TUBE with a hollow center. Use toggles it.
Radius is the outer radius, random the inner radius, wait the curvature.
*/
void SP_fx_cloudlayer(gentity_t*ent){
 G_SpawnFloat("radius","2048",&ent->radius);G_SpawnFloat("random","128",&ent->random);G_SpawnFloat("wait","0",&ent->wait);
 ent->s.eType=ET_CLOUD;ent->e_UseFunc=useF_cloudlayer_use;
 G_SetOrigin(ent,ent->s.origin);G_SetAngles(ent,ent->s.angles);ent->contents=0;
 VectorSet(ent->maxs,200,200,200);VectorScale(ent->maxs,-1,ent->mins);
 if(G_CloudPortable(*ent)){
  ent->s.modelindex=G_ModelIndex("models/map_objects/imperial/crate1.md3");
  VectorSet(ent->mins,-21,-21,0);VectorSet(ent->maxs,21,21,22);ent->contents=CONTENTS_ITEM;
  ent->svFlags|=SVF_BROADCAST|SVF_PLAYER_USABLE;
 }
 gi.linkentity(ent);
}
bool G_CloudHasLayers(){for(int i=1;i<globals.num_entities;++i)if(G_CloudClass(g_entities[i]))return true;return false;}
void G_CloudRestore(){
 for(int i=1;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_CloudClass(e))continue;
  if(e.e_UseFunc==useF_NULL)e.e_UseFunc=useF_cloudlayer_use;
  jar::cloud::Settings s;if(G_CloudPortable(e)&&(!G_CloudRead(e,s)||e.e_UseFunc!=useF_cloudlayer_use||e.s.eType!=ET_CLOUD))gi.Error(ERR_DROP,"Invalid saved cloud controller");
  if(G_CloudPortable(e))e.svFlags|=SVF_BROADCAST|SVF_PLAYER_USABLE;
  gi.linkentity(&e);
 }
}
