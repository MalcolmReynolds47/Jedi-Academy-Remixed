// Portable breakables use stock model geometry and native event payloads.
// Authored brush portals, model variants and scripts remain map-owned. GPL-2.0-only.
static const char cargoBarrelEffect[]="thermal/explosion";
static const char breakMetalEffect[]="chunks/metalexplode";
static const char breakExplosionSound[]="sound/weapons/explosions/cargoexplode.wav";
static const char gasIdleEffect[]="env/mini_gasjet";
static const char gasPainEffect[]="env/mini_flamejet";
static const char smallCargoPickup[]="item_medpak_instant";
static void BreakThink(gentity_t*);
static qboolean WelderEmitting(const gentity_t*);
static qboolean BreakReady(const gentity_t*e){return sandbox.integer&&!(e->flags&FL_INACTIVE)&&!objects[e->s.number].copyPending;}
static qboolean BreakFixedFuse(int kind){return kind==JAR_SMALL_CARGO_CONTROLLER||JAR_ControllerStockExplosive(kind);}
typedef struct { int slot; unsigned epoch; } breakGasPulse_t;
static breakGasPulse_t breakGasPulses[MAX_GENTITIES];
static int breakGasPulseCount;
static void BreakReset(void){breakGasPulseCount=0;}
qboolean JAR_MP_BreakableDamageBlocked(gentity_t*e){
 if(!e)return qfalse;
 const int kind=ControllerKind(e->s.number);
 if(JAR_ControllerEmplaced(kind))return !BreakReady(e)||e->genericValue4;
 return JAR_ControllerBreakable(kind)&&(!BreakReady(e)||e->genericValue11!=1);
}
qboolean JAR_MP_SmallCargoDamageBlocked(const gentity_t*target,const gentity_t*inflictor,int dflags,int mod){
 if(!target||ControllerKind(target->s.number)!=JAR_SMALL_CARGO_CONTROLLER)return qfalse;
 // SP only passes the heavy flag for direct hits. MP missile impact does not
 // carry its projectile flags through G_Damage, so recover them here.
 if(dflags&DAMAGE_RADIUS)return qtrue;
 if(dflags&DAMAGE_HEAVY_WEAP_CLASS)return qfalse;
 if(!inflictor||!inflictor->inuse||inflictor->s.eType!=ET_MISSILE)return qtrue;
 if(inflictor->dflags&DAMAGE_HEAVY_WEAP_CLASS)return qfalse;
 if(mod==MOD_VEHICLE&&(inflictor->dflags&DAMAGE_JAR_CARGO_WALKER_SHOT))return qfalse;
 if(mod==MOD_TURBLAST)return qfalse; // Native turbolaser and deployed E-Web.
 return !(mod==MOD_TARGET_LASER&&inflictor->classname&&!strcmp(inflictor->classname,"turret_proj"));
}
qboolean JAR_MP_BreakableRadiusSource(const gentity_t*source,unsigned*epoch){
 if(!source||!source->inuse||source->s.number<MAX_CLIENTS||source->s.number>=ENTITYNUM_WORLD)return qfalse;
 const int n=source->s.number,kind=ControllerKind(n);
 if(!JAR_ControllerBreakable(kind)&&!JAR_ControllerEmplaced(kind)&&kind!=JAR_WELDER_CONTROLLER)return qfalse;
 if(epoch)*epoch=controllerEntityEpoch[n];
 return qtrue;
}
qboolean JAR_MP_BreakableRadiusDenied(gentity_t*source,gentity_t*target){
 if(!JAR_MP_BreakableRadiusSource(source,NULL))return qfalse;
 return !BreakReady(source)||(ControllerKind(source->s.number)==JAR_WELDER_CONTROLLER&&!WelderEmitting(source))||
  !target||objects[target->s.number].copyPending||!LaserDamageAllowed(objects[source->s.number].owner,target->s.number);
}
static void BreakVisibility(gentity_t*e,qboolean visible){
 int n=e->s.number;qboolean changed=visible==!!(e->s.eFlags&EF_NODRAW);
 if(changed){if(objects[n].heldBy)Drop(objects[n].heldBy-1,qfalse);PhysicsForget(n);}
 if(visible){e->s.eFlags&=~EF_NODRAW;e->r.svFlags&=~SVF_NOCLIENT;e->r.contents=CONTENTS_SOLID;e->clipmask=MASK_PLAYERSOLID;
  const int kind=ControllerKind(n);
  if(kind==JAR_CARGO_BARREL_CONTROLLER)e->r.contents|=CONTENTS_OPAQUE;
  if(kind==JAR_EXPLODING_CRATE_CONTROLLER||kind==JAR_SMALL_CARGO_CONTROLLER)
   e->r.contents|=CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;}
 else {e->s.eFlags|=EF_NODRAW;e->r.svFlags|=SVF_NOCLIENT;e->s.solid=0;e->r.contents=e->clipmask=0;
  JAR_RigidForget(n);VectorClear(objects[n].velocity);VectorClear(objects[n].angular);VectorClear(e->s.pos.trDelta);VectorClear(e->s.apos.trDelta);}
 if(visible&&e->genericValue11==1&&(e->spawnflags&128))e->r.svFlags|=SVF_PLAYER_USABLE;else e->r.svFlags&=~SVF_PLAYER_USABLE;
 PhysicsPublish(e);trap->LinkEntity((sharedEntity_t*)e);
}
// Optional cosmetic events must never exhaust G_Spawn during chain reactions.
static gentity_t*BreakEvent(const vec3_t at,int event){
 gentity_t*e=G_TrySpawn();if(!e)return NULL;e->s.eType=ET_EVENTS+event;e->classname="jar_break_effect";e->eventTime=level.time;e->freeAfterEvent=qtrue;
 G_SetOrigin(e,(float*)at);trap->LinkEntity((sharedEntity_t*)e);return e;
}
static void BreakLocalUp(const gentity_t*e,float height,vec3_t point,vec3_t up){
 AngleVectors(e->r.currentAngles,NULL,NULL,up);
 VectorMA(e->r.currentOrigin,height,up,point);
}
static void SandboxDirectedEffect(int effect,const vec3_t point,const vec3_t up){
 gentity_t*event=BreakEvent(point,EV_PLAY_EFFECT_ID);
 if(event){event->s.eventParm=effect;VectorCopy(point,event->s.origin);vectoangles(up,event->s.angles);}
}
// Damage from a tank may trigger more tanks. Process those pulses in the same
// tick, without nesting a native radius damage stack frame for every tank.
static void BreakGasPulse(gentity_t*source){
 if(breakGasPulseCount>=MAX_GENTITIES)return;
 const int slot=source->s.number;
 breakGasPulses[breakGasPulseCount++]=(breakGasPulse_t){slot,controllerEntityEpoch[slot]};
 if(breakGasPulseCount>1)return;
 for(int i=0;i<breakGasPulseCount;++i){
  const breakGasPulse_t pulse=breakGasPulses[i];gentity_t*tank=&g_entities[pulse.slot];
  if(!tank->inuse||controllerEntityEpoch[pulse.slot]!=pulse.epoch||ControllerKind(pulse.slot)!=JAR_GAS_TANK_CONTROLLER||!BreakReady(tank))continue;
  vec3_t point,up;
  BreakLocalUp(tank,46,point,up);SandboxDirectedEffect(tank->genericValue15,point,up);
  VectorMA(point,32,up,point);
  G_RadiusDamage(point,tank,32,32,tank,NULL,MOD_UNKNOWN);
 }
 breakGasPulseCount=0;
}
static void BreakEffects(gentity_t*e,gentity_t*a){
 vec3_t lo,hi,extent,center,dir={0,0,1};
 VectorAdd(e->r.currentOrigin,e->r.mins,lo);VectorAdd(e->r.currentOrigin,e->r.maxs,hi);VectorSubtract(hi,lo,extent);VectorAdd(lo,hi,center);VectorScale(center,.5f,center);
 float scale=sqrtf(sqrtf(fmaxf(0,extent[0]*extent[1]*extent[2])))*1.75f;int size=scale>48?2:scale>24?1:0;
 const int kind=ControllerKind(e->s.number);
 const qboolean modelBreak=kind==31||BreakFixedFuse(kind);
 const int base=modelBreak?Q_irand(20,25):Q_irand(18,23);
 const long long scaledChunks=(long long)base*e->genericValue12/100;
 const int chunks=scaledChunks>USHRT_MAX?USHRT_MAX:(int)scaledChunks;
 if(kind==JAR_SMALL_CARGO_CONTROLLER)AngleVectors(e->r.currentAngles,dir,NULL,NULL);
 else if(a&&a!=e&&a->client){VectorSubtract(center,a->r.currentOrigin,dir);VectorNormalize(dir);}
 gentity_t*te;
 if(!(e->spawnflags&2048)&&(te=BreakEvent(center,EV_MISC_MODEL_EXP))){VectorCopy(hi,te->s.origin2);VectorCopy(lo,te->s.angles2);te->s.time=size;te->s.eventParm=e->material;}
 if(chunks&&(te=BreakEvent(center,EV_DEBRIS))){te->s.owner=te->s.number;VectorCopy(center,te->s.origin);VectorCopy(dir,te->s.angles);VectorCopy(hi,te->s.origin2);VectorCopy(lo,te->s.angles2);te->s.speed=300;te->s.eventParm=chunks;te->s.trickedentindex=e->material;te->s.apos.trBase[0]=scale/base;}
 if(!(e->spawnflags&2048)&&(modelBreak||e->splashDamage>0)&&(te=BreakEvent(center,EV_GENERAL_SOUND)))
  te->s.eventParm=kind==JAR_SMALL_CARGO_CONTROLLER?e->genericValue13:G_SoundIndex(breakExplosionSound);
}
static void BreakSmallCargoFinish(gentity_t*e,gentity_t*activator,unsigned epoch){
 const int n=e->s.number,owner=objects[n].owner;
 const int pickupModel=e->genericValue15;
 const unsigned group=objects[n].order;
 vec3_t origin,blast;
 VectorCopy(e->r.currentOrigin,origin);
 VectorCopy(origin,blast);
 if(e->r.mins[2]>-4)blast[2]+=(e->r.mins[2]+e->r.maxs[2])*.5f;
 G_UseTargets(e,activator);
 if(e->inuse&&controllerEntityEpoch[n]==epoch&&e->splashDamage>0&&e->splashRadius>0)
  G_RadiusDamage(blast,e,e->splashDamage,e->splashRadius,e,NULL,MOD_UNKNOWN);

 // Match the stock northeast drop after the blast. The captured origin and
 // ownership are still valid if a linked callback freed the source.
 gitem_t*item=BG_FindItem(smallCargoPickup);
 if(item&&!G_ItemDisabled(item)){
  origin[0]+=16+Q_flrand(-1,1)*8;
  origin[1]+=16+Q_flrand(-1,1)*8;
  origin[2]+=16;
  gentity_t*drop=JAR_MP_TryLaunchItem(item,origin,vec3_origin);
  if(drop){
   // Use the SP medpak's dose, appearance and hull on the native MP item.
   drop->count=20;
   drop->s.generic1=JAR_MP_CARGO_DROP_TAG;
   drop->s.modelindex2=pickupModel;
   VectorClear(drop->s.angles);
   VectorClear(drop->s.apos.trBase);
   VectorClear(drop->r.currentAngles);
   VectorSet(drop->r.mins,-8,-8,-3);
   VectorSet(drop->r.maxs,8,8,16);
   TrackTemporaryPickup(drop,owner,group);
   trap->LinkEntity((sharedEntity_t*)drop);
  }
 }
 if(e->inuse&&controllerEntityEpoch[n]==epoch){
  // The native pickup has first claim on scarce slots, ahead of cosmetics.
  BreakEffects(e,NULL);
  G_FreeEntity(e);
 }
}
static void BreakFinish(gentity_t*e){
 int n=e->s.number,kind=ControllerKind(n);unsigned epoch=controllerEntityEpoch[n];gentity_t*a=ControllerActivator(e);vec3_t center;
 const qboolean stock=JAR_ControllerStockExplosive(kind);
 if(kind==JAR_CARGO_BARREL_CONTROLLER)VectorCopy(e->r.currentOrigin,center);
 else if(stock){vec3_t up;BreakLocalUp(e,kind==JAR_GAS_TANK_CONTROLLER?20:32,center,up);}
 else {VectorAdd(e->r.mins,e->r.maxs,center);VectorMA(e->r.currentOrigin,.5f,center,center);}
 e->genericValue11=0;e->health=0;e->takedamage=qfalse;e->think=NULL;e->nextthink=0;BreakVisibility(e,qfalse);
 ++objects[n].propertyRevision;SelectionChanged(n);
 if(kind==JAR_SMALL_CARGO_CONTROLLER){BreakSmallCargoFinish(e,a,epoch);return;}
 if(kind==JAR_CARGO_BARREL_CONTROLLER){
  gentity_t*effect=BreakEvent(center,EV_PLAY_EFFECT_ID);
  if(effect){effect->s.eventParm=e->genericValue13;VectorCopy(center,effect->s.origin);VectorCopy(e->r.currentAngles,effect->s.angles);}
 }else BreakEffects(e,a);
 // Raven's model breakables run their death target before the blast.
 if(stock){
  G_UseTargets(e,a);
  if(!e->inuse||controllerEntityEpoch[n]!=epoch)return;
 }
 // The native radius path retains line-of-sight/falloff; its owned-source
 // guard checks current tool permissions before each victim.
 if(e->splashDamage>0&&e->splashRadius>0&&(kind!=31||!(e->spawnflags&2048)))G_RadiusDamage(center,e,e->splashDamage,e->splashRadius,e,NULL,MOD_UNKNOWN);
 if(!e->inuse||controllerEntityEpoch[n]!=epoch)return;
 if(!stock)G_UseTargets(e,ControllerActivator(e));
 if(JAR_ControllerTerminalBreakable(kind)&&e->inuse&&controllerEntityEpoch[n]==epoch)G_FreeEntity(e);
}
static void BreakThink(gentity_t*e){
 const int kind=ControllerKind(e->s.number);
 if(!BreakReady(e)){
  e->nextthink=level.time+(e->genericValue11==2&&BreakFixedFuse(kind)?JAR_STOCK_EXPLOSIVE_FUSE:FRAMETIME);
  return;
 }
 if(e->genericValue11==2){BreakFinish(e);return;}
 if(kind==JAR_GAS_TANK_CONTROLLER&&e->genericValue11==1){
  vec3_t point,up;
  e->nextthink=level.time+Q_irand(12000,JAR_GAS_TANK_IDLE_MAX);
  BreakLocalUp(e,50,point,up);SandboxDirectedEffect(e->genericValue13,point,up);
 }
}
static void BreakDie(gentity_t*e,gentity_t*i,gentity_t*a,int damage,int mod){
 if(!BreakReady(e)||e->genericValue11!=1)return;
 ControllerCaptureActivator(e,a);e->health=0;e->takedamage=qfalse;e->genericValue11=2;e->r.svFlags&=~SVF_PLAYER_USABLE;
 ++objects[e->s.number].propertyRevision;
 // Always defer by at least one tick: chains cannot recurse through G_Damage.
 const int kind=ControllerKind(e->s.number);
 int delay=e->delay;
 if(kind==JAR_CARGO_BARREL_CONTROLLER)delay=Q_irand(100,500);
 else if(BreakFixedFuse(kind))delay=JAR_STOCK_EXPLOSIVE_FUSE;
 e->think=BreakThink;e->nextthink=JAR_NumberDeadline(level.time,delay?delay:1);SelectionChanged(e->s.number);
}
static void BreakPain(gentity_t*e,gentity_t*a,int damage){
 if(!BreakReady(e)||e->genericValue11!=1)return;
 const int kind=ControllerKind(e->s.number);
 const qboolean gasPulse=kind==JAR_GAS_TANK_CONTROLLER&&!e->genericValue14&&damage>0;
 if(gasPulse)e->genericValue14=1;
 e->health=Com_Clampi(0,e->genericValue4,e->health);++objects[e->s.number].propertyRevision;SelectionChanged(e->s.number);
 if(gasPulse)BreakGasPulse(e);
 if(JAR_ControllerTerminalBreakable(kind))return;
 if(e->painDebounceTime>level.time)return;e->painDebounceTime=JAR_NumberDeadline(level.time,e->wait);ControllerCaptureActivator(e,a);G_UseTargets2(e,a,e->target2);
}
static void BreakUse(gentity_t*e,gentity_t*other,gentity_t*a){
 const int kind=ControllerKind(e->s.number);
 if(!BreakReady(e)||(e->genericValue11!=1&&(kind!=JAR_CARGO_BARREL_CONTROLLER||e->genericValue11!=2))||
    (a&&a->client&&!ActionActivator(e,a)))return;
 ControllerCaptureActivator(e,a);
 if(JAR_ControllerTerminalBreakable(kind)){BreakFinish(e);return;}
 if(e->spawnflags&64){if(kind!=31)G_UseTargets(e,a);return;}
 BreakDie(e,other,a,0,MOD_UNKNOWN);
}
static void BreakState(int n,jarControllerState_t*s){
 gentity_t*e=&g_entities[n];const int kind=ControllerKind(n);
 s->active=e->genericValue11;s->initial=e->genericValue4;s->count=Com_Clampi(0,s->initial,e->health);
 s->damage=e->splashDamage;s->speed=e->splashRadius;s->inactive=!!(e->flags&FL_INACTIVE);
 if(kind==JAR_SMALL_CARGO_CONTROLLER)return;
 if(JAR_ControllerStockExplosive(kind)){
  s->flags=kind==JAR_GAS_TANK_CONTROLLER&&e->genericValue14?JAR_GAS_TANK_PULSE_SPENT:0;
  return;
 }
 s->flags=e->spawnflags;s->waitMs=e->delay;s->randomMs=JAR_NumberMillis(e->wait);s->repeats=e->material;s->pushSpeed=e->genericValue12;
}
static qboolean BreakApply(int n,const jarControllerState_t*s){
 const int kind=ControllerKind(n);
 const qboolean stock=JAR_ControllerStockExplosive(kind);
 const qboolean smallCargo=kind==JAR_SMALL_CARGO_CONTROLLER;
 if(!JAR_ControllerStateValid(kind,s))return qfalse;gentity_t*e=&g_entities[n];
 if(s->active&&BodyDormant(n)){trace_t tr;trap->Trace(&tr,e->r.currentOrigin,e->r.mins,e->r.maxs,e->r.currentOrigin,n,MASK_PLAYERSOLID,qfalse,0,0);if(tr.startsolid||tr.allsolid)return qfalse;}
 int old=e->genericValue11;e->spawnflags=stock?0:s->flags;e->genericValue11=s->active;e->genericValue4=s->initial;e->health=s->count;e->delay=s->waitMs;e->wait=(float)s->randomMs;
 e->material=(material_t)s->repeats;e->splashDamage=s->damage;e->splashRadius=s->speed;e->genericValue12=s->pushSpeed;
 if(stock){
  e->material=kind==JAR_GAS_TANK_CONTROLLER?MAT_METAL3:MAT_CRATE1;
  e->genericValue12=100;
  e->genericValue14=!!(s->flags&JAR_GAS_TANK_PULSE_SPENT);
 }
 if(smallCargo){e->material=MAT_CRATE1;e->genericValue12=150;}
 e->flags&=~(FL_INACTIVE|FL_DMG_BY_SABER_ONLY|FL_DMG_BY_HEAVY_WEAP_ONLY);if(s->inactive)e->flags|=FL_INACTIVE;if(s->flags&16)e->flags|=FL_DMG_BY_SABER_ONLY;if(s->flags&32)e->flags|=FL_DMG_BY_HEAVY_WEAP_ONLY;
 e->takedamage=s->active==1&&!(s->flags&1);e->pain=BreakPain;e->die=BreakDie;e->use=smallCargo?NULL:BreakUse;
 if(s->active==2){
  e->think=BreakThink;
  if(old!=2||!e->nextthink)e->nextthink=JAR_NumberDeadline(level.time,BreakFixedFuse(kind)?JAR_STOCK_EXPLOSIVE_FUSE:s->waitMs?s->waitMs:1);
 }else if(kind==JAR_GAS_TANK_CONTROLLER&&s->active==1){
  e->think=BreakThink;
  if(old!=1||!e->nextthink)e->nextthink=level.time+Q_irand(6000,18000);
 }else {e->think=NULL;e->nextthink=0;}
 BreakVisibility(e,s->active!=0);return qtrue;
}
static qboolean BreakInit(gentity_t*e){
 const int n=e->s.number,kind=ControllerKind(n);
 jarControllerState_t s={0};s.active=1;s.initial=s.count=40;
 if(kind==JAR_SMALL_CARGO_CONTROLLER){
  gitem_t*item=BG_FindItem(smallCargoPickup);
  if(!item||!ModelRoom(JAR_MP_CARGO_DROP_MODEL)||!MediaValidate(10,breakMetalEffect)||!MediaValidate(9,breakExplosionSound))return qfalse;
  fileHandle_t file=0;
  const int size=trap->FS_Open(JAR_MP_CARGO_DROP_MODEL,&file,FS_READ);
  if(file)trap->FS_Close(file);
  if(!file||size<=0)return qfalse;
  G_EffectIndex(breakMetalEffect);
  e->genericValue13=G_SoundIndex(breakExplosionSound);
  e->genericValue15=G_ModelIndex(JAR_MP_CARGO_DROP_MODEL);
  if(!e->genericValue13||!e->genericValue15)return qfalse;
  RegisterItem(item);
  SaveRegisteredItems();
  s.damage=1;s.speed=96;
  return BreakApply(n,&s);
 }
 if(kind==JAR_CARGO_BARREL_CONTROLLER){
  if(!MediaValidate(10,cargoBarrelEffect))return qfalse;
  e->genericValue13=G_EffectIndex(cargoBarrelEffect);
  if(!e->genericValue13)return qfalse;
  s.damage=100;s.speed=200;
  return BreakApply(n,&s);
 }
 if(JAR_ControllerStockExplosive(kind)){
  const qboolean gas=kind==JAR_GAS_TANK_CONTROLLER;
  if(!MediaValidate(10,breakMetalEffect)||!MediaValidate(9,breakExplosionSound)||
     (gas&&(!MediaValidate(10,gasIdleEffect)||!MediaValidate(10,gasPainEffect))))return qfalse;
  G_EffectIndex(breakMetalEffect);G_SoundIndex(breakExplosionSound);
  if(gas){e->genericValue13=G_EffectIndex(gasIdleEffect);e->genericValue15=G_EffectIndex(gasPainEffect);}
  s.initial=s.count=gas?20:40;s.damage=gas?32:50;s.speed=gas?48:128;
  return BreakApply(n,&s);
 }
 // Native CG_Chunks also precaches the stock debris models and impact sounds.
 const char*fx[]={"chunks/glassbreak","chunks/metalexplode","chunks/sparkexplode","chunks/grateexplode","chunks/rockbreaklg","chunks/rockbreakmed","chunks/ropebreak"};
 for(unsigned i=0;i<sizeof(fx)/sizeof(fx[0]);++i)G_EffectIndex(fx[i]);G_SoundIndex(breakExplosionSound);
 s.randomMs=1;s.pushSpeed=100;s.repeats=kind==32?MAT_GLASS:kind==31?MAT_NONE:MAT_METAL;
 return BreakApply(n,&s);
}
