// Native planting, prop attachment, collision and save boundaries.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_sandbox_charges.h"
#include "game/jar_physics.h"
#include "game/w_local.h"
#include "memory_save.h"

game_import_t gi{};
game_export_t globals{};
level_locals_t level{};
gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray& _TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static uint64_t lives[32]{};
static int controlled=-1,beamQueries=0,blocked=-1,explosions=0,boomCredit=-1;
static bool sweeping=false;
static bool useConvexDepth=false;
static vec3_t boomOrigin{};

uint64_t G_EntityLifetime(const gentity_t* e){return e&&e->inuse?lives[e->s.number]:0;}
bool G_JAR_WorkshopControls(const gentity_t* e){return e->s.number==controlled;}
bool G_JAR_WorkshopPropDepth(const gentity_t& fixed,const vec3_t origin,const vec3_t,
 const vec3_t,const JarSweepHull* hull,float& depth){
 if(!useConvexDepth)return false;
 CHECK(hull&&fixed.s.number==7);
 depth=1-origin[0];
 return true;
}
void G_SetOrigin(gentity_t* e,const vec3_t origin){
 VectorCopy(origin,e->currentOrigin);VectorCopy(origin,e->s.pos.trBase);VectorClear(e->s.pos.trDelta);
 e->s.pos.trType=TR_STATIONARY;e->s.pos.trTime=e->s.pos.trDuration=0;
}
#include "game/g_entity_angles.inc"
void WP_Stick(gentity_t* ent,trace_t* hit,float offset){
 vec3_t origin,angles;VectorMA(hit->endpos,offset,hit->plane.normal,origin);vectoangles(hit->plane.normal,angles);
 G_SetOrigin(ent,origin);G_SetAngles(ent,angles);gi.linkentity(ent);
}
void G_PlayEffect(int,const vec3_t,const vec3_t){}
void G_UseTargets(gentity_t*,gentity_t*){throw std::runtime_error("Unexpected output");}
void G_RadiusDamage(const vec3_t origin,gentity_t* attacker,float,float,gentity_t*,int){
 ++explosions;boomCredit=attacker->s.number;VectorCopy(origin,boomOrigin);
}

#define TRIPWIRE_STYLE 2
#include "game/wp_det_pack_stick.inc"
#include "game/wp_trip_mine_stick.inc"
#undef TRIPWIRE_STYLE

static gentity_t& Spawn(int slot,const char* classname,int weapon=WP_NONE){
 auto& e=g_entities[slot];e.inuse=e.linked=qtrue;e.s.number=slot;e.classname=const_cast<char*>(classname);
 e.s.weapon=weapon;e.s.groundEntityNum=ENTITYNUM_NONE;lives[slot]=1;return e;
}

