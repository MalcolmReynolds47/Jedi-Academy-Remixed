#include "subbsp_mover_fixture.h"
#include "game/g_bsp_section_bob.inc"
int BMS_START=0,BMS_END=2;
int main(){return RunTests("native sub-BSP translating movers, oriented triggers and saved geometry",[]{
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;vec3_t lo,hi;if(e->bmodel)jar::volume::Hull(e->mins,e->maxs,e->currentAngles,lo,hi);else{VectorCopy(e->mins,lo);VectorCopy(e->maxs,hi);}VectorAdd(e->currentOrigin,lo,e->absmin);VectorAdd(e->currentOrigin,hi,e->absmax);};
 const vec3_t rotations[]={{0,0,0},{0,90,0},{30,45,20},{90,-35,12},{-75,123,-61},{0,-1,0},{0,-2,0}};
 for(const auto &rotation:rotations)for(const char*name:{"func_door","func_button"})for(const char*angle:{"0","37","-1","-2"})for(int flags:{0,1}){
  scenario=std::string(name)+" angle "+angle+" flags "+std::to_string(flags)+" rotation "+std::to_string(rotation[0])+","+std::to_string(rotation[1]);
  auto*r=root(rotation);std::string keys="\"angle\" \""+std::string(angle)+"\" \"lip\" \"2\" \"modelAngles\" \"10 30 40\"";auto*e=child(r,name,keys.c_str(),flags);
  if(!strcmp(name,"func_door"))SP_func_door(e);else SP_func_button(e);
  vec3_t local,world,origin;const float yaw=float(atof(angle));if(yaw==-1||yaw==-2)VectorSet(local,0,0,yaw==-1?1:-1);else{vec3_t a={0,yaw,0};AngleVectors(a,local,nullptr,nullptr);}rotate(rotation,local,world);
  const float distance=fabs(local[0])*80+fabs(local[1])*8+fabs(local[2])*24-2;vec3_t localOrigin={7,11,13};point(*r,localOrigin,origin);
  vec3_t end;VectorMA(origin,distance,world,end);const bool reverse=!strcmp(name,"func_door")&&flags==1;same(e->pos1,reverse?end:origin);same(e->pos2,reverse?origin:end);same(e->movedir,world);
  sameAxes(e->currentAngles,rotation);sameAxes(e->s.apos.trBase,rotation);sameAxes(e->s.angles,rotation);CHECK(abs(e->s.pos.trDuration-int(distance*1000/e->speed))<=1);
  vec3_t appearance,modelAxis[3],baseAxis[3],expected[3];VectorAdd(e->currentAngles,e->modelAngles,appearance);AnglesToAxis(appearance,modelAxis);AnglesToAxis(rotation,baseAxis);vec3_t localAppearance={10,30,40};AnglesToAxis(localAppearance,expected);
  for(int a=0;a<3;++a){vec3_t v{};for(int b=0;b<3;++b)VectorMA(v,expected[a][b],baseAxis[b],v);same(v,modelAxis[a]);}
  trajectory(*e);
 }
 // Platform height follows section-local Z; both explicit and inferred height.
 for(const auto &rotation:rotations)for(bool explicitHeight:{false,true}){
  scenario="platform";
  auto*r=root(rotation);auto*e=child(r,"func_plat",explicitHeight?"\"height\" \"80\"":"");{G_SubBSPCapture capture(e);SP_func_plat(e);}auto*t=lastSpawn;
  CHECK(t!=e&&t->owner==e&&G_SubBSPOwner(t)==r&&t->e_TouchFunc==touchF_Touch_PlatCenterTrigger);
  vec3_t travel={0,0,explicitHeight?80.f:16.f},expected,actual;rotate(rotation,travel,expected);VectorSubtract(e->pos2,e->pos1,actual);same(actual,expected);sameAxes(e->currentAngles,rotation);same(t->currentOrigin,e->pos1);
  CHECK(t->pos1[0]==-7&&t->pos2[0]==7&&t->pos1[1]==0&&t->pos2[1]==1&&t->pos1[2]==-12&&t->pos2[2]==20);
  gentity_t actor;gclient_t client{};actor.client=&client;VectorSet(actor.mins,-1,-1,-1);VectorSet(actor.maxs,1,1,1);vec3_t inside={0,.5f,0};point(*t,inside,actor.currentOrigin);Touch_PlatCenterTrigger(t,&actor,nullptr);CHECK(uses==1);
  vec3_t outside={50,50,50};point(*t,outside,actor.currentOrigin);Touch_PlatCenterTrigger(t,&actor,nullptr);CHECK(uses==1);trajectory(*e);
 }
 // Door team bounds use actual local geometry, not the renderer/collision sphere.
 auto*r=root(rotations[2]);auto*e=child(r,"func_door");SP_func_door(e);auto*part=child(r,"func_door");SP_func_door(part);e->teamchain=part;vec3_t offset={90,0,0},delta;rotate(r->currentAngles,offset,delta);VectorAdd(part->currentOrigin,delta,part->currentOrigin);e->takedamage=qtrue;
 {G_SubBSPCapture capture(e);Think_SpawnNewDoorTrigger(e);}auto*t=lastSpawn;CHECK(matched==1&&part->takedamage&&G_SubBSPOwner(t)==r);CHECK(near(t->pos1[0],-41)&&near(t->pos2[0],131)&&near(t->pos1[1],-125)&&near(t->pos2[1],125)&&near(t->pos1[2],-13)&&near(t->pos2[2],13));
 gentity_t actor;VectorSet(actor.mins,-1,-1,-1);VectorSet(actor.maxs,1,1,1);vec3_t inside={0,100,0};point(*t,inside,actor.currentOrigin);Touch_DoorTrigger(t,&actor,nullptr);CHECK(uses==1);
 e->spawnflags|=MOVER_LOCKED;Touch_DoorTrigger(t,&actor,nullptr);CHECK(uses==1);e->spawnflags=0;t->svFlags|=SVF_INACTIVE;Touch_DoorTrigger(t,&actor,nullptr);CHECK(uses==1);t->svFlags&=~SVF_INACTIVE;
 // Reject a point in the linked AABB's corner but outside the tilted volume.
 for(int i=0;i<3;++i)actor.currentOrigin[i]=t->absmax[i]-.1f;CHECK(!jar::subBSPMover::Touches(*t,actor));Touch_DoorTrigger(t,&actor,nullptr);CHECK(uses==1);
 // OBB/AABB contact, including a face/edge overlap with the actor center outside.
 vec3_t origin{},angles={0,45,0},lo={-20,-3,-4},hi={20,3,4};gentity_t volume;VectorCopy(angles,volume.currentAngles);jar::subBSPMover::Trigger(volume,volume,lo,hi);VectorSet(actor.mins,-2,-2,-2);VectorSet(actor.maxs,2,2,2);vec3_t edge={0,5,0};point(volume,edge,actor.currentOrigin);CHECK(jar::subBSPMover::Touches(volume,actor));edge[1]=7;point(volume,edge,actor.currentOrigin);CHECK(!jar::subBSPMover::Touches(volume,actor));
 // Native fields used by the trigger survive the actual entity save serializer.
 MemorySave save;ojk::SavedGameHelper saved(&save);t->sg_export(saved);save.reset_buffer_offset();gentity_t restored;restored.sg_import(saved);saved.ensure_all_data_read();same(restored.pos1,t->pos1);same(restored.pos2,t->pos2);same(restored.currentAngles,t->currentAngles);same(restored.currentOrigin,t->currentOrigin);
 // Pointer/string relocation belongs to full ReadGEntities, outside this fixture.
 restored.model=t->model;point(*t,inside,actor.currentOrigin);CHECK(jar::subBSPMover::Touches(restored,actor));G_FreeEntity(r);CHECK(!t->inuse&&!e->inuse&&!part->inuse);
 // Nested sections use their immediate composed frame, and clean up together.
 r=root(rotations[2]);auto*nested=child(r,"misc_bsp","\"angles\" \"15 70 25\"");nested->s.modelindex=2;CHECK(G_SubBSPBegin(nested));e=child(nested,"func_plat","\"height\" \"80\"");{G_SubBSPCapture capture(e);SP_func_plat(e);}t=lastSpawn;vec3_t vertical={0,0,80},worldVertical,actualTravel;rotate(nested->currentAngles,vertical,worldVertical);VectorSubtract(e->pos2,e->pos1,actualTravel);same(actualTravel,worldVertical);CHECK(G_SubBSPOwner(e)==nested&&G_SubBSPOwner(t)==nested);G_FreeEntity(r);CHECK(!nested->inuse&&!e->inuse&&!t->inuse);
 // Bobbing axes are section-local. Pause/resume retains the transformed path.
 for(const auto &rotation:rotations)for(int axis=0;axis<3;++axis)for(bool off:{false,true}){
  scenario="bobbing "+std::to_string(axis)+" off "+std::to_string(off);
  r=root(rotation);e=child(r,"func_bobbing","\"height\" \"32\" \"phase\" \"0.25\"",(axis==0?1:axis==1?2:0)|(off?4:0));e->targetname="toggle";SP_func_bobbing(e);vec3_t local{},expected;local[axis]=32;rotate(rotation,local,expected);same(e->s.pos.trDelta,expected);CHECK(e->s.pos.trDuration==4000&&e->s.pos.trType==(off?TR_INTERPOLATE:TR_SINE));vec3_t at;EvaluateTrajectory(&e->s.pos,level.time,at);same(at,e->currentOrigin);
  if(off){func_bobbing_use(e,nullptr,nullptr);EvaluateTrajectory(&e->s.pos,level.time,at);same(at,e->currentOrigin);}level.time+=750;EvaluateTrajectory(&e->s.pos,level.time,e->currentOrigin);func_bobbing_use(e,nullptr,nullptr);VectorCopy(e->currentOrigin,at);level.time+=600;func_bobbing_use(e,nullptr,nullptr);vec3_t resumed;EvaluateTrajectory(&e->s.pos,level.time,resumed);same(at,resumed);
 }
 // Portable root moves retain the complete native bobbing path, rather than
 // treating its current origin as the next cycle's rest point.
 for(const auto &rotation:rotations)for(bool paused:{false,true}){
  scenario="portable section bobbing rebase";
  r=root(rotation);
  e=child(r,"func_bobbing","\"height\" \"20\" \"speed\" \"3\" \"phase\" \"0.25\" \"angles\" \"17 21 13\" \"modelAngles\" \"10 30 40\"");
  SP_func_bobbing(e);level.time+=230;EvaluateTrajectory(&e->s.pos,level.time,e->currentOrigin);
  if(paused)func_bobbing_use(e,nullptr,nullptr);
  const auto trajectory=e->s.pos;const float phase=e->radius;
  vec3_t oldPose,oldOrigin,oldAppearance;
  VectorCopy(e->currentOrigin,oldPose);VectorCopy(e->s.origin,oldOrigin);VectorAdd(e->currentAngles,e->modelAngles,oldAppearance);
  vec3_t target={-1400,2600,790},targetAngles={55,-25,72};
  auto expectedPoint=[&](const vec3_t input,vec3_t output){
   vec3_t oldBasis[3],local,delta;AnglesToAxis(rotation,oldBasis);VectorSubtract(input,r->currentOrigin,delta);
   for(int i=0;i<3;++i)local[i]=DotProduct(delta,oldBasis[i]);rotate(targetAngles,local,output);VectorAdd(output,target,output);
  };
  CHECK(BSPSectionRebaseBobbing(*e,r->currentOrigin,rotation,target,targetAngles));
  CHECK(e->linked&&e->s.pos.trType==trajectory.trType&&e->s.pos.trTime==trajectory.trTime&&e->s.pos.trDuration==trajectory.trDuration&&e->radius==phase);
  vec3_t expected;expectedPoint(oldPose,expected);same(e->currentOrigin,expected);expectedPoint(oldOrigin,expected);same(e->s.origin,expected);
  for(int offset:{0,350,1000,2000}){
   vec3_t before,after;EvaluateTrajectory(&trajectory,level.time+offset,before);EvaluateTrajectory(&e->s.pos,level.time+offset,after);
   expectedPoint(before,expected);same(after,expected);
  }
  vec3_t oldBasis[3],oldVisible[3],newVisible[3],appearance;
  AnglesToAxis(rotation,oldBasis);AnglesToAxis(oldAppearance,oldVisible);
  VectorAdd(e->currentAngles,e->modelAngles,appearance);AnglesToAxis(appearance,newVisible);
  for(int i=0;i<3;++i){vec3_t local;for(int a=0;a<3;++a)local[a]=DotProduct(oldVisible[i],oldBasis[a]);rotate(targetAngles,local,expected);same(newVisible[i],expected);}
  if(paused){level.time+=510;func_bobbing_use(e,nullptr,nullptr);vec3_t resumed;EvaluateTrajectory(&e->s.pos,level.time,resumed);same(resumed,e->currentOrigin,.06f);same(e->s.pos.trBase,e->s.origin);}
  // A pure translation changes neither amplitude nor orientation.
  vec3_t delta={64,-91,7},shift,originalDelta,originalAngles;
  VectorAdd(target,delta,shift);VectorCopy(e->s.pos.trDelta,originalDelta);VectorCopy(e->currentAngles,originalAngles);VectorAdd(e->currentOrigin,delta,expected);
  CHECK(BSPSectionRebaseBobbing(*e,target,targetAngles,shift,targetAngles));same(e->currentOrigin,expected);same(e->s.pos.trDelta,originalDelta);same(e->currentAngles,originalAngles);
  CHECK(!BSPSectionRebaseBobbing(*r,target,targetAngles,shift,targetAngles));
 }
 // Ordinary authored maps and workshop box stand-ins retain native behavior.
 root(origin);e=child(nullptr,"func_door","\"angle\" \"90\"");SP_func_door(e);CHECK(near(e->pos2[1]-e->pos1[1],0));VectorSet(e->absmin,0,0,0);VectorSet(e->absmax,40,8,80);Think_SpawnNewDoorTrigger(e);t=lastSpawn;CHECK(!t->model&&t->mins[1]==-120&&t->maxs[1]==128);
 r=root(rotations[2]);e=child(r,"func_plat");e->model="@jar_box";SP_func_plat(e);CHECK(e->pos2[2]-e->pos1[2]==16&&e->pos1[0]==e->pos2[0]&&!lastSpawn->model);
 });}
