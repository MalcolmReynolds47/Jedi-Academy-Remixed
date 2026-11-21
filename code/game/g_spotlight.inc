// Portable branch of the native spotlight; map-authored callbacks stay native.
#include "g_spotlight.h"
#include <set>
#include <vector>
namespace {std::set<int> portableSpotlights;}
bool G_SpotlightMoved(gentity_t&e){
 if(!G_SpotlightPortable(e))return false;
 VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);VectorCopy(e.currentAngles,e.s.apos.trBase);e.s.apos.trType=TR_INTERPOLATE;
 gi.linkentity(&e);return true;
}
bool G_SpotlightSpawn(gentity_t*e){
 if(!e||!G_SpotlightPortable(*e))return false;
 e->s.modelindex=G_ModelIndex(jar::spotlight::model);e->s.eType=ET_GENERAL;e->contents=CONTENTS_ITEM;e->takedamage=qfalse;e->svFlags|=SVF_PLAYER_USABLE;
 VectorSet(e->mins,-12,-12,-12);VectorSet(e->maxs,12,12,12);e->e_UseFunc=useF_spotlight_use;e->e_ThinkFunc=thinkF_NULL;e->nextthink=0;e->enemy=nullptr;e->painDebounceTime=0;
 e->alt_fire=qfalse;e->delay=0;portableSpotlights.insert(e->s.number);return G_SpotlightSet(*e,jar::spotlight::Settings{});
}
void G_SpotlightUse(gentity_t*e){jar::spotlight::Settings s;if(e&&G_SpotlightRead(*e,s)){s.enabled=!s.enabled;G_SpotlightSet(*e,s);}}
namespace {
gentity_t*spotlightTarget(gentity_t&e){
 if(e.enemy&&e.enemy!=&e&&e.enemy->inuse&&e.target&&e.enemy->targetname&&!strcmp(e.target,e.enemy->targetname))return e.enemy;
 e.enemy=nullptr;if(!e.target||!*e.target||e.painDebounceTime>level.time)return nullptr;
 e.painDebounceTime=level.time+1000;auto*t=G_Find(nullptr,FOFS(targetname),e.target);if(t&&t!=&e&&t->inuse)e.enemy=t;return e.enemy;
}
void spotlightFrame(gentity_t&e){
 jar::spotlight::Settings s;if(!G_SpotlightRead(e,s)||!s.enabled||(e.svFlags&SVF_INACTIVE)){e.radius=0;e.alt_fire=qfalse;return;}
 if(e.attackDebounceTime>level.time)return;e.attackDebounceTime=level.time+50;
 vec3_t forward;AngleVectors(e.currentAngles,forward,nullptr,nullptr);
 if(s.tracking)if(auto*t=spotlightTarget(e)){vec3_t direction;VectorSubtract(t->currentOrigin,e.currentOrigin,direction);if(VectorNormalize(direction)>.001f){VectorCopy(direction,forward);vec3_t angles;vectoangles(direction,angles);angles[ROLL]=e.currentAngles[ROLL];G_SetAngles(&e,angles);VectorCopy(angles,e.s.angles);VectorCopy(angles,e.s.apos.trBase);e.s.apos.trType=TR_INTERPOLATE;}}
 vec3_t end;VectorMA(e.currentOrigin,12,forward,e.pos3);VectorMA(e.pos3,s.range,forward,end);trace_t hit{};
 gi.trace(&hit,e.pos3,nullptr,nullptr,end,e.s.number,MASK_SHOT,(EG2_Collision)0,0);
 if(hit.startsolid||hit.allsolid){e.radius=0;e.alt_fire=qfalse;return;}
 VectorCopy(hit.endpos,e.s.origin2);e.radius=Distance(e.pos3,hit.endpos);
 // Player detection uses the cone and an unobstructed ray, not proximity to a wall hit.
 auto&p=g_entities[0];bool detected=false;
 if(p.inuse&&p.client&&p.health>0){vec3_t point,toPlayer;VectorAdd(p.currentOrigin,p.mins,point);for(int a=0;a<3;++a)point[a]+=(p.maxs[a]-p.mins[a])*.5f;
  VectorSubtract(point,e.pos3,toPlayer);const float distance=VectorNormalize(toPlayer);
  if(distance>.001f&&distance<=s.range&&DotProduct(forward,toPlayer)>=std::cos(s.cone*.01745329252f)){
   trace_t sight{};gi.trace(&sight,e.pos3,nullptr,nullptr,point,e.s.number,MASK_SHOT,(EG2_Collision)0,0);detected=!sight.startsolid&&!sight.allsolid&&(sight.fraction>=1||sight.entityNum==0);
  }
 }
 const bool entering=detected&&!e.alt_fire;e.alt_fire=detected?qtrue:qfalse;
 if(entering&&e.target2&&e.delay<=level.time){e.delay=level.time+1000;G_UseTargets2(&e,&p,e.target2);} // May delete any member; no accesses afterward.
}
}
void G_SpotlightFrame(){
 const std::vector<int> snapshot(portableSpotlights.begin(),portableSpotlights.end());
 for(int n:snapshot){auto&e=g_entities[n];if(!G_SpotlightPortable(e)){portableSpotlights.erase(n);continue;}spotlightFrame(e);}
}
void G_SpotlightForget(gentity_t*e){if(!e)return;portableSpotlights.erase(e->s.number);for(int n:portableSpotlights){auto&s=g_entities[n];if(s.enemy==e){s.enemy=nullptr;s.painDebounceTime=level.time+1000;s.attackDebounceTime=0;}}}
void G_SpotlightReset(){portableSpotlights.clear();}
void G_SpotlightRestore(){G_SpotlightReset();for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_SpotlightPortable(e))continue;jar::spotlight::Settings s;if(!G_SpotlightRead(e,s)||e.e_UseFunc!=useF_spotlight_use||e.e_ThinkFunc!=thinkF_NULL||e.s.eType!=ET_GENERAL)gi.Error(ERR_DROP,"Invalid saved portable spotlight");portableSpotlights.insert(i);e.painDebounceTime=0;e.attackDebounceTime=0;G_SpotlightMoved(e);}}
