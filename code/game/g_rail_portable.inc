// Saved, removable rail traffic. No pointers are retained between frames.
// Movers use the native pusher path; authored rail registries stay untouched.
#include "g_rail_portable.h"
#include "g_workshop.h"
#include "g_mover_motion.h"
#include "qcommon/jar_volume.h"
#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include <string>
namespace {
struct RailFrameLookup {
 std::map<std::string,gentity_t*> names;
 std::set<int> riders;
};
const RailFrameLookup*railLookup=nullptr;
void railStop(gentity_t&e,int state){G_SetOrigin(&e,e.currentOrigin);VectorClear(e.s.pos.trDelta);e.count=state;gi.linkentity(&e);}
bool railFinite(const vec3_t v){for(int a=0;a<3;++a)if(!std::isfinite(v[a])||std::fabs(v[a])>131072)return false;return true;}
gentity_t*railNamed(const char*name){
 if(!name||!*name)return nullptr;
 if(railLookup){auto it=railLookup->names.find(name);return it==railLookup->names.end()?nullptr:it->second;}
 auto*e=G_Find(nullptr,FOFS(targetname),name);return e&&G_RailPortable(*e)?e:nullptr;
}
bool railRoute(const gentity_t&e,gentity_t*&track,gentity_t*&lane){
 track=lane=nullptr;const auto kind=jar::rail::Class(e.classname);
 if(kind==jar::rail::Track)track=const_cast<gentity_t*>(&e);
 else {auto*target=railNamed(e.target);if(!target||!jar::rail::Link(kind,jar::rail::Class(target->classname)))return false;
  if(jar::rail::Class(target->classname)==jar::rail::Lane){lane=target;track=railNamed(lane->target);}else track=target;
  if(kind==jar::rail::Lane)lane=const_cast<gentity_t*>(&e);
 }
 return track&&jar::rail::Class(track->classname)==jar::rail::Track;
}
bool railWidth(const gentity_t&track,const gentity_t*lane,float&lo,float&hi){
 jar::rail::Settings s;if(!G_RailRead(track,s))return false;lo=-s.width*.5f;hi=s.width*.5f;
 if(lane){jar::rail::Settings l;if(!G_RailRead(*lane,l))return false;vec3_t axis[3],delta;AnglesToAxis(track.currentAngles,axis);VectorSubtract(lane->currentOrigin,track.currentOrigin,delta);const float center=DotProduct(delta,axis[1]);lo=std::max(lo,center-l.width*.5f);hi=std::min(hi,center+l.width*.5f);}
 return hi-lo>=1;
}
void railPoint(const gentity_t&track,float along,float across,vec3_t out){vec3_t axis[3];AnglesToAxis(track.currentAngles,axis);VectorMA(track.currentOrigin,along,axis[0],out);VectorMA(out,across,axis[1],out);VectorMA(out,32,axis[2],out);}
bool railRider(const gentity_t&e){if(railLookup)return railLookup->riders.count(e.s.number)!=0;for(int i=0;i<globals.num_entities;++i){const auto&r=g_entities[i];if(r.inuse&&r.s.number!=e.s.number&&(r.client?r.client->ps.groundEntityNum:r.s.groundEntityNum)==e.s.number)return true;}return false;}
void railTravel(gentity_t&e,const vec3_t end,float speed){
 vec3_t delta;VectorSubtract(end,e.currentOrigin,delta);const float distance=VectorNormalize(delta);
 VectorCopy(end,e.pos1);VectorCopy(e.currentOrigin,e.s.pos.trBase);VectorScale(delta,speed,e.s.pos.trDelta);
 e.s.pos.trType=TR_LINEAR_STOP;e.s.pos.trTime=level.time;e.s.pos.trDuration=std::max(1,int(std::ceil(distance*1000/speed)));e.count=1;gi.linkentity(&e);
}
bool railLaunch(gentity_t&e,gentity_t&track,gentity_t*lane){
 jar::rail::Settings s,m;float lo,hi;if(!G_RailRead(track,s)||!G_RailRead(e,m)||!railWidth(track,lane,lo,hi)||railRider(e))return false;
 vec3_t axis[3];AnglesToAxis(track.currentAngles,axis);float half=0,center=0;for(int a=0;a<3;++a){half+=std::fabs(axis[1][a])*(e.maxs[a]-e.mins[a])*.5f;center+=axis[1][a]*(e.maxs[a]+e.mins[a])*.5f;}
 lo+=half+1-center;hi-=half+1+center;if(hi<lo)return false;
 const int columns=m.center?1:std::min(64,1+int((hi-lo)/s.cell));const int first=Q_irand(0,columns-1);
 for(int i=0;i<columns;++i){const float across=m.center?(lo+hi)*.5f:lo+((first+i)%columns)*s.cell;vec3_t start,end,offset;
  railPoint(track,0,across,start);railPoint(track,s.length,across,end);G_JAR_MoverPathOffset(e,offset);VectorAdd(start,offset,start);VectorAdd(end,offset,end);
  if(!railFinite(start)||!railFinite(end))return false;
  trace_t tr{};gi.trace(&tr,start,e.mins,e.maxs,start,e.s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);if(tr.startsolid||tr.allsolid)continue;
  G_SetOrigin(&e,start);e.s.eFlags^=EF_TELEPORT_BIT;VectorCopy(start,e.pos2);railTravel(e,end,s.speed);return true;
 }return false;
}
}
bool G_RailCanMove(const gentity_t&e,const vec3_t destination){
 if(!G_RailPortable(e)||jar::rail::Class(e.classname)!=jar::rail::Mover)return true;
 if(!railFinite(destination))return false;
 // Native pushers move actors, but authored movers intentionally ignore world walls.
 // Portable rails need a hull sweep before the native push/rollback transaction.
 trace_t hit{};gi.trace(&hit,e.currentOrigin,e.mins,e.maxs,destination,e.s.number,MASK_SOLID,(EG2_Collision)0,0);
 return !hit.startsolid&&!hit.allsolid&&hit.fraction==1;
}
bool G_RailMoved(gentity_t&e){
 if(!G_RailPortable(e))return false;const vec3_t lo={-21,-21,0},hi={21,21,22};jar::volume::Hull(lo,hi,e.currentAngles,e.mins,e.maxs);
 VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);return true;
}
bool G_RailSpawn(gentity_t&e){
 if(!G_RailPortable(e))return false;jar::rail::Settings s;s.kind=jar::rail::Class(e.classname);if(s.kind==jar::rail::Lane)s.width=128;
 if(!G_RailSet(e,s))return false;e.s.modelindex=G_ModelIndex("models/map_objects/imperial/crate1.md3");e.takedamage=qfalse;e.bmodel=qfalse;
 e.s.eType=s.kind==jar::rail::Mover?ET_MOVER:ET_GENERAL;e.contents=s.kind==jar::rail::Mover?CONTENTS_SOLID:CONTENTS_ITEM;e.clipmask=MASK_SOLID|CONTENTS_BODY;
 e.e_UseFunc=useF_portable_rail_use;e.e_ReachedFunc=reachedF_NULL;e.e_BlockedFunc=blockedF_NULL;e.e_ThinkFunc=thinkF_NULL;e.nextthink=0;
 e.svFlags|=SVF_PLAYER_USABLE|SVF_USE_CURRENT_ORIGIN;e.delay=level.time;e.count=0;e.fly_sound_debounce_time=level.time;
 if(s.kind==jar::rail::Mover)e.mass=150;G_RailMoved(e);return true;
}
void portable_rail_use(gentity_t*e,gentity_t*,gentity_t*){
 jar::rail::Settings s;if(!e||!G_RailRead(*e,s))return;s.enabled=!s.enabled;G_RailSet(*e,s);e->delay=level.time+int(s.delay*1000);
}
void G_RailPortableFrame(){
 // Build one list for scheduling; tracks and lanes remain normal saved entities.
 bool present=false;for(int i=0;i<globals.num_entities;++i)if(G_RailPortable(g_entities[i])&&jar::rail::Class(g_entities[i].classname)==jar::rail::Mover){present=true;break;}if(!present)return;
 RailFrameLookup lookup;
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!e.inuse)continue;
  if(e.targetname&&*e.targetname)lookup.names.emplace(e.targetname,G_RailPortable(e)?&e:nullptr);
  const int ground=e.client?e.client->ps.groundEntityNum:e.s.groundEntityNum;if(ground!=i&&ground>=0&&ground<ENTITYNUM_WORLD)lookup.riders.insert(ground);
 }
 struct LookupScope{LookupScope(const RailFrameLookup&l){railLookup=&l;}~LookupScope(){railLookup=nullptr;}} scope(lookup);
 std::map<int,std::vector<gentity_t*>> groups;
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_RailPortable(e)||jar::rail::Class(e.classname)!=jar::rail::Mover||G_JAR_WorkshopControls(&e))continue;
  gentity_t*track,*lane;jar::rail::Settings m,t,l;
  const bool route=railRoute(e,track,lane)&&G_RailRead(e,m)&&G_RailRead(*track,t)&&(!lane||G_RailRead(*lane,l));
  const bool running=route&&m.enabled&&t.enabled&&(!lane||l.enabled)&&!G_JAR_WorkshopControls(track)&&(!lane||!G_JAR_WorkshopControls(lane))&&level.time>=track->delay&&(!lane||level.time>=lane->delay);
  if(!running){if(e.count==1)railStop(e,2);continue;}
  if(e.count==2){if(railFinite(e.pos1))railTravel(e,e.pos1,t.speed);else railStop(e,0);}
  if(e.count==1&&int64_t(level.previousTime)>=int64_t(e.s.pos.trTime)+e.s.pos.trDuration){railStop(e,0);e.delay=level.time+int(m.delay*1000);}
  if(!e.count&&level.time>=e.delay)groups[track->s.number].push_back(&e);
 }
 for(auto&group:groups){auto&track=g_entities[group.first];jar::rail::Settings s;if(!G_RailRead(track,s)||level.time<track.fly_sound_debounce_time)continue;
  track.fly_sound_debounce_time=level.time+std::max(1,int(std::ceil(s.cell*1000/s.speed)));
  auto&list=group.second;const int start=Q_irand(0,int(list.size())-1);int launched=0;
  for(size_t at=0;at<list.size()&&launched<s.count;++at){auto*e=list[(start+at)%list.size()];gentity_t*t,*lane;if(railRoute(*e,t,lane)&&t==&track&&railLaunch(*e,track,lane))++launched;}
 }
}
bool G_RailRestore(){
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_RailPortable(e))continue;jar::rail::Settings s;
  if(!G_RailRead(e,s)||!railFinite(e.currentOrigin)||!railFinite(e.currentAngles)||e.e_UseFunc!=useF_portable_rail_use||e.count<0||e.count>2)return false;
  if(s.kind==jar::rail::Mover&&e.count&&(!railFinite(e.pos1)||!railFinite(e.s.pos.trBase)||!railFinite(e.s.pos.trDelta)||e.s.pos.trDuration<0||e.s.pos.trDuration>30000000))return false;
  e.s.modelindex=G_ModelIndex("models/map_objects/imperial/crate1.md3");G_RailMoved(e);gi.linkentity(&e);
 }return true;
}
bool G_RailPreview(const gentity_t&e,vec3_t corners[4]){
 if(!G_RailPortable(e))return false;gentity_t*track,*lane;float lo,hi;jar::rail::Settings s;
 if(!railRoute(e,track,lane)||!G_RailRead(*track,s)||!railWidth(*track,lane,lo,hi))return false;
 railPoint(*track,0,lo,corners[0]);railPoint(*track,s.length,lo,corners[1]);railPoint(*track,s.length,hi,corners[2]);railPoint(*track,0,hi,corners[3]);return true;
}
