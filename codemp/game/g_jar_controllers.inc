// Portable native target controllers. Connections are owned entity references,
// never authored-map target names or client-provided callback/script names.
extern void SP_target_give(gentity_t*);
extern void SP_target_score(gentity_t*);
extern void SP_target_kill(gentity_t*);
extern void Use_target_push(gentity_t*,gentity_t*,gentity_t*);
extern void target_teleporter_use(gentity_t*,gentity_t*,gentity_t*);
extern void SP_func_timer(gentity_t*);
extern void func_timer_think(gentity_t*);
extern void SP_target_random(gentity_t*);
extern void target_random_use(gentity_t*,gentity_t*,gentity_t*);
extern void SP_target_print(gentity_t*);
extern void SP_target_activate(gentity_t*);
extern void SP_target_deactivate(gentity_t*);
extern void SP_target_relay(gentity_t*);
extern void SP_target_delay(gentity_t*);
extern void SP_target_counter(gentity_t*);
extern void Think_Target_Delay(gentity_t*);
static void AlwaysThink(gentity_t*entity);
static void ControllerCaptureActivator(gentity_t*e,gentity_t*a);
static gentity_t*ControllerActivator(gentity_t*e);
static qboolean ControllerReaches(int from,int to);
static qboolean ActionActivator(gentity_t* action,gentity_t* actor);
#include "g_jar_distance.inc"
#include "g_jar_wall.inc"
#include "g_jar_sliding_door.inc"
#include "g_jar_bobbing.inc"
#include "g_jar_angular.inc"
#include "g_jar_button.inc"
#include "g_jar_movement.inc"
static void StartMarkerUse(gentity_t*entity,gentity_t*other,gentity_t*actor){JAR_MP_TeleportTarget(entity,actor);}

static void StaticBrushUse(gentity_t*entity,gentity_t*other,gentity_t*actor){
 if(!sandbox.integer||(entity->flags&FL_INACTIVE)||objects[entity->s.number].copyPending)return;
 ControllerCaptureActivator(entity,actor);
 JAR_MP_UseTargets(entity,actor,"@jar_primary");
}

static void AlwaysSchedule(gentity_t*entity,int remaining){
 entity->think=remaining?AlwaysThink:NULL;
 entity->nextthink=remaining?JAR_NumberDeadline(level.time,remaining):0;
}

static void AlwaysThink(gentity_t*entity){
 const int n=entity->s.number;
 AlwaysSchedule(entity,0);
 ++objects[n].propertyRevision;
 SelectionChanged(n);
 if(sandbox.integer&&!(entity->flags&FL_INACTIVE)&&!objects[n].copyPending)
  JAR_MP_UseTargets(entity,entity,"@jar_primary");
}

static void AlwaysUse(gentity_t*entity,gentity_t*other,gentity_t*activator){
 const int n=entity->s.number;
 if(!sandbox.integer||(entity->flags&FL_INACTIVE)||objects[n].copyPending||entity->nextthink>0)return;
 AlwaysSchedule(entity,entity->delay);
 ++objects[n].propertyRevision;
 SelectionChanged(n);
}

static void VisibleArm(gentity_t*entity,int armed){
 entity->genericValue1=armed;
 entity->timestamp=armed?level.time+JAR_VISIBLE_SAMPLE_MS:0;
}

static void VisibleUse(gentity_t*entity,gentity_t*other,gentity_t*activator){
 const int n=entity->s.number;
 if(!sandbox.integer||(entity->flags&FL_INACTIVE)||objects[n].copyPending||entity->genericValue1)return;
 VisibleArm(entity,1);
 ++objects[n].propertyRevision;
 SelectionChanged(n);
}

