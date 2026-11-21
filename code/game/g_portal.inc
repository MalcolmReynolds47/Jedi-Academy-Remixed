// Portable camera panels keep authored portals/configstrings untouched.
#include "g_portal.h"
#include <set>
extern gentity_t*G_JAR_SpawnSetupChild(gentity_t*);
namespace {std::set<int> portablePortals;}
bool G_PortalMoved(gentity_t&e){
 if(!G_PortalPortable(e))return false;
 VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);
 if(G_PortalSurface(e)){
  jar::portal::Settings s;if(!G_PortalRead(e,s))return false;
  vec3_t axis[3];AnglesToAxis(e.currentAngles,axis);
  for(int a=0;a<3;++a){e.maxs[a]=2*std::fabs(axis[0][a])+s.width*.5f*std::fabs(axis[1][a])+s.height*.5f*std::fabs(axis[2][a]);e.mins[a]=-e.maxs[a];}
 }
 gi.linkentity(&e);return true;
}
namespace {
void portalInitialize(gentity_t&e){
 e.owner=nullptr;e.painDebounceTime=0;e.contents=CONTENTS_ITEM;e.svFlags|=SVF_PLAYER_USABLE|SVF_BROADCAST;e.e_UseFunc=useF_portable_portal_use;e.e_ThinkFunc=thinkF_NULL;e.nextthink=0;
 // Broadcast entities bypass the server's portal PVS recursion. A surface must
 // follow the normal visibility path so actors at its remote camera are sent.
 if(G_PortalSurface(e)){e.s.eType=ET_PORTAL;e.svFlags=(e.svFlags|SVF_PORTAL)&~SVF_BROADCAST;e.s.modelindex=0;portablePortals.insert(e.s.number);VectorCopy(e.currentOrigin,e.s.origin2);}
 else {e.s.eType=ET_GENERAL;e.s.modelindex=G_ModelIndex(JAR_PORTAL_CAMERA_MODEL);VectorSet(e.mins,-12,-12,-12);VectorSet(e.maxs,12,12,12);}
 G_PortalSet(e,jar::portal::Settings{});
}
}
bool G_PortalSpawnMember(gentity_t& e){
 if(!G_PortalPortable(e))return false;
 portalInitialize(e);
 return true;
}
bool G_PortalSpawnPair(gentity_t*e){
 if(!e||!G_PortalPortable(*e))return false;
 auto*h=G_JAR_SpawnSetupChild(e);if(!h)return false;
 const bool surface=G_PortalSurface(*e);
 h->classname=G_NewString(surface?"misc_portal_camera":"misc_portal_surface");h->model=G_NewString(surface?"@jar_portal_camera":"@jar_portal_surface");
 if(!e->targetname)e->targetname=G_NewString(va("jar_portal_%d_%d",e->s.number,level.time));
 h->targetname=G_NewString(va("jar_portal_%d_%d_camera",h->s.number,level.time));
 vec3_t forward,pos,angles;AngleVectors(e->currentAngles,forward,nullptr,nullptr);
 // Place the companion in front of the root, on the same side as the player
 // when placing a screen on a wall. When placing the camera itself, keep the
 // screen behind the camera and facing the player. Stop before host scenery.
 if(!surface)VectorScale(forward,-1,forward);
 VectorMA(e->currentOrigin,128,forward,pos);
 trace_t clearance{};
 gi.trace(&clearance,e->currentOrigin,nullptr,nullptr,pos,e->s.number,MASK_SOLID,G2_NOCOLLIDE,0);
 if(clearance.startsolid||clearance.allsolid||clearance.fraction<.25f)return false;
 VectorMA(e->currentOrigin,128*clearance.fraction-(clearance.fraction<1?16:0),forward,pos);
 VectorCopy(e->currentAngles,angles);
 if(!surface)vectoangles(forward,angles);
 G_SetOrigin(h,pos);G_SetAngles(h,angles);portalInitialize(*e);portalInitialize(*h);
 auto*s=surface?e:h;auto*c=surface?h:e;s->target=G_NewString(c->targetname);s->owner=c;VectorCopy(c->currentOrigin,s->s.origin2);return true;
}
bool G_PortalView(gentity_t&e,jar::portal::View&view){
 jar::portal::Settings settings;if(!G_PortalSurface(e)||!G_PortalRead(e,settings))return false;
 VectorCopy(e.currentOrigin,view.origin);AnglesToAxis(e.currentAngles,view.axis);view.width=settings.width;view.height=settings.height;view.mirror=settings.mirror?1:0;
 VectorCopy(e.currentOrigin,view.cameraOrigin);AnglesToAxis(e.currentAngles,view.cameraAxis);
 if(!settings.enabled)return false;
 if(!settings.mirror){if(!G_PortalLinkMatches(e,e.owner,e.target)||(e.owner->spawnflags&128))return false;
  const auto&camera=*e.owner;VectorCopy(camera.currentOrigin,view.cameraOrigin);vec3_t angles;VectorCopy(camera.currentAngles,angles);
  const auto*target=camera.owner;
  if(!camera.count&&G_PortalLinkMatches(camera,target,camera.target)){
   vec3_t direction;VectorSubtract(target->currentOrigin,camera.currentOrigin,direction);if(VectorLengthSquared(direction)>.001f){vectoangles(direction,angles);angles[ROLL]=camera.currentAngles[ROLL];}
  }
  AnglesToAxis(angles,view.cameraAxis);
 }
 return jar::portal::Valid(view);
}
void G_PortalFrame(){
 for(auto it=portablePortals.begin();it!=portablePortals.end();){auto&e=g_entities[*it];if(!G_PortalSurface(e)){it=portablePortals.erase(it);continue;}++it;
  if(auto* camera=G_PortalResolveLink(e))G_PortalResolveLink(*camera);
  jar::portal::View view;if(G_PortalView(e,view))VectorCopy(view.cameraOrigin,e.s.origin2);else VectorCopy(e.currentOrigin,e.s.origin2);
 }
}
void portable_portal_use(gentity_t*e,gentity_t*,gentity_t*){
 jar::portal::Settings s;if(!e||!G_PortalRead(*e,s))return;s.enabled=!s.enabled;G_PortalSet(*e,s);G_PortalFrame();
}
void G_PortalForget(gentity_t*e){
 if(!e)return;portablePortals.erase(e->s.number);
 for(int n:portablePortals){auto&s=g_entities[n];if(s.owner==e){s.owner=nullptr;s.painDebounceTime=level.time+1000;VectorCopy(s.currentOrigin,s.s.origin2);}
  else if(s.owner&&G_PortalCamera(*s.owner)&&s.owner->owner==e){s.owner->owner=nullptr;s.owner->painDebounceTime=level.time+1000;}}
}
void G_PortalReset(){portablePortals.clear();}
void G_PortalRestore(){
 G_PortalReset();for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_PortalPortable(e))continue;
  jar::portal::Settings s;if(!G_PortalRead(e,s)||e.e_UseFunc!=useF_portable_portal_use||e.s.eType!=(G_PortalSurface(e)?ET_PORTAL:ET_GENERAL)||(G_PortalSurface(e)&&!(e.svFlags&SVF_PORTAL)))gi.Error(ERR_DROP,"Invalid saved portable portal settings");
  e.painDebounceTime=0;if(G_PortalSurface(e))portablePortals.insert(i);G_PortalMoved(e);
 }G_PortalFrame();
}
