// Portable controller properties; no pointers, target names or executable text.
// GPL-2.0-only.
#pragma once
#include <string.h>
#include "bg_jar_movement.h"
#include "jar_mp_number.h"
#include "jar_mp_media.h"
#include "jar_mp_music.h"
#include "jar_mp_animated.h"
#include "jar_mp_door_camera.h"
#include "jar_mp_welder.h"
#include "jar_mp_crystal_crate.h"
#include "jar_mp_emplaced.h"
#include "../../shared/qcommon/jar_always_trigger.h"
#include "../../shared/qcommon/jar_visible_trigger.h"
#include "../../shared/qcommon/jar_distance_trigger.h"
#include "../../shared/qcommon/jar_wall.h"
#include "jar_mp_sliding_door.h"
#include "qcommon/jar_probe.h"
#include "qcommon/jar_button.h"
#include "jar_mp_bobbing.h"
#include "jar_mp_searchlight.h"
#include "jar_mp_portal.h"
#include "../../shared/qcommon/jar_angular_mover.h"
#include "../../shared/qcommon/jar_movement_limits.h"
#include "../../shared/qcommon/jar_directional_trigger.h"
typedef struct {
 int waitMs,randomMs,flags,repeats,count,initial,inactive,speed,active,damage,pushSpeed;
 char text[64];
} jarControllerState_t;
enum {
 JAR_SPACE_FLOOR_ANCHOR=1,
 // These fields are consumed by native world geometry and packed gameplay data.
 JAR_MP_GEOMETRY_SPAN=WORLD_SIZE,
 JAR_MP_ENTITY_HEALTH_MAX=(1<<10)-1,
 JAR_MP_DAMAGE_MAX=SHRT_MAX,
 JAR_MP_ANGLE_MAX=180,
 JAR_MP_CONE_MAX=90,
 JAR_MP_SLIDE_SPEED_MAX=INT_MAX/100
};
// Space uses the existing speed/damage/pushSpeed slots for width/depth/height.
// A 1.204 record has only speed, which was the cube's half-size.
static void JAR_SpaceDimensions(const jarControllerState_t*s,int size[3]){
 if(s->damage&&s->pushSpeed){size[0]=s->speed;size[1]=s->damage;size[2]=s->pushSpeed;}
 else size[0]=size[1]=size[2]=2*s->speed;
}
static void JAR_SpaceBounds(const jarControllerState_t*s,vec3_t lo,vec3_t hi){
 int size[3];JAR_SpaceDimensions(s,size);
 for(int a=0;a<3;++a){
  lo[a]=a==2&&(s->flags&JAR_SPACE_FLOOR_ANCHOR)?0:-.5f*size[a];
  hi[a]=lo[a]+size[a];
 }
}
enum {
 JAR_SECTION_CONTROLLER = 35, JAR_CARGO_BARREL_CONTROLLER, JAR_EMPLACED_CONTROLLER,
 JAR_EWEB_CONTROLLER, JAR_EXPLODING_CRATE_CONTROLLER, JAR_GAS_TANK_CONTROLLER,
 JAR_PLASMA_SHOOTER_CONTROLLER, JAR_GRENADE_SHOOTER_CONTROLLER, JAR_ROCKET_SHOOTER_CONTROLLER,
 JAR_BOMB_CONTROLLER, JAR_BEACON_CONTROLLER, JAR_SMALL_CARGO_CONTROLLER,
 JAR_GUN_RACK_CONTROLLER, JAR_AMMO_RACK_CONTROLLER, JAR_DOOR_CAMERA_CONTROLLER,
 JAR_WELDER_CONTROLLER, JAR_CRYSTAL_CRATE_CONTROLLER, JAR_BLASTER_SHOOTER_CONTROLLER,
 JAR_LOCATION_MARKER_CONTROLLER, JAR_LOCATION_VOLUME_CONTROLLER,
 JAR_ALWAYS_CONTROLLER,
 JAR_PLAYER_START_CONTROLLER, JAR_DEATHMATCH_START_CONTROLLER,
 JAR_VISIBLE_CONTROLLER,
 JAR_DISTANCE_CONTROLLER,
 JAR_WALL_CONTROLLER,
 JAR_SLIDING_DOOR_CONTROLLER,
 JAR_BUTTON_CONTROLLER,
 JAR_LIFT_CONTROLLER,
 JAR_REFLECTION_CONTROLLER,
 JAR_BOBBING_CONTROLLER,
 JAR_GRAVITY_CONTROLLER,
 JAR_FRICTION_CONTROLLER,
 JAR_DIRECTIONAL_CONTROLLER,
 JAR_SEARCHLIGHT_CONTROLLER,
 JAR_PORTAL_SURFACE_CONTROLLER, JAR_PORTAL_CAMERA_CONTROLLER,
 JAR_SPINNER_CONTROLLER, JAR_PENDULUM_CONTROLLER,
 JAR_STATIC_BRUSH_CONTROLLER,
 JAR_SPACE_CONTROLLER,
 JAR_CONTROLLER_MAX = JAR_SPACE_CONTROLLER
};
#define JAR_MP_SPINNER_ID 88599552u
#define JAR_MP_PENDULUM_ID 97415810u
enum { JAR_ANGULAR_STOPPED, JAR_ANGULAR_MOVING, JAR_ANGULAR_BLOCKED };
#define JAR_MP_DISTANCE_ID 89037917u
#define JAR_MP_SMALL_CARGO_ID 87961440u
#define JAR_MP_GUN_RACK_ID 96929416u
#define JAR_MP_AMMO_RACK_ID 98056028u
#define JAR_MP_RACK_ITEM_TAG 248
enum { JAR_MP_GUN_RACK_SLOTS = 3, JAR_MP_GUN_RACK_MASK = (1 << JAR_MP_GUN_RACK_SLOTS) - 1,
       JAR_MP_AMMO_RACK_SLOTS = 5, JAR_MP_AMMO_RACK_MASK = (1 << JAR_MP_AMMO_RACK_SLOTS) - 1 };