static int ControllerClass(const char*name){
 if(!strcmp(name,"target_print"))return 1;if(!strcmp(name,"target_relay"))return 2;if(!strcmp(name,"target_delay"))return 3;if(!strcmp(name,"target_counter"))return 4;if(!strcmp(name,"target_position")||!strcmp(name,"info_null")||!strcmp(name,"info_notnull"))return 5;if(!strcmp(name,"target_push"))return 6;if(!strcmp(name,"target_teleporter"))return 7;if(!strcmp(name,"misc_teleporter_dest"))return 8;if(!strcmp(name,"target_speaker"))return 9;if(!strcmp(name,"fx_runner"))return 10;if(!strcmp(name,"target_activate"))return 11;if(!strcmp(name,"target_deactivate"))return 12;if(!strcmp(name,"target_laser"))return 13;if(!strcmp(name,"target_random"))return 14;if(!strcmp(name,"func_timer"))return 15;if(!strcmp(name,"target_give"))return 16;if(!strcmp(name,"target_score"))return 17;if(!strcmp(name,"target_kill"))return 18;if(!strcmp(name,"trigger_multiple"))return 19;if(!strcmp(name,"trigger_once"))return 20;if(!strcmp(name,"trigger_hurt"))return 21;if(!strcmp(name,"trigger_push"))return 22;if(!strcmp(name,"trigger_teleport"))return 23;if(!strcmp(name,"misc_ammo_floor_unit"))return 24;if(!strcmp(name,"misc_shield_floor_unit"))return 25;if(!strcmp(name,"misc_model_shield_power_converter"))return 26;if(!strcmp(name,"misc_model_ammo_power_converter"))return 27;if(!strcmp(name,"misc_model_health_power_converter"))return 28;if(!strcmp(name,"func_usable"))return 29;if(!strcmp(name,"func_breakable"))return 30;if(!strcmp(name,"misc_model_breakable"))return 31;if(!strcmp(name,"func_glass"))return 32;if(!strcmp(name,"light")||!strcmp(name,"misc_dlight"))return 33;
 if(!strcmp(name,"target_play_music"))return JAR_MUSIC_CONTROLLER;
 if(!strcmp(name,"object_cargo_barrel1"))return JAR_CARGO_BARREL_CONTROLLER;
 if(!strcmp(name,"emplaced_gun"))return JAR_EMPLACED_CONTROLLER;
 if(!strcmp(name,"emplaced_eweb"))return JAR_EWEB_CONTROLLER;
 if(!strcmp(name,"misc_exploding_crate"))return JAR_EXPLODING_CRATE_CONTROLLER;
 if(!strcmp(name,"misc_gas_tank"))return JAR_GAS_TANK_CONTROLLER;
 if(!strcmp(name,"shooter_plasma"))return JAR_PLASMA_SHOOTER_CONTROLLER;
 if(!strcmp(name,"shooter_grenade"))return JAR_GRENADE_SHOOTER_CONTROLLER;
 if(!strcmp(name,"shooter_rocket"))return JAR_ROCKET_SHOOTER_CONTROLLER;
 if(!strcmp(name,"misc_weapon_shooter"))return JAR_BLASTER_SHOOTER_CONTROLLER;
 if(!strcmp(name,"misc_model_bomb_planted"))return JAR_BOMB_CONTROLLER;
 if(!strcmp(name,"misc_model_beacon"))return JAR_BEACON_CONTROLLER;
 if(!strcmp(name,"misc_model_cargo_small"))return JAR_SMALL_CARGO_CONTROLLER;
 if(!strcmp(name,"misc_model_gun_rack"))return JAR_GUN_RACK_CONTROLLER;
 if(!strcmp(name,"misc_model_ammo_rack"))return JAR_AMMO_RACK_CONTROLLER;
 if(!strcmp(name,"misc_model_jabba_cam"))return JAR_DOOR_CAMERA_CONTROLLER;
 if(!strcmp(name,"misc_model_welder"))return JAR_WELDER_CONTROLLER;
 if(!strcmp(name,"misc_crystal_crate"))return JAR_CRYSTAL_CRATE_CONTROLLER;
 if(!strcmp(name,"misc_teleporter"))return 23;
 if(!strcmp(name,"target_location"))return JAR_LOCATION_MARKER_CONTROLLER;
 if(!strcmp(name,"trigger_location"))return JAR_LOCATION_VOLUME_CONTROLLER;
 if(!strcmp(name,"trigger_always"))return JAR_ALWAYS_CONTROLLER;
 if(!strcmp(name,"trigger_visible"))return JAR_VISIBLE_CONTROLLER;
 if(!strcmp(name,"trigger_entdist"))return JAR_DISTANCE_CONTROLLER;
 if(JAR_WallClass(name))return JAR_WALL_CONTROLLER;
 if(!strcmp(name,"func_door"))return JAR_SLIDING_DOOR_CONTROLLER;
 if(!strcmp(name,"func_plat"))return JAR_LIFT_CONTROLLER;
 if(!strcmp(name,"func_button"))return JAR_BUTTON_CONTROLLER;
 if(!strcmp(name,"misc_cubemap"))return JAR_REFLECTION_CONTROLLER;
 if(!strcmp(name,"func_bobbing"))return JAR_BOBBING_CONTROLLER;
 if(!strcmp(name,"func_rotating"))return JAR_SPINNER_CONTROLLER;
 if(!strcmp(name,"func_pendulum"))return JAR_PENDULUM_CONTROLLER;
 if(!strcmp(name,"func_static")||!strcmp(name,"func_group"))return JAR_STATIC_BRUSH_CONTROLLER;
 if(!strcmp(name,"target_gravity_change"))return JAR_GRAVITY_CONTROLLER;
 if(!strcmp(name,"target_friction_change"))return JAR_FRICTION_CONTROLLER;
 if(!strcmp(name,"trigger_bidirectional"))return JAR_DIRECTIONAL_CONTROLLER;
 if(!strcmp(name,"trigger_space"))return JAR_SPACE_CONTROLLER;
 if(!strcmp(name,"misc_spotlight"))return JAR_SEARCHLIGHT_CONTROLLER;
 if(!strcmp(name,"misc_portal_surface"))return JAR_PORTAL_SURFACE_CONTROLLER;
 if(!strcmp(name,"misc_portal_camera"))return JAR_PORTAL_CAMERA_CONTROLLER;
 if(!strcmp(name,"info_player_start"))return JAR_PLAYER_START_CONTROLLER;
 if(!strcmp(name,"info_player_deathmatch"))return JAR_DEATHMATCH_START_CONTROLLER;
 if(JAR_MP_SectionKind(name))return JAR_SECTION_CONTROLLER;
 return 0;
}
static int ControllerKind(int n){return n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&g_entities[n].inuse&&objects[n].owner&&objects[n].catalog>0&&objects[n].catalog<=JAR_MP_CATALOG_COUNT&&jarMPCatalog[objects[n].catalog-1].kind==5?ControllerClass(jarMPCatalog[objects[n].catalog-1].name):0;}
static int ReflectionCount(int skip){
 int count=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(n!=skip&&ControllerKind(n)==JAR_REFLECTION_CONTROLLER)++count;
 return count;
}
static void ReflectionSync(gentity_t* e){
 e->s.speed=e->genericValue2;
 e->s.time=sandbox.integer&&e->genericValue1&&!(e->flags&FL_INACTIVE)?e->genericValue3:0;
}
static void ReflectionUse(gentity_t* e,gentity_t* other,gentity_t* activator){
 if(!sandbox.integer||(e->flags&FL_INACTIVE))return;
 e->genericValue1=!e->genericValue1;
 ReflectionSync(e);SelectionChanged(e->s.number);
}
static qboolean ControllerOutput(int n){int kind=ControllerKind(n);return (kind>=2&&kind<=4)||kind==6||kind==7||kind==10||kind==13||kind==14||kind==15||kind==16||kind==19||kind==20||kind==22||kind==23||kind==29||kind==JAR_BOMB_CONTROLLER||kind==JAR_ALWAYS_CONTROLLER||kind==JAR_VISIBLE_CONTROLLER||kind==JAR_DISTANCE_CONTROLLER||kind==JAR_DIRECTIONAL_CONTROLLER||kind==JAR_SEARCHLIGHT_CONTROLLER||kind==JAR_STATIC_BRUSH_CONTROLLER||JAR_ControllerPortal(kind)||JAR_ControllerSlide(kind)||kind==JAR_BUTTON_CONTROLLER||JAR_ControllerBreakable(kind)||JAR_ControllerSwitch(kind)||JAR_ControllerShooter(kind);}
// Native constantLight is RGB plus a radius in four-unit steps. Publish it
// on the existing prop body so movement, visibility and removal stay native.
static void LightSync(int n){
 gentity_t*e=&g_entities[n];
 e->s.constantLight=sandbox.integer&&e->genericValue1&&!(e->flags&FL_INACTIVE)&&!objects[n].copyPending
  ?(int)(127u|(178u<<8)|(255u<<16)|((unsigned)(e->genericValue2/4)<<24)):0;
}
static void LightUse(gentity_t*e,gentity_t*other,gentity_t*activator){
 if(!sandbox.integer||(e->flags&FL_INACTIVE)||objects[e->s.number].copyPending)return;
 e->genericValue1=!e->genericValue1;LightSync(e->s.number);SelectionChanged(e->s.number);
}
static qboolean ControllerInit(gentity_t*e,const jarMPEntry_t*entry,qboolean hiddenBeacon,const jarControllerState_t*saved){
 int oldCount=level.numSpawnVars;qboolean oldSpawning=level.spawning;level.numSpawnVars=0;level.spawning=qtrue;
 e->target="@jar_primary";e->target2="@jar_secondary";e->targetname="Sandbox controller";Q_strncpyz(objects[e->s.number].controllerText,(ControllerClass(entry->name)==5||ControllerClass(entry->name)==8)?"Sandbox destination":"Sandbox target activated",64);e->message=objects[e->s.number].controllerText;
 switch(ControllerClass(entry->name)){case 1:e->wait=1;SP_target_print(e);break;case 2:SP_target_relay(e);break;case 3:SP_target_delay(e);break;case 4:e->count=2;SP_target_counter(e);break;case 6:e->speed=600;e->spawnflags=2;e->use=Use_target_push;break;case 7:e->use=target_teleporter_use;break;case 11:SP_target_activate(e);break;case 12:SP_target_deactivate(e);break;case 16:SP_target_give(e);break;case 17:SP_target_score(e);break;case 18:SP_target_kill(e);break;case 14:SP_target_random(e);break;case 33:e->use=LightUse;e->genericValue1=1;e->genericValue2=160;LightSync(e->s.number);break;case 15:SP_func_timer(e);e->random=0;e->r.svFlags&=~SVF_NOCLIENT;break;}
 level.numSpawnVars=oldCount;level.spawning=oldSpawning;
 if(ControllerClass(entry->name)==JAR_MUSIC_CONTROLLER)return MusicInit(e);
 const int kind=ControllerClass(entry->name);
 if(kind==JAR_STATIC_BRUSH_CONTROLLER){e->use=StaticBrushUse;e->r.svFlags|=SVF_PLAYER_USABLE;return qtrue;}
 if(kind==JAR_SEARCHLIGHT_CONTROLLER){SearchlightInit(e);return qtrue;}
 if(JAR_ControllerPortal(kind)){PortalInit(e,kind,saved);return qtrue;}
 if(kind==JAR_DIRECTIONAL_CONTROLLER)return DirectionalInit(e);
 if(JAR_ControllerMovement(kind)){
  e->speed=kind==JAR_GRAVITY_CONTROLLER?JAR_MOVEMENT_DEFAULT_GRAVITY:JAR_MOVEMENT_DEFAULT_FRICTION;
  e->use=MovementUse;
  return qtrue;
 }
 if(kind==JAR_REFLECTION_CONTROLLER){
  e->use=ReflectionUse;e->genericValue1=1;e->genericValue2=JAR_PROBE_DEFAULT_RADIUS;e->genericValue3=1;
  e->r.svFlags|=SVF_BROADCAST;e->s.bolt2=JAR_MP_PROBE_TAG;
  ReflectionSync(e);return qtrue;
 }
 if(JAR_ControllerStart(kind)){
  Q_strncpyz(e->message,kind==JAR_PLAYER_START_CONTROLLER?"Player start":"Deathmatch start",64);
  e->use=StartMarkerUse;
  return qtrue;
 }
 if(kind==JAR_ALWAYS_CONTROLLER){
  e->delay=JAR_ALWAYS_DEFAULT_MS;e->use=AlwaysUse;AlwaysSchedule(e,e->delay);return qtrue;
 }
 if(kind==JAR_VISIBLE_CONTROLLER){
  e->speed=JAR_VISIBLE_RANGE_DEFAULT;
  e->damage=JAR_VISIBLE_CONE_DEFAULT;
  e->use=VisibleUse;
  VisibleArm(e,1);
  return qtrue;
 }
 if(kind==JAR_DISTANCE_CONTROLLER){
  e->speed=JAR_DISTANCE_DEFAULT;
  e->spawnflags=JAR_DISTANCE_PLAYER;
  e->use=DistanceUse;
  return qtrue;
 }
 if(kind==JAR_WALL_CONTROLLER){e->use=WallUse;WallFields(e,!saved||saved->active);return qtrue;}
 if(JAR_ControllerSlide(kind)){SlidingDoorInit(e,saved,kind==JAR_LIFT_CONTROLLER?qtrue:qfalse);return qtrue;}
 if(kind==JAR_BOBBING_CONTROLLER){BobbingInit(e,saved);return qtrue;}
 if(JAR_ControllerAngular(kind)){AngularInit(e,kind,saved);return qtrue;}
 if(kind==JAR_BUTTON_CONTROLLER)return ButtonInit(e);
 if(JAR_ControllerLocation(kind))return LocationInit(e,kind);
 if(JAR_ControllerAnimatedTag(kind))return AnimatedInit(e,kind,hiddenBeacon);
 if(JAR_ControllerShooter(kind))return ShooterInit(e,kind);
 if(JAR_ControllerEmplaced(kind))return EmplacedInit(e,kind);
 if(kind==JAR_CRYSTAL_CRATE_CONTROLLER)return qtrue; // Native pose and model are published first.
 return JAR_ControllerBreakable(ControllerClass(entry->name))?BreakInit(e):ControllerClass(entry->name)==29?UsableInit(e):JAR_ControllerStation(ControllerClass(entry->name))?StationInit(e):JAR_ControllerTrigger(ControllerClass(entry->name))?TriggerInit(e):ControllerClass(entry->name)==13?LaserInit(e):JAR_ControllerMedia(ControllerClass(entry->name))?MediaInit(e):qtrue;
}
static void ControllerState(int n,jarControllerState_t*s){
 const gentity_t*e=&g_entities[n];int kind=ControllerKind(n);memset(s,0,sizeof(*s));if(!kind)return;
 if(kind==JAR_SEARCHLIGHT_CONTROLLER){SearchlightState(n,s);return;}
 if(JAR_ControllerPortal(kind)){PortalState(n,s);return;}
 if(kind==JAR_DIRECTIONAL_CONTROLLER){DirectionalState(n,s);return;}
 if(JAR_ControllerMovement(kind)){s->speed=(int)e->speed;s->inactive=!!(e->flags&FL_INACTIVE);return;}
 if(kind==JAR_REFLECTION_CONTROLLER){s->active=e->genericValue1;s->speed=e->genericValue2;s->inactive=!!(e->flags&FL_INACTIVE);return;}
 if(kind==JAR_ALWAYS_CONTROLLER){s->waitMs=e->delay;s->active=e->nextthink>0;s->inactive=(e->flags&FL_INACTIVE)!=0;return;}
 if(kind==JAR_VISIBLE_CONTROLLER){
  s->speed=(int)e->speed;s->damage=e->damage;s->flags=e->spawnflags;
  s->active=e->genericValue1;s->inactive=(e->flags&FL_INACTIVE)!=0;
  return;
 }
 if(kind==JAR_DISTANCE_CONTROLLER){
  s->speed=(int)e->speed;s->flags=e->spawnflags;s->inactive=(e->flags&FL_INACTIVE)!=0;
  s->count=DistanceInputStatus(n,0);s->initial=DistanceInputStatus(n,1);
  return;
 }
 if(kind==JAR_WALL_CONTROLLER){s->active=e->genericValue1;s->inactive=!!(e->flags&FL_INACTIVE);return;}
 if(JAR_ControllerSlide(kind)){SlidingDoorState(e,s);return;}
 if(kind==JAR_BOBBING_CONTROLLER){BobbingState(e,s);return;}
 if(JAR_ControllerAngular(kind)){AngularState(e,s);return;}
 if(kind==JAR_BUTTON_CONTROLLER){ButtonState(e,s);return;}
 if(JAR_ControllerLocation(kind)){LocationState(n,s);return;}
 if(kind==JAR_MUSIC_CONTROLLER){MusicState(n,s);return;}
 if(JAR_ControllerAnimatedTag(kind)){AnimatedState(n,s);return;}
 if(JAR_ControllerShooter(kind)){ShooterState(n,s);return;}
 if(JAR_ControllerRack(kind)){RackState(n,s);return;}
 if(kind==JAR_DOOR_CAMERA_CONTROLLER){DoorCameraState(n,s);return;}
 if(kind==JAR_WELDER_CONTROLLER){WelderState(n,s);return;}
 if(kind==JAR_CRYSTAL_CRATE_CONTROLLER){CrystalCrateState(n,s);return;}
 if(JAR_ControllerEmplaced(kind)){EmplacedState(n,s);return;}
 if(JAR_ControllerBreakable(kind)){BreakState(n,s);return;}if(kind==29){UsableState(n,s);return;}if(JAR_ControllerStation(kind)){StationState(n,s);return;}if(JAR_ControllerTrigger(kind)){TriggerState(n,s);return;}if(kind==13){LaserState(n,s);return;}if(JAR_ControllerMedia(kind)){MediaState(n,s);return;}
 s->inactive=(e->flags&FL_INACTIVE)?1:0;s->flags=e->spawnflags;if(kind==6)s->speed=(int)e->speed;
 if(kind==1||JAR_ControllerPosition(kind))Q_strncpyz(s->text,e->message?e->message:"",sizeof(s->text));
 if(kind==3||kind==15){s->waitMs=JAR_NumberMillis((double)e->wait*1000);s->randomMs=JAR_NumberMillis((double)e->random*1000);}
 if(kind==14)s->active=e->use!=NULL;else if(kind==15){s->active=e->nextthink>0;s->flags=0;}
 if(kind==17)s->count=e->count;
 if(kind==33){s->active=e->genericValue1;s->speed=e->genericValue2;}
 if(kind==4){s->count=e->count;s->initial=e->genericValue1;s->repeats=e->bounceCount;}
}
static qboolean ControllerStateApply(int n,const jarControllerState_t*s){
 if(ControllerKind(n)==JAR_SEARCHLIGHT_CONTROLLER)return SearchlightApply(n,s);
 if(JAR_ControllerPortal(ControllerKind(n)))return PortalApply(n,s);
 if(ControllerKind(n)==JAR_DIRECTIONAL_CONTROLLER)return DirectionalApply(n,s);
 if(ControllerKind(n)==JAR_REFLECTION_CONTROLLER){
  if(!JAR_ControllerStateValid(JAR_REFLECTION_CONTROLLER,s))return qfalse;
  gentity_t* e=&g_entities[n];e->genericValue1=s->active;e->genericValue2=s->speed;
  if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
  ReflectionSync(e);return qtrue;
 }
 if(ControllerKind(n)==JAR_ALWAYS_CONTROLLER){
  if(!JAR_ControllerStateValid(JAR_ALWAYS_CONTROLLER,s))return qfalse;
  gentity_t*e=&g_entities[n];e->delay=s->waitMs;
  if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
  if(!s->active)AlwaysSchedule(e,0);else if(!e->nextthink)AlwaysSchedule(e,e->delay);
  return qtrue;
 }
 if(ControllerKind(n)==JAR_VISIBLE_CONTROLLER){
  if(!JAR_ControllerStateValid(JAR_VISIBLE_CONTROLLER,s))return qfalse;
  gentity_t*e=&g_entities[n];
  e->speed=(float)s->speed;e->damage=s->damage;e->spawnflags=s->flags;
  if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
  if(s->active!=e->genericValue1)VisibleArm(e,s->active);
  return qtrue;
 }
 if(ControllerKind(n)==JAR_DISTANCE_CONTROLLER){
  if(!JAR_ControllerStateValid(JAR_DISTANCE_CONTROLLER,s))return qfalse;
  gentity_t*e=&g_entities[n];e->speed=(float)s->speed;e->spawnflags=s->flags;
  if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
  return qtrue;
 }
 if(ControllerKind(n)==JAR_WALL_CONTROLLER){
  gentity_t*e=&g_entities[n];
  if(!JAR_ControllerStateValid(JAR_WALL_CONTROLLER,s)||!WallSet(e,s->active))return qfalse;
  if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
  return qtrue;
 }
 if(JAR_ControllerSlide(ControllerKind(n)))return SlidingDoorApply(&g_entities[n],s);
 if(ControllerKind(n)==JAR_BOBBING_CONTROLLER)return BobbingApply(&g_entities[n],s);
 if(JAR_ControllerAngular(ControllerKind(n)))return AngularApply(&g_entities[n],ControllerKind(n),s);
 if(ControllerKind(n)==JAR_BUTTON_CONTROLLER)return ButtonApply(&g_entities[n],s);
 if(JAR_ControllerLocation(ControllerKind(n)))return LocationApply(n,s);
 if(JAR_ControllerAnimatedTag(ControllerKind(n)))return AnimatedApply(n,s);
 if(JAR_ControllerShooter(ControllerKind(n)))return ShooterApply(n,s);
 if(ControllerKind(n)==JAR_DOOR_CAMERA_CONTROLLER)return DoorCameraApply(n,s);
 if(ControllerKind(n)==JAR_WELDER_CONTROLLER)return WelderApply(n,s);
 if(ControllerKind(n)==JAR_CRYSTAL_CRATE_CONTROLLER)return CrystalCrateApply(n,s);
 if(JAR_ControllerRack(ControllerKind(n))){
  jarControllerState_t live={0};
  RackState(n,&live);
  return live.flags==s->flags; // Copy seeds the rack before exposing its children.
 }
 if(JAR_ControllerEmplaced(ControllerKind(n)))return EmplacedApply(n,s);
 gentity_t*e=&g_entities[n];int kind=ControllerKind(n);if(kind==JAR_MUSIC_CONTROLLER)return MusicApply(n,s);if(JAR_ControllerBreakable(kind))return BreakApply(n,s);if(kind==29)return UsableApply(n,s);if(JAR_ControllerStation(kind))return StationApply(n,s);if(JAR_ControllerTrigger(kind))return TriggerApply(n,s);if(kind==13)return LaserApply(n,s);if(JAR_ControllerMedia(kind))return MediaApply(n,s);e->spawnflags=s->flags;
 if(JAR_ControllerMovement(kind))e->speed=(float)s->speed;
 if(kind==6){e->speed=(float)s->speed;e->noise_index=s->flags&1?G_SoundIndex("sound/weapons/force/jump.wav"):0;}
 if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
 if(kind==JAR_SECTION_CONTROLLER)SectionFireSync(n);
 if(kind==1||JAR_ControllerPosition(kind)){Q_strncpyz(objects[n].controllerText,s->text,sizeof(objects[n].controllerText));e->message=objects[n].controllerText;}
 if(kind==3||kind==15){e->wait=s->waitMs*.001f;e->random=s->randomMs*.001f;}
 if(kind==14)e->use=s->active?target_random_use:NULL;
 if(kind==15){if(!s->active)e->nextthink=0;else if(!e->nextthink){e->think=func_timer_think;e->nextthink=JAR_NumberDeadline(level.time,s->waitMs);e->activator=e;}}
 if(kind==17)e->count=s->count;
 if(kind==33){e->genericValue1=s->active;e->genericValue2=s->speed;LightSync(n);}
 if(kind==4){e->count=s->count;e->genericValue1=s->initial;e->bounceCount=s->repeats;}
 return qtrue;
}
static void ControllerForget(int n){
 MusicStop(n);
 for(int i=MAX_CLIENTS;i<level.num_entities;++i){
  for(int output=0;output<2;++output)if(objects[i].output[output]==n+1){objects[i].output[output]=0;++objects[i].propertyRevision;if(ControllerKind(i)==13){objects[i].laserActive=0;LaserSync(i,NULL);}}
  qboolean changed=qfalse;
  for(int input=0;input<2;++input)if(objects[i].distanceInput[input]==n+1){
   objects[i].distanceInput[input]=JAR_DISTANCE_LINK_MISSING;
   changed=qtrue;
  }
  if(changed&&i!=n){++objects[i].propertyRevision;SelectionChanged(i);}
 }
}
static qboolean ControllerReaches(int from,int to){
 int stack[MAX_GENTITIES],count=0;byte seen[MAX_GENTITIES]={0};stack[count++]=from;seen[from]=1;
 while(count){int n=stack[--count];if(n==to)return qtrue;for(int o=0;o<2;++o){int next=objects[n].output[o]-1;if(next>=MAX_CLIENTS&&next<ENTITYNUM_WORLD&&!seen[next]){seen[next]=1;stack[count++]=next;}}}return qfalse;
}
static void ControllerCaptureActivator(gentity_t*e,gentity_t*a){
 jarObject_t*o=&objects[e->s.number];o->activationClient=o->activationActor=0;
 if(!a||!a->inuse||!a->client||a->s.number<0||a->s.number>=ENTITYNUM_WORLD)return;
 int n=a->s.number;if(n<MAX_CLIENTS){if(a->client->pers.connected==CON_CONNECTED){o->activationClient=n+1;o->activationEpoch=controllerClientEpoch[n];}}
 else {o->activationActor=n+1;o->activationActorEpoch=controllerEntityEpoch[n];}
}
static gentity_t*ControllerActivator(gentity_t*e){
 jarObject_t*o=&objects[e->s.number];int c=o->activationClient-1;
 if(c>=0&&c<MAX_CLIENTS&&o->activationEpoch==controllerClientEpoch[c]&&g_entities[c].inuse&&g_entities[c].client&&g_entities[c].client->pers.connected==CON_CONNECTED)return &g_entities[c];
 c=o->activationActor-1;if(c>=MAX_CLIENTS&&c<ENTITYNUM_WORLD&&o->activationActorEpoch==controllerEntityEpoch[c]&&g_entities[c].inuse&&g_entities[c].client)return &g_entities[c];return e;
}
static void ControllerInvoke(gentity_t*e,gentity_t*other,gentity_t*activator){
 int n=e->s.number;if(n<0||n>=ENTITYNUM_WORLD||!e->inuse||!e->use||controllerCalling[n]||controllerDepth>=64)return;
 const int kind=ControllerKind(n);
 // Native triggers capture only accepted activations, after their cooldown checks.
 if(kind!=19&&kind!=20&&kind!=JAR_ALWAYS_CONTROLLER&&kind!=JAR_VISIBLE_CONTROLLER&&kind!=JAR_DISTANCE_CONTROLLER&&kind!=JAR_WALL_CONTROLLER&&kind!=JAR_BOBBING_CONTROLLER&&!JAR_ControllerAngular(kind)&&!JAR_ControllerSlide(kind)&&!JAR_ControllerBreakable(kind)&&!JAR_ControllerMovement(kind)&&kind!=JAR_DIRECTIONAL_CONTROLLER){
  ControllerCaptureActivator(e,activator);++objects[n].propertyRevision;
 }
 controllerCalling[n]=1;++controllerDepth;GlobalUse(e,other,activator?activator:e);--controllerDepth;controllerCalling[n]=0;
}
// State links deliberately do not invoke the destination. Inactive targets
// must remain reachable by Activate; native pending Delay events retain timing.
qboolean JAR_MP_SetActiveTargets(gentity_t*e,qboolean active){
 if(!e||!JAR_ControllerSwitch(ControllerKind(e->s.number)))return qfalse;
 if(!sandbox.integer||(e->flags&FL_INACTIVE)||objects[e->s.number].copyPending)return qtrue;
 jarObject_t*o=&objects[e->s.number];
 for(int output=0;output<2;++output){
  int n=o->output[output]-1;
  if(n==e->s.number||(output&&o->output[0]==o->output[1])||!ControllerKind(n)||JAR_ControllerRack(ControllerKind(n))||!ForcePermission(o->owner,n)||objects[n].copyPending)continue;
  gentity_t*target=&g_entities[n];int flags=active?(target->flags&~FL_INACTIVE):(target->flags|FL_INACTIVE);
  if(flags==target->flags)continue;
  target->flags=flags;++objects[n].propertyRevision;
  if(!active&&JAR_ControllerStation(ControllerKind(n)))StationStop(target);
  if(!active&&ControllerKind(n)==JAR_MUSIC_CONTROLLER)MusicStop(n);
  if(!active&&ControllerKind(n)==JAR_ALWAYS_CONTROLLER)AlwaysSchedule(target,0);
  if(!active&&JAR_ControllerShooter(ControllerKind(n)))ShooterStop(target);
  if(JAR_ControllerAnimatedTag(ControllerKind(n)))AnimatedSetEnabled(target);
  if(JAR_ControllerEmplaced(ControllerKind(n)))EmplacedEnabled(target);
  if(ControllerKind(n)==JAR_DOOR_CAMERA_CONTROLLER&&!active)target->s.loopSound=0;
  if(ControllerKind(n)==JAR_WELDER_CONTROLLER)WelderSetEnabled(target);
  if(JAR_ControllerMedia(ControllerKind(n)))MediaSync(n);else if(ControllerKind(n)==13)LaserSync(n,NULL);else if(JAR_ControllerTrigger(ControllerKind(n))||ControllerKind(n)==JAR_DIRECTIONAL_CONTROLLER)TriggerSync(n);else if(ControllerKind(n)==33)LightSync(n);else if(ControllerKind(n)==JAR_SECTION_CONTROLLER)SectionFireSync(n);
  SelectionChanged(n);
 }
 return qtrue;
}
qboolean JAR_MP_UseTargets(gentity_t*e,gentity_t*activator,const char*name){
 if(!e||!ControllerKind(e->s.number))return qfalse;
 if(JAR_ControllerShooter(ControllerKind(e->s.number)))return qtrue; // Primary is an aim link.
 if(!sandbox.integer||!name||((ControllerKind(e->s.number)==15||JAR_ControllerTrigger(ControllerKind(e->s.number)))&&(e->flags&FL_INACTIVE))||(JAR_ControllerTrigger(ControllerKind(e->s.number))&&!objects[e->s.number].triggerActive))return qtrue;
 int output=!strcmp(name,"@jar_secondary")?1:!strcmp(name,"@jar_primary")?0:-1;if(output<0)return qtrue;
 jarObject_t*o=&objects[e->s.number];int next=o->output[output]-1;
 if(next<MAX_CLIENTS||next>=ENTITYNUM_WORLD||!ForcePermission(o->owner,next)||objects[next].copyPending)return qtrue;
 // Delays must not act as a different player after connection-slot reuse.
 gentity_t*safe=ControllerActivator(e);
 ControllerInvoke(&g_entities[next],e,safe);return qtrue;
}
qboolean JAR_MP_RandomTargets(gentity_t*e,gentity_t*activator){
 if(!e||ControllerKind(e->s.number)!=14)return qfalse;
 if(!sandbox.integer||(e->flags&FL_INACTIVE)||objects[e->s.number].copyPending)return qtrue;
 if(e->spawnflags&1)e->use=NULL;
 int choices[2],count=0;jarObject_t*o=&objects[e->s.number];
 for(int i=0;i<2;++i){int n=o->output[i]-1;
  if(n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&n!=e->s.number&&!(i&&o->output[0]==o->output[1])&&ForcePermission(o->owner,n)&&!objects[n].copyPending&&g_entities[n].use&&!(g_entities[n].flags&FL_INACTIVE))choices[count++]=i;
 }
 if(count)JAR_MP_UseTargets(e,activator,choices[Q_irand(0,count-1)]?"@jar_secondary":"@jar_primary");
 SelectionChanged(e->s.number);return qtrue;
}
static void ControllerLink(gentity_t*p,int target,qboolean alternate){
 jarPlayer_t*cl=&players[p->s.number];if(alternate){cl->first=ENTITYNUM_NONE;Tell(p,"Link source cleared.");return;}
 if(!Edit(p,target)||!objects[target].catalog){Tell(p,"Link uses spawned objects you may edit; map mission wiring is preserved.");return;}
 if(cl->first==ENTITYNUM_NONE){if(!ControllerOutput(target)){Tell(p,"This entity's Link output controller is not available in MP yet.");return;}cl->first=target;Tell(p,"Source selected. Left click a spawned destination; right cancels.");return;}
 int source=cl->first;cl->first=ENTITYNUM_NONE;
 if(!Edit(p,source)||!ControllerOutput(source)||source==target){Tell(p,"Choose two different editable spawned entities.");return;}
 int kind=ControllerKind(source),dest=ControllerKind(target);qboolean spatial=kind==6||kind==7||kind==22||kind==23||(kind==10&&!cl->linkOutput);
 const int role=cl->linkOutput;
 if(role>=JAR_DISTANCE_REFERENCE){
  if(kind!=JAR_DISTANCE_CONTROLLER||role>=JAR_DISTANCE_LINKS){Tell(p,"Reference and NPC inputs are for distance triggers. Choose Primary for other controllers.");return;}
  if(!DistanceEndpoint(source,target)){Tell(p,"Choose an initialized spawned entity the controller owner may edit.");return;}
  if(role==JAR_DISTANCE_SUBJECT&&(!g_entities[target].NPC||!g_entities[target].client||g_entities[target].health<=0)){
   Tell(p,"Choose a living spawned NPC or vehicle for the NPC input.");return;
  }
  DistanceLinkChange(p,source,role,target+1);
  Tell(p,role==JAR_DISTANCE_REFERENCE?"Distance reference linked.":"NPC subject linked.");
  return;
 }
 if((kind==15||kind==JAR_STATIC_BRUSH_CONTROLLER)&&cl->linkOutput){Tell(p,"This controller uses Primary output.");return;}
 if((kind==JAR_ALWAYS_CONTROLLER||kind==JAR_VISIBLE_CONTROLLER)&&cl->linkOutput){Tell(p,"This trigger uses Primary output.");return;}
 if(kind==JAR_BOMB_CONTROLLER&&cl->linkOutput){Tell(p,"The planted bomb uses Primary output.");return;}
 if((JAR_ControllerTerminalBreakable(kind)||kind==JAR_CRYSTAL_CRATE_CONTROLLER)&&cl->linkOutput){Tell(p,"This breakable uses Primary output.");return;}
 if(JAR_ControllerPortal(kind)){
  if(role||(kind==JAR_PORTAL_SURFACE_CONTROLLER&&dest!=JAR_PORTAL_CAMERA_CONTROLLER)){
   Tell(p,kind==JAR_PORTAL_SURFACE_CONTROLLER?"Use Primary to link a portal camera.":"Use Primary to link a camera aim target.");return;
  }
 }
 else if(JAR_ControllerRack(dest)&&kind!=13&&!JAR_ControllerShooter(kind)&&!(kind==JAR_SEARCHLIGHT_CONTROLLER&&!role)){Tell(p,"The rack has no activation action.");return;}
 else if(JAR_ControllerEmplaced(dest)&&!JAR_ControllerSwitch(kind)&&!JAR_ControllerShooter(kind)&&!(kind==JAR_SEARCHLIGHT_CONTROLLER&&!role)){Tell(p,"Use an Enable or Disable controller to link an emplaced gun.");return;}
 else if(kind==JAR_SEARCHLIGHT_CONTROLLER&&!role){ /* Aim may track any owned entity. */ }
 else if(kind==13||JAR_ControllerShooter(kind)){if(cl->linkOutput){Tell(p,"Aiming uses Primary. Choose a spawned entity or marker.");return;}}
 else if(kind==16){if(!g_entities[target].item||jarMPCatalog[objects[target].catalog-1].kind!=4){Tell(p,"Give links use spawned pickups.");return;}}
 else if(spatial){if(!JAR_ControllerPosition(dest)){Tell(p,"Aim/teleport destinations need a position, teleport destination or player-start marker.");return;}if(cl->linkOutput){Tell(p,"Choose Primary output for a destination link.");return;}}
 else if(!dest||(!JAR_ControllerSwitch(kind)&&(!g_entities[target].use||dest==JAR_SECTION_CONTROLLER))){Tell(p,"This destination has no activation callback.");return;}
 if(ControllerReaches(target,source)){Tell(p,"This connection would create an activation cycle.");return;}
 if(kind==JAR_DISTANCE_CONTROLLER){
  DistanceLinkChange(p,source,role,target+1);
  Tell(p,role==JAR_DISTANCE_OUTSIDE?"Outside-range output linked.":"Inside-range output linked.");
  return;
 }
 objects[source].output[role]=target+1;++objects[source].propertyRevision;SelectionChanged(source);
 if(JAR_ControllerTrigger(kind))TriggerSync(source);
 if(kind==13)LaserSync(source,NULL);
 if(JAR_ControllerShooter(kind)){Tell(p,"Shooter aim linked to the destination.");return;}
 if(kind==JAR_BUTTON_CONTROLLER){Tell(p,cl->linkOutput?"Button Released output linked.":"Button Pressed output linked.");return;}
 if(kind==JAR_LIFT_CONTROLLER){Tell(p,cl->linkOutput?"Lift Lowered output linked.":"Lift Raised output linked.");return;}
 if(kind==JAR_DIRECTIONAL_CONTROLLER){Tell(p,cl->linkOutput?"Reverse output linked.":"Forward output linked.");return;}
 if(kind==JAR_SEARCHLIGHT_CONTROLLER){Tell(p,role?"Searchlight detection output linked.":"Searchlight aim linked.");return;}
 if(JAR_ControllerPortal(kind)){Tell(p,kind==JAR_PORTAL_SURFACE_CONTROLLER?"Portal camera linked.":"Portal camera aim linked.");return;}
 Tell(p,kind==16?"Pickup linked. Activate Give to collect its linked items.":kind==13?"Laser target linked. It follows the entity while permission is retained.":JAR_ControllerSwitch(kind)?"Enabled-state link added. Activate this controller to apply it.":spatial?"Destination marker linked. Move it to change the destination.":cl->linkOutput?"Secondary output linked to destination.":"Primary output linked to destination.");
}
static void ControllerActivate(gentity_t*p,int target,qboolean alternate){
 if(!Manipulate(p,target)){Tell(p,"Select an editable entity to activate.");return;}
 if(alternate){players[p->s.number].selected=target;SelectionEcho(p);Tell(p,va("Selected %s (#%d).",g_entities[target].classname?g_entities[target].classname:"entity",target));return;}
 if(!g_entities[target].use){Tell(p,"This entity has no native activation action.");return;}
 if(objects[target].copyPending){Tell(p,"Wait for the spawned entity to initialize.");return;}
 const int kind=ControllerKind(target);
 if((JAR_ControllerAnimatedTag(kind)||kind==JAR_DOOR_CAMERA_CONTROLLER||kind==JAR_WELDER_CONTROLLER||kind==JAR_ALWAYS_CONTROLLER||kind==JAR_VISIBLE_CONTROLLER||kind==JAR_DISTANCE_CONTROLLER||kind==JAR_WALL_CONTROLLER||JAR_ControllerSlide(kind)||kind==JAR_BUTTON_CONTROLLER||kind==JAR_BOBBING_CONTROLLER||JAR_ControllerAngular(kind)||JAR_ControllerMovement(kind)||kind==JAR_DIRECTIONAL_CONTROLLER||kind==JAR_SEARCHLIGHT_CONTROLLER||kind==JAR_STATIC_BRUSH_CONTROLLER||kind==JAR_SPACE_CONTROLLER)&&(g_entities[target].flags&FL_INACTIVE)){
  Tell(p,"Enable the controller before activating it.");return;
 }
 if(kind==JAR_BOMB_CONTROLLER&&g_entities[target].s.time!=JAR_MP_BOMB_ARMED){Tell(p,"The planted bomb was already disarmed.");return;}
 if(JAR_ControllerShooter(kind)&&(g_entities[target].flags&FL_INACTIVE)){
  Tell(p,"Enable the shooter before firing.");return;
 }
 if(JAR_ControllerStockExplosive(kind)){
  if(g_entities[target].flags&FL_INACTIVE){Tell(p,"Enable the explosive before detonating it.");return;}
  if(g_entities[target].genericValue11==2){Tell(p,"Detonation pending.");return;}
 }
 if(kind==JAR_CRYSTAL_CRATE_CONTROLLER&&(g_entities[target].flags&FL_INACTIVE)){Tell(p,"Enable the crystal crate before detonating it.");return;}
 if(JAR_ControllerEmplaced(kind)){
  gentity_t*gun=&g_entities[target];
  if(!JAR_MP_EmplacedAllowsSeat(gun,p)){Tell(p,"Freeze the gun before mounting it.");return;}
  vec3_t eye,center;trace_t tr;
  VectorCopy(p->client->ps.origin,eye);eye[2]+=p->client->ps.viewheight;
  VectorAdd(gun->r.mins,gun->r.maxs,center);VectorMA(gun->r.currentOrigin,.5f,center,center);
  trap->Trace(&tr,eye,NULL,NULL,center,p->s.number,MASK_PLAYERSOLID,qfalse,0,0);
  if(tr.entityNum!=target||!JAR_MP_NativeEmplacedUse(gun,p))Tell(p,"Approach the gun from behind while standing to mount it.");
  return;
 }
 const qboolean wasHiddenBeacon=kind==JAR_BEACON_CONTROLLER&&BodyDormant(target);
 ControllerInvoke(&g_entities[target],p,p);
 if(wasHiddenBeacon&&BodyDormant(target)){Tell(p,"Beacon deployment is blocked.");return;}
 if(kind!=6&&kind!=7&&kind!=JAR_SECTION_CONTROLLER&&kind!=JAR_DISTANCE_CONTROLLER&&kind!=JAR_WALL_CONTROLLER&&!JAR_ControllerSlide(kind)&&!JAR_ControllerStart(kind))Tell(p,"Entity activated.");
}
