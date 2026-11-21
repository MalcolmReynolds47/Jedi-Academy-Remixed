// Production searchlight controller, cgame submission, lifecycle and save fields.
#include "cgame/cg_headers.h"
#include "cgame/FxUtil.h"
#include "memory_save.h"
#include <deque>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;static int searches=0,traces=0,uses=0,killOnUse=-1;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
int G_ModelIndex(const char*s){CHECK(!strcmp(s,"models/map_objects/kejim/impcam.md3"));return 7;}
void G_SetAngles(gentity_t*e,const vec3_t a){VectorCopy(a,e->currentAngles);VectorCopy(a,e->s.apos.trBase);}
gentity_t*G_Find(gentity_t*,int field,const char*name){++searches;CHECK(field==FOFS(targetname));for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(e.inuse&&e.targetname&&!strcmp(name,e.targetname))return &e;}return nullptr;}
void G_SpotlightForget(gentity_t*);
void G_UseTargets2(gentity_t*e,gentity_t*p,const char*name){CHECK(e->s.number==2&&p==&g_entities[0]&&!strcmp(name,"detected"));++uses;if(killOnUse>=0){G_SpotlightForget(&g_entities[killOnUse]);g_entities[killOnUse].inuse=qfalse;}}
#include "game/g_spotlight.inc"
static int cones=0,lights=0;static float coneLength=0,coneWidth=0,lightRadius=0;static vec3_t coneStart{},lightColor{};
qhandle_t cgi_R_RegisterShader(const char*s){CHECK(!strcmp(s,"gfx/effects/light_cone"));return 9;}
CCylinder*FX_AddCylinder(int client,vec3_t start,vec3_t,float a,float b,float,float c,float d,float,float length,float length2,float,float alpha,float alpha2,float,vec3_t color,vec3_t color2,float,int life,qhandle_t shader,int flags,int,int){CHECK(client==-1&&a==2&&b==2&&c==d&&length==length2&&alpha==alpha2&&life==1&&shader==9&&!flags);++cones;coneWidth=c;coneLength=length;VectorCopy(start,coneStart);for(int i=0;i<3;++i)CHECK(color[i]==color2[i]);return nullptr;}
void cgi_R_AddLightToScene(const vec3_t,float radius,float r,float g,float b){++lights;lightRadius=radius;VectorSet(lightColor,r,g,b);}
#include "cgame/cg_spotlight.inc"
int main(){return RunTests("spotlight tracking, detection, render submission and saved state",[]{
 globals.num_entities=16;level.time=1000;gi.linkentity=[](gentity_t*){};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid save");};
 static bool obstruct=false,solid=false;gi.trace=[](trace_t*t,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){++traces;CHECK(pass>=2&&mask==MASK_SHOT);*t={};t->entityNum=ENTITYNUM_WORLD;t->startsolid=solid?qtrue:qfalse;const bool sight=std::fabs(end[0]-g_entities[0].currentOrigin[0])<.001f&&std::fabs(end[1]-g_entities[0].currentOrigin[1])<.001f;t->fraction=sight?(obstruct?.5f:1.f):.5f;for(int i=0;i<3;++i)t->endpos[i]=start[i]+t->fraction*(end[i]-start[i]);};
 auto make=[](int n)->gentity_t&{auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=G_NewString("misc_spotlight");e.model=G_NewString("@jar_spotlight");CHECK(G_SpotlightSpawn(&e));return e;};
 auto&e=make(2);CHECK(e.e_UseFunc==useF_spotlight_use&&e.e_ThinkFunc==thinkF_NULL&&e.contents==CONTENTS_ITEM&&e.s.modelindex==7&&e.wait==2048&&e.speed==8);
 auto&t=g_entities[3];t.inuse=qtrue;t.s.number=3;t.targetname=G_NewString("aim");VectorSet(t.currentOrigin,500,0,0);e.target=G_NewString("aim");e.target2=G_NewString("detected");
 static gclient_t client{};auto&p=g_entities[0];p.inuse=qtrue;p.client=&client;p.health=100;VectorSet(p.mins,-8,-8,-8);VectorSet(p.maxs,8,8,8);VectorSet(p.currentOrigin,500,0,0);
 G_SpotlightFrame();CHECK(e.enemy==&t&&e.radius==1024&&e.pos3[0]==12&&uses==1&&e.alt_fire&&searches==1);
 const int sampled=traces;for(int i=0;i<10;++i){G_SpotlightMoved(e);G_SpotlightFrame();}CHECK(traces==sampled&&uses==1);level.time+=50;G_SpotlightFrame();CHECK(uses==1);
 for(int i=0;i<5;++i){G_SpotlightSet(e,jar::spotlight::Settings{});G_SpotlightFrame();}CHECK(uses==1&&e.delay==2000); // Reapplying properties cannot refire a held detection.
 centity_t cent{};cent.gent=&e;CG_PortableSpotlight(&cent);CHECK(cones==1&&lights==1&&coneLength==1024&&coneStart[0]==12&&lightRadius==225&&std::fabs(coneWidth-1024*std::tan(8*.01745329252f))<.01f);
 obstruct=true;level.time+=50;G_SpotlightFrame();CHECK(!e.alt_fire);obstruct=false;level.time+=1000;G_SpotlightFrame();CHECK(uses==2);level.time+=1000;G_SpotlightFrame();CHECK(uses==2);
 VectorSet(t.currentOrigin,500,500,0);VectorCopy(t.currentOrigin,p.currentOrigin);e.currentAngles[ROLL]=25;level.time+=50;G_SpotlightFrame();CHECK(std::fabs(e.currentAngles[YAW]-45)<.001f&&e.currentAngles[ROLL]==25&&uses==2);
 jar::spotlight::Settings s;CHECK(G_SpotlightRead(e,s));s.tracking=false;s.range=512;s.light=0;s.cone=15;VectorSet(s.rgb,.2f,.4f,.8f);CHECK(G_SpotlightSet(e,s));VectorClear(e.currentAngles);G_SpotlightMoved(e);G_SpotlightFrame();CHECK(e.currentAngles[YAW]==0&&!e.alt_fire);CG_PortableSpotlight(&cent);CHECK(cones==2&&lights==1&&coneLength==256);
 s.light=100;CHECK(G_SpotlightSet(e,s));G_SpotlightFrame();CG_PortableSpotlight(&cent);CHECK(lights==2&&lightColor[0]==.2f&&lightColor[2]==.8f);
 G_SpotlightUse(&e);G_SpotlightFrame();CG_PortableSpotlight(&cent);CHECK(e.spawnflags&1&&e.radius==0&&cones==3);G_SpotlightUse(&e);G_SpotlightFrame();CHECK(e.radius>0);
 e.svFlags|=SVF_INACTIVE;G_SpotlightFrame();CHECK(!e.radius);e.svFlags&=~SVF_INACTIVE;level.time+=50;solid=true;G_SpotlightFrame();CHECK(!e.radius&&!e.alt_fire);solid=false;
 auto bad=s;bad.rgb[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!G_SpotlightSet(e,bad)&&e.pos1[0]==.2f);bad=s;bad.range=8193;CHECK(!G_SpotlightSet(e,bad));
 s.tracking=true;CHECK(G_SpotlightSet(e,s));G_SpotlightFrame();G_SpotlightForget(&t);t.inuse=qfalse;CHECK(!e.enemy);const int looked=searches;level.time+=50;G_SpotlightFrame();CHECK(searches==looked);t.inuse=qtrue;t.targetname=G_NewString("reused");level.time+=1000;G_SpotlightFrame();CHECK(!e.enemy);t.targetname=G_NewString("aim");level.time+=1000;G_SpotlightFrame();CHECK(e.enemy==&t);
 MemorySave save;ojk::SavedGameHelper h(&save);gentity_t stored{},restored{};stored.wait=e.wait;stored.speed=e.speed;stored.random=e.random;stored.count=e.count;stored.spawnflags=e.spawnflags;stored.alt_fire=e.alt_fire;stored.delay=e.delay;stored.e_UseFunc=e.e_UseFunc;stored.e_ThinkFunc=e.e_ThinkFunc;VectorCopy(e.pos1,stored.pos1);stored.sg_export(h);save.reset_buffer_offset();restored.sg_import(h);h.ensure_all_data_read();CHECK(restored.wait==512&&restored.speed==15&&restored.random==100&&restored.pos1[2]==.8f&&restored.e_UseFunc==useF_spotlight_use&&restored.alt_fire==e.alt_fire&&restored.delay==e.delay);
 const auto prior=uses;G_SpotlightRestore();G_SpotlightFrame();CHECK(uses==prior);e.count=2;bool rejected=false;try{G_SpotlightRestore();}catch(...){rejected=true;}CHECK(rejected);e.count=0;G_SpotlightRestore();
 // Detection may delete a future registry member or itself without invalidating traversal.
 auto&other=make(4);killOnUse=4;s.range=2048;G_SpotlightUse(&e);G_SpotlightSet(e,s);level.time+=1000;G_SpotlightFrame();CHECK(!other.inuse);killOnUse=2;G_SpotlightUse(&e);G_SpotlightSet(e,s);level.time+=1000;G_SpotlightFrame();CHECK(!e.inuse&&portableSpotlights.empty());G_SpotlightReset();
 // Largest allowed cone preserves its advertised half-angle through cgame.
 auto&wide=make(5);s.range=8192;s.cone=30;CHECK(G_SpotlightSet(wide,s));wide.radius=8192;VectorSet(wide.s.origin2,8204,0,0);cent.gent=&wide;VectorClear(cent.lerpOrigin);VectorClear(cent.lerpAngles);CG_PortableSpotlight(&cent);CHECK(coneLength==8192&&std::fabs(coneWidth-4729.653f)<.01f);G_SpotlightReset();
 });}