int main(){return RunTests("native planted charge attachment, beam, explosion and save lifecycle",[]{
 globals.num_entities=32;level.time=1000;
 auto& shooter=Spawn(0,"player");auto& prop=Spawn(1,"jar_sandbox_prop");
 auto& det=Spawn(2,"detpack",WP_DET_PACK);auto& mine=Spawn(3,"tripmine",WP_TRIP_MINE);
 auto& authored=Spawn(4,"misc_trip_mine",WP_TRIP_MINE);
 prop.s.eType=ET_GENERAL;VectorSet(prop.currentOrigin,0,0,100);
 VectorSet(prop.mins,-10,-10,-10);VectorSet(prop.maxs,10,10,10);
 for(auto* charge:{&det,&mine,&authored}){charge->s.eType=ET_MISSILE;charge->s.eFlags=EF_MISSILE_STICK;charge->owner=&shooter;}
 gi.linkentity=[](gentity_t* e){e->linked=qtrue;};gi.unlinkentity=[](gentity_t* e){e->linked=qfalse;};
 gi.trace=[](trace_t* tr,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision mode,int){
  CHECK(mask==MASK_SHOT);*tr={};tr->fraction=1;tr->entityNum=ENTITYNUM_WORLD;VectorCopy(end,tr->endpos);
  if(mode==G2_RETURNONHIT){
   CHECK(pass==3);
   if(g_entities[1].linked){CHECK(start[0]>end[0]);tr->endpos[0]+=6;} // Native placement trace.
   else{++beamQueries;tr->fraction=.25f;vec3_t delta;VectorSubtract(end,start,delta);VectorMA(start,.25f,delta,tr->endpos);}
  }else{
   CHECK((VectorCompare(start,end)!=qfalse)!=sweeping);
   CHECK(!g_entities[1].linked&&!g_entities[2].linked&&!g_entities[3].linked);
   if(pass==blocked)tr->startsolid=qtrue;
  }
 };
 trace_t contact{};VectorSet(contact.endpos,0,0,90);VectorSet(contact.plane.normal,0,0,-1);
 charge_stick(&det,&prop,&contact);
 CHECK(det.s.groundEntityNum==1&&det.currentOrigin[2]==89&&det.activator==&shooter&&!det.owner&&det.nextthink==-1);
 VectorSet(contact.endpos,10,0,100);VectorSet(contact.plane.normal,1,0,0);
 mine.count=2;touchLaserTrap(&mine,&prop,&contact);mine.s.eFlags|=EF_FIRING;mine.s.loopSound=15;
 CHECK(mine.s.groundEntityNum==1&&mine.s.origin2[0]==16&&mine.activator==&shooter&&mine.e_ThinkFunc==thinkF_laserTrapThink);
 charge_stick(&authored,&prop,&contact);CHECK(authored.s.groundEntityNum==ENTITYNUM_NONE);
 det.e_ThinkFunc=thinkF_WP_Explode;det.nextthink=1500;det.splashDamage=90;det.splashRadius=160;
 const int mineDeadline=mine.nextthink;
 std::vector<jar::physics::MountedHull> hulls;
 G_SandboxChargesHulls(prop,hulls);
 CHECK(hulls.size()==2&&hulls[0].entity==2&&hulls[0].half.z==10&&hulls[0].center.z==-11&&
       hulls[1].entity==3&&hulls[1].half.x==4&&hulls[1].center.x==10);
 controlled=3;G_SandboxChargesHulls(prop,hulls);CHECK(hulls.size()==1);controlled=-1;

 VectorSet(prop.currentOrigin,20,30,100);VectorSet(prop.currentAngles,0,90,0);
 G_SandboxChargesFrame();
 CHECK(std::fabs(mine.currentOrigin[0]-20)<.001f&&std::fabs(mine.currentOrigin[1]-40)<.001f&&mine.movedir[1]>.999f);
 CHECK(std::fabs(mine.s.origin2[1]-46)<.001f&&mine.pos4[1]>500&&beamQueries==1&&prop.linked);
 CHECK(det.currentOrigin[0]==20&&det.currentOrigin[1]==30&&det.currentOrigin[2]==89&&det.nextthink==1500);
 G_SandboxChargesFrame();CHECK(beamQueries==1); // A stable prop does no beam work.

 {
  auto& other=Spawn(5,"jar_sandbox_prop");other.s.eType=ET_GENERAL;
  const auto previousTrace=gi.trace;
  static int scenario=0;
  gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
   *hit={};hit->fraction=1;hit->entityNum=ENTITYNUM_NONE;
   if(pass==1){
    CHECK(g_entities[1].linked&&!g_entities[2].linked&&!g_entities[3].linked);
    CHECK(mask==(MASK_SOLID|CONTENTS_BODY|SANDBOX_MOUNTED_CHARGE_CONTENTS));
    if(scenario==0){hit->fraction=.75f;hit->entityNum=ENTITYNUM_WORLD;}
   }else if(pass==5){
    CHECK(mask&SANDBOX_MOUNTED_CHARGE_CONTENTS);
    CHECK(g_entities[2].linked&&(g_entities[2].contents&SANDBOX_MOUNTED_CHARGE_CONTENTS));
    hit->fraction=.25f;hit->entityNum=2;
   }else{
    CHECK(mask==MASK_SHOT&&!g_entities[1].linked&&!g_entities[2].linked&&!g_entities[3].linked);
    if(pass==2&&scenario==0){hit->fraction=.5f;hit->entityNum=ENTITYNUM_WORLD;hit->plane.normal[2]=1;}
    if(pass==2&&scenario==1)hit->startsolid=hit->allsolid=qtrue;
    if(pass==2&&scenario==2&&!VectorCompare(start,end))hit->startsolid=qtrue;
   }
   vec3_t delta;VectorSubtract(end,start,delta);VectorMA(start,hit->fraction,delta,hit->endpos);
  };
  trace_t hit{};bool chargeContact=false;std::vector<SandboxChargeMove> step;
  const vec3_t target={36,30,84};
  {
   SandboxMountedChargeCollision collision;
   CHECK((det.contents&SANDBOX_MOUNTED_CHARGE_CONTENTS)&&(mine.contents&SANDBOX_MOUNTED_CHARGE_CONTENTS));
   CHECK(!(authored.contents&SANDBOX_MOUNTED_CHARGE_CONTENTS));
   CHECK(G_SandboxPropTrace(prop,target,hit,step,chargeContact)&&chargeContact&&hit.fraction==.375f&&hit.plane.normal[2]==1);
   CHECK(hit.endpos[0]==26&&hit.endpos[2]==94&&step.size()==2&&step[0].origin[2]==83&&step[1].beamStart[1]==46);
   CHECK(det.currentOrigin[2]==89&&prop.linked&&det.linked&&mine.linked);
   JarSweepHull hull{};hull.pointCount=8;
   gi.JAR_TraceMovingHull=[](trace_t* hit,const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs,
      int pass,int mask,const JarSweepHull& geometry){
    CHECK(pass==1&&geometry.pointCount==8&&!g_entities[2].linked&&!g_entities[3].linked);
    gi.trace(hit,start,mins,maxs,end,pass,mask,G2_NOCOLLIDE,0);
   };
   step.clear();
   CHECK(G_SandboxPropTrace(prop,target,hit,step,chargeContact,&hull)&&chargeContact&&hit.fraction==.375f);
   CHECK(hit.endpos[0]==26&&hit.endpos[2]==94&&step.size()==2);
   gi.JAR_TraceMovingHull=nullptr;
   step.clear();scenario=1;
   CHECK(!G_SandboxPropTrace(prop,target,hit,step,chargeContact)&&hit.allsolid&&prop.linked&&det.linked);
   step.clear();scenario=2;
   CHECK(G_SandboxPropTrace(prop,target,hit,step,chargeContact)&&!chargeContact&&hit.fraction==1&&step[0].origin[2]==73);
   step.clear();
   CHECK(G_SandboxPropTrace(other,target,hit,step,chargeContact)&&chargeContact&&hit.entityNum==2&&step.empty());
  }
  CHECK(!(det.contents&SANDBOX_MOUNTED_CHARGE_CONTENTS)&&!(mine.contents&SANDBOX_MOUNTED_CHARGE_CONTENTS));
  gi.trace=previousTrace;other.inuse=qfalse;
 }
 {
  auto& loose=Spawn(6,"jar_sandbox_prop");auto& neighbor=Spawn(7,"jar_sandbox_prop");
  for(auto* body:{&loose,&neighbor}){
   body->s.eType=ET_GENERAL;body->contents=CONTENTS_BODY;
   VectorSet(body->mins,-10,-10,-10);VectorSet(body->maxs,10,10,10);
  }
  neighbor.currentOrigin[0]=12;
  const auto previousTrace=gi.trace;
  static bool worldBlocked=false;
  gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** out,int){out[0]=&g_entities[7];return 1;};
  gi.trace=[](trace_t* hit,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision,int){
   CHECK(pass==6);
   *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
   if(g_entities[7].linked||worldBlocked)hit->startsolid=hit->allsolid=qtrue;
  };
  trace_t hit{};bool contact=false;std::vector<SandboxChargeMove> moves;
  vec3_t target={1,0,0};
  CHECK(!G_SandboxPropTrace(loose,target,hit,moves,contact)&&neighbor.linked);
  target[0]=-1;
  CHECK(G_SandboxPropTrace(loose,target,hit,moves,contact)&&neighbor.linked&&hit.endpos[0]==-1);
  worldBlocked=true;
  CHECK(!G_SandboxPropTrace(loose,target,hit,moves,contact)&&neighbor.linked);
  const auto previousHullTrace=gi.JAR_TraceMovingHull;
  gi.JAR_TraceMovingHull=[](trace_t* hit,const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs,
   int pass,int mask,const JarSweepHull&){
   gi.trace(hit,start,mins,maxs,end,pass,mask,G2_NOCOLLIDE,0);
  };
  worldBlocked=false;
  useConvexDepth=true;
  JarSweepHull hull{};
  // The fixture's hull separates in the opposite direction to its box.
  CHECK(!G_SandboxPropTrace(loose,target,hit,moves,contact,&hull)&&neighbor.linked);
  target[0]=1;
  CHECK(G_SandboxPropTrace(loose,target,hit,moves,contact,&hull)&&neighbor.linked&&hit.endpos[0]==1);
  useConvexDepth=false;
  gi.JAR_TraceMovingHull=previousHullTrace;
  gi.trace=previousTrace;gi.EntitiesInBox=nullptr;loose.inuse=neighbor.inuse=qfalse;
 }

 const vec3_t destination={300,0,20};
 std::vector<SandboxChargeMove> plan;
 CHECK(G_SandboxChargesPlan(prop,destination,plan)&&plan.size()==2);
 const float height=G_SandboxChargesFloorOffset(plan,10);CHECK(height==11);
 G_SandboxChargesRaise(plan,height);
 gi.unlinkentity(&prop);int budget=8;blocked=3;
 CHECK(!G_SandboxChargesClear(plan,budget)&&budget==6&&det.linked&&mine.linked);
 CHECK(det.currentOrigin[0]==20&&mine.currentOrigin[1]==40);
 blocked=-1;CHECK(G_SandboxChargesClear(plan,budget)&&budget==4);
 VectorSet(prop.currentOrigin,300,0,31);gi.linkentity(&prop);G_SandboxChargesCommit(plan);
 CHECK(det.currentOrigin[0]==300&&det.currentOrigin[2]==20&&mine.currentOrigin[1]==10&&mine.currentOrigin[2]==31);
 CHECK(det.s.eFlags&EF_TELEPORT_BIT);CHECK(mine.s.eFlags&EF_TELEPORT_BIT);
 CHECK(mine.nextthink==mineDeadline&&mine.s.loopSound==15&&det.activator==&shooter&&beamQueries==2&&prop.linked);

 MemorySave memory;ojk::SavedGameHelper save(&memory);G_SandboxChargesExport(false,save);
 G_SandboxChargesReset();memory.reset_buffer_offset();++lives[1];++lives[2];++lives[3];
 CHECK(G_SandboxChargesImport(false,save));plan.clear();
 G_SandboxChargesHulls(prop,hulls);
 CHECK(hulls.size()==2&&hulls[0].lifetime==2&&hulls[1].lifetime==2&&hulls[1].center.x==10);
 CHECK(G_SandboxChargesPlan(prop,prop.currentOrigin,plan)&&plan.size()==2&&plan.front().origin[2]==20);
 MemorySave malformed;ojk::SavedGameHelper bad(&malformed);
 bad.write<uint32_t>(2);
 for(int i=0;i<2;++i){
  bad.write<uint16_t>(2);bad.write<uint16_t>(1);bad.write<float>(vec3_origin);bad.write<float>(vec3_origin);bad.write<float>(vec3_origin);
 }
 malformed.reset_buffer_offset();CHECK(!G_SandboxChargesImport(false,bad));
 plan.clear();CHECK(G_SandboxChargesPlan(prop,prop.currentOrigin,plan)&&plan.size()==2);
 const vec3_t shifted={310,20,60},turned={0,180,0},pad={1,1,0};
 plan.clear();mine.svFlags|=SVF_NO_TELEPORT;
 CHECK(!G_SandboxChargesPlan(prop,shifted,plan)&&plan.empty());
 CHECK(!G_SandboxChargesPlanPose(prop,shifted,turned,plan,1)&&plan.empty());
 controlled=3;CHECK(!G_SandboxChargesPlanPose(prop,shifted,turned,plan,2)&&plan.empty());controlled=-1;
 CHECK(G_SandboxChargesPlanPose(prop,shifted,turned,plan,2)&&plan.size()==2&&plan[1].support==&prop);
 {
  SandboxChargesUnlinked unlinked(plan);gi.unlinkentity(&prop);sweeping=true;
  blocked=3;CHECK(!G_SandboxChargeFits(plan[1],mine.currentOrigin,pad));blocked=-1;
  CHECK(G_SandboxChargeFits(plan[1],mine.currentOrigin,pad)&&mine.currentOrigin[1]==10);
  sweeping=false;gi.linkentity(&prop);
 }
 const int flags=mine.s.eFlags;
 G_SandboxChargesPlace(plan);G_SandboxChargesRefresh(plan);
 CHECK(std::fabs(mine.currentOrigin[0]-300)<.001f&&std::fabs(mine.currentOrigin[1]-20)<.001f&&mine.movedir[0]<-.999f&&
       mine.s.eFlags==flags&&mine.nextthink==mineDeadline&&det.nextthink==1500);
 plan.clear();CHECK(G_SandboxChargesPlanPose(prop,prop.currentOrigin,prop.currentAngles,plan,2));
 G_SandboxChargesPlace(plan);G_SandboxChargesRefresh(plan);mine.svFlags&=~SVF_NO_TELEPORT;
 MemorySave autoMemory;ojk::SavedGameHelper autosave(&autoMemory);G_SandboxChargesExport(true,autosave);
 autoMemory.reset_buffer_offset();uint32_t autoCount=1;autosave.read<uint32_t>(autoCount);CHECK(!autoCount);autosave.ensure_all_data_read();

 WP_Explode(&det);CHECK(explosions==1&&boomCredit==0&&boomOrigin[0]==300&&boomOrigin[2]==20);
 G_SandboxChargesFrame();CHECK(det.s.groundEntityNum==ENTITYNUM_NONE);
 G_SandboxChargesHulls(prop,hulls);CHECK(hulls.size()==1&&hulls[0].entity==3);
 G_SandboxChargesForget(prop);CHECK(mine.s.groundEntityNum==ENTITYNUM_NONE&&mine.nextthink==mineDeadline);
 G_SandboxChargesHulls(prop,hulls);CHECK(hulls.empty());
 ++lives[1];VectorSet(prop.currentOrigin,900,0,90);G_SandboxChargesFrame();
 CHECK(mine.currentOrigin[0]==300&&mine.currentOrigin[2]==31);
});}
