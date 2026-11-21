// Stock components reproduce the native door camera's extension action.
#include "g_door_camera.h"
#include <set>
#include <vector>
extern gentity_t*G_JAR_SpawnSetupChild(gentity_t*);
namespace {std::set<int> portableDoorCameras;}
bool G_DoorCameraMoved(gentity_t&e){
 if(!G_DoorCameraPortable(e))return false;
 VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);VectorCopy(e.currentAngles,e.s.apos.trBase);e.s.apos.trType=TR_INTERPOLATE;
 if(!G_DoorCameraParts(e)){e.s.loopSound=0;return true;}
 vec3_t axis[3],pos;AnglesToAxis(e.currentAngles,axis);const float length=jar::doorCamera::closed+e.random;
 auto place=[&](gentity_t*h,const vec3_t p,const vec3_t angles){G_SetOrigin(h,p);G_SetAngles(h,angles);VectorCopy(p,h->s.origin);VectorCopy(angles,h->s.angles);h->s.pos.trType=h->s.apos.trType=TR_INTERPOLATE;gi.linkentity(h);};
 auto*shaft=e.lastEnemy;auto*head=shaft->lastEnemy;
 VectorMA(e.currentOrigin,length,axis[0],pos);place(head,pos,e.currentAngles);
 // The stock mount points down local Z. Compose -90 local pitch with the
 // root basis so its pole telescopes forward without discarding root roll.
 vec3_t shaftAxis[3],shaftAngles,zeroRoll[3];VectorCopy(axis[2],shaftAxis[0]);VectorCopy(axis[1],shaftAxis[1]);VectorScale(axis[0],-1,shaftAxis[2]);
 vectoangles(shaftAxis[0],shaftAngles);AnglesToAxis(shaftAngles,zeroRoll);shaftAngles[2]=std::atan2(DotProduct(shaftAxis[1],zeroRoll[2]),DotProduct(shaftAxis[1],zeroRoll[1]))*180.f/3.14159265358979323846f;
 VectorSet(shaft->s.modelScale,1,1,length/jar::doorCamera::shaftLength);place(shaft,e.currentOrigin,shaftAngles);
 // One conservative enclosing hull moves the whole assembly under the Physgun.
 for(int a=0;a<3;++a){const float center=axis[0][a]*length*.5f,extent=std::fabs(axis[0][a])*(length*.5f+14)+std::fabs(axis[1][a])*14+std::fabs(axis[2][a])*14;e.mins[a]=center-extent;e.maxs[a]=center+extent;shaft->mins[a]=e.mins[a];shaft->maxs[a]=e.maxs[a];}
 gi.linkentity(shaft);gi.linkentity(&e);return true;
}
bool G_DoorCameraSpawn(gentity_t*e){
 if(!e||!G_DoorCameraPortable(*e))return false;
 auto*a=G_JAR_SpawnSetupChild(e);if(!a)return false;auto*h=G_JAR_SpawnSetupChild(e);if(!h)return false;
 for(auto*p:{a,h}){p->classname=G_NewString("info_notnull");p->model=G_NewString(p==a?jar::doorCamera::shaft:jar::doorCamera::head);p->s.modelindex=G_ModelIndex(p->model);p->s.eType=ET_GENERAL;p->owner=e;p->contents=CONTENTS_ITEM;p->takedamage=qfalse;VectorSet(p->mins,-14,-14,-14);VectorSet(p->maxs,14,14,14);}
 e->lastEnemy=a;a->lastEnemy=h;e->s.modelindex=G_ModelIndex(jar::doorCamera::base);e->s.eType=ET_GENERAL;e->contents=CONTENTS_ITEM;e->takedamage=qfalse;e->svFlags|=SVF_PLAYER_USABLE;
 e->mass=20; // Fixed assembly mass; extending its enclosing hull adds no mass.
 e->e_UseFunc=useF_jabba_cam_use;e->e_ThinkFunc=thinkF_NULL;e->nextthink=0;e->random=0;e->alt_fire=qfalse;e->s.loopSound=0;
 e->noise_index=G_SoundIndex("sound/movers/objects/cameramove_lp2.wav");portableDoorCameras.insert(e->s.number);return G_DoorCameraSet(*e,jar::doorCamera::Settings{});
}
void G_DoorCameraUse(gentity_t*e){jar::doorCamera::Settings s;if(e&&G_DoorCameraRead(*e,s)){s.extended=!s.extended;G_DoorCameraSet(*e,s);}}
void G_DoorCameraFrame(){
 const std::vector<int> snapshot(portableDoorCameras.begin(),portableDoorCameras.end());
 for(int n:snapshot){auto&e=g_entities[n];if(!G_DoorCameraPortable(e)){portableDoorCameras.erase(n);continue;}jar::doorCamera::Settings s;
  const float seconds=std::max(0.f,float(int64_t(level.time)-e.painDebounceTime)/1000.f);e.painDebounceTime=level.time;
  if(!G_DoorCameraRead(e,s)||!G_DoorCameraParts(e)||(e.svFlags&SVF_INACTIVE)){e.s.loopSound=0;continue;}
  const float target=s.extended?s.travel:0,step=s.speed*seconds;const float wanted=target>e.random?std::min(target,e.random+step):std::max(target,e.random-step);
  if(wanted==e.random){e.s.loopSound=0;if(target==e.random)e.alt_fire=qfalse;continue;}
  vec3_t forward,end;AngleVectors(e.currentAngles,forward,nullptr,nullptr);VectorMA(e.currentOrigin,jar::doorCamera::closed+wanted,forward,end);
  auto*h=e.lastEnemy->lastEnemy;trace_t hit{};gi.trace(&hit,h->currentOrigin,h->mins,h->maxs,end,e.s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);
  const float prior=e.random;e.alt_fire=hit.startsolid||hit.allsolid||hit.fraction<1?qtrue:qfalse;
  if(!hit.startsolid&&!hit.allsolid)e.random+=(wanted-e.random)*hit.fraction;
  e.s.loopSound=std::fabs(prior-e.random)>.001f&&e.random!=target?e.noise_index:0;G_DoorCameraMoved(e);
 }
}
void G_DoorCameraForget(gentity_t*e){
 if(!e)return;
 auto*r=G_DoorCameraRoot(e);if(!r)return;
 auto*a=r->lastEnemy;auto*h=G_DoorCameraPart(a,r,jar::doorCamera::shaft)?a->lastEnemy:nullptr;
 r->s.loopSound=0;
 if(e==r||e==a){if(G_DoorCameraPart(h,r,jar::doorCamera::head))h->owner=nullptr;if(G_DoorCameraPart(a,r,jar::doorCamera::shaft)){a->owner=nullptr;a->lastEnemy=nullptr;}r->lastEnemy=nullptr;}
 else {a->lastEnemy=nullptr;e->owner=nullptr;}
 if(e==r)portableDoorCameras.erase(e->s.number);
}
void G_DoorCameraReset(){for(int n:portableDoorCameras)if(G_DoorCameraPortable(g_entities[n]))g_entities[n].s.loopSound=0;portableDoorCameras.clear();}
void G_DoorCameraRestore(){
 G_DoorCameraReset();for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_DoorCameraPortable(e))continue;jar::doorCamera::Settings s;
  if(!G_DoorCameraRead(e,s)||!std::isfinite(e.random)||e.random<0||e.random>s.travel||e.e_UseFunc!=useF_jabba_cam_use||e.e_ThinkFunc!=thinkF_NULL||e.s.eType!=ET_GENERAL)gi.Error(ERR_DROP,"Invalid saved portable door camera");
  e.painDebounceTime=level.time;e.s.loopSound=0;e.noise_index=G_SoundIndex("sound/movers/objects/cameramove_lp2.wav");portableDoorCameras.insert(i);G_DoorCameraMoved(e);
 }
}
