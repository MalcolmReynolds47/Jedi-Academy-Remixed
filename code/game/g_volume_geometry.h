// Include after g_local.h. Portable native volumes own pos1/pos2 as local bounds;
// those existing native fields are serialized without changing entity layouts.
// GPL-2.0-only.
#ifndef G_VOLUME_GEOMETRY_H
#define G_VOLUME_GEOMETRY_H
#include "qcommon/jar_volume.h"
#include "qcommon/jar_contact_trigger.h"
#include "qcommon/jar_push.h"
#include "qcommon/jar_teleport.h"
#include "qcommon/jar_weather.h"
namespace jar { namespace volumeEntity {
inline bool LocationClass(const char* name){return name&&!strcmp(name,"trigger_location");}
inline bool Class(const char*name){return name&&(contactTrigger::Class(name)||push::Class(name)==push::Volume||
 teleport::Class(name)==teleport::Volume||LocationClass(name)||!strcmp(name,"trigger_space")||
 !strcmp(name,"trigger_bidirectional")||!strcmp(name,"trigger_shipboundary")||weather::ZoneClass(name));}
inline bool Portable(const gentity_t&e){
 return e.inuse&&!e.bmodel&&Class(e.classname)&&e.model&&
  !strcmp(e.model,contactTrigger::Class(e.classname)?contactTrigger::model:
   push::Class(e.classname)==push::Volume?push::volumeModel:
   teleport::Class(e.classname)==teleport::Volume?teleport::volumeModel:"@jar_box");
}
inline bool Dimensions(const gentity_t&e,float*out){
 if(!Portable(e)||!volume::Valid(e.pos1,e.pos2))return false;
 VectorSubtract(e.pos2,e.pos1,out);return true;
}
inline bool Refresh(gentity_t&e){
 if(!Portable(e)||!volume::Valid(e.pos1,e.pos2))return false;
 for(int a=0;a<3;++a)if(!std::isfinite(e.currentAngles[a]))return false;
 volume::Hull(e.pos1,e.pos2,e.currentAngles,e.mins,e.maxs);return true;
}
inline void InitializeLocation(gentity_t& e){
 // Match the marker bounds used by older saves that had no local volume data.
 VectorSet(e.pos1, -21, -21, 0);
 VectorSet(e.pos2, 21, 21, 22);
}
inline void Initialize(gentity_t&e){const bool ship=e.classname&&!strcmp(e.classname,"trigger_shipboundary");const bool weatherZone=weather::ZoneClass(e.classname);const float radius=ship?512.f:weatherZone?256.f:64.f,height=ship?512.f:weatherZone?256.f:128.f;VectorSet(e.pos1,-radius,-radius,0);VectorSet(e.pos2,radius,radius,height);Refresh(e);}
inline bool ResizedBounds(const gentity_t&e,const float*size,float*lo,float*hi){
 if(!Portable(e)||!volume::Valid(e.pos1,e.pos2))return false;
 for(int a=0;a<3;++a)if(!std::isfinite(size[a])||size[a]<volume::minimum||size[a]>volume::maximum)return false;
 for(int a=0;a<3;++a){lo[a]=a==2?e.pos1[a]:(e.pos1[a]+e.pos2[a]-size[a])*.5f;hi[a]=lo[a]+size[a];}
 return volume::Valid(lo,hi);
}
inline bool Resize(gentity_t&e,const float*size){
 vec3_t lo,hi;if(!ResizedBounds(e,size,lo,hi))return false;
 VectorCopy(lo,e.pos1);VectorCopy(hi,e.pos2);Refresh(e);return true;
}
inline void Corners(const gentity_t&e,float out[8][3],float padding=0){
 const bool oriented=Portable(e)&&volume::Valid(e.pos1,e.pos2);
 const float*lo=oriented?e.pos1:e.mins,*hi=oriented?e.pos2:e.maxs;vec3_t axis[3];AnglesToAxis(oriented?e.currentAngles:vec3_origin,axis);
 for(int mask=0;mask<8;++mask){VectorCopy(e.currentOrigin,out[mask]);for(int a=0;a<3;++a)VectorMA(out[mask],mask&(1<<a)?hi[a]+padding:lo[a]-padding,axis[a],out[mask]);}
}
} }
#endif
