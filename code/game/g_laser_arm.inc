// Stock-art portable laser arm. Included by g_turret.cpp. GPL-2.0-only.
#include "g_laser_arm.h"
extern gentity_t*G_JAR_SpawnSetupChild(gentity_t*);
namespace {
bool laserPart(const gentity_t*e,const gentity_t*root,const char*model){
 return e&&e->inuse&&e->owner==root&&e->model&&!strcmp(e->model,model)&&e->classname&&!strcmp(e->classname,"info_notnull");
}
bool laserParts(const gentity_t&e){return laserPart(e.lastEnemy,&e,jar::laserArm::armModel)&&laserPart(e.lastEnemy->lastEnemy,&e,jar::laserArm::headModel);}
void laserStop(gentity_t&e){e.alt_fire=qfalse;e.s.loopSound=0;e.nextthink=0;}
}
gentity_t*G_LaserArmRoot(gentity_t*e){
 if(!e||!e->inuse)return nullptr;if(G_LaserArmPortable(*e))return e;
 auto*r=e->owner;if(!r||!G_LaserArmPortable(*r)||!laserPart(r->lastEnemy,r,jar::laserArm::armModel))return nullptr;
 return e==r->lastEnemy||(e==r->lastEnemy->lastEnemy&&laserPart(e,r,jar::laserArm::headModel))?r:nullptr;
}
bool G_LaserArmMoved(gentity_t&e){
 if(!G_LaserArmPortable(e))return false;
 VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);
 if(!laserParts(e)){laserStop(e);return true;}
 vec3_t baseAxis[3],local[3],world[3],armAngles,headAngles,pos;
 AnglesToAxis(e.currentAngles,baseAxis);
 auto place=[&](gentity_t*h,float x,float z,const vec3_t angles){
  vec3_t azimuth={0,e.pos2[1],0},azimuthAxis[3];AnglesToAxis(azimuth,azimuthAxis);
  VectorCopy(e.currentOrigin,pos);for(int a=0;a<3;++a)VectorMA(pos,x*azimuthAxis[0][a],baseAxis[a],pos);VectorMA(pos,z,baseAxis[2],pos);
  AnglesToAxis(angles,local);
  for(int a=0;a<3;++a)for(int b=0;b<3;++b)world[a][b]=local[a][0]*baseAxis[0][b]+local[a][1]*baseAxis[1][b]+local[a][2]*baseAxis[2][b];
  vec3_t final,zeroRoll[3];vectoangles(world[0],final);AnglesToAxis(final,zeroRoll);final[2]=std::atan2(DotProduct(world[1],zeroRoll[2]),DotProduct(world[1],zeroRoll[1]))*180.f/3.14159265358979323846f;G_SetOrigin(h,pos);G_SetAngles(h,final);VectorCopy(pos,h->s.origin);VectorCopy(final,h->s.angles);gi.linkentity(h);
 };
 VectorSet(armAngles,0,e.pos2[1],0);VectorSet(headAngles,e.pos2[0],e.pos2[1],0);
 place(e.lastEnemy,0,-16,armAngles);place(e.lastEnemy->lastEnemy,18,-16,headAngles);return true;
}
bool G_LaserArmBeam(gentity_t&e,vec3_t start,vec3_t end){
 if(!G_LaserArmPortable(e)||!laserParts(e)||!e.alt_fire||!e.painDebounceTime||level.time>=e.attackDebounceTime)return false;
 auto*h=e.lastEnemy->lastEnemy;vec3_t forward;AngleVectors(h->currentAngles,forward,nullptr,nullptr);
 VectorMA(h->currentOrigin,12,forward,start);VectorCopy(e.s.origin2,end);return true;
}
bool G_LaserArmSpawn(gentity_t*e){
 if(!e||!G_LaserArmPortable(*e))return false;
 auto*a=G_JAR_SpawnSetupChild(e);if(!a)return false;auto*h=G_JAR_SpawnSetupChild(e);if(!h)return false;
 for(auto*p:{a,h}){p->classname=G_NewString("info_notnull");p->model=G_NewString(p==a?jar::laserArm::armModel:jar::laserArm::headModel);p->s.modelindex=G_ModelIndex(p->model);p->s.eType=ET_GENERAL;p->owner=e;p->contents=CONTENTS_ITEM;VectorSet(p->mins,-12,-12,-12);VectorSet(p->maxs,12,12,12);}
 e->lastEnemy=a;a->lastEnemy=h;
 e->s.modelindex=G_ModelIndex(jar::laserArm::baseModel);e->s.eType=ET_GENERAL;e->contents=CONTENTS_ITEM;e->svFlags|=SVF_PLAYER_USABLE|SVF_BROADCAST;
 VectorSet(e->mins,-16,-16,-32);VectorSet(e->maxs,32,16,16);
 e->e_UseFunc=useF_laser_arm_use;e->e_ThinkFunc=thinkF_laser_arm_fire;
 G_SoundIndex("sound/weapons/laser_trap/fire.mp3");G_SoundIndex("sound/weapons/laser_trap/hum_loop.wav");G_SoundIndex("sound/weapons/laser_trap/stick.mp3");
 G_LaserArmSet(*e,jar::laserArm::Settings{});e->nextthink=0;gi.linkentity(e);return true;
}
bool G_LaserArmResumeBurst(gentity_t& e,int remaining,int nextSample){
 if(!G_LaserArmPortable(e)||!laserParts(e)||e.count||e.e_ThinkFunc!=thinkF_laser_arm_fire||
    remaining<1||remaining>jar::laserArm::maxBurstMs||
    nextSample<1||nextSample>FRAMETIME)return false;
 e.alt_fire=qtrue;
 e.s.loopSound=G_SoundIndex("sound/weapons/laser_trap/hum_loop.wav");
 e.attackDebounceTime=level.time+remaining;
 // No copied trace or damage. The first scheduled think samples the new pose.
 e.painDebounceTime=0;
 e.nextthink=level.time+std::min(nextSample,remaining);
 return true;
}
void G_LaserArmThink(gentity_t*e){
 jar::laserArm::Settings settings;if(!e||!G_LaserArmRead(*e,settings)||!laserParts(*e)){if(e)laserStop(*e);return;}
 if(!e->alt_fire||level.time>=e->attackDebounceTime){laserStop(*e);return;}
 G_LaserArmMoved(*e);auto*h=e->lastEnemy->lastEnemy;vec3_t forward,start,end;
 AngleVectors(h->currentAngles,forward,nullptr,nullptr);VectorMA(h->currentOrigin,12,forward,start);VectorMA(start,settings.range,forward,end);
 // Parts are CONTENTS_ITEM, outside MASK_SHOT. Damage trace cannot hit itself.
 trace_t hit;gi.trace(&hit,start,nullptr,nullptr,end,e->s.number,MASK_SHOT,(EG2_Collision)0,0);VectorCopy(hit.endpos,e->s.origin2);
 e->nextthink=std::min(level.time+FRAMETIME,e->attackDebounceTime);
 if(level.time<e->painDebounceTime)return;e->painDebounceTime=level.time+FRAMETIME;
 if(!hit.startsolid&&!hit.allsolid&&hit.fraction<1&&hit.entityNum>=0&&hit.entityNum<ENTITYNUM_WORLD&&settings.damage){
  auto&victim=g_entities[hit.entityNum];if(victim.inuse&&victim.takedamage){
   // Native destruction can free this assembly. Do not access it after damage.
   G_Damage(&victim,e,e,forward,hit.endpos,settings.damage,DAMAGE_IGNORE_TEAM,MOD_UNKNOWN);
  }
 }
}
void G_LaserArmControlledFrame(gentity_t*e){
 // Freeze/Physgun suspend native motion. Timed firing still ends normally.
 if(e&&G_LaserArmPortable(*e)&&e->nextthink>0&&e->nextthink<=level.time)G_LaserArmThink(e);
}
void G_LaserArmUse(gentity_t*e,gentity_t*){
 jar::laserArm::Settings s;if(!e||!G_LaserArmRead(*e,s)||!laserParts(*e))return;
 if(!s.action){
  const bool was=e->alt_fire!=qfalse;e->alt_fire=qtrue;e->attackDebounceTime=level.time+int(std::lround(s.duration*1000));
  e->s.loopSound=G_SoundIndex("sound/weapons/laser_trap/hum_loop.wav");if(!was)G_Sound(e,G_SoundIndex("sound/weapons/laser_trap/fire.mp3"));G_LaserArmThink(e);return;
 }
 const float step=s.speed*(FRAMETIME/1000.f);
 if(s.action==1||s.action==2)e->pos2[1]=AngleNormalize180(s.yaw+(s.action==1?step:-step));
 else e->pos2[0]=std::max(-45.f,std::min(90.f,s.pitch+(s.action==3?-step:step)));
 G_LaserArmMoved(*e);G_Sound(e,G_SoundIndex("sound/weapons/laser_trap/stick.mp3"));
}
void G_LaserArmForget(gentity_t*e){
 if(!e)return;
 if(G_LaserArmPortable(*e)){
  auto*a=e->lastEnemy;auto*h=laserPart(a,e,jar::laserArm::armModel)?a->lastEnemy:nullptr;
  if(laserPart(h,e,jar::laserArm::headModel))h->owner=nullptr;
  if(laserPart(a,e,jar::laserArm::armModel)){a->owner=nullptr;a->lastEnemy=nullptr;}
  e->lastEnemy=nullptr;laserStop(*e);return;
 }
 auto*r=G_LaserArmRoot(e);if(r){laserStop(*r);if(r->lastEnemy==e){auto*h=e->lastEnemy;if(h&&h->inuse&&h->owner==r)h->owner=nullptr;r->lastEnemy=nullptr;e->lastEnemy=nullptr;}else r->lastEnemy->lastEnemy=nullptr;}
 if(r)e->owner=nullptr;
}
void G_LaserArmRestore(){
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_LaserArmPortable(e))continue;
  jar::laserArm::Settings s;if(!G_LaserArmRead(e,s))gi.Error(ERR_DROP,"Invalid saved laser-arm settings");
  G_LaserArmMoved(e);
  if(e.alt_fire&&e.attackDebounceTime<=level.time)laserStop(e);
  else if(e.alt_fire&&e.nextthink<=level.time)e.nextthink=level.time+1;
 }
}