static int JAR_ControllerRack(int kind){return kind==JAR_GUN_RACK_CONTROLLER||kind==JAR_AMMO_RACK_CONTROLLER;}
static int JAR_ControllerSlide(int kind){return kind==JAR_SLIDING_DOOR_CONTROLLER||kind==JAR_LIFT_CONTROLLER;}
static int JAR_MP_RackSlotCount(int kind){
 return kind==JAR_GUN_RACK_CONTROLLER?JAR_MP_GUN_RACK_SLOTS:kind==JAR_AMMO_RACK_CONTROLLER?JAR_MP_AMMO_RACK_SLOTS:0;
}
static int JAR_MP_RackSlotsRemaining(int kind,int consumedMask){
 const int slots=JAR_MP_RackSlotCount(kind);
 int remaining=0;
 for(int slot=0;slot<slots;++slot)if(!(consumedMask&(1<<slot)))++remaining;
 return remaining;
}
// ET_ITEM keeps its native item index. The tag marks a dropped item
// whose secondary model index supplies the stock SP presentation.
#define JAR_MP_CARGO_DROP_TAG 249
#define JAR_MP_CARGO_DROP_MODEL "models/items/medpac.md3"
enum { JAR_GAS_TANK_PULSE_SPENT = 2, JAR_GAS_TANK_IDLE_MAX = 28000, JAR_STOCK_EXPLOSIVE_FUSE = 50 };
enum { JAR_SHOOTER_ALT = 1, JAR_SHOOTER_REPEAT = 2, JAR_SHOOTER_MIN_MS = 50, JAR_SHOOTER_MAX_MS = 60000 };
static int JAR_ControllerEmplaced(int kind){return kind==JAR_EMPLACED_CONTROLLER||kind==JAR_EWEB_CONTROLLER;}
static int JAR_ControllerDebris(int kind){return kind>=30&&kind<=32;}
static int JAR_ControllerStockExplosive(int kind){return kind==JAR_EXPLODING_CRATE_CONTROLLER||kind==JAR_GAS_TANK_CONTROLLER;}
static int JAR_ControllerTerminalBreakable(int kind){return kind==JAR_CARGO_BARREL_CONTROLLER||kind==JAR_SMALL_CARGO_CONTROLLER||JAR_ControllerStockExplosive(kind);}
static int JAR_ControllerBreakable(int kind){return JAR_ControllerDebris(kind)||JAR_ControllerTerminalBreakable(kind)||kind==JAR_CRYSTAL_CRATE_CONTROLLER;}
static int JAR_ControllerStation(int kind){return kind>=24&&kind<=28;}
static int JAR_ControllerTrigger(int kind){return (kind>=19&&kind<=23)||kind==JAR_SPACE_CONTROLLER;}
static int JAR_ControllerMedia(int kind){return kind==9||kind==10;}
static int JAR_ControllerSwitch(int kind){return kind==11||kind==12;}
static int JAR_ControllerShooter(int kind){
 return (kind>=JAR_PLASMA_SHOOTER_CONTROLLER&&kind<=JAR_ROCKET_SHOOTER_CONTROLLER)||kind==JAR_BLASTER_SHOOTER_CONTROLLER;
}
static int JAR_ControllerLocation(int kind){return kind==JAR_LOCATION_MARKER_CONTROLLER||kind==JAR_LOCATION_VOLUME_CONTROLLER;}
static int JAR_ControllerStart(int kind){return kind==JAR_PLAYER_START_CONTROLLER||kind==JAR_DEATHMATCH_START_CONTROLLER;}
static int JAR_ControllerMovement(int kind){return kind==JAR_GRAVITY_CONTROLLER||kind==JAR_FRICTION_CONTROLLER;}
static int JAR_ControllerPointGizmo(int kind){return JAR_ControllerStart(kind)||kind==JAR_VISIBLE_CONTROLLER||kind==JAR_DISTANCE_CONTROLLER||kind==JAR_REFLECTION_CONTROLLER||kind==JAR_DIRECTIONAL_CONTROLLER||kind==JAR_SPACE_CONTROLLER||JAR_ControllerMovement(kind);}
static int JAR_ControllerPosition(int kind){return kind==5||kind==8||JAR_ControllerStart(kind);}
static int JAR_ControllerPortal(int kind){return kind==JAR_PORTAL_SURFACE_CONTROLLER||kind==JAR_PORTAL_CAMERA_CONTROLLER;}
static int JAR_ControllerAngular(int kind){return kind==JAR_SPINNER_CONTROLLER||kind==JAR_PENDULUM_CONTROLLER;}
static int JAR_ControllerAnimatedTag(int kind){return kind==JAR_BOMB_CONTROLLER?JAR_MP_BOMB_TAG:kind==JAR_BEACON_CONTROLLER?JAR_MP_BEACON_TAG:0;}
static int JAR_ControllerText(const char*s){
 size_t n;if(!s||(n=strlen(s))>63||!n)return 0;
 for(;*s;++s){unsigned char c=(unsigned char)*s;if(c<32||c>126||c=='"'||c=='\\'||c==';')return 0;}
 return 1;
}
static int JAR_ControllerInterval(int interval,int jitter,int minimum,int centered){
 return interval>=minimum&&jitter>=0&&jitter<=INT_MAX-interval&&(!centered||jitter<=interval-minimum);
}
static int JAR_MP_ButtonValid(jarButtonState_t state){
 if(state.waitMs<0||!JAR_ButtonFrameValid(state.phase,state.frame))return 0;
 if(state.phase==JAR_BUTTON_IDLE)return !state.remaining;
 return state.remaining>0&&state.remaining<=(state.phase==JAR_BUTTON_HELD?state.waitMs:JAR_BUTTON_TICK);
}
static int JAR_MP_DistanceSettingsValid(int range,int flags){
 return range>0&&range<=JAR_MP_GEOMETRY_SPAN&&flags>=JAR_DISTANCE_PLAYER&&flags<=(JAR_DISTANCE_PLAYER|JAR_DISTANCE_NPC);
}
static int JAR_MP_AngularSettingsValid(int pendulum,int axis,int rate,int period,int phase){
 if(axis<0||axis>=JAR_ANGULAR_AXES)return 0;
 // The swept-motion step must represent at least one millisecond.
 if(!pendulum)return rate>=-8000&&rate<=8000&&period==JAR_ANGULAR_PERIOD&&!phase;
 return rate>=0&&rate<=JAR_MP_ANGLE_MAX&&period>0&&phase>=0&&phase<period&&rate*6283.185307179586<=8000.0*period;
}
static int JAR_ControllerStateValid(int kind,const jarControllerState_t*s){
 if(kind<0||kind>JAR_CONTROLLER_MAX||s->inactive<0||s->inactive>1||!memchr(s->text,0,sizeof(s->text)))return 0;
 if(kind==JAR_SPACE_CONTROLLER){
  if(s->text[0]||s->waitMs||s->randomMs||s->repeats||s->count||s->initial||s->active<0||s->active>1)return 0;
  if(!s->damage&&!s->pushSpeed)return !s->flags&&s->speed>0&&s->speed<=JAR_MP_GEOMETRY_SPAN/2;
  if(s->flags&~JAR_SPACE_FLOOR_ANCHOR)return 0;
  const int size[3]={s->speed,s->damage,s->pushSpeed};
  for(int a=0;a<3;++a)if(size[a]<=0||size[a]>JAR_MP_GEOMETRY_SPAN)return 0;
  return 1;
 }
 if(JAR_ControllerAngular(kind)){
  return JAR_MP_AngularSettingsValid(kind==JAR_PENDULUM_CONTROLLER,s->flags,s->speed,s->waitMs,s->pushSpeed)&&
   s->active>=0&&s->active<=1&&s->repeats>=JAR_ANGULAR_STOPPED&&s->repeats<=JAR_ANGULAR_BLOCKED&&
   !s->text[0]&&!s->randomMs&&!s->count&&!s->initial&&!s->damage;
 }
 if(JAR_ControllerPortal(kind)){
  const int surface=kind==JAR_PORTAL_SURFACE_CONTROLLER;
  return s->active>=0&&s->active<=1&&s->flags>=0&&s->flags<=1&&
   (surface?(s->speed>0&&s->speed<=JAR_MP_GEOMETRY_SPAN&&
             s->pushSpeed>0&&s->pushSpeed<=USHRT_MAX):(!s->speed&&!s->pushSpeed))&&
   !s->text[0]&&!s->waitMs&&!s->randomMs&&!s->damage&&!s->repeats&&!s->count&&!s->initial;
 }
 if(kind==JAR_SEARCHLIGHT_CONTROLLER){
  return s->speed>0&&s->speed<=JAR_MP_GEOMETRY_SPAN&&
   s->damage>=0&&s->damage<=JAR_MP_CONE_MAX&&
   s->pushSpeed>=0&&s->pushSpeed<=USHRT_MAX&&s->active>=0&&s->active<=1&&
   s->flags>=0&&s->flags<=(JAR_SEARCHLIGHT_TRACK|JAR_SEARCHLIGHT_WHITE)&&
   !s->text[0]&&!s->waitMs&&!s->randomMs&&!s->repeats&&!s->count&&!s->initial;
 }
 if(kind==JAR_DIRECTIONAL_CONTROLLER){
  return s->speed>0&&s->speed<=JAR_MP_GEOMETRY_SPAN/2&&
   s->waitMs>=-1&&s->initial>=0&&
   s->count>=0&&s->count<=s->initial&&s->active>=0&&s->active<=1&&s->repeats>=0&&s->repeats<=1&&
   (!s->repeats||!s->active)&&(!s->initial||s->count||s->repeats)&&
   (!s->repeats||s->waitMs==-1||(s->initial&&!s->count))&&
   !(s->flags&~JAR_DIRECTIONAL_PLAYER_ONLY)&&!s->text[0]&&!s->randomMs&&!s->damage&&!s->pushSpeed;
 }
 if(JAR_ControllerMovement(kind)){
  return s->speed>=0&&s->speed<=JAR_MP_ACTOR_MOVEMENT_MAX&&!s->text[0]&&
   !s->waitMs&&!s->randomMs&&!s->flags&&!s->repeats&&!s->count&&!s->initial&&!s->active&&!s->damage&&!s->pushSpeed;
 }
 if(kind==JAR_REFLECTION_CONTROLLER){
  return JAR_ProbeRadiusValid(s->speed)&&s->active>=0&&s->active<=1&&!s->text[0]&&
   !s->waitMs&&!s->randomMs&&!s->flags&&!s->repeats&&!s->count&&!s->initial&&!s->damage&&!s->pushSpeed;
 }
 if(kind==JAR_BOBBING_CONTROLLER){
  return s->speed>=0&&s->speed<=JAR_MP_GEOMETRY_SPAN&&
   s->waitMs>0&&
   s->pushSpeed>=0&&s->pushSpeed<JAR_BOB_CYCLE&&s->active>=0&&s->active<=1&&
   s->repeats>=JAR_BOB_STOPPED&&s->repeats<=JAR_BOB_BLOCKED&&
   !s->text[0]&&!s->randomMs&&!s->flags&&!s->count&&!s->initial&&!s->damage;
 }
 if(JAR_ControllerStart(kind)){
  return JAR_ControllerText(s->text)&&s->flags>=0&&s->flags<=1&&!s->waitMs&&!s->randomMs&&
   !s->repeats&&!s->count&&!s->initial&&!s->speed&&!s->active&&!s->damage&&!s->pushSpeed;
 }
 if(kind==JAR_ALWAYS_CONTROLLER){
  return s->waitMs>0&&s->active>=0&&s->active<=1&&
   (!s->inactive||!s->active)&&!s->text[0]&&!s->randomMs&&!s->flags&&!s->repeats&&
   !s->count&&!s->initial&&!s->speed&&!s->damage&&!s->pushSpeed;
 }
 if(kind==JAR_VISIBLE_CONTROLLER){
  return s->speed>0&&s->speed<=JAR_MP_GEOMETRY_SPAN&&s->damage>=0&&s->damage<=JAR_MP_CONE_MAX&&
   !(s->flags&~(JAR_VISIBLE_IGNORE_WALLS|JAR_VISIBLE_FORCE_SIGHT))&&s->active>=0&&s->active<=1&&!s->text[0]&&
   !s->waitMs&&!s->randomMs&&!s->repeats&&!s->count&&!s->initial&&!s->pushSpeed;
 }
 if(kind==JAR_DISTANCE_CONTROLLER){
  return JAR_MP_DistanceSettingsValid(s->speed,s->flags)&&!s->text[0]&&!s->waitMs&&!s->randomMs&&!s->repeats&&
   !s->active&&!s->damage&&!s->pushSpeed&&s->count>=0&&s->count<=2&&s->initial>=0&&s->initial<=2;
 }
 if(kind==JAR_WALL_CONTROLLER){
  return s->active>=0&&s->active<=1&&!s->text[0]&&!s->waitMs&&!s->randomMs&&!s->flags&&!s->repeats&&
   !s->count&&!s->initial&&!s->speed&&!s->damage&&!s->pushSpeed;
 }
 if(JAR_ControllerSlide(kind)){
  return !s->text[0]&&!s->randomMs&&!s->count&&!s->initial&&s->active>=0&&s->active<=1&&
   s->flags>=0&&s->flags<JAR_SLIDE_DIRECTIONS&&(kind!=JAR_LIFT_CONTROLLER||s->flags==JAR_SLIDE_UP)&&
   s->repeats>=JAR_SLIDE_REST&&s->repeats<=JAR_SLIDE_PAUSED&&
   s->waitMs>=0&&
   s->speed>0&&s->speed<=JAR_MP_SLIDE_SPEED_MAX&&
   s->damage>0&&s->damage<=JAR_MP_GEOMETRY_SPAN&&
   s->pushSpeed>=0&&s->pushSpeed<=s->damage*JAR_SLIDE_MILLI;
 }
 if(kind==JAR_BUTTON_CONTROLLER){
  return !s->text[0]&&!s->randomMs&&!s->flags&&!s->count&&!s->initial&&!s->repeats&&
   !s->speed&&!s->damage&&!s->pushSpeed&&s->active>=JAR_BUTTON_IDLE&&s->active<=JAR_BUTTON_RETURNING&&
   s->waitMs>=0;
 }
 if(JAR_ControllerLocation(kind)){
  return JAR_ControllerText(s->text)&&s->flags>=0&&s->flags<=7&&
   (kind==JAR_LOCATION_VOLUME_CONTROLLER?(s->speed>0&&s->speed<=JAR_MP_GEOMETRY_SPAN/2):!s->speed)&&
   !s->waitMs&&!s->randomMs&&!s->repeats&&!s->count&&!s->initial&&!s->active&&!s->damage&&!s->pushSpeed;
 }
 if(kind==JAR_MUSIC_CONTROLLER){
  int first,second;
  return !s->waitMs&&!s->randomMs&&!s->flags&&!s->repeats&&!s->count&&!s->initial&&!s->speed&&!s->damage&&!s->pushSpeed&&s->active>=0&&s->active<=1&&(!s->inactive||!s->active)&&JAR_MusicTracks(s->text,1,&first,&second);
 }
 if(kind==33)return !s->text[0]&&!s->waitMs&&!s->randomMs&&!s->flags&&!s->repeats&&!s->count&&!s->initial&&!s->damage&&!s->pushSpeed&&s->active>=0&&s->active<=1&&s->speed>=4&&s->speed<=1020&&!(s->speed%4);
 if(kind==JAR_DOOR_CAMERA_CONTROLLER){
  return !s->text[0]&&!s->waitMs&&!s->randomMs&&!s->flags&&!s->repeats&&!s->count&&!s->initial&&
   s->active>=0&&s->active<=1&&
   s->damage>0&&s->damage<=JAR_MP_GEOMETRY_SPAN*JAR_MP_DOOR_CAMERA_MILLI&&
   s->speed>0&&
   s->pushSpeed>=0&&s->pushSpeed<=s->damage;
 }
 if(kind==JAR_WELDER_CONTROLLER){
  return !s->text[0]&&!s->randomMs&&!s->flags&&!s->repeats&&!s->count&&!s->initial&&
   s->active>=0&&s->active<=1&&s->damage>=0&&s->damage<=JAR_MP_DAMAGE_MAX&&
   s->waitMs>0&&
   s->speed>0&&s->speed<=JAR_MP_GEOMETRY_SPAN*JAR_MP_WELDER_MILLI&&
   s->pushSpeed>=0&&s->pushSpeed<=JAR_MP_GEOMETRY_SPAN*JAR_MP_WELDER_MILLI;
 }
 if(kind==JAR_CRYSTAL_CRATE_CONTROLLER){
  return !s->text[0]&&!s->waitMs&&!s->randomMs&&!(s->flags&~JAR_MP_CRYSTAL_CRATE_SHOT_ONLY)&&s->active==1&&
   s->initial>=1&&s->initial<=INT_MAX-JAR_MP_DAMAGE_MAX&&s->count>=1&&s->count<=s->initial&&
   s->damage>=0&&s->damage<=JAR_MP_DAMAGE_MAX&&s->repeats>=0&&s->repeats<=JAR_MP_DAMAGE_MAX&&
   s->speed>=0&&s->speed<=JAR_MP_GEOMETRY_SPAN&&
   s->pushSpeed>=0&&s->pushSpeed<=JAR_MP_GEOMETRY_SPAN*JAR_MP_CRYSTAL_CRATE_MILLI;
 }
 if(JAR_ControllerRack(kind)){
  const int mask=kind==JAR_AMMO_RACK_CONTROLLER?JAR_MP_AMMO_RACK_MASK:JAR_MP_GUN_RACK_MASK;
  return !s->text[0]&&!s->waitMs&&!s->randomMs&&!(s->flags&~mask)&&
   !s->repeats&&!s->count&&!s->initial&&!s->inactive&&!s->speed&&!s->active&&!s->damage&&!s->pushSpeed;
 }
 if(JAR_ControllerAnimatedTag(kind)){
  return !s->text[0]&&!s->waitMs&&!s->randomMs&&!s->flags&&!s->repeats&&!s->count&&!s->initial&&
   !s->speed&&!s->damage&&!s->pushSpeed&&s->active>=0&&s->active<=2;
 }
 if(JAR_ControllerShooter(kind)){
  return !s->text[0]&&!s->repeats&&!s->count&&!s->initial&&!s->speed&&!s->damage&&!s->pushSpeed&&
   !(s->flags&~(JAR_SHOOTER_ALT|JAR_SHOOTER_REPEAT))&&JAR_ControllerInterval(s->waitMs,s->randomMs,1,0)&&s->active>=0&&s->active<=1&&
   (!s->active||(!s->inactive&&(s->flags&JAR_SHOOTER_REPEAT)));
 }
 if(JAR_ControllerEmplaced(kind)){
  if(s->text[0]||s->waitMs||s->randomMs||s->flags||s->repeats||s->damage||s->initial<1||s->initial>INT_MAX-JAR_MP_DAMAGE_MAX||
     s->pushSpeed<0||s->pushSpeed>(kind==JAR_EWEB_CONTROLLER?JAR_MP_EWEB_WIRE_AMMO_MAX:0)||
     s->speed<0||s->speed>JAR_MP_ANGLE_MAX||s->active<0||s->active>2)return 0;
  return s->active==1?s->count>0&&s->count<=s->initial:s->count==0;
 }
 if(JAR_ControllerTerminalBreakable(kind)){
  const int allowedFlags=kind==JAR_GAS_TANK_CONTROLLER?JAR_GAS_TANK_PULSE_SPENT:0;
  if(s->text[0]||s->waitMs||s->randomMs||(s->flags&~allowedFlags)||s->repeats||s->pushSpeed||s->initial<1||s->initial>INT_MAX-JAR_MP_DAMAGE_MAX||
     s->damage<0||s->damage>JAR_MP_DAMAGE_MAX||s->speed<0||s->speed>JAR_MP_GEOMETRY_SPAN)return 0;
  return (s->active==1&&s->count>0&&s->count<=s->initial)||(s->active==2&&!s->count);
 }
 if(JAR_ControllerDebris(kind)){
  if(s->text[0]||s->waitMs<0||s->randomMs<0||s->flags<0||(s->flags&~(1|16|32|64|128|2048))||((s->flags&48)==48)||s->active<0||s->active>2||s->initial<1||s->initial>INT_MAX-JAR_MP_DAMAGE_MAX||s->count<0||s->count>s->initial||s->repeats<0||s->repeats>16||s->damage<0||s->damage>JAR_MP_DAMAGE_MAX||s->speed<0||s->speed>JAR_MP_GEOMETRY_SPAN||s->pushSpeed<0)return 0;
  return s->active==1?s->count>0:s->count==0;
 }
 if(kind==29){
  if(s->text[0]||s->randomMs||s->speed||s->damage||s->pushSpeed||s->waitMs<0||s->flags<0||(s->flags&~120)||s->active<0||s->active>1||s->repeats<0||s->repeats>3||s->initial<0||s->initial>INT_MAX-JAR_MP_DAMAGE_MAX||s->count<0||s->count>s->initial)return 0;
  if(s->flags&8)return s->active&&s->repeats!=1&&(s->repeats!=2||s->waitMs>0);
  return !s->waitMs&&s->repeats<=1&&(!s->repeats||!s->active);
 }
 if(kind==1||kind==5||kind==8){if(!JAR_ControllerText(s->text))return 0;}else if(JAR_ControllerMedia(kind)){if(!JAR_MediaPath(kind,s->text))return 0;}else if(s->text[0])return 0;
 if(s->active<0||s->active>1||(!JAR_ControllerMedia(kind)&&kind!=13&&kind!=14&&kind!=15&&!JAR_ControllerTrigger(kind)&&s->active))return 0;
 if(kind==13||kind==21){if(s->damage<0||s->damage>JAR_MP_DAMAGE_MAX)return 0;}else if(s->damage)return 0;
 if(JAR_ControllerTrigger(kind)){if(s->speed<=0||s->speed>JAR_MP_GEOMETRY_SPAN/2)return 0;}else if(kind==6){if(s->speed< -JAR_MP_GEOMETRY_SPAN||s->speed>JAR_MP_GEOMETRY_SPAN)return 0;}else if(s->speed)return 0;
 if(kind==22){if(s->pushSpeed< -JAR_MP_GEOMETRY_SPAN||s->pushSpeed>JAR_MP_GEOMETRY_SPAN)return 0;}else if(s->pushSpeed)return 0;
 if(JAR_ControllerStation(kind)){if(s->waitMs<=0||s->randomMs)return 0;}
 else if(kind==3||kind==15||kind==19){if(!JAR_ControllerInterval(s->waitMs,s->randomMs,1,1))return 0;}
 else if(kind==22){if(s->randomMs||s->waitMs< -1||(!(s->flags&4)&&s->waitMs))return 0;}
 else if(JAR_ControllerMedia(kind)){
  if(!JAR_ControllerInterval(s->waitMs,s->randomMs,0,0))return 0;
  if(kind==10&&!s->waitMs)return 0;
  if(kind==9){if(s->waitMs&&s->randomMs>=s->waitMs)return 0;if(!s->waitMs&&s->randomMs)return 0;}
 }else if(s->waitMs||s->randomMs)return 0;
 if(JAR_ControllerStation(kind)){if(s->initial<1||s->count<0||s->count>s->initial||s->repeats)return 0;}
 else if(kind==4){if(s->initial<1||s->count<0||s->count>s->initial||s->repeats< -1)return 0;}
 else if(kind==19||kind==20){if(s->count||s->repeats||s->initial<0||s->initial>(kind==20?0:2))return 0;}
 else if(kind==17){if(s->count<SHRT_MIN||s->count>SHRT_MAX||s->initial||s->repeats)return 0;}
 else if(s->count||s->initial||s->repeats)return 0;
 if(JAR_ControllerStation(kind)){if(s->flags<0||s->flags>1)return 0;}
 else if(kind==1){if(s->flags<0||s->flags>4)return 0;}
 else if(kind==2){if(s->flags<0||s->flags>2)return 0;}
 else if(kind==3||kind==14){if(s->flags<0||s->flags>1)return 0;}
 else if(kind==19||kind==20){if(s->flags<0||(s->flags&~31)||((s->flags&17)==17))return 0;}
 else if(kind==21){if(s->flags!=0&&s->flags!=16)return 0;}
 else if(kind==22){int mode=s->flags&~2048;if(s->flags<0||(mode!=0&&mode!=4&&mode!=20)||(!mode&&(s->flags&2048)))return 0;}
 else if(kind==23){if(s->flags<0||s->flags>1)return 0;}
 else if(kind==18){if(s->flags<0||s->flags>2)return 0;}
 else if(kind==6){if(s->flags<0||s->flags>3)return 0;}
 else if(kind==9){
  if(s->flags!=0&&s->flags!=2&&s->flags!=4&&s->flags!=8)return 0;
  if((s->flags==2||s->flags==8)&&s->waitMs)return 0;
  if(s->flags!=2&&!s->waitMs&&s->active)return 0;
 }else if(kind==10){if(s->flags!=0&&s->flags!=2)return 0;if(s->flags==2&&s->active)return 0;}
 else if(s->flags)return 0;
 return kind||!s->inactive;
}
static int JAR_ControllerUnhex(const char*hex,char*out){
 size_t n=strlen(hex);if(!n||n>126||(n&1))return 0;memset(out,0,64);
 for(size_t i=0;i<n;i+=2){int value=0;for(int j=0;j<2;++j){int c=hex[i+j],v=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(v<0)return 0;value=value*16+v;}if(!value)return 0;out[i/2]=(char)value;}
 out[n/2]=0;return JAR_ControllerText(out);
}
static void JAR_ControllerHex(const char*text,char*out){static const char digits[]="0123456789abcdef";while(*text){unsigned char c=(unsigned char)*text++;*out++=digits[c>>4];*out++=digits[c&15];}*out=0;}
