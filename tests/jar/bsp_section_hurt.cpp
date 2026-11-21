// Native hurt spawn/touch and server contact with recorded combat and link
// imports. The retail-admission fixture validates the raw brush separately.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/bg_local.h"
#include "cgame/cg_local.h"
#include "../../shared/qcommon/jar_sweep_hull.h"
#include "memory_save.h"

game_import_t gi{};
game_export_t globals{};
level_locals_t level{};
cg_t cg{};
gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray& _TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static gentity_t* owner=nullptr;
static int damageCalls=0,soundCalls=0,soundRegisters=0,legacyContacts=0;
static int brushPadTraces=0;
static int brushContactMask=CONTENTS_TRIGGER;
static int amount=0,flags=0,means=0;
gentity_t* G_SubBSPOwner(const gentity_t*){return owner;}
static uint64_t npcLife=1;
uint64_t G_EntityLifetime(const gentity_t* e){return e&&e->inuse?(e==&g_entities[6]?npcLife:1):0;}
void G_UseTargets(gentity_t*,gentity_t*){throw std::runtime_error("Unexpected trigger output");}
gentity_t* G_Find(gentity_t*,int field,const char* name){
 if(field!=FOFS(targetname))return nullptr;
 for(int slot=9;slot<globals.num_entities;++slot){
  auto& target=g_entities[slot];if(target.inuse&&target.targetname&&!strcmp(name,target.targetname))return &target;
 }
 return nullptr;
}
bool G_JAR_WorkshopControls(const gentity_t*){return false;}
static int constrainedVehicleMember=-1;
bool G_JAR_WorkshopTransportAllowed(const gentity_t& member){return member.s.number!=constrainedVehicleMember;}
static int groupTeleportCalls=0;
bool G_JAR_WorkshopTeleportAssembly(gentity_t&,gentity_t&){++groupTeleportCalls;return false;}
static int telefrags=0;
void G_KillBox(gentity_t*){++telefrags;}
void SetClientViewAngle(gentity_t* actor,vec3_t angles){VectorCopy(angles,actor->client->ps.viewangles);}
void PlayerStateToEntityState(playerState_t* ps,entityState_t* state){
 VectorCopy(ps->origin,state->pos.trBase);VectorCopy(ps->velocity,state->pos.trDelta);state->eFlags=ps->eFlags;
}
void EvaluateTrajectoryDelta(const trajectory_t* path,int time,vec3_t velocity){
 VectorCopy(path->trDelta,velocity);
 if(path->trType==TR_GRAVITY)velocity[2]-=800.f*(time-path->trTime)*.001f;
}
qboolean G_ActivateBehavior(gentity_t*,int){return qfalse;}
void JET_FlyStart(gentity_t*){throw std::runtime_error("Unexpected jet activation");}
void CGCam_Fade(vec4_t,vec4_t,float){throw std::runtime_error("Unexpected fade override");}
void G_Sound(gentity_t*,int sound){CHECK(sound==5);++soundCalls;}
int G_SoundIndex(const char* sound){CHECK(!strcmp(sound,"sound/world/electro"));++soundRegisters;return 5;}
qboolean G_SpawnString(const char*,const char* fallback,char** out){*out=const_cast<char*>(fallback);return qfalse;}
void InitTrigger(gentity_t* e){e->bmodel=qtrue;e->s.modelindex=3;e->contents=CONTENTS_TRIGGER;e->svFlags=SVF_NOCLIENT;}
void G_Damage(gentity_t* victim,gentity_t* inflictor,gentity_t* attacker,
              const vec3_t,const vec3_t,int n,int f,int m,int){
    CHECK(inflictor==attacker);
    ++damageCalls;amount=n;flags=f;means=m;victim->health-=n;
}
clipHandle_t SV_ClipHandleForEntity(const gentity_t*){++legacyContacts;return 1;}
void CM_TransformedBoxTrace(trace_t* trace,const vec3_t,const vec3_t,const vec3_t,const vec3_t,
                           clipHandle_t model,int mask,const vec3_t,const vec3_t){
    CHECK(model==1&&mask==-1);
    trace->startsolid=qtrue;
}
void CM_HullTrace(trace_t* trace,const vec3_t start,const vec3_t end,clipHandle_t model,int mask,
                  const vec3_t origin,const vec3_t angles,const JarSweepHull& hull){
    CHECK(model==1&&mask==brushContactMask&&hull.pointCount==8&&VectorCompare(start,end));
    CHECK(start[0]==10&&start[1]==20&&start[2]==38&&origin[0]==42&&angles[YAW]==35);
    CHECK(hull.points[0][0]==-16&&hull.points[7][2]==32);
    trace->startsolid=qtrue;
    ++brushPadTraces;
}
#include "game/g_contact_clocks.inc"
#include "game/g_contact_trigger.inc"
#include "game/g_sandbox_push.inc"
#include "game/g_sandbox_teleport.inc"
#include "game/g_native_teleport.inc"
#include "game/g_trigger_hurt.inc"
#include "server/sv_entity_contact.inc"

constexpr float USE_DISTANCE=64.f;
static qboolean ValidUseTarget(gentity_t* marker){return marker->svFlags&SVF_PLAYER_USABLE?qtrue:qfalse;}
void GEntity_UseFunc(gentity_t* self,gentity_t*,gentity_t* activator){
 if(G_PushKind(*self)==jar::push::Marker)G_PushUse(*self,activator);
 else CHECK(G_TeleportUse(*self,activator));
}
#include "game/g_vehicle_transport_use.inc"

static cvar_t fixtureGravity{};
cvar_t* g_gravity=&fixtureGravity;
pml_t pml{};
static int rollImpacts=0,rollPortalQueries=0;
void G_MissileImpact(gentity_t* missile,trace_t*,int=0){++rollImpacts;missile->s.eType=ET_GENERAL;}
bool G_JAR_WorkshopTeleportProjectile(gentity_t& missile,const vec3_t previous,const vec3_t end,bool clipped,
 const float* rollingVelocity){
 ++rollPortalQueries;
 return G_TeleportSweepProjectile(missile,previous,end,clipped,rollingVelocity);
}
#include "game/g_missile_roll.inc"

static void Place(gentity_t& e,const vec3_t point,const vec3_t angles){
    VectorCopy(point,e.currentOrigin);
    VectorCopy(angles,e.currentAngles);
    VectorCopy(point,e.s.pos.trBase);
    VectorCopy(angles,e.s.apos.trBase);
    G_BSPSectionRefreshBoxBounds(e);
}
static qboolean Sample(gentity_t& hazard,const vec3_t center,const vec3_t half){
    vec3_t lo,hi;VectorSubtract(center,half,lo);VectorAdd(center,half,hi);
    return SV_EntityContact(lo,hi,&hazard);
}

