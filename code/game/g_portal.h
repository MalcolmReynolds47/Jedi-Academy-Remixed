// Portable surface/camera lifetime and native saved fields. GPL-2.0-only.
#ifndef G_JAR_PORTAL_H
#define G_JAR_PORTAL_H
#include "qcommon/jar_portal.h"
inline bool G_PortalSurface(const gentity_t&e){return e.inuse&&jar::portal::SurfaceClass(e.classname)&&e.model&&!strcmp(e.model,"@jar_portal_surface");}
inline bool G_PortalCamera(const gentity_t&e){return e.inuse&&jar::portal::CameraClass(e.classname)&&e.model&&!strcmp(e.model,"@jar_portal_camera");}
inline bool G_PortalPortable(const gentity_t&e){return G_PortalSurface(e)||G_PortalCamera(e);}
inline bool G_PortalLinkMatches(const gentity_t& source,const gentity_t* target,const char* name){
 return target&&target!=&source&&target->inuse&&!target->freeAfterEvent&&
  target->targetname&&name&&*name&&!strcmp(target->targetname,name)&&
  (!G_PortalSurface(source)||G_PortalCamera(*target));
}
inline gentity_t* G_PortalLinkTarget(const gentity_t& source,const char* name){
 if(!name||!*name)return nullptr;
 if(G_PortalLinkMatches(source,source.owner,name))return source.owner;
 auto* target=G_Find(nullptr,FOFS(targetname),name);
 return G_PortalLinkMatches(source,target,name)?target:nullptr;
}
inline void G_PortalLinkChanged(gentity_t& source,gentity_t* target){
 source.owner=target;
 source.painDebounceTime=0;
}
inline gentity_t* G_PortalResolveLink(gentity_t& source){
 if(G_PortalLinkMatches(source,source.owner,source.target))return source.owner;
 source.owner=nullptr;
 if(!source.target||!*source.target||source.painDebounceTime>level.time)return nullptr;
 source.painDebounceTime=level.time+1000;
 source.owner=G_PortalLinkTarget(source,source.target);
 return source.owner;
}
inline bool G_PortalRead(const gentity_t&e,jar::portal::Settings&s){if(!G_PortalPortable(e))return false;s.width=e.radius;s.height=e.random;s.mirror=e.count==1;s.enabled=(e.spawnflags&128)==0;return e.count>=0&&e.count<=1&&jar::portal::Valid(s);}
bool G_PortalMoved(gentity_t&);
inline bool G_PortalSet(gentity_t&e,const jar::portal::Settings&s){if(!G_PortalPortable(e)||!jar::portal::Valid(s))return false;e.radius=s.width;e.random=s.height;e.count=s.mirror?1:0;e.spawnflags=s.enabled?e.spawnflags&~128:e.spawnflags|128;G_PortalMoved(e);return true;}
bool G_PortalSpawnPair(gentity_t*);
bool G_PortalSpawnMember(gentity_t&);
bool G_PortalView(gentity_t&,jar::portal::View&);
void G_PortalFrame();
void G_PortalForget(gentity_t*);
void G_PortalReset();
void G_PortalRestore();
#endif
