// Completes the unused native ship-boundary controller. GPL-2.0-only.
#include "g_shipboundary.h"
#include "g_shipboundary_properties.inc"
#include "g_volume_geometry.h"
#include "qcommon/ojk_saved_game_helper.h"
#include <map>
#include <climits>
namespace {
struct ShipTurn {int boundary=0,target=0,pilot=0,end=0;};
std::map<int,ShipTurn> shipTurns;
bool shipEntity(int index){return index>=0&&index<ENTITYNUM_WORLD&&g_entities[index].inuse;}
bool shipVolume(int index){return shipEntity(index)&&G_ShipBoundaryClass(g_entities[index]);}
bool shipEnabled(const gentity_t&e){return e.linked&&(e.contents&CONTENTS_TRIGGER)&&!(e.svFlags&SVF_INACTIVE);}
bool shipContains(const gentity_t&v,const vec3_t p){return jar::volumeEntity::Portable(v)?jar::volume::Contains(v.pos1,v.pos2,v.currentOrigin,v.currentAngles,p):G_PointInBounds(p,v.absmin,v.absmax)!=qfalse;}
bool shipTouched(const gentity_t&v,const gentity_t&ship){
 if(!jar::volumeEntity::Portable(v))return true; // Native brush contact was tested by the trigger enumerator.
 if(shipContains(v,ship.client->ps.origin))return true;
 vec3_t delta;VectorSubtract(ship.client->ps.origin,ship.lastOrigin,delta);const float distance=VectorNormalize(delta);float hit;
 return distance>0&&distance<=1024&&jar::volume::Ray(v.pos1,v.pos2,v.currentOrigin,v.currentAngles,ship.lastOrigin,delta,distance,hit);
}
bool shipFighter(const gentity_t&e){return e.inuse&&e.health>0&&e.client&&!e.client->noclip&&e.m_pVehicle&&e.m_pVehicle->m_pVehicleInfo&&e.m_pVehicle->m_pVehicleInfo->type==VH_FIGHTER;}
bool shipPilot(const gentity_t&e){auto*p=e.m_pVehicle?e.m_pVehicle->m_pPilot:nullptr;return p&&p->inuse&&p->health>0&&p->client&&p->s.m_iVehicleNum==e.s.number;}
bool shipIdentity(int slot,const ShipTurn&t){
 if(!shipEntity(slot)||!shipVolume(t.boundary)||!shipEntity(t.target)||!shipEntity(t.pilot)||t.end<=0)return false;
 const auto&vehicle=g_entities[slot];const auto&volume=g_entities[t.boundary];
 if(!shipFighter(vehicle)||!shipPilot(vehicle)||vehicle.m_pVehicle->m_iRemovedSurfaces||vehicle.m_pVehicle->m_pPilot!=&g_entities[t.pilot]||!shipEnabled(volume))return false;
 if(!volume.target||!g_entities[t.target].targetname||Q_stricmp(volume.target,g_entities[t.target].targetname))return false;
 if(t.target==slot||t.target==t.boundary||slot==t.pilot)return false;
 if(t.end>level.time)for(int index:{slot,t.pilot}){const auto&ps=g_entities[index].client->ps;if(ps.vehTurnaroundIndex!=t.target||ps.vehTurnaroundTime!=t.end)return false;}
 for(float coordinate:g_entities[t.target].currentOrigin)if(!std::isfinite(coordinate)||std::fabs(coordinate)>65535)return false;
 return true;
}
void shipClearFields(int slot,const ShipTurn&t){
 for(int index:{slot,t.pilot})if(shipEntity(index)&&g_entities[index].client){auto&ps=g_entities[index].client->ps;
  if(ps.vehTurnaroundIndex==t.target&&ps.vehTurnaroundTime==t.end)ps.vehTurnaroundIndex=ps.vehTurnaroundTime=0;
 }
}
bool shipActive(int slot){auto it=shipTurns.find(slot);if(it==shipTurns.end())return false;const auto&t=it->second;
 if(!shipIdentity(slot,t)){shipClearFields(slot,t);shipTurns.erase(it);return false;}
 if(t.end<=level.time){shipClearFields(slot,t);return false;}
 const auto&ps=g_entities[slot].client->ps;
 return ps.vehTurnaroundIndex==t.target&&ps.vehTurnaroundTime==t.end;
}
}
void G_ShipBoundaryReset(){shipTurns.clear();}
void G_ShipBoundaryReleaseAll(){for(const auto&pair:shipTurns)shipClearFields(pair.first,pair.second);shipTurns.clear();}
void G_ShipBoundaryForget(gentity_t*entity){
 if(!entity)return;const int n=entity->s.number;
 for(auto i=shipTurns.begin();i!=shipTurns.end();){const auto&t=i->second;
  if(i->first==n||t.boundary==n||t.target==n||t.pilot==n){shipClearFields(i->first,t);i=shipTurns.erase(i);}else ++i;
 }
}
void G_ShipBoundaryFrame(){
 for(auto i=shipTurns.begin();i!=shipTurns.end();){const auto&t=i->second;
  if(!shipIdentity(i->first,t)){shipClearFields(i->first,t);i=shipTurns.erase(i);continue;}
  if(t.end<=level.time){shipClearFields(i->first,t);if(!shipContains(g_entities[t.boundary],g_entities[i->first].client->ps.origin)){i=shipTurns.erase(i);continue;}}
  ++i;
 }
}
void shipboundary_touch(gentity_t*self,gentity_t*other,trace_t*){
 if(!self||!G_ShipBoundaryClass(*self)||!shipEnabled(*self)||!other||other->s.number<=0||other->s.number>=ENTITYNUM_WORLD||!shipFighter(*other)||!shipTouched(*self,*other)||self->count<100||self->count>60000)return;
 if(!self->target||!*self->target)return;auto*target=G_Find(nullptr,FOFS(targetname),self->target);
 if(!target||!target->inuse||target==other||target==self)return;
 for(float coordinate:target->currentOrigin)if(!std::isfinite(coordinate)||std::fabs(coordinate)>65535)return;
 if(!shipPilot(*other)||other->m_pVehicle->m_iRemovedSurfaces){
  if(!(self->spawnflags&1))G_Damage(other,other,other,nullptr,other->client->ps.origin,99999,DAMAGE_NO_PROTECTION,MOD_SUICIDE);
  return;
 }
 // Retain the original lease while inside, even after it expires. Leaving
 // rearms the volume; per-frame contacts cannot indefinitely renew steering.
 const auto previous=shipTurns.find(other->s.number);
 if(previous!=shipTurns.end()){
  if(shipIdentity(previous->first,previous->second))return;
  shipClearFields(previous->first,previous->second);shipTurns.erase(previous);
 }
 ShipTurn turn;turn.boundary=self->s.number;turn.target=target->s.number;turn.pilot=other->m_pVehicle->m_pPilot->s.number;
 turn.end=int(std::min<int64_t>(INT_MAX,int64_t(level.time)+self->count));shipTurns.emplace(other->s.number,turn);
 for(auto*entity:{other,other->m_pVehicle->m_pPilot}){entity->client->ps.vehTurnaroundIndex=turn.target;entity->client->ps.vehTurnaroundTime=turn.end;}
}
void shipboundary_use(gentity_t*self,gentity_t*,gentity_t*){
 if(!self||!G_ShipBoundaryClass(*self)||(self->svFlags&SVF_INACTIVE))return;
 self->contents=(self->contents&CONTENTS_TRIGGER)?(jar::volumeEntity::Portable(*self)?0:CONTENTS_ITEM):CONTENTS_TRIGGER;
 if(!(self->contents&CONTENTS_TRIGGER))G_ShipBoundaryForget(self);gi.linkentity(self);
}
void G_ShipBoundaryGuide(Vehicle_t*vehicle){
 auto*ship=vehicle?vehicle->m_pParentEntity:nullptr;
 if(!ship||!shipActive(ship->s.number))return;
 if(vehicle->m_iBoarding||(ship->client->ps.eFlags&EF_FORCE_GRIPPED)||(vehicle->m_pPilot->client->ps.eFlags&EF_FORCE_GRIPPED))return;
 const auto&turn=shipTurns.find(ship->s.number)->second;vec3_t direction,angles;
 VectorSubtract(g_entities[turn.target].currentOrigin,ship->currentOrigin,direction);if(VectorLengthSquared(direction)<1)return;vectoangles(direction,angles);
 if(!std::isfinite(vehicle->m_fTimeModifier))return;
 const float dt=std::max(0.f,std::min(.2f,vehicle->m_fTimeModifier/60.f));if(dt<=0)return;vec3_t next;
 for(int axis=0;axis<3;++axis){const float from=vehicle->m_vPrevOrientation[axis],to=axis==ROLL?0:angles[axis];if(!std::isfinite(from)||std::fabs(from)>100000)return;next[axis]=std::remainder(from+std::max(-90.f*dt,std::min(90.f*dt,AngleSubtract(to,from))),360.f);}
 VectorCopy(next,vehicle->m_vOrientation);
 vehicle->m_ucmd.forwardmove=127;vehicle->m_ucmd.rightmove=vehicle->m_ucmd.upmove=0;vehicle->m_fStrafeTime=0;
 // The normal vehicle update copies these angles to the parent and computes
 // movement from them. Pilot view locking below also supports unrestrained aim.
 SetClientViewAngle(vehicle->m_pPilot,vehicle->m_vOrientation);
}
void G_ShipBoundaryPilot(gentity_t*pilot,usercmd_t*cmd){
 if(!pilot||!pilot->client||!cmd)return;const int slot=pilot->s.m_iVehicleNum;
 if(!shipEntity(slot)||!shipActive(slot)||g_entities[slot].m_pVehicle->m_pPilot!=pilot)return;
 auto&ship=g_entities[slot];if(ship.m_pVehicle->m_iBoarding||((ship.client->ps.eFlags|pilot->client->ps.eFlags)&EF_FORCE_GRIPPED))return;
 SetClientViewAngle(pilot,ship.m_pVehicle->m_vOrientation);
 for(int axis=0;axis<3;++axis)cmd->angles[axis]=ANGLE2SHORT(pilot->client->ps.viewangles[axis])-pilot->client->ps.delta_angles[axis];
 cmd->forwardmove=127;cmd->rightmove=cmd->upmove=0;
}
/*QUAKED trigger_shipboundary (.5 .5 .5) ? PRESERVE_EMPTY x x x x x x INACTIVE
Turns an inhabited fighter toward target for traveltime milliseconds (100-60000,
default 3000). Use toggles the volume. PRESERVE_EMPTY ignores empty/damaged ships;
without it the original native destruction behavior applies. Exiting rearms it.
*/
void SP_trigger_shipboundary(gentity_t*self){
 InitTrigger(self);if(jar::volumeEntity::Portable(*self))jar::volumeEntity::Initialize(*self);
 G_SpawnInt("traveltime","3000",&self->count);self->count=std::max(100,std::min(60000,self->count));
 self->e_TouchFunc=touchF_shipboundary_touch;self->e_UseFunc=useF_shipboundary_use;gi.linkentity(self);
}
bool G_ShipBoundaryHasVolumes(){for(int i=1;i<globals.num_entities;++i)if(G_ShipBoundaryClass(g_entities[i]))return true;return false;}
void G_ShipBoundaryRestore(){
 for(int i=1;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_ShipBoundaryClass(e))continue;
  if(e.e_TouchFunc==touchF_NULL){e.e_TouchFunc=touchF_shipboundary_touch;e.e_UseFunc=useF_shipboundary_use;e.count=std::max(100,std::min(60000,e.count));}
  if(e.e_TouchFunc!=touchF_shipboundary_touch||e.e_UseFunc!=useF_shipboundary_use||e.count<100||e.count>60000||(jar::volumeEntity::Portable(e)&&!jar::volumeEntity::Refresh(e)))gi.Error(ERR_DROP,"Invalid saved ship boundary");
  gi.linkentity(&e);
 }
}
void G_ShipBoundaryExport(bool autosave,ojk::SavedGameHelper&h){
 G_ShipBoundaryFrame();h.write<uint32_t>(autosave?0:uint32_t(shipTurns.size()));
 if(!autosave)for(const auto&pair:shipTurns){h.write<int32_t>(pair.first);const auto&t=pair.second;for(int value:{t.boundary,t.target,t.pilot,t.end})h.write<int32_t>(value);}
}
bool G_ShipBoundaryImport(bool autosave,ojk::SavedGameHelper&h){
 uint32_t count=0;h.read<uint32_t>(count);if(count>=ENTITYNUM_WORLD||(autosave&&count))return false;
 std::map<int,ShipTurn> candidate;
 for(unsigned i=0;i<count;++i){int slot;ShipTurn t;h.read<int32_t>(slot);h.read<int32_t>(t.boundary);h.read<int32_t>(t.target);h.read<int32_t>(t.pilot);h.read<int32_t>(t.end);
  if(candidate.count(slot)||!shipIdentity(slot,t)||int64_t(t.end)>int64_t(level.time)+60000)return false;
  if(t.end>level.time){for(int index:{slot,t.pilot}){const auto&ps=g_entities[index].client->ps;if(ps.vehTurnaroundIndex!=t.target||ps.vehTurnaroundTime!=t.end)return false;}}
  candidate.emplace(slot,t);
 }
 shipTurns=std::move(candidate);
 if(autosave)for(int i=0;i<globals.num_entities;++i)if(g_entities[i].inuse&&g_entities[i].client)g_entities[i].client->ps.vehTurnaroundIndex=g_entities[i].client->ps.vehTurnaroundTime=0;
 return true;
}
