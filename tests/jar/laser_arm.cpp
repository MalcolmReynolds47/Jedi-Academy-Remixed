// Production portable callbacks, poses, damage cadence and lifetime. GPL-2.0-only.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "test_support.h"
#include <deque>
#include <string>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};
gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
int G_ModelIndex(const char*){return 1;}int G_SoundIndex(const char*){return 2;}
static int sounds=0,hits=0,totalDamage=0,nextChild=20;
void G_Sound(gentity_t*,int){++sounds;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentAngles);VectorCopy(p,e->s.apos.trBase);}
gentity_t*G_JAR_SpawnSetupChild(gentity_t*){auto&e=g_entities[nextChild];e.inuse=qtrue;e.s.number=nextChild++;globals.num_entities=nextChild;return &e;}
static bool destroyOnHit=false;
void G_LaserArmForget(gentity_t*);
void G_Damage(gentity_t*victim,gentity_t*source,gentity_t*attacker,const vec3_t,const vec3_t,int damage,int flags,int,int){
 CHECK(victim==&g_entities[3]&&source==attacker&&flags==DAMAGE_IGNORE_TEAM);++hits;totalDamage+=damage;
 if(destroyOnHit){G_LaserArmForget(source);source->inuse=qfalse;}
}
#include "game/g_laser_arm.inc"
int main(){return RunTests("portable laser-arm native behavior",[]{
 gi.linkentity=[](gentity_t*){};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid save");};
 gi.trace=[](trace_t*t,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){*t={};CHECK(pass==2&&mask==MASK_SHOT);t->fraction=.5f;t->entityNum=3;for(int a=0;a<3;++a)t->endpos[a]=(start[a]+end[a])*.5f;};
 auto&e=g_entities[2];e.inuse=qtrue;e.s.number=2;e.classname=G_NewString("misc_laser_arm");e.model=G_NewString("@jar_laser_arm");VectorSet(e.currentOrigin,100,200,300);
 CHECK(G_LaserArmSpawn(&e));auto*a=e.lastEnemy;auto*h=a->lastEnemy;CHECK(a&&h&&a->owner==&e&&h->owner==&e);
 CHECK(G_LaserArmRoot(a)==&e&&G_LaserArmRoot(h)==&e&&G_LaserArmRoot(&e)==&e);
 CHECK(h->currentOrigin[0]==118&&h->currentOrigin[2]==284&&a->currentOrigin[0]==100);
 CHECK(e.e_UseFunc==useF_laser_arm_use&&e.e_ThinkFunc==thinkF_laser_arm_fire&&!e.alt_fire&&!e.nextthink);
 CHECK(a->contents==CONTENTS_ITEM&&h->contents==CONTENTS_ITEM);
 vec3_t begin,end;CHECK(!G_LaserArmBeam(e,begin,end));
 g_entities[3].inuse=qtrue;g_entities[3].takedamage=qtrue;level.time=1000;
 G_LaserArmUse(&e,nullptr);CHECK(hits==1&&totalDamage==5&&sounds==1&&e.alt_fire&&e.s.loopSound&&e.attackDebounceTime==4000);
 CHECK(G_LaserArmBeam(e,begin,end)&&begin[0]==130&&end[0]==2178);
 for(int i=0;i<20;++i){G_LaserArmUse(&e,nullptr);G_LaserArmControlledFrame(&e);}CHECK(hits==1&&sounds==1); // Input spam never multiplies tick damage.
 for(level.time=1100;level.time<4000;level.time+=100)G_LaserArmThink(&e);
 CHECK(hits==30&&totalDamage==150);G_LaserArmControlledFrame(&e);CHECK(!e.alt_fire&&!e.nextthink&&!e.s.loopSound&&!G_LaserArmBeam(e,begin,end));
 jar::laserArm::Settings s;CHECK(G_LaserArmRead(e,s));s.damage=0;s.duration=.1f;s.range=64;CHECK(G_LaserArmSet(e,s));G_LaserArmUse(&e,nullptr);CHECK(hits==30);level.time+=100;CHECK(!G_LaserArmBeam(e,begin,end));G_LaserArmThink(&e);CHECK(!e.alt_fire);
 s.action=1;s.speed=360;s.yaw=170;CHECK(G_LaserArmSet(e,s));G_LaserArmUse(&e,nullptr);CHECK(std::fabs(e.pos2[1]+154)<.01f);
 s.action=2;s.yaw=-170;CHECK(G_LaserArmSet(e,s));G_LaserArmUse(&e,nullptr);CHECK(std::fabs(e.pos2[1]-154)<.01f);
 s.action=3;s.pitch=-40;CHECK(G_LaserArmSet(e,s));G_LaserArmUse(&e,nullptr);CHECK(e.pos2[0]==-45);
 s.action=4;s.pitch=80;CHECK(G_LaserArmSet(e,s));G_LaserArmUse(&e,nullptr);CHECK(e.pos2[0]==90);
 // Base rotation composes with local aim and moves all parts, including roll.
 s.yaw=90;s.pitch=20;CHECK(G_LaserArmSet(e,s));VectorSet(e.currentAngles,15,45,30);G_LaserArmMoved(e);
 vec3_t rootAxis[3],localAxis[3],actual[3],angles={20,90,0};AnglesToAxis(e.currentAngles,rootAxis);AnglesToAxis(angles,localAxis);AnglesToAxis(h->currentAngles,actual);
 for(int i=0;i<3;++i)for(int j=0;j<3;++j){float expected=0;for(int k=0;k<3;++k)expected+=localAxis[i][k]*rootAxis[k][j];CHECK(std::fabs(expected-actual[i][j])<.0001f);}
 vec3_t prior;VectorCopy(h->currentOrigin,prior);e.currentOrigin[0]+=75;G_LaserArmMoved(e);CHECK(std::fabs(h->currentOrigin[0]-prior[0]-75)<.001f);
 const auto previous=e.radius;s.range=std::numeric_limits<float>::quiet_NaN();CHECK(!G_LaserArmSet(e,s)&&e.radius==previous);s.range=9000;CHECK(!G_LaserArmSet(e,s));
 // Same native saved fields and resolved native links are used on restore.
 CHECK(G_LaserArmRead(e,s));s.action=0;s.damage=7;s.duration=2;CHECK(G_LaserArmSet(e,s));
 const int beforeResume=hits, beforeSounds=sounds;
 CHECK(!G_LaserArmResumeBurst(e,200,FRAMETIME+1));
 CHECK(G_LaserArmResumeBurst(e,200,40)&&!G_LaserArmBeam(e,begin,end));
 CHECK(hits==beforeResume&&sounds==beforeSounds&&e.s.loopSound);
 level.time+=30;G_LaserArmRestore();G_LaserArmControlledFrame(&e);
 CHECK(e.alt_fire&&e.nextthink==level.time+10&&G_LaserArmRoot(h)==&e&&hits==beforeResume);
 level.time+=10;G_LaserArmControlledFrame(&e);
 CHECK(hits==beforeResume+1&&G_LaserArmBeam(e,begin,end));
 level.time+=160;G_LaserArmRestore();CHECK(!e.alt_fire&&!e.nextthink&&!e.s.loopSound);
 G_LaserArmUse(&e,nullptr);
 // World/invalid/startsolid trace endpoints cannot index a victim or apply damage.
 const int before=hits;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*t={};t->fraction=0;t->entityNum=-1;VectorCopy(end,t->endpos);};G_LaserArmThink(&e);CHECK(hits==before);
 level.time+=100;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*t={};t->startsolid=qtrue;t->entityNum=3;VectorCopy(end,t->endpos);};G_LaserArmThink(&e);CHECK(hits==before);
 // Removing/reusing a child disables the old assembly and clears reciprocal links.
 G_LaserArmForget(h);h->inuse=qfalse;CHECK(!a->lastEnemy&&!e.alt_fire&&!e.s.loopSound);h->inuse=qtrue;h->classname=G_NewString("unrelated");CHECK(!G_LaserArmRoot(h));G_LaserArmUse(&e,nullptr);CHECK(!e.alt_fire);
 G_LaserArmForget(&e);CHECK(!a->owner&&!e.lastEnemy);
 auto&unrelated=g_entities[8];unrelated.inuse=qtrue;unrelated.owner=&g_entities[3];G_LaserArmForget(&unrelated);CHECK(unrelated.owner==&g_entities[3]);
 CHECK(G_LaserArmSpawn(&e));a=e.lastEnemy;h=a->lastEnemy;G_LaserArmForget(a);CHECK(!e.lastEnemy&&!h->owner&&!a->owner);
 CHECK(G_LaserArmSpawn(&e));level.time+=100;destroyOnHit=true;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*t={};t->fraction=.5f;t->entityNum=3;VectorCopy(end,t->endpos);};G_LaserArmUse(&e,nullptr);CHECK(!e.inuse&&!e.lastEnemy);
 });}