int main(){return RunTests("portable stock hurt, native contact/damage and save",[]{
    gi.linkentity=[](gentity_t* e){e->linked=qtrue;};
    gi.unlinkentity=[](gentity_t* e){e->linked=qfalse;};
    gi.bIsFromZone=[](const void*,memtag_t)->qboolean{return qfalse;};
    gentity_t root{};root.inuse=root.bmodel=qtrue;root.classname="misc_bsp";root.model="#mp/duel5";
    root.e_UseFunc=useF_target_teleporter_use;
    gentity_t hazard{};hazard.inuse=qtrue;hazard.classname="trigger_hurt";hazard.model="*1";
    hazard.damage=50;hazard.wait=1;
    owner=&root;SP_trigger_hurt(&hazard);
    CHECK(hazard.e_TouchFunc==touchF_hurt_touch&&hazard.linked&&hazard.wait==1000&&!soundRegisters);
    const vec3_t localLo={-60,-12,-8},localHi={60,12,8},zero={0,0,0};
    const vec3_t malformed={60,0,8};
    CHECK(!G_BSPSectionInitializeHurt(hazard,malformed,localHi)&&hazard.bmodel&&hazard.linked);
    CHECK(G_BSPSectionInitializeHurt(hazard,localLo,localHi)&&hazard.linked&&!hazard.bmodel);
    CHECK(hazard.s.eType==ET_INVISIBLE&&!hazard.s.modelindex&&hazard.damage==50&&hazard.wait==1000);
    CHECK(VectorCompare(hazard.mins,localLo)&&VectorCompare(hazard.maxs,localHi));

    // At this pose the outside actor box overlaps all six face projections
    // and the broad AABB. A cross-product axis separates the two boxes.
    const vec3_t tilted={29,43,23},edge={-52,-52,14},actorHalf={5,8,14};
    Place(hazard,zero,tilted);
    for(int i=0;i<3;++i)CHECK(edge[i]+actorHalf[i]>hazard.mins[i]&&edge[i]-actorHalf[i]<hazard.maxs[i]);
    CHECK(!Sample(hazard,edge,actorHalf)&&Sample(hazard,zero,actorHalf)&&!legacyContacts);

    // Non-centered stock brush bounds are relative to the section, not a
    // separate brush entity origin. Relinking after the pose changes keeps
    // both the broad hull and exact contact in the same frame.
    VectorSet(hazard.s.origin2,-4672,1856,-1344);VectorSet(hazard.s.angles2,-4224,2336,-144);
    const vec3_t offset={1250,-430,95},localCenter={-4448,2096,-744};
    Place(hazard,offset,tilted);
    vec3_t axes[3],sample;AnglesToAxis(tilted,axes);VectorCopy(offset,sample);
    for(int i=0;i<3;++i)VectorMA(sample,localCenter[i],axes[i],sample);
    CHECK(Sample(hazard,sample,actorHalf)&&!Sample(hazard,zero,actorHalf));
    hazard.svFlags|=SVF_INACTIVE;
    CHECK(!Sample(hazard,sample,actorHalf)&&!legacyContacts);
    hazard.svFlags&=~SVF_INACTIVE;
    gi.unlinkentity(&hazard);
    CHECK(!Sample(hazard,sample,actorHalf)&&!legacyContacts);
    gi.linkentity(&hazard);

    gclient_t client{};gentity_t player{};player.inuse=player.takedamage=qtrue;player.client=&client;
    player.health=200;
    // The swept trigger query must use its sampled hull, not the player's
    // present origin, which can already lie on the far side of the volume.
    VectorSet(player.currentOrigin,17000,19000,21500);
    trace_t touch{};
    level.time=1100;
    if(Sample(hazard,sample,actorHalf))hurt_touch(&hazard,&player,&touch);
    CHECK(damageCalls==1&&amount==50&&flags==0&&means==MOD_TRIGGER_HURT&&player.health==150);
    level.time=2099;hurt_touch(&hazard,&player,&touch);CHECK(damageCalls==1);
    level.time=2100;hurt_touch(&hazard,&player,&touch);CHECK(damageCalls==2&&player.health==100&&!soundCalls);

    MemorySave save;ojk::SavedGameHelper h(&save);
    hazard.sg_export(h);save.reset_buffer_offset();
    gentity_t restored{};restored.sg_import(h);h.ensure_all_data_read();
    CHECK(G_BSPSectionHurtTagged(restored)&&restored.e_TouchFunc==touchF_hurt_touch&&restored.damage==50);
    CHECK(restored.wait==1000&&restored.painDebounceTime==2100&&Sample(restored,sample,actorHalf));

    root.model="#mp/duel9";
    gentity_t fall{};fall.inuse=qtrue;fall.classname="trigger_hurt";fall.model="*1";fall.damage=-1;
    SP_trigger_hurt(&fall);
    CHECK(fall.spawnflags==4&&fall.linked);
    fall.spawnflags|=2;
    CHECK(!G_BSPSectionInitializeHurt(fall,localLo,localHi)&&fall.bmodel&&fall.linked);
    fall.spawnflags&=~2;
    CHECK(G_BSPSectionInitializeHurt(fall,localLo,localHi)&&fall.damage==Q3_INFINITE);
    CHECK(fall.spawnflags==(4|8|32)&&!fall.noise_index&&!soundRegisters);
    level.time=3200;hurt_touch(&fall,&player,&touch);
    CHECK(damageCalls==3&&amount==Q3_INFINITE&&means==MOD_FALLING&&flags==(DAMAGE_NO_PROTECTION|DAMAGE_NO_ARMOR));
    CHECK(player.health<0&&(client.ps.pm_flags&PMF_SLOW_MO_FALL)&&!soundCalls);

    // An unrelated authored hurt retains native sound and brush contact.
    owner=nullptr;gentity_t authored{};authored.inuse=qtrue;authored.classname="trigger_hurt";
    SP_trigger_hurt(&authored);
    CHECK(soundRegisters==1&&authored.noise_index==5&&authored.damage==5&&authored.bmodel);
    CHECK(!G_BSPSectionHurtContact(authored,localLo,localHi)&&Sample(authored,zero,actorHalf)&&legacyContacts==1);
    gentity_t angledPad{};
    angledPad.inuse=angledPad.bmodel=qtrue;
    angledPad.s.time2=BSPSectionBrushPadTag;
    angledPad.contents=CONTENTS_TRIGGER;
    angledPad.svFlags=SVF_NOCLIENT;
    VectorSet(angledPad.currentOrigin,42,1,2);
    VectorSet(angledPad.currentAngles,10,35,0);
    const vec3_t foot={-6,4,6},head={26,36,70};
    CHECK(SV_EntityContact(foot,head,&angledPad)&&brushPadTraces==1);
    angledPad.s.time2=BSPSectionBrushHurtTag;
    CHECK(SV_EntityContact(foot,head,&angledPad)&&brushPadTraces==2);
    angledPad.s.time2=BSPSectionLavaTag;
    brushContactMask=CONTENTS_NODROP;
    CHECK(SV_EntityContact(foot,head,&angledPad)&&brushPadTraces==3);
    player.health=100;level.time=4300;hurt_touch(&authored,&player,&touch);
    CHECK(soundCalls==1&&damageCalls==4&&player.health==95);

    using namespace jar::contactTrigger;
    auto& volume=g_entities[4];volume.inuse=qtrue;volume.s.number=4;volume.classname="trigger_hurt";
    volume.model=const_cast<char*>(model);volume.contents=CONTENTS_TRIGGER;
    volume.e_UseFunc=useF_hurt_use;volume.e_TouchFunc=touchF_hurt_touch;
    VectorSet(volume.pos1,-40,-6,0);VectorSet(volume.pos2,40,6,24);volume.currentAngles[YAW]=45;
    CHECK(jar::volumeEntity::Refresh(volume));gi.linkentity(&volume);
    CHECK(G_ContactSettings(volume,{Hurt,0,500,0,Actors,true,true,25,Electrical}));
    auto& local=g_entities[0];auto& npc=g_entities[6];gclient_t localClient{},npcClient{};gNPC_t npcData{};
    local.client=&localClient;npc.client=&npcClient;npc.NPC=&npcData;npc.s.number=6;
    globals.num_entities=8;
    for(auto* actor:{&local,&npc}){
        actor->inuse=actor->takedamage=qtrue;actor->health=100;
        VectorSet(actor->mins,-3,-3,-2);VectorSet(actor->maxs,3,3,2);
        VectorSet(actor->currentOrigin,100,100,10);
    }
    const vec3_t miss={24,-24,10},hit={5,11,10};
    level.time=5000;G_ContactTouchAt(volume,&local,hit);CHECK(damageCalls==4);
    localClient.usercmd.buttons=npcClient.usercmd.buttons=BUTTON_USE;
    G_ContactTouchAt(volume,&local,miss);CHECK(damageCalls==4);
    G_ContactTouchAt(volume,&local,hit);G_ContactTouchAt(volume,&local,hit);
    CHECK(damageCalls==5&&local.health==75&&localClient.ps.powerups[PW_SHOCKED]==6000);
    level.time=5200;G_ContactTouchAt(volume,&npc,hit);
    CHECK(damageCalls==6&&npc.health==75&&npcClient.ps.powerups[PW_SHOCKED]==6200);

    MemorySave timers;ojk::SavedGameHelper timersIO(&timers);
    G_ContactClocksExport(false,timersIO);timers.reset_buffer_offset();G_ContactClocksReset();
    level.time=8000;CHECK(G_ContactClocksImport(false,timersIO));timersIO.ensure_all_data_read();
    level.time=8299;G_ContactTouchAt(volume,&local,hit);CHECK(damageCalls==6);
    level.time=8300;G_ContactTouchAt(volume,&local,hit);G_ContactTouchAt(volume,&npc,hit);
    CHECK(damageCalls==7&&local.health==50&&npc.health==75);
    hurt_use(&volume,nullptr,&local);CHECK(volume.linked&&!(volume.contents&CONTENTS_TRIGGER));
    level.time=8500;G_ContactTouchAt(volume,&npc,hit);CHECK(damageCalls==7);
    hurt_use(&volume,nullptr,&local);CHECK(volume.contents&CONTENTS_TRIGGER);
    CHECK(G_ContactSettings(volume,{Hurt,0,500,0,NPCs,false,true,25,Falling}));
    G_ContactTouchAt(volume,&npc,hit);CHECK(damageCalls==8&&npc.health==50&&means==MOD_FALLING&&flags==DAMAGE_NO_ARMOR);
    ++npcLife;G_ContactTouchAt(volume,&npc,hit);CHECK(damageCalls==9&&npc.health==25);
    level.time=9000;G_ContactTouchAt(volume,&local,hit);CHECK(damageCalls==9&&local.health==50);
    CHECK(soundCalls==1);
    MemorySave malformedTimers;ojk::SavedGameHelper malformedIO(&malformedTimers);
    malformedIO.write<uint32_t>(2);
    for(int i=0;i<2;++i){malformedIO.write<uint16_t>(4);malformedIO.write<uint16_t>(6);malformedIO.write<uint32_t>(500);}
    malformedTimers.reset_buffer_offset();
    CHECK(!G_ContactClocksImport(false,malformedIO));
    G_ContactClockReady(volume,npc,500);
    CHECK(!G_ContactClockReady(volume,npc,500));

    {
      using namespace jar::push;
      globals.num_entities=12;
      auto& pad=g_entities[8];pad.inuse=qtrue;pad.s.number=8;pad.classname="trigger_push";
      pad.model=const_cast<char*>(volumeModel);pad.contents=CONTENTS_TRIGGER;
      pad.e_TouchFunc=touchF_trigger_push_touch;pad.currentAngles[YAW]=45;
      VectorSet(pad.pos1,-40,-6,0);VectorSet(pad.pos2,40,6,24);
      auto& aim=g_entities[9];aim.inuse=qtrue;aim.targetname="fixture_aim";pad.target=aim.targetname;
      VectorSet(aim.currentOrigin,0,0,112);
      local.health=npc.health=100;
      CHECK(G_PushSet(pad,{Volume,500,300,jar::push::Actors,Linked,false,true}));
      level.time=10000;G_PushTouchAt(pad,&local,miss);CHECK(!VectorLengthSquared(localClient.ps.velocity));
      G_PushTouchAt(pad,&local,hit);CHECK(localClient.ps.velocity[2]==500&&(localClient.ps.pm_flags&PMF_TRIGGER_PUSHED));
      VectorSet(aim.currentOrigin,100,0,12);
      level.time=10100;G_PushTouchAt(pad,&local,hit);CHECK(localClient.ps.velocity[2]==500);
      G_PushTouchAt(pad,&npc,hit);CHECK(npcClient.ps.velocity[0]==500);
      level.time=10300;aim.inuse=qfalse;G_PushTouchAt(pad,&local,hit);
      CHECK(pad.inuse&&!pad.nextthink&&localClient.ps.velocity[2]==500);
      aim.inuse=qtrue;G_PushTouchAt(pad,&local,hit);CHECK(localClient.ps.velocity[0]==500);

      auto& marker=g_entities[10];marker.inuse=qtrue;marker.s.number=10;marker.classname="target_push";
      marker.model=const_cast<char*>(markerModel);marker.e_UseFunc=useF_Use_target_push;
      marker.currentAngles[YAW]=90;
      CHECK(G_PushSet(marker,{Marker,250,100,jar::push::Player,Forward,true,true}));
      PushActor(marker,local,local.currentOrigin,true);
      CHECK(std::fabs(localClient.ps.velocity[0]-500)<.1f&&std::fabs(localClient.ps.velocity[1]-250)<.1f);

      auto& prop=g_entities[11];prop.inuse=qtrue;prop.s.number=11;prop.classname="jar_sandbox_prop";
      VectorSet(prop.mins,-2,-2,-2);VectorSet(prop.maxs,2,2,2);
      VectorSet(prop.currentOrigin,40,-40,10);
      const vec3_t previous={-40,40,10},highStart={-40,40,40};
      gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** list,int){list[0]=&g_entities[8];return 1;};
      CHECK(G_PushSet(pad,{Volume,900,300,Props,Linked,false,true}));
      const auto deliver=[](gentity_t& e,const vec3_t velocity,bool rigid){CHECK(rigid);VectorCopy(velocity,e.s.pos.trDelta);};
      VectorSet(prop.currentOrigin,40,-40,40);
      G_PushPropSweep(prop,highStart,true,deliver);CHECK(!VectorLengthSquared(prop.s.pos.trDelta));
      VectorSet(prop.currentOrigin,40,-40,10);
      G_PushPropSweep(prop,previous,true,deliver);CHECK(prop.s.pos.trDelta[0]==900);
      VectorClear(prop.s.pos.trDelta);G_PushPropSweep(prop,previous,true,deliver);
      CHECK(!VectorLengthSquared(prop.s.pos.trDelta));
    }
    {
      using namespace jar::teleport;
      globals.num_entities=18;level.time=20000;
      auto& gate=g_entities[13];auto& exitGate=g_entities[14];auto& destination=g_entities[15];
      for(auto* e:{&gate,&exitGate}){
        e->inuse=qtrue;e->s.number=int(e-g_entities);e->classname="trigger_teleport";
        e->model=const_cast<char*>(volumeModel);e->e_TouchFunc=touchF_trigger_teleporter_touch;
        e->contents=CONTENTS_TRIGGER;e->currentAngles[YAW]=45;
        VectorSet(e->pos1,-40,-6,0);VectorSet(e->pos2,40,6,48);
        CHECK(G_TeleportSet(*e,{Volume,500,jar::teleport::Actors,true,true,true}));
      }
      destination.inuse=qtrue;destination.targetname="arrival";
      VectorSet(destination.currentOrigin,300,0,0);VectorSet(destination.currentAngles,0,15,0);
      VectorCopy(destination.currentOrigin,exitGate.currentOrigin);gate.target=exitGate.target=destination.targetname;
      local.health=100;VectorSet(local.mins,-3,-3,-24);VectorSet(local.maxs,3,3,24);
      VectorSet(localClient.ps.viewangles,10,90,0);VectorSet(localClient.ps.velocity,12,34,56);
      gi.trace=[](trace_t* tr,const vec3_t start,const vec3_t mins,const vec3_t,const vec3_t,int pass,int mask,EG2_Collision,int){
        CHECK(start[0]==300&&start[2]==25&&mins[2]==-24&&pass==0&&mask==MASK_PLAYERSOLID);
        *tr={};tr->fraction=1;tr->startsolid=qtrue;
      };
      CHECK(!G_TeleportTouchAt(gate,&local,hit)&&!G_TeleportArrivalPending(local)&&!telefrags);
      gi.trace=[](trace_t* tr,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*tr={};tr->fraction=1;};
      CHECK(!G_TeleportTouchAt(gate,&local,miss));
      constrainedVehicleMember=local.s.number;
      CHECK(!G_TeleportTouchAt(gate,&local,hit)&&groupTeleportCalls==1&&!G_TeleportArrivalPending(local));
      constrainedVehicleMember=-1;
      CHECK(G_TeleportTouchAt(gate,&local,hit)&&!telefrags&&G_TeleportArrivalPending(local));
      CHECK(local.currentOrigin[0]==300&&local.currentOrigin[2]==25&&local.lastOrigin[0]==300&&localClient.ps.viewangles[YAW]==90);
      CHECK(localClient.ps.velocity[1]==34&&local.s.pos.trDelta[1]==34&&localClient.ps.groundEntityNum==ENTITYNUM_NONE);
      level.time+=1000;CHECK(!G_TeleportTouchAt(exitGate,&local,local.currentOrigin));
      gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** list,int){list[0]=&g_entities[14];return 1;};
      G_TeleportRefreshArrival(local,local.currentOrigin);CHECK(G_TeleportArrivalPending(local));
      MemorySave arrivals;ojk::SavedGameHelper arrivalIO(&arrivals);
      G_TeleportArrivalsExport(false,arrivalIO);G_TeleportArrivalsReset();arrivals.reset_buffer_offset();
      CHECK(G_TeleportArrivalsImport(false,arrivalIO)&&G_TeleportArrivalPending(local));arrivalIO.ensure_all_data_read();
      const vec3_t outside={500,0,25};G_TeleportRefreshArrival(local,outside);CHECK(!G_TeleportArrivalPending(local));
      CHECK(G_TeleportTouchAt(exitGate,&local,local.currentOrigin));
      auto& marker=g_entities[16];marker.inuse=qtrue;marker.s.number=16;marker.classname="target_teleporter";
      marker.model=const_cast<char*>(markerModel);marker.target=destination.targetname;marker.e_UseFunc=useF_target_teleporter_use;
      CHECK(G_TeleportSet(marker,{Marker,500,jar::teleport::Player,false,false,true}));
      CHECK(!G_TeleportUse(marker,&npc));
      CHECK(!G_TeleportUse(marker,&local)); // Two different sources in one frame.
      ++level.time;
      constrainedVehicleMember=local.s.number;
      CHECK(!G_TeleportUse(marker,&local)&&groupTeleportCalls==1&&localClient.ps.viewangles[YAW]==90);
      constrainedVehicleMember=-1;
      CHECK(G_TeleportUse(marker,&local)&&localClient.ps.viewangles[YAW]==15&&!VectorLengthSquared(localClient.ps.velocity)&&!telefrags);
      TeleportPlayer(&local,destination.currentOrigin,destination.currentAngles);CHECK(telefrags==1);
    }
    {
      using namespace jar::teleport;
      globals.num_entities=24;level.time=30000;
      auto& near=g_entities[13];auto& far=g_entities[18];auto& destination=g_entities[15];auto& otherDest=g_entities[19];
      far=near;far.s.number=18;far.currentOrigin[0]=40;
      otherDest.inuse=qtrue;otherDest.targetname="far_arrival";otherDest.currentOrigin[0]=600;
      far.target=otherDest.targetname;destination.currentAngles[YAW]=90;
      CHECK(G_TeleportSet(near,{Volume,500,NoActors,false,true,true,true,true}));
      CHECK(G_TeleportSet(far,{Volume,500,NoActors,true,false,true,true,true}));
      gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** list,int){
        list[0]=&g_entities[18];list[1]=&g_entities[14];list[2]=&g_entities[13];return 3;
      };
      gi.trace=[](trace_t* tr,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*tr={};tr->fraction=1;};
      auto& missile=g_entities[20];missile.inuse=qtrue;missile.s.number=20;missile.s.eType=ET_MISSILE;
      missile.s.weapon=WP_ROCKET_LAUNCHER;missile.s.pos.trType=TR_LINEAR;missile.s.pos.trDelta[0]=900;
      missile.e_ThinkFunc=thinkF_rocketThink;missile.nextthink=31000;
      VectorSet(missile.mins,-2,-2,-2);VectorSet(missile.maxs,2,2,2);
      const vec3_t start={-60,0,10},end={100,0,10},wall={-20,0,10};
      VectorSet(missile.movedir,1,0,0);missile.enemy=&npc;
      CHECK(!G_TeleportSweepProjectile(missile,start,wall,true));
      CHECK(G_TeleportSweepProjectile(missile,start,end,true));
      CHECK(missile.currentOrigin[0]==300&&missile.currentOrigin[2]==3&&std::fabs(missile.s.pos.trDelta[1]-900)<.1f);
      CHECK(missile.s.pos.trTime==level.time&&missile.s.pos.trType==TR_LINEAR&&missile.nextthink==31000&&
            missile.e_ThinkFunc==thinkF_rocketThink&&(missile.s.eFlags&EF_TELEPORT_BIT)&&
            missile.movedir[YAW]>.999f&&std::fabs(missile.movedir[0])<.01f&&missile.enemy==&npc);
      CHECK(G_TeleportArrivalPending(missile));
      level.time+=500;G_TeleportRefreshArrival(missile,missile.currentOrigin);CHECK(G_TeleportArrivalPending(missile));

      auto& other=g_entities[21];other=missile;other.s.number=21;other.s.pos.trType=TR_GRAVITY;
      other.s.pos.trDelta[0]=700;other.s.pos.trDelta[1]=0;other.s.pos.trTime=level.time-100;
      gi.trace=[](trace_t* tr,const vec3_t pos,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){
        *tr={};tr->fraction=1;tr->startsolid=pos[0]==300?qtrue:qfalse;
      };
      CHECK(G_TeleportSweepProjectile(other,start,end,false));
      CHECK(other.currentOrigin[0]==600&&other.s.pos.trType==TR_GRAVITY&&other.s.pos.trDelta[0]==700&&
            std::fabs(other.s.pos.trDelta[2]+80)<.01f);
      other.s.eFlags|=EF_MISSILE_STICK;CHECK(!G_TeleportObjectKind(other));

      auto& cargo=g_entities[22];cargo.inuse=qtrue;cargo.s.number=22;cargo.classname="jar_sandbox_prop";
      VectorSet(cargo.mins,-4,-4,-4);VectorSet(cargo.maxs,4,4,4);
      CHECK(G_TeleportCrossings(cargo,start,end,false,true).empty());
      CHECK(G_TeleportSet(far,{Volume,500,NoActors,true,false,true,true,true,false,true}));
      const auto assemblyCrossings=G_TeleportCrossings(cargo,start,end,false,true);
      CHECK(assemblyCrossings.size()==1&&assemblyCrossings[0].source==&far&&assemblyCrossings[0].settings.assemblies);
      auto& deck=g_entities[24];deck.inuse=qtrue;deck.s.number=24;deck.classname="func_bobbing";
      deck.model=const_cast<char*>(JAR_BOB_PORTABLE);
      VectorCopy(cargo.mins,deck.mins);VectorCopy(cargo.maxs,deck.maxs);
      CHECK(G_TeleportCrossings(deck,start,end,false,true).empty());
      CHECK(G_TeleportSet(far,{Volume,500,NoActors,true,false,true,true,true,false,true,false,true}));
      CHECK(G_TeleportCrossings(deck,start,end,false,false).empty());
      const auto moverCrossings=G_TeleportCrossings(deck,start,end,false,true);
      CHECK(moverCrossings.size()==1&&moverCrossings[0].source==&far&&moverCrossings[0].settings.movers);
      const auto moveProp=[](gentity_t& e,const vec3_t pos,const gentity_t&,const jar::teleport::Settings& settings,bool rigid){
        CHECK(rigid&&!settings.keepVelocity);VectorCopy(pos,e.currentOrigin);return true;
      };
      CHECK(G_TeleportSweepObject(cargo,start,end,false,true,moveProp));
      CHECK(cargo.currentOrigin[0]==600&&cargo.currentOrigin[2]==5&&G_TeleportArrivalPending(cargo));
      auto& peer=g_entities[23];peer.inuse=qtrue;peer.s.number=23;peer.classname="jar_sandbox_prop";
      G_TeleportMarkArrival(peer);
      MemorySave objectArrivals;ojk::SavedGameHelper objectIO(&objectArrivals);
      G_TeleportArrivalsExport(false,objectIO);objectArrivals.reset_buffer_offset();G_TeleportArrivalsReset();
      CHECK(G_TeleportArrivalsImport(false,objectIO)&&G_TeleportArrivalPending(cargo)&&
            G_TeleportArrivalPending(peer)&&G_TeleportArrivalPending(missile));
    }
    {
      using namespace jar::teleport;
      globals.num_entities=30;level.time=40000;
      G_TeleportArrivalsReset();G_ContactClocksReset();
      auto& marker=g_entities[16];auto& gate=g_entities[13];auto& target=g_entities[15];
      auto& hull=g_entities[25];auto& droid=g_entities[26];
      gclient_t hullClient{},droidClient{};Vehicle_t vehicle{};vehicleInfo_t definition{};
      hull.inuse=hull.linked=qtrue;hull.s.number=25;hull.client=&hullClient;hull.health=1000;
      hullClient.NPC_class=CLASS_VEHICLE;hull.m_pVehicle=&vehicle;vehicle.m_pParentEntity=&hull;
      vehicle.m_pVehicleInfo=&definition;hull.crotchBolt=4;hull.playerModel=0;
      vehicle.m_pPilot=&local;hull.owner=&local;hull.s.m_iVehicleNum=1;
      local.inuse=local.linked=qtrue;local.health=100;local.owner=&hull;local.s.m_iVehicleNum=hull.s.number;
      localClient.ps.pm_type=PM_NORMAL;
      VectorSet(hull.mins,-30,-30,-10);VectorSet(hull.maxs,30,30,10);
      VectorClear(hull.currentOrigin);VectorSet(local.currentOrigin,0,0,20);
      VectorSet(target.currentOrigin,300,0,0);VectorSet(target.currentAngles,15,90,10);
      gi.G2API_GetBoltMatrix=[](CGhoul2Info_v&,int model,int bolt,mdxaBone_t* out,const vec3_t angles,
                                const vec3_t origin,int,qhandle_t*,const vec3_t){
        CHECK(model==0&&(bolt==4||bolt==5)&&origin[0]==300&&angles[YAW]==90);
        out->matrix[0][3]=origin[0]+(bolt==4?0.f:8.f);
        out->matrix[1][3]=origin[1];out->matrix[2][3]=origin[2]+(bolt==4?20.f:12.f);
        return qtrue;
      };
      gi.G2API_GiveMeVectorFromMatrix=[](mdxaBone_t& matrix,Eorientations mode,vec3_t& out){
        if(mode==ORIGIN)VectorSet(out,matrix.matrix[0][3],matrix.matrix[1][3],matrix.matrix[2][3]);
        else {CHECK(mode==NEGATIVE_Y);VectorSet(out,0,1,0);}
      };
      definition.type=VH_SPEEDER;
      CHECK(G_TeleportSet(marker,{Marker,500,jar::teleport::Actors,false,true,true}));
      CHECK(!G_TeleportUse(marker,&local));
      CHECK(G_TeleportSet(marker,{Marker,500,jar::teleport::Actors,false,true,true,false,false,true}));
      gi.trace=[](trace_t* tr,const vec3_t pos,const vec3_t,const vec3_t,const vec3_t,int pass,int mask,EG2_Collision,int){
        CHECK(!g_entities[0].linked&&!g_entities[25].linked&&mask==MASK_PLAYERSOLID&&pos[0]==300);
        *tr={};tr->fraction=1;tr->startsolid=pass==0?qtrue:qfalse;
      };
      CHECK(!G_TeleportUse(marker,&local)&&hull.currentOrigin[0]==0&&local.currentOrigin[0]==0&&hull.linked&&local.linked);
      constrainedVehicleMember=0;
      CHECK(!G_TeleportUse(marker,&local)&&hull.currentOrigin[0]==0&&local.linked);
      constrainedVehicleMember=-1;
      vehicle.m_iBoarding=100;
      CHECK(!G_TeleportUse(marker,&local)&&hull.currentOrigin[0]==0);
      vehicle.m_iBoarding=0;
      gi.trace=[](trace_t* tr,const vec3_t pos,const vec3_t,const vec3_t,const vec3_t,int pass,int mask,EG2_Collision,int){
        CHECK(!g_entities[0].linked&&!g_entities[25].linked&&!g_entities[26].linked);
        CHECK(mask==MASK_PLAYERSOLID&&(pass==25||pass==0||pass==26));
        CHECK(pos[0]==(pass==26?308:300)&&pos[2]==(pass==25?11:pass==0?31:23));
        *tr={};tr->fraction=1;
      };
      for(const auto type:{VH_SPEEDER,VH_WALKER,VH_ANIMAL,VH_FIGHTER}){
        definition.type=type;VectorClear(vehicle.m_vOrientation);VectorSet(hullClient.ps.velocity,400,0,0);
        VectorSet(vehicle.jarPushDrift,100,0,0);
        hullClient.ps.speed=400;vehicle.m_LandTrace.fraction=.2f;
        CHECK(G_TeleportUse(marker,&local)&&hull.linked&&local.linked);
        CHECK(hull.currentOrigin[0]==300&&local.currentOrigin[2]==31&&vehicle.m_pPilot==&local&&
              local.owner==&hull&&local.s.m_iVehicleNum==25&&hull.s.m_iVehicleNum==1);
        CHECK(vehicle.m_vOrientation[YAW]==90&&vehicle.m_vPrevOrientation[YAW]==90&&
              vehicle.m_vOrientation[PITCH]==(type==VH_FIGHTER?15:0)&&vehicle.m_LandTrace.fraction==1);
        CHECK(std::fabs(VectorLength(hullClient.ps.velocity)-400)<.1f&&hullClient.ps.velocity[1]>380&&
              hullClient.ps.speed==400&&G_TeleportArrivalPending(hull)&&G_TeleportArrivalPending(local));
        CHECK(vehicle.jarPushDrift[1]>96&&vehicle.jarPushDrift[2]==0);
        CHECK(!G_TeleportUse(marker,&local));
        level.time+=600;
      }
      droid.inuse=qtrue;droid.s.number=26;droid.client=&droidClient;droid.health=100;
      droid.owner=&hull;droid.s.m_iVehicleNum=25;vehicle.m_pDroidUnit=&droid;vehicle.m_iDroidUnitTag=5;
      CHECK(G_TeleportSet(marker,{Marker,500,jar::teleport::Actors,false,false,true,false,false,true}));
      CHECK(G_TeleportUse(marker,&local)&&droid.currentOrigin[0]==308&&!droid.linked&&
            droid.owner==&hull&&vehicle.m_pDroidUnit==&droid&&!hullClient.ps.speed&&
            !VectorLengthSquared(hullClient.ps.velocity)&&!VectorLengthSquared(vehicle.jarPushDrift));
      level.time+=600;G_TeleportArrivalsReset();
      const int priorGroupCalls=groupTeleportCalls;
      constrainedVehicleMember=local.s.number;
      CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,false,true,true,false,true,true}));
      CHECK(!G_TeleportTouchAt(gate,&hull,hit)&&groupTeleportCalls==priorGroupCalls+1);
      CHECK(!G_TeleportUse(marker,&local)&&groupTeleportCalls==priorGroupCalls+1);
      constrainedVehicleMember=-1;
      CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,false,true,false,false,true}));
      CHECK(G_TeleportTouchAt(gate,&hull,hit)&&G_TeleportArrivalPending(hull));
      MemorySave vehicleArrivals;ojk::SavedGameHelper vehicleIO(&vehicleArrivals);
      G_TeleportArrivalsExport(false,vehicleIO);G_TeleportArrivalsReset();vehicleArrivals.reset_buffer_offset();
      CHECK(G_TeleportArrivalsImport(false,vehicleIO)&&G_TeleportArrivalPending(hull)&&
            G_TeleportArrivalPending(local)&&G_TeleportArrivalPending(droid));
      vehicleIO.ensure_all_data_read();
      CHECK(telefrags==1); // Only the authored reference call used KillBox.

      using Push=jar::push::Settings;
      auto& push=g_entities[8];auto& pushMarker=g_entities[10];
      CHECK(G_PushKind(push)==jar::push::Volume&&G_PushKind(pushMarker)==jar::push::Marker);
      G_ContactClocksReset();
      vec3_t touchPoint{};VectorCopy(push.currentOrigin,touchPoint);
      VectorSet(push.currentAngles,0,90,0);VectorSet(pushMarker.currentAngles,0,90,0);
      VectorSet(hullClient.ps.velocity,100,0,0);VectorClear(vehicle.jarPushDrift);
      hullClient.ps.speed=100;VectorClear(localClient.ps.velocity);
      CHECK(G_PushSet(push,Push{jar::push::Volume,500,300,jar::push::Props,jar::push::Forward,false,true}));
      G_PushTouchAt(push,&hull,touchPoint);CHECK(hullClient.ps.velocity[0]==100&&hullClient.ps.speed==100);
      CHECK(G_PushSet(push,Push{jar::push::Volume,500,300,jar::push::Props,jar::push::Forward,false,true,true}));
      G_PushTouchAt(push,&local,touchPoint);CHECK(hullClient.ps.velocity[0]==100);
      constrainedVehicleMember=26;
      G_PushTouchAt(push,&hull,touchPoint);CHECK(hullClient.ps.velocity[0]==100);
      constrainedVehicleMember=-1;
      G_PushTouchAt(push,&hull,touchPoint);
      CHECK(hullClient.ps.velocity[1]==500&&hullClient.ps.speed==0&&vehicle.jarPushDrift[1]==500&&hull.s.pos.trDelta[1]==500);
      CHECK(!VectorLengthSquared(localClient.ps.velocity)&&local.owner==&hull&&vehicle.m_pPilot==&local&&vehicle.m_pDroidUnit==&droid);
      VectorSet(hullClient.ps.velocity,100,500,0);hullClient.ps.speed=100;
      G_PushTouchAt(push,&hull,touchPoint);CHECK(hullClient.ps.velocity[0]==100);
      CHECK(G_PushSet(pushMarker,Push{jar::push::Marker,250,100,jar::push::Player,jar::push::Forward,true,true,true}));
      G_PushUse(pushMarker,&local);
      CHECK(std::fabs(hullClient.ps.velocity[0]-100)<.001f&&hullClient.ps.velocity[1]==750&&
            hullClient.ps.speed==100&&vehicle.jarPushDrift[1]==750&&vehicle.jarPushDrift[2]==0);
      G_PushUse(pushMarker,&hull);CHECK(hullClient.ps.velocity[1]==750); // Same hull cooldown via either caller.
      level.time+=500;
      vehicle.m_iBoarding=1;G_PushUse(pushMarker,&local);CHECK(hullClient.ps.velocity[1]==750);
      vehicle.m_iBoarding=0;VectorSet(pushMarker.currentAngles,-90,0,0);
      G_PushUse(pushMarker,&local);CHECK(hullClient.ps.velocity[2]==250&&vehicle.jarPushDrift[2]==0&&hullClient.ps.groundEntityNum==ENTITYNUM_NONE);
      level.time+=500;pushMarker.svFlags|=SVF_PLAYER_USABLE;
      localClient.ps.useTime=0;VectorSet(localClient.renderInfo.eyePoint,10,20,30);VectorClear(localClient.ps.viewangles);
      gi.trace=[](trace_t* tr,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
       CHECK(pass==0&&(mask&CONTENTS_ITEM)&&start[0]==10&&end[0]==74&&end[2]==30);
       *tr={};tr->fraction=.5f;tr->entityNum=10;
      };
      CHECK(G_TryUseVehicleTransportMarker(local)&&hullClient.ps.velocity[2]==500&&localClient.ps.useTime>0);
      level.time+=500;
      CHECK(G_TryUseVehicleTransportMarker(local)&&hullClient.ps.velocity[2]==500);
      CHECK(G_PushSet(pushMarker,Push{jar::push::Marker,250,100,jar::push::Player,jar::push::Forward,true,true}));
      CHECK(!G_TryUseVehicleTransportMarker(local)&&hullClient.ps.velocity[2]==500);
    }
    {
      using namespace jar::teleport;
      globals.num_entities=40;level.time=50000;level.previousTime=49900;
      G_TeleportArrivalsReset();G_ContactClocksReset();
      auto& gate=g_entities[31];gate.inuse=qtrue;gate.s.number=31;gate.classname="trigger_teleport";
      gate.model=const_cast<char*>(volumeModel);gate.contents=CONTENTS_TRIGGER;gate.e_TouchFunc=touchF_trigger_teleporter_touch;
      VectorSet(gate.pos1,-8,-16,-4);VectorSet(gate.pos2,8,16,48);
      auto& exit=g_entities[32];exit.inuse=qtrue;exit.s.number=32;exit.targetname="projectile_exit";
      VectorSet(exit.currentOrigin,500,0,20);VectorSet(exit.currentAngles,0,90,0);
      gate.target=exit.targetname;
      CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,true,true,false,true}));
      gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** out,int){out[0]=&g_entities[31];return 1;};
      gi.trace=[](trace_t* tr,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int,int mask,EG2_Collision,int){
        CHECK((mask&MASK_SHOT)==MASK_SHOT&&VectorCompare(start,end)&&start[0]==500);
        *tr={};tr->fraction=1;VectorCopy(end,tr->endpos);
      };
      const vec3_t from={-60,0,10},to={100,0,10};
      auto& det=g_entities[33];auto& trip=g_entities[34];auto& stopped=g_entities[35];auto& rolling=g_entities[36];
      const auto initialize=[&](gentity_t& missile,int slot,int weapon){
        missile.inuse=qtrue;missile.s.number=slot;missile.s.eType=ET_MISSILE;missile.s.weapon=weapon;
        missile.s.eFlags=0;missile.s.pos.trType=TR_GRAVITY;missile.s.pos.trTime=level.time;
        VectorCopy(from,missile.currentOrigin);VectorCopy(from,missile.s.pos.trBase);VectorSet(missile.s.pos.trDelta,600,0,0);
        VectorSet(missile.mins,-4,-4,-4);VectorSet(missile.maxs,4,4,4);missile.clipmask=MASK_SHOT;
        missile.owner=&local;missile.nextthink=level.time+800;missile.delay=level.time+40000;missile.count=2;
      };
      initialize(det,33,WP_DET_PACK);initialize(trip,34,WP_TRIP_MINE);
      det.s.eFlags=trip.s.eFlags=EF_MISSILE_STICK;
      det.e_TouchFunc=touchF_charge_stick;trip.e_TouchFunc=touchF_touchLaserTrap;
      det.e_ThinkFunc=thinkF_G_FreeEntity;
      for(auto* charge:{&det,&trip}){
        const auto touch=charge->e_TouchFunc;
        CHECK(G_TeleportSweepProjectile(*charge,from,to,false));
        CHECK(charge->owner==&local&&charge->e_TouchFunc==touch&&charge->s.eType==ET_MISSILE&&
              charge->s.pos.trType==TR_GRAVITY&&(charge->s.eFlags&EF_MISSILE_STICK)&&
              charge->nextthink==50800&&charge->delay==90000&&charge->count==2&&
              charge->currentOrigin[0]==500&&charge->s.pos.trDelta[1]==600);
      }
      CHECK(det.e_ThinkFunc==thinkF_G_FreeEntity&&G_TeleportArrivalPending(det)&&G_TeleportArrivalPending(trip));
      trip.s.pos.trType=TR_STATIONARY;
      CHECK(!G_TeleportObjectKind(trip)&&!G_TeleportSweepProjectile(trip,from,to,false)&&trip.currentOrigin[0]==500);
      trip.s.pos.trType=TR_GRAVITY;trip.s.eType=ET_GENERAL;CHECK(!G_TeleportObjectKind(trip));
      trip.s.eType=ET_MISSILE;trip.e_TouchFunc=touchF_NULL;CHECK(!G_TeleportObjectKind(trip));
      trip.e_TouchFunc=touchF_touchLaserTrap;

      initialize(stopped,35,WP_FLECHETTE);stopped.s.pos.trType=TR_STATIONARY;
      stopped.e_ThinkFunc=thinkF_WP_flechette_alt_blow;
      const vec3_t restingPoint={0,0,10};VectorCopy(restingPoint,stopped.currentOrigin);
      CHECK(G_TeleportSweepProjectile(stopped,restingPoint,restingPoint,false));
      CHECK(stopped.s.pos.trType==TR_STATIONARY&&!VectorLengthSquared(stopped.s.pos.trDelta)&&
            stopped.e_ThinkFunc==thinkF_WP_flechette_alt_blow&&stopped.nextthink==50800);
      stopped.s.weapon=WP_THERMAL;stopped.e_ThinkFunc=thinkF_thermalDetonatorExplode;
      CHECK(G_TeleportObjectKind(stopped)==Projectile);
      stopped.s.eFlags|=EF_HELD_BY_SAND_CREATURE;CHECK(!G_TeleportObjectKind(stopped));
      stopped.s.eFlags&=~EF_HELD_BY_SAND_CREATURE;

      static float wallX=30;
      static int impactEntity=37;
      static bool noImpact=false,occupiedExit=false;
      g_entities[37].inuse=g_entities[37].takedamage=qtrue;
      gi.trace=[](trace_t* tr,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int,
                   EG2_Collision collision,int){
        *tr={};tr->fraction=1;tr->entityNum=ENTITYNUM_NONE;VectorCopy(end,tr->endpos);
        CHECK(pass==36);
        if(collision==G2_NOCOLLIDE&&VectorCompare(start,end)&&start[0]==500){
          tr->startsolid=occupiedExit?qtrue:qfalse;
        }else if(start[0]<wallX&&end[0]>wallX){
          tr->fraction=(wallX-start[0])/(end[0]-start[0]);tr->entityNum=impactEntity;
          vec3_t delta;VectorSubtract(end,start,delta);VectorMA(start,tr->fraction,delta,tr->endpos);
          VectorSet(tr->plane.normal,-1,0,0);tr->surfaceFlags=noImpact?SURF_NOIMPACT:0;
        }
      };
      const auto roll=[&]{
        initialize(rolling,36,WP_THERMAL);rolling.s.pos.trType=TR_INTERPOLATE;
        rolling.e_ThinkFunc=thinkF_WP_ThermalThink;rolling.has_bounced=qtrue;
        rolling.s.pos.trDelta[0]=1600;fixtureGravity.value=0;
        rollImpacts=rollPortalQueries=0;
      };
      roll();
      CHECK(G_RollMissile(&rolling)&&rollPortalQueries==1&&!rollImpacts&&rolling.currentOrigin[0]==500&&
            rolling.s.pos.trType==TR_INTERPOLATE&&rolling.s.pos.trDelta[1]==1600&&
            rolling.e_ThinkFunc==thinkF_WP_ThermalThink&&rolling.nextthink==50800&&rolling.has_bounced);
      G_TeleportArrivalsReset();G_ContactClocksReset();
      roll();wallX=-30;
      CHECK(!G_RollMissile(&rolling)&&rollImpacts==1&&rolling.s.eType==ET_GENERAL);
      roll();wallX=30;occupiedExit=true;
      CHECK(!G_RollMissile(&rolling)&&rollImpacts==1&&rolling.s.eType==ET_GENERAL);
      occupiedExit=false;
      roll();fixtureGravity.value=800;
      CHECK(G_TeleportSet(gate,{Volume,500,NoActors,true,true,true,false,true}));
      CHECK(G_RollMissile(&rolling)&&rolling.s.pos.trDelta[0]==1600&&std::fabs(rolling.s.pos.trDelta[2]+80)<.01f);
      G_TeleportArrivalsReset();G_ContactClocksReset();
      roll();wallX=-30;impactEntity=ENTITYNUM_WORLD;gate.currentOrigin[0]=-75;
      CHECK(G_RollMissile(&rolling)&&rollPortalQueries==2&&!rollImpacts&&rolling.currentOrigin[0]==500&&rolling.s.pos.trDelta[0]<-700);
      G_TeleportArrivalsReset();G_ContactClocksReset();
      roll();noImpact=true;
      CHECK(!G_RollMissile(&rolling)&&rollPortalQueries==1&&rolling.currentOrigin[0]<-75);
      noImpact=false;

      for(const auto* missile:{&det,&trip,&stopped,&rolling})G_TeleportMarkArrival(*missile);
      MemorySave arrivals;ojk::SavedGameHelper io(&arrivals);
      G_TeleportArrivalsExport(false,io);G_TeleportArrivalsReset();arrivals.reset_buffer_offset();
      CHECK(G_TeleportArrivalsImport(false,io));io.ensure_all_data_read();
      for(const auto* missile:{&det,&trip,&stopped,&rolling})CHECK(G_TeleportArrivalPending(*missile));
    }
});}
