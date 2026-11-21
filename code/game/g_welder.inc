// Portable branch of the native welder. Included by g_misc.cpp. GPL-2.0-only.
#include "g_welder.h"
#include <map>
#include <vector>
namespace {
std::map<int,uint64_t> portableWelders;
uint64_t welderSerial=0;
void welderRegister(gentity_t&e){portableWelders[e.s.number]=++welderSerial;}
}
bool G_WelderMoved(gentity_t&e){
 if(!G_WelderPortable(e))return false;
 VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);VectorCopy(e.currentAngles,e.s.apos.trBase);e.s.apos.trType=TR_INTERPOLATE;
 gi.linkentity(&e);return true;
}
bool G_WelderSpawn(gentity_t*e){
 if(!e||!G_WelderPortable(*e))return false;
 e->s.modelindex=G_ModelIndex(jar::welder::model);e->s.eType=ET_GENERAL;e->contents=CONTENTS_ITEM;e->takedamage=qfalse;e->svFlags|=SVF_PLAYER_USABLE;
 // Conservative rotation-independent hull around the unscaled retail DEMP2.
 VectorSet(e->mins,-16,-16,-16);VectorSet(e->maxs,16,16,16);
 e->e_UseFunc=useF_welder_use;e->e_ThinkFunc=thinkF_NULL;e->nextthink=0;e->attackDebounceTime=0;e->s.loopSound=0;
 e->fxID=G_EffectIndex(jar::welder::effect);e->noise_index=G_SoundIndex(jar::welder::sound);
 welderRegister(*e);return G_WelderSet(*e,jar::welder::Settings{});
}
void G_WelderUse(gentity_t*e){jar::welder::Settings s;if(e&&G_WelderRead(*e,s)){s.enabled=!s.enabled;G_WelderSet(*e,s);}}
namespace {
void welderFrame(gentity_t&e){
 jar::welder::Settings s;
 if(!G_WelderRead(e,s)||!s.enabled||(e.svFlags&SVF_INACTIVE)){e.s.loopSound=0;return;}
 e.s.loopSound=e.noise_index;
 if(e.attackDebounceTime>level.time)return;
 e.attackDebounceTime=level.time+int(std::lround(s.interval*1000));
 vec3_t axis[3],tip,direction;AnglesToAxis(e.currentAngles,axis);
 VectorMA(e.currentOrigin,s.reach,axis[0],tip);
 // Verified retail tag_flash lateral/vertical offset; reach extends its X axis.
 VectorMA(tip,-.7046813f,axis[1],tip);VectorMA(tip,-3.21862f,axis[2],tip);
 trace_t hit{};gi.trace(&hit,e.currentOrigin,nullptr,nullptr,tip,e.s.number,MASK_SHOT,(EG2_Collision)0,0);
 if(hit.startsolid||hit.allsolid)return;
 VectorCopy(hit.endpos,tip);
 if(hit.fraction<1){VectorCopy(hit.plane.normal,direction);VectorMA(tip,.5f,direction,tip);}
 else VectorScale(axis[0],-1,direction);
 G_PlayEffect(e.fxID,tip,direction);
 // Native radial damage checks LOS. A destruction callback may delete/reuse
 // this controller or any other member; do not access e after this call.
 if(s.damage)G_RadiusDamage(tip,&e,float(s.damage),s.radius,&e,MOD_UNKNOWN);
}
}
void G_WelderFrame(){
 const std::vector<std::pair<int,uint64_t>> snapshot(portableWelders.begin(),portableWelders.end());
 for(const auto&entry:snapshot){
  auto it=portableWelders.find(entry.first);if(it==portableWelders.end()||it->second!=entry.second)continue;
  auto&e=g_entities[entry.first];if(!G_WelderPortable(e)){portableWelders.erase(it);continue;}welderFrame(e);
 }
}
void G_WelderForget(gentity_t*e){if(!e)return;portableWelders.erase(e->s.number);if(G_WelderPortable(*e))e->s.loopSound=0;}
void G_WelderReset(){for(const auto&entry:portableWelders){auto&e=g_entities[entry.first];if(G_WelderPortable(e))e.s.loopSound=0;}portableWelders.clear();}
void G_WelderRestore(){
 G_WelderReset();
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_WelderPortable(e))continue;
  jar::welder::Settings s;if(!G_WelderRead(e,s)||e.e_UseFunc!=useF_welder_use||e.e_ThinkFunc!=thinkF_NULL||e.s.eType!=ET_GENERAL)gi.Error(ERR_DROP,"Invalid saved portable welder");
  e.s.loopSound=0;e.fxID=G_EffectIndex(jar::welder::effect);e.noise_index=G_SoundIndex(jar::welder::sound);welderRegister(e);G_WelderMoved(e);
 }
}
