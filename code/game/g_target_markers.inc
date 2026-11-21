// Native marker lifetime and lookup. GPL-2.0-only.
#include "g_target_markers.h"
#include "g_reference_tags.h"
#include "g_combat_points.h"
#include "g_waypoints.h"
#include "g_volume_geometry.h"
#include <map>
#include <string>
#include <cmath>
namespace {
std::map<int,uint64_t> portableInterests;
uint64_t markerGeneration=0;
bool markerClass(const gentity_t&e,const char*name){return e.inuse&&e.classname&&!strcmp(e.classname,name);}
void linkLocations(const gentity_t*skip=nullptr){
 level.locationHead=nullptr;level.locationLinked=qtrue;
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(&e!=skip&&markerClass(e,"target_location")){e.nextTrain=level.locationHead;level.locationHead=&e;}}
}
}
bool G_PortableInterest(const gentity_t&e){return markerClass(e,"target_interest")&&e.model&&!strcmp(e.model,"@jar_interest");}
void G_RegisterInterest(gentity_t*e){if(e&&G_PortableInterest(*e)&&e->s.number>=0&&e->s.number<ENTITYNUM_WORLD)portableInterests[e->s.number]=++markerGeneration;}
void G_TargetMarkersReset(){portableInterests.clear();}
void G_TargetMarkersRestore(){
 G_TargetMarkersReset();
 for (int i = 0; i < globals.num_entities; ++i) {
  auto& entity = g_entities[i];
  G_RegisterInterest(&entity);
  if (!jar::volumeEntity::LocationClass(entity.classname) || !jar::volumeEntity::Portable(entity)) continue;
  if (VectorCompare(entity.pos1, vec3_origin) && VectorCompare(entity.pos2, vec3_origin))
   jar::volumeEntity::InitializeLocation(entity);
  if (jar::volumeEntity::Refresh(entity)) gi.linkentity(&entity);
 }
 linkLocations();
}
void G_TargetMarkersForget(gentity_t*e){
 if(!e)return;
 if(portableInterests.erase(e->s.number))for(int i=0;i<globals.num_entities;++i){auto&actor=g_entities[i];if(actor.inuse&&actor.client&&actor.client->renderInfo.lookMode==LM_ENT&&actor.client->renderInfo.lookTarget==e->s.number){actor.client->renderInfo.lookTarget=ENTITYNUM_NONE;actor.client->renderInfo.lookTargetClearTime=0;}}
 if(markerClass(*e,"target_location")){linkLocations(e);e->nextTrain=nullptr;}
}
bool G_TargetMarkersPresent(){
 if(!portableInterests.empty())return true;
 for(int i=0;i<globals.num_entities;++i)if(markerClass(g_entities[i],"target_location"))return true;return false;
}
int G_ChoosePortableInterest(gentity_t*self,const vec3_t eyes,float&bestDistance){
 int chosen=ENTITYNUM_NONE;
 for(const auto&entry:portableInterests){auto&e=g_entities[entry.first];if(!G_PortableInterest(e)||(e.svFlags&SVF_INACTIVE))continue;
  vec3_t delta;VectorSubtract(e.currentOrigin,eyes,delta);const float horizontal=(std::fabs(delta[0])+std::fabs(delta[1]))*.5f,distance=VectorLengthSquared(delta);
  if((horizontal<48&&std::fabs(delta[2])>horizontal)||!std::isfinite(distance)||distance>=256*256||distance>=bestDistance)continue;
  if(gi.inPVS(e.currentOrigin,eyes)&&G_ClearLineOfSight(eyes,e.currentOrigin,self->s.number,MASK_OPAQUE)){bestDistance=distance;chosen=entry.first;}
 }return chosen;
}
bool G_FirePortableInterest(gentity_t*self,int index){
 const auto entry=portableInterests.find(index);if(entry==portableInterests.end())return false;const auto generation=entry->second;auto&e=g_entities[index];
 if(!G_PortableInterest(e)||(e.svFlags&SVF_INACTIVE))return false;
 if(self->client&&(self->client->renderInfo.lookMode!=LM_ENT||self->client->renderInfo.lookTarget!=index)&&e.target&&e.target[0]){
  // A target can delete this marker. Its command string must outlive that callback.
  const std::string target=e.target;G_UseTargets2(self,self,target.c_str());
 }
 const auto current=portableInterests.find(index);return self->inuse&&self->client&&current!=portableInterests.end()&&current->second==generation&&G_PortableInterest(e)&&!(e.svFlags&SVF_INACTIVE);
}
void marker_use(gentity_t*self,gentity_t*,gentity_t*){
 if(self&&G_WaypointPortable(*self)){jar::waypoint::Settings s;if(G_WaypointRead(*self,s)){s.enabled=!s.enabled;G_WaypointSet(*self,s);}return;}
 if(self&&G_CombatPointPortable(*self)){jar::combatPoint::Settings s;if(G_CombatPointRead(*self,s)){s.enabled=!s.enabled;G_CombatPointSet(*self,s);}return;}
 if(!self||(!G_PortableInterest(*self)&&!markerClass(*self,"target_location")&&!G_ReferencePortable(*self)))return;
 self->svFlags^=SVF_INACTIVE;
 if(self->svFlags&SVF_INACTIVE)for(int i=0;i<globals.num_entities;++i){auto&actor=g_entities[i];if(actor.inuse&&actor.client&&actor.client->renderInfo.lookMode==LM_ENT&&actor.client->renderInfo.lookTarget==self->s.number){actor.client->renderInfo.lookTarget=ENTITYNUM_NONE;actor.client->renderInfo.lookTargetClearTime=0;}}
}
void target_location_linkup(gentity_t*){linkLocations();}
void SP_target_location(gentity_t*self){
 self->e_ThinkFunc=thinkF_NULL;self->nextthink=0;self->e_UseFunc=useF_marker_use;
 G_SetOrigin(self,self->s.origin);linkLocations();
}
char*G_NearestLocation(gentity_t*self){
 if(!self||!self->inuse)return nullptr;
 gentity_t*nearest=nullptr,*visible=nullptr;float nearestDistance=Q3_INFINITE,visibleDistance=Q3_INFINITE;
 // Bound traversal and validate pointers before dereference, including loaded lists.
 auto*point=level.locationHead;
 for(int visited=0;point&&visited<globals.num_entities;++visited){
  const auto address=reinterpret_cast<uintptr_t>(point),base=reinterpret_cast<uintptr_t>(g_entities);
  if(address<base||address>=base+sizeof(g_entities)||((address-base)%sizeof(gentity_t)))break;
  auto*next=point->nextTrain;
  if(markerClass(*point,"target_location")&&!(point->svFlags&SVF_INACTIVE)&&point->message&&point->message[0]){
   const float distance=DistanceSquared(self->currentOrigin,point->currentOrigin);
   if(std::isfinite(distance)){
    if(distance<nearestDistance){nearest=point;nearestDistance=distance;}
    if(distance<visibleDistance&&gi.inPVS(self->currentOrigin,point->currentOrigin)&&G_ClearLineOfSight(self->currentOrigin,point->currentOrigin,self->s.number,MASK_OPAQUE)){visible=point;visibleDistance=distance;}
   }
  }point=next;
 }return visible?visible->message:nearest?nearest->message:nullptr;
}
