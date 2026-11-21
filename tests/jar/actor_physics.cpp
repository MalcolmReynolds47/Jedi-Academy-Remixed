#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_sandbox_charges.h"
#include "game/jar_physics.h"
#include "game/jar_actor_contact.h"
#include "test_support.h"
#include <map>
game_import_t gi{};game_export_t globals{};level_locals_t level{};
gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static cvar_t gravity{};cvar_t*g_gravity=&gravity;
jar::physics::World physicsWorld;
std::map<unsigned,int> nativeControls;
uint64_t G_EntityLifetime(const gentity_t* e){return e&&e->inuse?1:0;}
bool G_JAR_WorkshopControls(const gentity_t* e){return e&&nativeControls.count(e->s.number)!=0;}
bool G_JAR_WorkshopPropDepth(const gentity_t&,const vec3_t,const vec3_t,const vec3_t,const JarSweepHull*,float&){return false;}
jar::physics::Vec pv(const float*v){return {v[0],v[1],v[2]};}
void nativeVector(jar::physics::Vec v,float*out){out[0]=v.x;out[1]=v.y;out[2]=v.z;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);VectorClear(e->s.pos.trDelta);if(e->client)VectorCopy(p,e->client->ps.origin);}
void G_SetAngles(gentity_t*e,const vec3_t a){VectorCopy(a,e->currentAngles);VectorCopy(a,e->s.apos.trBase);}
void impactSound(unsigned,float){}
static const jar::physics::NativePropHull* testHull=nullptr;
const jar::physics::NativePropHull* propTraceHull(const gentity_t&,const vec3_t){return testHull;}
#include "game/g_workshop_actorphysics.inc"
static gentity_t*testActor;static float fraction=1;
static bool blockCharge=false;static int chargeChecks=0;
int main(){return RunTests("actual actor bridge: ride moving props, mass response and blocked displacement",[]{
 gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t**list,int){list[0]=testActor;return 1;};
 gi.trace=[](trace_t*t,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
  *t={};t->fraction=fraction;
  if(pass==8){
   CHECK(mask==MASK_SHOT&&!g_entities[7].linked&&!g_entities[8].linked&&!g_entities[0].linked);
   ++chargeChecks;t->fraction=blockCharge?.5f:1;
  }
  for(int a=0;a<3;++a)t->endpos[a]=start[a]+t->fraction*(end[a]-start[a]);
 };
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;for(int a=0;a<3;++a){e->absmin[a]=e->currentOrigin[a]+e->mins[a];e->absmax[a]=e->currentOrigin[a]+e->maxs[a];}};
 gi.unlinkentity=[](gentity_t*e){e->linked=qfalse;};
 auto& prop=g_entities[7];auto& actor=g_entities[0];gclient_t client{};
 actor.inuse=prop.inuse=qtrue;actor.client=&client;testActor=&actor;actor.health=100;actor.s.number=0;actor.mass=80;
 globals.num_entities=9;
 prop.s.number=7;VectorSet(prop.mins,-10,-10,-10);VectorSet(prop.maxs,10,10,10);
 VectorSet(actor.mins,-16,-16,-24);VectorSet(actor.maxs,16,16,40);
 gravity.value=800;
 CHECK(physicsWorld.Start({{-1024,-1024,0},{1024,-1024,0},{1024,1024,0}}));
 jar::physics::Pose p;p.position={20,0,100};p.linear={200,0,0};CHECK(physicsWorld.Prop(7,1,{10,10,10},{},p,false));
 VectorSet(prop.currentOrigin,20,0,100);VectorSet(actor.currentOrigin,0,0,134);gi.linkentity(&actor);client.ps.groundEntityNum=7;
 vec3_t old={0,0,100};physicsActors(prop,old,vec3_origin,p,.05f,false);
 CHECK(actor.currentOrigin[0]==20&&client.ps.origin[0]==20&&actor.currentOrigin[2]==134);
 CHECK(physicsWorld.Read(7,p)&&p.linear.z<0); // rider weight is an actual downward impulse
 // A ceiling/wall blocking the rider stops the support instead of embedding it.
 fraction=0;VectorSet(prop.currentOrigin,40,0,100);old[0]=20;physicsActors(prop,old,vec3_origin,p,.05f,false);
 CHECK(prop.currentOrigin[0]==20&&actor.currentOrigin[0]==20);CHECK(physicsWorld.Read(7,p)&&p.position.x==20&&p.linear.x==0);
 // A fast prop hits an actor even when its final hull is on the far side.
 fraction=1;client.ps.groundEntityNum=ENTITYNUM_NONE;VectorClear(client.ps.velocity);VectorSet(actor.currentOrigin,0,0,100);gi.linkentity(&actor);
 VectorSet(old,-60,0,100);VectorSet(prop.currentOrigin,60,0,100);p.position={60,0,100};p.linear={500,0,0};physicsWorld.Correct(7,p.position,p.linear);
 physicsActors(prop,old,vec3_origin,p,.05f,false);CHECK(client.ps.velocity[0]>0&&client.ps.velocity[0]<500);CHECK(prop.currentOrigin[0]<60);
 CHECK(physicsWorld.Read(7,p)&&p.linear.x<500&&std::fabs(p.linear.x-client.ps.velocity[0])<.01f);
 // An actor backed against solid world does not make the prop pass through.
 fraction=0;VectorClear(client.ps.velocity);VectorSet(actor.currentOrigin,0,0,100);gi.linkentity(&actor);VectorSet(prop.currentOrigin,60,0,100);p.linear={500,0,0};
 physicsActors(prop,old,vec3_origin,p,.05f,false);CHECK(prop.currentOrigin[0]<-25.9f&&actor.currentOrigin[0]==0);CHECK(physicsWorld.Read(7,p)&&p.linear.x==0);

 auto& det=g_entities[8];det.inuse=qtrue;det.s.number=8;det.s.eType=ET_GENERAL;
 det.classname=const_cast<char*>("detpack");det.s.weapon=WP_DET_PACK;det.s.eFlags=EF_MISSILE_STICK;
 det.e_DieFunc=dieF_WP_ExplosiveDie;det.e_ThinkFunc=thinkF_WP_Explode;det.nextthink=1700;
 det.s.pos.trType=TR_STATIONARY;det.contents=CONTENTS_SHOTCLIP;
 prop.classname=const_cast<char*>("jar_sandbox_prop");prop.s.eType=ET_GENERAL;
 VectorSet(det.mins,-2,-2,-2);VectorSet(det.maxs,2,2,2);
 const vec3_t oldAngles={0,25,0},newAngles={0,45,0};
 G_SetOrigin(&prop,old);G_SetAngles(&prop,oldAngles);gi.linkentity(&prop);
 const vec3_t detStart={-60,50,100};
 G_SetOrigin(&det,detStart);gi.linkentity(&det);G_SandboxChargeAttach(det,prop);
 CHECK(det.s.groundEntityNum==7);
 const vec3_t solved={60,0,100};
 p.position=pv(solved);p.angles=pv(newAngles);p.linear={500,0,0};p.angular={0,0,1};
 const auto solvedPose=p;
 G_SetOrigin(&prop,solved);G_SetAngles(&prop,newAngles);CHECK(physicsWorld.CorrectPose(7,p));
 VectorClear(client.ps.velocity);
 fraction=1;blockCharge=true;
 physicsActors(prop,old,oldAngles,p,.05f,false);
 CHECK(chargeChecks==1&&VectorCompare(prop.currentOrigin,old)&&VectorCompare(prop.currentAngles,oldAngles));
 CHECK(actor.currentOrigin[0]==0&&client.ps.velocity[0]==0&&VectorCompare(det.currentOrigin,detStart)&&det.nextthink==1700);
 CHECK(actor.linked&&prop.linked&&det.linked&&physicsWorld.Read(7,p)&&p.position.x==-60&&std::fabs(p.angles.y-25)<.01f&&p.linear.x==0&&p.angular.z==0);
 p=solvedPose;G_SetOrigin(&prop,solved);G_SetAngles(&prop,newAngles);CHECK(physicsWorld.CorrectPose(7,p));
 blockCharge=false;
 physicsActors(prop,old,oldAngles,p,.05f,false);
 CHECK(chargeChecks==2&&actor.currentOrigin[0]>0&&client.ps.velocity[0]>0&&prop.currentOrigin[0]<60);
 CHECK(det.currentOrigin[1]>46&&det.nextthink==1700&&det.linked);

 // A clear native-world trace still cannot let the staged actor and charge cross.
 G_SetOrigin(&prop,old);G_SetAngles(&prop,vec3_origin);gi.linkentity(&prop);
 const vec3_t frontCharge={-25,0,100};
 G_SetOrigin(&det,frontCharge);G_SandboxChargeAttach(det,prop);
 const vec3_t actorStart={0,0,100};
 G_SetOrigin(&actor,actorStart);VectorClear(client.ps.velocity);gi.linkentity(&actor);
 p=solvedPose;p.angles={};G_SetOrigin(&prop,solved);G_SetAngles(&prop,vec3_origin);
 CHECK(physicsWorld.CorrectPose(7,p));
 physicsActors(prop,old,vec3_origin,p,.05f,false);
 CHECK(chargeChecks==3&&VectorCompare(prop.currentOrigin,old)&&VectorCompare(actor.currentOrigin,actorStart));
 CHECK(VectorCompare(det.currentOrigin,frontCharge)&&det.nextthink==1700&&actor.linked&&det.linked);

 // The moving Box3D prop uses the same model hull for actor response.
 G_SandboxChargesReset();
 auto diagonal=std::make_shared<jar::physics::PropGeometry>();
 for(float z:{-8.f,8.f})for(float extent:{-64.f,64.f})for(float width:{-2.f,2.f})
  diagonal->Add({extent+width,extent-width,z});
 jar::physics::NativePropHull shape;
 CHECK(shape.Build(*diagonal,{}));testHull=&shape;
 VectorSet(prop.mins,-66,-66,-8);VectorSet(prop.maxs,66,66,8);
 VectorSet(actor.mins,-2,-2,-2);VectorSet(actor.maxs,2,2,2);
 VectorSet(old,0,0,60);VectorSet(prop.currentOrigin,0,0,-60);
 p.position=pv(prop.currentOrigin);p.angles={};p.linear={0,0,-500};p.angular={};
 CHECK(physicsWorld.Prop(7,1,{66,66,8},{},p,false,nullptr,diagonal));
 const vec3_t emptyCorner={45,-45,0};
 G_SetOrigin(&actor,emptyCorner);gi.linkentity(&actor);VectorClear(client.ps.velocity);
 physicsActors(prop,old,vec3_origin,p,.05f,false);
 CHECK(VectorCompare(actor.currentOrigin,emptyCorner)&&VectorCompare(client.ps.velocity,vec3_origin));
 CHECK(prop.currentOrigin[2]==-60);
 G_SetOrigin(&actor,vec3_origin);gi.linkentity(&actor);
 physicsActors(prop,old,vec3_origin,p,.05f,false);
 CHECK(client.ps.velocity[2]<0&&prop.currentOrigin[2]>-60);
 testHull=nullptr;
 });}
