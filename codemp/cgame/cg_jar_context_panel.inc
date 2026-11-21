// Shared SP context presentation, with authoritative MP property adapters.
#include "game/jar_mp_crystal_crate.h"
#include "game/jar_mp_door_camera.h"
#include "game/jar_mp_welder.h"

enum {
 CONTEXT_CAMERA_TRAVEL=215, CONTEXT_CAMERA_SPEED, CONTEXT_WELDER_REACH, CONTEXT_WELDER_RADIUS, CONTEXT_WELDER_INTERVAL,
 CONTEXT_CRYSTAL_HEALTH, CONTEXT_CRYSTAL_HIT_DAMAGE, CONTEXT_CRYSTAL_HIT_RADIUS, CONTEXT_CRYSTAL_BLAST_DAMAGE, CONTEXT_CRYSTAL_BLAST_RADIUS,
 CONTEXT_VISIBLE_RANGE, CONTEXT_VISIBLE_CONE, CONTEXT_DISTANCE_RANGE,
 CONTEXT_SLIDE_TRAVEL, CONTEXT_SLIDE_SPEED, CONTEXT_SLIDE_WAIT, CONTEXT_BUTTON_WAIT, CONTEXT_PROBE_RADIUS,
 CONTEXT_BOB_AMPLITUDE, CONTEXT_BOB_PERIOD, CONTEXT_MARKER_GRAVITY, CONTEXT_MARKER_FRICTION,
 CONTEXT_ACTOR_GRAVITY, CONTEXT_ACTOR_FRICTION, CONTEXT_DIRECTIONAL_WAIT, CONTEXT_DIRECTIONAL_USES,
 CONTEXT_SEARCHLIGHT_RANGE, CONTEXT_SEARCHLIGHT_ANGLE, CONTEXT_SEARCHLIGHT_LIGHT,
 CONTEXT_PORTAL_WIDTH, CONTEXT_PORTAL_HEIGHT, CONTEXT_SPINNER_RATE, CONTEXT_PENDULUM_SWING,
 CONTEXT_PENDULUM_PERIOD, CONTEXT_SPACE_WIDTH, CONTEXT_SPACE_DEPTH, CONTEXT_SPACE_HEIGHT,
 CONTEXT_SEARCHLIGHT_COLOR
};
typedef struct {
 const char *command;
 int scale;
} controllerNumberField_t;
static const controllerNumberField_t controllerNumberFields[]={
 {"controller_camera_travel",JAR_MP_DOOR_CAMERA_MILLI},
 {"controller_camera_speed",JAR_MP_DOOR_CAMERA_MILLI},
 {"controller_welder_reach",JAR_MP_WELDER_MILLI},
 {"controller_welder_radius",JAR_MP_WELDER_MILLI},
 {"controller_welder_interval",JAR_MP_WELDER_MILLI},
 {"controller_count",0},
 {"controller_crystal_hit_damage",0},
 {"controller_crystal_hit_radius",JAR_MP_CRYSTAL_CRATE_MILLI},
 {"controller_damage",0},
 {"controller_crystal_blast_radius",0},
 {"controller_visible_range",0},
 {"controller_visible_cone",0},
 {"controller_distance_range",0},
 {"controller_slide_travel",0},
 {"controller_slide_speed",0},
 {"controller_slide_wait",0},
 {"controller_button_wait",0},
 {"controller_probe_radius",0},
 {"controller_bob_amplitude",0},
 {"controller_bob_period",0},
 {"controller_movement_value",0},
 {"controller_movement_value",0},
 {"actor_gravity",0},
 {"actor_friction",0},
 {"controller_directional_wait",0},
 {"controller_directional_uses",0},
 {"controller_searchlight_range",0},
 {"controller_searchlight_angle",0},
 {"controller_searchlight_light",0},
 {"controller_portal_width",0},
 {"controller_portal_height",0},
 {"controller_angular_rate",0},
 {"controller_angular_rate",0},
 {"controller_angular_period",0},
 {"controller_space_width",0},
 {"controller_space_depth",0},
 {"controller_space_height",0}
};
static int breakPage;static unsigned breakIdentity;
void CG_JAR_Controller(void){
 unsigned identity;int kind;jarControllerState_t next={0};
 if(!contextIdentity||!(contextCaps&JAR_CONTEXT_CONTROLLER)||!JAR_MP_ParseId(CG_Argv(1),&identity)||identity!=contextIdentity||!SelectionNumber(CG_Argv(2),1,JAR_CONTROLLER_MAX,&kind))return;
 int*fields[]={&next.waitMs,&next.randomMs,&next.flags,&next.repeats,&next.count,&next.initial,&next.inactive};
 for(int i=0;i<7;++i)if(!JAR_ParseInt(CG_Argv(3+i),fields[i]))return;
 const char*hex=CG_Argv(10);if(strcmp(hex,"-")&&!JAR_ControllerUnhex(hex,next.text))return;
 if(!JAR_ParseInt(CG_Argv(11),&next.speed))return;
 const int structural=JAR_ControllerShooter(kind)||JAR_ControllerAnimatedTag(kind)||JAR_ControllerRack(kind);
 const int actuator=JAR_ControllerSlide(kind)||JAR_ControllerAngular(kind)||JAR_ControllerPortal(kind)||
                   kind==JAR_BOBBING_CONTROLLER||kind==JAR_SEARCHLIGHT_CONTROLLER;
 const int damage=JAR_ControllerBreakable(kind)||JAR_ControllerSlide(kind)||kind==13||kind==21||
                  kind==JAR_DOOR_CAMERA_CONTROLLER||kind==JAR_WELDER_CONTROLLER||kind==JAR_VISIBLE_CONTROLLER||
                  kind==JAR_SEARCHLIGHT_CONTROLLER||kind==JAR_SPACE_CONTROLLER;
 const int active=actuator||damage||structural||JAR_ControllerEmplaced(kind)||JAR_ControllerTrigger(kind)||
                  JAR_ControllerMedia(kind)||kind==14||kind==15||kind==29||kind==33||kind==JAR_MUSIC_CONTROLLER||
                  kind==JAR_ALWAYS_CONTROLLER||kind==JAR_WALL_CONTROLLER||kind==JAR_REFLECTION_CONTROLLER||
                  kind==JAR_BUTTON_CONTROLLER||kind==JAR_DIRECTIONAL_CONTROLLER;
 const int extent=JAR_ControllerBreakable(kind)||actuator||kind==22||kind==JAR_EWEB_CONTROLLER||
                  kind==JAR_DOOR_CAMERA_CONTROLLER||kind==JAR_WELDER_CONTROLLER||kind==JAR_SPACE_CONTROLLER;
 const int enabled[]={active,damage||structural,extent||structural};
 int *optional[]={&next.active,&next.damage,&next.pushSpeed};
 for(int i=0;i<ARRAY_LEN(optional);++i){
  const char *number=CG_Argv(12+i);
  if(enabled[i]&&*number&&!JAR_ParseInt(number,optional[i]))return;
 }
 if(!JAR_ControllerStateValid(kind,&next))return;
 if(kind!=contextControllerKind&&contextEditing>=200)contextEditing=0;
 if(breakIdentity!=contextIdentity||contextControllerKind!=kind){breakPage=0;breakIdentity=contextIdentity;}
 contextControllerKind=kind;contextController=next;
}
static qboolean ControllerInputKey(int key){
 static const char *const legacyFields[]={
  "controller_delay","controller_random","controller_count","controller_repeats","controller_speed",
  "controller_damage","controller_points","controller_extent","controller_pushspeed","controller_charge",
  "controller_chunks","controller_radius","controller_yaw","controller_ammo"
 };
 const int index=contextEditing-CONTEXT_CAMERA_TRAVEL;
 const controllerNumberField_t*field=index>=0&&index<ARRAY_LEN(controllerNumberFields)?&controllerNumberFields[index]:NULL;
 if(key&K_CHAR_FLAG){
  int c=key&~K_CHAR_FLAG;size_t len=strlen(contextInput);
  if(contextEditing==200){
   const size_t limit=contextControllerKind==JAR_MUSIC_CONTROLLER?sizeof(contextInput)-1:63;
   if(c>=32&&c<127&&c!='"'&&c!='\\'&&c!=';'&&len<limit){contextInput[len]=(char)c;contextInput[len+1]=0;}
  }else if(contextEditing==CONTEXT_SEARCHLIGHT_COLOR){
   if(c>0&&c<127&&len<6&&strchr("0123456789abcdefABCDEF",c)){contextInput[len]=(char)c;contextInput[len+1]=0;}
  }else JAR_NumberAppend(contextInput,sizeof(contextInput),c,field&&field->scale);
  return qtrue;
 }
 if(key==A_BACKSPACE){size_t len=strlen(contextInput);if(len)contextInput[len-1]=0;return qtrue;}
 if(key!=A_ENTER&&key!=A_KP_ENTER)return qfalse;
 char action[150];
 if(contextEditing==200&&contextControllerKind==JAR_MUSIC_CONTROLLER){
  char packed[64],hex[127];
  if(!JAR_MusicPack(contextInput,packed)){MenuStatus("Enter a stock music path or an intro and loop separated by one space.");return qtrue;}
  JAR_ControllerHex(packed,hex);Com_sprintf(action,sizeof(action),"controller_text_%s",hex);
 }else if(contextEditing==200){
  if(!JAR_ControllerText(contextInput)){MenuStatus("Enter 1-63 printable characters without quotes, semicolons or backslashes.");return qtrue;}
  char hex[127];JAR_ControllerHex(contextInput,hex);Com_sprintf(action,sizeof(action),"controller_text_%s",hex);
 }else if(contextEditing==CONTEXT_SEARCHLIGHT_COLOR){
  if(strlen(contextInput)!=6){MenuStatus("Enter a six-digit RGB color.");return qtrue;}
  Com_sprintf(action,sizeof(action),"controller_searchlight_color_%s",contextInput);
 }else{
  const int legacyIndex=contextEditing-201;
  const char *command=field?field->command:legacyIndex>=0&&legacyIndex<ARRAY_LEN(legacyFields)?legacyFields[legacyIndex]:NULL;
  int value;
  if(!command||!JAR_ParseScaled(contextInput,field?field->scale:0,&value)){
   MenuStatus(field&&field->scale?"Enter a valid number.":"Enter a valid whole number.");
   return qtrue;
  }
  Com_sprintf(action,sizeof(action),"%s_%d",command,value);
 }
 ContextProperty(action);contextEditing=0;return qtrue;
}
static qboolean ContextBox(float x,float y,float w,float h){return cursorX>=x&&cursorX<x+w&&cursorY>=y&&cursorY<y+h;}
static void ContextText(float x,float y,const char*s,float scale,qboolean focus){vec4_t normal={JAR_MENU_TEXT_COLOR},selected={JAR_MENU_FOCUS_COLOR};trap->R_Font_DrawString((int)x,(int)y,s,focus?selected:normal,jarMenuFont|STYLE_DROPSHADOW,-1,scale);}
static void ContextButton(float x,float y,float w,const char*s){ContextText(x,y,s,.5f,ContextBox(x,y,w,20));}
static void ContextWidget(jarContextPart_t part,int i,const char*s){jarContextRect_t r=JAR_ContextRect(part,i);ContextButton(r.x,r.y,r.w,s);}
static void ContextFieldAt(float x,float y,float width,const char*label,const char*value,int editor){
 char text[120];size_t n=strlen(value);Com_sprintf(text,sizeof(text),"%s: %s%s",label,value+(n>24?n-24:0),contextEditing==editor?"_":"");
 ContextButton(x,y,width,text);
}
static void ContextFieldDraw(float y,const char*label,const char*value,int editor){
 ContextFieldAt(402,y,220,label,value,editor);
}
static const char*const distanceRoles[]={"Inside","Outside","Reference","NPC"};
static const char*const distanceRoleCommands[]={
 "jar_setting output_primary","jar_setting output_secondary","jar_setting output_reference","jar_setting output_subject"
};
static void DistanceDraw(void){
 const jarControllerState_t*s=&contextController;
 ContextFieldDraw(148,"Range",contextEditing==CONTEXT_DISTANCE_RANGE?contextInput:va("%d",s->speed),CONTEXT_DISTANCE_RANGE);
 ContextButton(402,176,220,s->flags==3?"Subjects: Either":s->flags==JAR_DISTANCE_NPC?"Subjects: NPC":"Subjects: Player");
 ContextButton(402,204,220,s->inactive?"Disarmed - click to arm":"Armed - click to disarm");
 ContextText(402,232,s->count==1?"Reference: Linked":s->count==2?"Reference: Missing":"Reference: This marker",.43f,qfalse);
 ContextText(402,260,s->initial==1?"NPC: Linked":s->initial==2?"NPC: Missing":"NPC: Choose a subject",.43f,qfalse);
 if(s->count)ContextButton(552,232,66,"Reset");
 if(s->initial)ContextButton(552,260,66,"Clear");
 ContextText(402,289,va("Link role: %s",distanceRoles[linkOutput]),.43f,qfalse);
 for(int role=0;role<JAR_DISTANCE_LINKS;++role)ContextButton(402+(role%2)*112,310+(role/2)*22,104,distanceRoles[role]);
 ContextText(402,355,"Choose role, then use the Link tool.",.4f,qfalse);
}
static qboolean DistanceClick(void){
 if(ContextBox(402,148,220,20)){contextEditing=CONTEXT_DISTANCE_RANGE;contextInput[0]=0;return qtrue;}
 if(ContextBox(402,176,220,20)){ContextProperty("controller_distance_subjects");return qtrue;}
 if(ContextBox(402,204,220,20)){ContextProperty("controller_enabled");return qtrue;}
 if(contextController.count&&ContextBox(552,232,66,20)){ContextProperty("controller_distance_clear_reference");return qtrue;}
 if(contextController.initial&&ContextBox(552,260,66,20)){ContextProperty("controller_distance_clear_subject");return qtrue;}
 for(int role=0;role<JAR_DISTANCE_LINKS;++role)if(ContextBox(402+(role%2)*112,310+(role/2)*22,104,20)){
  trap->SendClientCommand(distanceRoleCommands[role]);
  return qtrue;
 }
 return qfalse;
}
static void SlidingDoorDraw(void){
 const jarControllerState_t*s=&contextController;
 const qboolean lift=contextControllerKind==JAR_LIFT_CONTROLLER;
 static const char*const directions[]={"Up","Left","Right","Down"};
 ContextFieldDraw(148,"Travel",contextEditing==CONTEXT_SLIDE_TRAVEL?contextInput:va("%d",s->damage),CONTEXT_SLIDE_TRAVEL);
 ContextFieldDraw(174,"Speed",contextEditing==CONTEXT_SLIDE_SPEED?contextInput:va("%d",s->speed),CONTEXT_SLIDE_SPEED);
 ContextFieldDraw(200,lift?"Auto-return ms (0 = off)":"Auto-close ms (0 = off)",contextEditing==CONTEXT_SLIDE_WAIT?contextInput:va("%d",s->waitMs),CONTEXT_SLIDE_WAIT);
 if(!lift)ContextButton(402,226,220,va("Direction: %s",directions[s->flags]));
 ContextButton(402,252,220,s->inactive?"Activation: Disabled":"Activation: Enabled");
 if(!s->inactive)ContextButton(402,278,220,lift?(s->active?"Lower":"Raise"):(s->active?"Close":"Open"));
 ContextText(402,304,s->repeats==JAR_SLIDE_BLOCKED?"Blocked. Clears when the path opens.":
  s->repeats==JAR_SLIDE_PAUSED?"Paused. Freeze or enable to resume.":
  s->repeats==JAR_SLIDE_MOVING?(lift?(s->active?"Raising":"Lowering"):(s->active?"Opening":"Closing")):
  lift?(s->pushSpeed?"Raised":"Lowered"):(s->pushSpeed?"Open":"Closed"),.43f,qfalse);
 ContextText(402,330,lift?"Primary: Raised / Secondary: Lowered":"Primary: Opened / Secondary: Closed",.4f,qfalse);
}
static qboolean SlidingDoorClick(void){
 for(int i=0;i<3;++i)if(ContextBox(402,148+26*i,220,20)){
  contextEditing=CONTEXT_SLIDE_TRAVEL+i;contextInput[0]=0;return qtrue;
 }
 if(contextControllerKind!=JAR_LIFT_CONTROLLER&&ContextBox(402,226,220,20)){ContextProperty("controller_slide_direction");return qtrue;}
 if(ContextBox(402,252,220,20)){ContextProperty("controller_enabled");return qtrue;}
 if(!contextController.inactive&&ContextBox(402,278,220,20)){ContextProperty("activate");return qtrue;}
 return qfalse;
}
static void BobbingDraw(void){
 const jarControllerState_t* s=&contextController;
 ContextFieldDraw(148,"Amplitude",contextEditing==CONTEXT_BOB_AMPLITUDE?contextInput:va("%d",s->speed),CONTEXT_BOB_AMPLITUDE);
 ContextFieldDraw(174,"Cycle",contextEditing==CONTEXT_BOB_PERIOD?contextInput:va("%d",s->waitMs),CONTEXT_BOB_PERIOD);
 ContextButton(402,205,220,s->active?"Pause motion":"Resume motion");
 ContextButton(402,233,220,s->inactive?"Enable platform":"Disable platform");
 const char* status=s->inactive?"Disabled":!s->active?"Paused":!contextFrozen?"Freeze the platform to move.":
  s->repeats==JAR_BOB_BLOCKED?"Blocked. Waiting for clear space.":s->repeats==JAR_BOB_MOVING?"Moving":"Ready";
 ContextText(402,271,status,.4f,qfalse);
}
static qboolean BobbingClick(void){
 for(int i=0;i<2;++i)if(ContextBox(402,148+26*i,220,20)){
  contextEditing=CONTEXT_BOB_AMPLITUDE+i;contextInput[0]=0;return qtrue;
 }
 if(ContextBox(402,205,220,20)){ContextProperty("controller_bob_running");return qtrue;}
 if(ContextBox(402,233,220,20)){ContextProperty("controller_enabled");return qtrue;}
 return qfalse;
}
static void AngularDraw(void){
 const jarControllerState_t* s=&contextController;
 const qboolean pendulum=contextControllerKind==JAR_PENDULUM_CONTROLLER;
 const int rate=pendulum?CONTEXT_PENDULUM_SWING:CONTEXT_SPINNER_RATE;
 ContextFieldDraw(148,pendulum?"Swing":"Rate (-180 to 180 deg/s)",contextEditing==rate?contextInput:va("%d",s->speed),rate);
 if(pendulum)ContextFieldDraw(174,"Cycle",contextEditing==CONTEXT_PENDULUM_PERIOD?contextInput:va("%d",s->waitMs),CONTEXT_PENDULUM_PERIOD);
 ContextButton(402,205,220,va("Axis: %s",JAR_AngularAxisLabel(s->flags)));
 ContextButton(402,233,220,s->active?"Pause motion":"Start motion");
 ContextButton(402,261,220,s->inactive?"Enable mover":"Disable mover");
 const char* status=s->inactive?"Disabled":!s->active?"Paused":!contextFrozen?"Freeze the mover to run it.":
  s->repeats==JAR_ANGULAR_BLOCKED?"Blocked. Waiting for clear space.":s->repeats==JAR_ANGULAR_MOVING?"Moving":"Ready";
 ContextText(402,295,status,.4f,qfalse);
}
static qboolean AngularClick(void){
 const qboolean pendulum=contextControllerKind==JAR_PENDULUM_CONTROLLER;
 if(ContextBox(402,148,220,20))contextEditing=pendulum?CONTEXT_PENDULUM_SWING:CONTEXT_SPINNER_RATE;
 else if(pendulum&&ContextBox(402,174,220,20))contextEditing=CONTEXT_PENDULUM_PERIOD;
 else if(ContextBox(402,205,220,20)){ContextProperty("controller_angular_axis");return qtrue;}
 else if(ContextBox(402,233,220,20)){ContextProperty("controller_angular_running");return qtrue;}
 else if(ContextBox(402,261,220,20)){ContextProperty("controller_enabled");return qtrue;}
 else return qfalse;
 contextInput[0]=0;
 return qtrue;
}
static void ReflectionDraw(void){
 const jarControllerState_t* s=&contextController;
 ContextFieldDraw(148,"Radius",contextEditing==CONTEXT_PROBE_RADIUS?contextInput:va("%d",s->speed),CONTEXT_PROBE_RADIUS);
 ContextButton(402,176,220,s->active?"Turn reflections off":"Turn reflections on");
 ContextButton(402,204,220,"Recapture map");
 ContextButton(402,232,220,s->inactive?"Enable probe":"Disable probe");
 ContextText(402,270,probeStatus,.4f,qfalse);
}
static qboolean ReflectionClick(void){
 if(ContextBox(402,148,220,20)){contextEditing=CONTEXT_PROBE_RADIUS;contextInput[0]=0;return qtrue;}
 if(ContextBox(402,176,220,20)){ContextProperty("controller_probe_active");return qtrue;}
 if(ContextBox(402,204,220,20)){ContextProperty("controller_probe_refresh");return qtrue;}
 if(ContextBox(402,232,220,20)){ContextProperty("controller_enabled");return qtrue;}
 return qfalse;
}
static void ButtonDraw(void){
 const jarControllerState_t* s=&contextController;
 ContextFieldDraw(148,"Hold (ms)",contextEditing==CONTEXT_BUTTON_WAIT?contextInput:va("%d",s->waitMs),CONTEXT_BUTTON_WAIT);
 ContextButton(402,176,220,s->inactive?"Activation: Disabled":"Activation: Enabled");
 if(!s->inactive&&s->active==JAR_BUTTON_IDLE)ContextButton(402,204,220,"Press");
 ContextText(402,232,JAR_ButtonPhaseName(s->active),.43f,qfalse);
 ContextText(402,260,"Primary: Pressed / Secondary: Released",.4f,qfalse);
}
static qboolean ButtonClick(void){
 if(ContextBox(402,148,220,20)){contextEditing=CONTEXT_BUTTON_WAIT;contextInput[0]=0;return qtrue;}
 if(ContextBox(402,176,220,20)){ContextProperty("controller_enabled");return qtrue;}
 if(!contextController.inactive&&contextController.active==JAR_BUTTON_IDLE&&ContextBox(402,204,220,20)){
  ContextProperty("activate");return qtrue;
 }
 return qfalse;
}
static qboolean MediaRepeats(void){return contextControllerKind==10||contextController.flags==0||contextController.flags==4;}
static qboolean MediaOneShot(void){return contextControllerKind==10?contextController.flags==2:contextController.flags!=2&&!contextController.waitMs;}
static void MediaDraw(void){
 const jarControllerState_t*s=&contextController;int kind=contextControllerKind;
 ContextFieldDraw(148,kind==9?"Sound":"Effect",contextEditing==200?contextInput:s->text,200);
 ContextButton(402,176,104,"Previous asset");ContextButton(514,176,104,"Next asset");
 const char*mode=kind==10?(s->flags?"One shot":"Repeating"):(s->flags==2?"Loop":s->flags==4?"Global":s->flags==8?"Activator":"Local");
 ContextButton(402,204,220,va("Mode: %s",mode));
 if(MediaRepeats()){
  ContextFieldDraw(232,kind==9?"Repeat ms":"Interval ms",contextEditing==201?contextInput:va("%d",s->waitMs),201);
  ContextFieldDraw(260,"Random ms",contextEditing==202?contextInput:va("%d",s->randomMs),202);
 }
 ContextButton(402,288,220,MediaOneShot()?"Play once":s->active?"Playing: On":"Playing: Off");
 ContextButton(402,316,220,s->inactive?"Enabled: Off":"Enabled: On");
}
static qboolean MediaClick(void){
 if(ContextBox(402,148,220,20)){contextEditing=200;contextInput[0]=0;return qtrue;}
 if(ContextBox(402,176,104,20)){ContextProperty("controller_asset_prev");return qtrue;}
 if(ContextBox(514,176,104,20)){ContextProperty("controller_asset_next");return qtrue;}
 if(ContextBox(402,204,220,20)){ContextProperty("controller_mode");return qtrue;}
 if(MediaRepeats()&&(ContextBox(402,232,220,20)||ContextBox(402,260,220,20))){contextEditing=cursorY>=260?202:201;contextInput[0]=0;return qtrue;}
 if(ContextBox(402,288,220,20)){ContextProperty(MediaOneShot()?"activate":"controller_playing");return qtrue;}
 if(ContextBox(402,316,220,20)){ContextProperty("controller_enabled");return qtrue;}
 return qfalse;
}
static void TriggerDraw(void){
 const jarControllerState_t*s=&contextController;int kind=contextControllerKind;
 if(kind==JAR_SPACE_CONTROLLER){
  int size[3];JAR_SpaceDimensions(s,size);
  const char*labels[]={"Width","Depth","Height"};
  for(int a=0;a<3;++a){
   const int field=CONTEXT_SPACE_WIDTH+a;
   ContextFieldDraw(148+28*a,labels[a],contextEditing==field?contextInput:va("%d",size[a]),field);
  }
  ContextText(402,232,"Zero gravity inside the volume.",.4f,qfalse);
  ContextText(402,260,"Breathing actors have five seconds of air.",.38f,qfalse);
  ContextButton(402,288,220,s->active?"Vacuum: On":"Vacuum: Off");
  ContextButton(402,316,220,s->inactive?"Enabled: Off":"Enabled: On");
  return;
 }
 ContextFieldDraw(148,"Half-size",contextEditing==208?contextInput:va("%d",s->speed),208);
 if(kind>=22){
  if(kind==22){
   ContextFieldDraw(176,"Speed",contextEditing==209?contextInput:va("%d",s->pushSpeed),209);
   ContextButton(402,204,220,s->flags&16?"Direction: relative to entity":s->flags&4?"Direction: linear":"Direction: arc to destination");
   if(s->flags&4){ContextFieldDraw(232,"Wait ms (-1 = once)",contextEditing==201?contextInput:va("%d",s->waitMs),201);if(s->waitMs>0)ContextButton(402,260,220,s->flags&2048?"Same-frame pushes: all":"Same-frame pushes: first");else ContextText(402,260,s->waitMs<0?"Pushes one entity, then disarms.":"Continuously pushes every contact.",.38f,qfalse);}
   else ContextText(402,232,"Arc requires an elevated destination.",.38f,qfalse);
  }else {ContextButton(402,176,220,s->flags?"Accept: spectators only":"Accept: actors and spectators");ContextText(402,204,"Link Primary to a destination marker.",.38f,qfalse);ContextText(402,232,"Move / rotate marker to place the exit.",.38f,qfalse);ContextText(402,260,"Blocked arrival leaves everyone in place.",.36f,qfalse);}
  ContextButton(402,288,220,s->active?"Armed: On":"Armed: Off");ContextButton(402,316,220,s->inactive?"Enabled: Off":"Enabled: On");return;
 }
 if(kind==19){ContextFieldDraw(176,"Cooldown ms",contextEditing==201?contextInput:va("%d",s->waitMs),201);ContextFieldDraw(204,"Random +/- ms",contextEditing==202?contextInput:va("%d",s->randomMs),202);}
 else if(kind==20){ContextText(402,176,"Fires Primary once on entry.",.4f,qfalse);ContextText(402,204,"Arm again to reuse this trigger.",.4f,qfalse);}
 else {ContextFieldDraw(176,"Damage",contextEditing==206?contextInput:va("%d",s->damage),206);ContextButton(402,204,220,s->flags&16?"Rate: once per second":"Rate: every native tick");}
 if(kind!=21){
  ContextButton(402,232,220,s->flags&1?"Accept: players":s->flags&16?"Accept: NPCs":"Accept: players and NPCs");
  const char*input[]={"Touch","Use","Fire","Use + Fire"};ContextButton(402,260,220,va("Input: %s",input[(s->flags&12)/4]));
  ContextButton(402,288,104,s->flags&2?"Facing: On":"Facing: Off");ContextButton(514,288,104,s->active?"Armed: On":"Armed: Off");
 }else {ContextButton(402,232,220,s->active?"Hurting: On":"Hurting: Off");ContextText(402,260,"Damage respects tool permissions.",.38f,qfalse);ContextText(402,288,"Rotate / resize the outlined volume.",.38f,qfalse);}
 ContextButton(402,316,220,s->inactive?"Enabled: Off":"Enabled: On");
}
static qboolean TriggerClick(void){
 int kind=contextControllerKind;
 if(kind==JAR_SPACE_CONTROLLER){
  for(int a=0;a<3;++a)if(ContextBox(402,148+28*a,220,20)){
   contextEditing=CONTEXT_SPACE_WIDTH+a;contextInput[0]=0;return qtrue;
  }
 }else if(ContextBox(402,148,220,20)){contextEditing=208;contextInput[0]=0;return qtrue;}
 if(kind>=22){
  if(kind!=JAR_SPACE_CONTROLLER&&ContextBox(402,176,220,20)){if(kind==22){contextEditing=209;contextInput[0]=0;}else ContextProperty("controller_spectators");return qtrue;}
  if(kind==22){
   if(ContextBox(402,204,220,20)){ContextProperty("controller_pushmode");return qtrue;}
   if((contextController.flags&4)&&ContextBox(402,232,220,20)){contextEditing=201;contextInput[0]=0;return qtrue;}
   if((contextController.flags&4)&&contextController.waitMs>0&&ContextBox(402,260,220,20)){ContextProperty("controller_multiple");return qtrue;}
  }
  if(ContextBox(402,288,220,20)){ContextProperty("controller_playing");return qtrue;}
  if(ContextBox(402,316,220,20)){ContextProperty("controller_enabled");return qtrue;}return qfalse;
 }
 if(ContextBox(402,176,220,20)&&kind!=20){contextEditing=kind==19?201:206;contextInput[0]=0;return qtrue;}
 if(ContextBox(402,204,220,20)&&kind!=20){if(kind==19){contextEditing=202;contextInput[0]=0;}else ContextProperty("controller_slow");return qtrue;}
 if(ContextBox(402,232,220,20)){ContextProperty(kind==21?"controller_playing":"controller_subjects");return qtrue;}
 if(kind!=21){
  if(ContextBox(402,260,220,20)){ContextProperty("controller_input");return qtrue;}
  if(ContextBox(402,288,104,20)){ContextProperty("controller_facing");return qtrue;}
  if(ContextBox(514,288,104,20)){ContextProperty("controller_playing");return qtrue;}
 }
 if(ContextBox(402,316,220,20)){ContextProperty("controller_enabled");return qtrue;}return qfalse;
}
static void MusicDraw(void){
 const jarControllerState_t *state=&contextController;
 char cue[JAR_MUSIC_CUE_SIZE];
 if(!JAR_MusicUnpack(state->text,cue))return;
 char *loop=strchr(cue,' ');
 if(loop)*loop++=0;
 if(contextEditing==200)ContextFieldDraw(148,"Music",contextInput,200);
 else{
  ContextButton(402,148,220,loop?"Edit intro and loop":"Edit music track");
  ContextText(402,177,cue,.38f,qfalse);
  if(loop)ContextText(402,197,loop,.38f,qfalse);
 }
 ContextButton(402,224,104,"Previous track");ContextButton(514,224,104,"Next track");
 if(!state->inactive)ContextButton(402,260,220,state->active?"Stop music":"Play music");
 ContextButton(402,288,220,state->inactive?"Enabled: Off":"Enabled: On");
}
static qboolean MusicClick(void){
 if(ContextBox(402,148,220,20)){
  JAR_MusicUnpack(contextController.text,contextInput);contextEditing=200;return qtrue;
 }
 if(ContextBox(402,224,104,20)){ContextProperty("controller_asset_prev");return qtrue;}
 if(ContextBox(514,224,104,20)){ContextProperty("controller_asset_next");return qtrue;}
 if(!contextController.inactive&&ContextBox(402,260,220,20)){ContextProperty("activate");return qtrue;}
 if(ContextBox(402,288,220,20)){ContextProperty("controller_enabled");return qtrue;}
 return qfalse;
}
static qboolean ControllerCanDetonate(int kind,const jarControllerState_t*s){
 return kind!=JAR_SMALL_CARGO_CONTROLLER&&!s->inactive&&(s->active==1||kind==JAR_CARGO_BARREL_CONTROLLER);
}
static const char* CameraStatus(const jarControllerState_t*s){
 if(s->pushSpeed==(s->active?s->damage:0))return s->active?"Extended":"Retracted";
 if(s->inactive)return "Paused";
 if(contextCatalog==JAR_MP_DOOR_CAMERA_ID&&selectedEntity>=MAX_CLIENTS&&selectedEntity<ENTITYNUM_WORLD){
  const centity_t*root=&cg_entities[selectedEntity];
  if(root->currentValid&&root->currentState.number==selectedEntity&&root->currentState.time2&&
     JAR_MP_DoorCameraRootState(&root->currentState)&&root->currentState.time==JAR_MP_DOOR_CAMERA_BLOCKED)
   return "Blocked";
 }
 return s->active?"Extending":"Retracting";
}
static void ControllerDraw(void){
 const jarControllerState_t*s=&contextController;int kind=contextControllerKind;
 if(kind==JAR_DISTANCE_CONTROLLER){DistanceDraw();return;}
 if(JAR_ControllerSlide(kind)){SlidingDoorDraw();return;}
 if(kind==JAR_BOBBING_CONTROLLER){BobbingDraw();return;}
 if(JAR_ControllerAngular(kind)){AngularDraw();return;}
 if(kind==JAR_BUTTON_CONTROLLER){ButtonDraw();return;}
 if(kind==JAR_REFLECTION_CONTROLLER){ReflectionDraw();return;}
 if(JAR_ControllerPortal(kind)){
  const qboolean surface=kind==JAR_PORTAL_SURFACE_CONTROLLER;
  if(surface){
   ContextFieldDraw(148,"Panel width",contextEditing==CONTEXT_PORTAL_WIDTH?contextInput:va("%d",s->speed),CONTEXT_PORTAL_WIDTH);
   ContextFieldDraw(176,"Panel height",contextEditing==CONTEXT_PORTAL_HEIGHT?contextInput:va("%d",s->pushSpeed),CONTEXT_PORTAL_HEIGHT);
  }
  ContextButton(402,204,220,surface?(s->flags?"View: Mirror":"View: Linked camera"):(s->flags?"Aim: Manual angles":"Aim: Linked target"));
  ContextButton(402,232,104,s->active?"Power: On":"Power: Off");
  ContextButton(514,232,104,s->inactive?"Disabled":"Enabled");
  ContextText(402,260,surface?"Link Primary to a portal camera.":"Link Primary to an aim target.",.38f,qfalse);
  return;
 }
 if(kind==JAR_SEARCHLIGHT_CONTROLLER){
  ContextFieldDraw(148,"Range",contextEditing==CONTEXT_SEARCHLIGHT_RANGE?contextInput:va("%d",s->speed),CONTEXT_SEARCHLIGHT_RANGE);
  ContextFieldDraw(176,"Cone half-angle",contextEditing==CONTEXT_SEARCHLIGHT_ANGLE?contextInput:va("%d",s->damage),CONTEXT_SEARCHLIGHT_ANGLE);
  ContextFieldDraw(204,"Impact light",contextEditing==CONTEXT_SEARCHLIGHT_LIGHT?contextInput:va("%d",s->pushSpeed),CONTEXT_SEARCHLIGHT_LIGHT);
  ContextFieldDraw(232,"Color (RGB hex)",contextEditing==CONTEXT_SEARCHLIGHT_COLOR?contextInput:va("%06X",s->flags&JAR_SEARCHLIGHT_WHITE),CONTEXT_SEARCHLIGHT_COLOR);
  ContextButton(402,260,220,s->flags&JAR_SEARCHLIGHT_TRACK?"Aim: Linked target":"Aim: Manual angles");
  ContextButton(402,288,104,s->active?"Light: On":"Light: Off");
  ContextButton(514,288,104,s->inactive?"Disabled":"Enabled");
  ContextText(402,316,"Link Primary aims. Secondary detects.",.38f,qfalse);
  return;
 }
 if(kind==JAR_DIRECTIONAL_CONTROLLER){
  ContextFieldDraw(148,"Half-size",contextEditing==208?contextInput:va("%d",s->speed),208);
  ContextFieldDraw(176,"Cooldown ms (-1 once)",contextEditing==CONTEXT_DIRECTIONAL_WAIT?contextInput:va("%d",s->waitMs),CONTEXT_DIRECTIONAL_WAIT);
  ContextFieldDraw(204,"Uses (0 unlimited)",contextEditing==CONTEXT_DIRECTIONAL_USES?contextInput:va("%d",s->initial),CONTEXT_DIRECTIONAL_USES);
  ContextButton(402,232,220,s->flags&JAR_DIRECTIONAL_PLAYER_ONLY?"Subjects: Players":"Subjects: Players and NPCs");
  ContextButton(402,260,220,s->active?"Armed":s->repeats?"Spent - Rearm":"Disarmed - Arm");
  ContextButton(402,288,220,s->inactive?"Activation: Disabled":"Activation: Enabled");
  ContextText(402,316,s->initial?va("Uses left: %d",s->count):"Link Primary forward, Secondary reverse.",.38f,qfalse);
  return;
 }
 if(JAR_ControllerMovement(kind)){
  const qboolean gravity=kind==JAR_GRAVITY_CONTROLLER;
  const int field=gravity?CONTEXT_MARKER_GRAVITY:CONTEXT_MARKER_FRICTION;
  ContextFieldDraw(148,gravity?"Gravity":"Ground friction",
                   contextEditing==field?contextInput:va("%d",s->speed),field);
  ContextButton(402,176,220,s->inactive?"Activation: Disabled":"Activation: Enabled");
  if(!s->inactive)ContextButton(402,204,220,"Activate");
  ContextText(402,232,"Affects the player or NPC who activates it.",.38f,qfalse);
  return;
 }
 if(kind==JAR_WALL_CONTROLLER){
  ContextButton(402,148,220,s->active?"Wall: Solid":"Wall: Off");
  ContextButton(402,176,220,s->inactive?"Activation: Disabled":"Activation: Enabled");
  ContextText(402,204,"Off walls stay selectable as gizmos.",.43f,qfalse);
  return;
 }
 if(kind==JAR_SECTION_CONTROLLER){ContextButton(402,148,220,"Enter section");return;}
 if(JAR_ControllerStart(kind)){
  ContextFieldDraw(148,"Label",contextEditing==200?contextInput:s->text,200);
  ContextButton(402,176,220,va("%s: %s",kind==JAR_PLAYER_START_CONTROLLER?"Use on entry":"Use on respawn",s->flags&1?"On":"Off"));
  if(!s->inactive)ContextButton(402,204,220,"Visit start");
  else ContextText(402,204,"Disabled by link.",.43f,qfalse);
  ContextText(402,232,"Freeze it for Sandbox FFA or Team DM.",.4f,qfalse);
  return;
 }
 if(kind==JAR_ALWAYS_CONTROLLER){
  ContextFieldDraw(148,"Delay",contextEditing==201?contextInput:va("%d",s->waitMs),201);
  ContextText(402,176,s->active?"One shot pending":"Link Primary, then Activate to arm.",.4f,qfalse);
  ContextButton(402,232,220,s->inactive?"Enabled: Off":"Enabled: On");
  return;
 }
 if(kind==JAR_VISIBLE_CONTROLLER){
  ContextFieldDraw(148,"Range",contextEditing==CONTEXT_VISIBLE_RANGE?contextInput:va("%d",s->speed),CONTEXT_VISIBLE_RANGE);
  ContextFieldDraw(176,"Half-angle",contextEditing==CONTEXT_VISIBLE_CONE?contextInput:va("%d",s->damage),CONTEXT_VISIBLE_CONE);
  ContextText(402,204,s->active?"Armed. Look at this marker to fire.":"Spent. Activate to rearm.",.4f,qfalse);
  ContextButton(402,232,220,s->flags&JAR_VISIBLE_IGNORE_WALLS?"Ignore walls: On":"Ignore walls: Off");
  ContextButton(402,260,220,s->flags&JAR_VISIBLE_FORCE_SIGHT?"Require Force Sight: On":"Require Force Sight: Off");
  ContextButton(402,288,220,s->inactive?"Enabled: Off":"Enabled: On");
  return;
 }
 if(JAR_ControllerLocation(kind)){
  static const char *const colors[]={"Default","Red","Green","Yellow","Blue","Cyan","Magenta","White"};
  ContextFieldDraw(148,"Location name",contextEditing==200?contextInput:s->text,200);
  ContextButton(402,176,220,va("Chat color: %s",colors[s->flags]));
  if(kind==JAR_LOCATION_VOLUME_CONTROLLER)
   ContextFieldDraw(204,"Half-size",contextEditing==208?contextInput:va("%d",s->speed),208);
  ContextButton(402,232,220,s->inactive?"Enabled: Off":"Enabled: On");
  return;
 }
 if(kind==JAR_DOOR_CAMERA_CONTROLLER){
  ContextFieldDraw(148,"Travel",contextEditing==CONTEXT_CAMERA_TRAVEL?contextInput:va("%.3f",(double)s->damage/JAR_MP_DOOR_CAMERA_MILLI),CONTEXT_CAMERA_TRAVEL);
  ContextFieldDraw(176,"Speed",contextEditing==CONTEXT_CAMERA_SPEED?contextInput:va("%.3f",(double)s->speed/JAR_MP_DOOR_CAMERA_MILLI),CONTEXT_CAMERA_SPEED);
  if(!s->inactive)ContextButton(402,204,220,s->active?"Retract":"Extend");
  ContextButton(402,232,220,s->inactive?"Disabled":"Enabled");
  ContextText(402,260,CameraStatus(s),.43f,qfalse);
  ContextText(402,288,va("Extension %.3f units",(double)s->pushSpeed/JAR_MP_DOOR_CAMERA_MILLI),.4f,qfalse);
  return;
 }
 if(kind==JAR_WELDER_CONTROLLER){
  ContextFieldDraw(148,"Reach",contextEditing==CONTEXT_WELDER_REACH?contextInput:va("%.3f",(double)s->speed/JAR_MP_WELDER_MILLI),CONTEXT_WELDER_REACH);
  ContextFieldDraw(176,"Damage radius",contextEditing==CONTEXT_WELDER_RADIUS?contextInput:va("%.3f",(double)s->pushSpeed/JAR_MP_WELDER_MILLI),CONTEXT_WELDER_RADIUS);
  ContextFieldDraw(204,"Interval",contextEditing==CONTEXT_WELDER_INTERVAL?contextInput:va("%.3f",(double)s->waitMs/JAR_MP_WELDER_MILLI),CONTEXT_WELDER_INTERVAL);
  ContextFieldDraw(232,"Damage",contextEditing==206?contextInput:va("%d",s->damage),206);
  if(!s->inactive)ContextButton(402,260,220,s->active?"On - turn off":"Off - turn on");
  else ContextText(402,260,s->active?"Paused":"Off",.43f,qfalse);
  ContextButton(402,288,220,s->inactive?"Disabled":"Enabled");
  ContextText(402,316,"Damage 0 makes it decorative.",.4f,qfalse);
  return;
 }
 if(kind==JAR_CRYSTAL_CRATE_CONTROLLER){
  ContextFieldDraw(148,"Maximum health",contextEditing==CONTEXT_CRYSTAL_HEALTH?contextInput:va("%d",s->initial),CONTEXT_CRYSTAL_HEALTH);
  ContextFieldDraw(174,"Hit damage",contextEditing==CONTEXT_CRYSTAL_HIT_DAMAGE?contextInput:va("%d",s->repeats),CONTEXT_CRYSTAL_HIT_DAMAGE);
  ContextFieldDraw(200,"Hit radius",contextEditing==CONTEXT_CRYSTAL_HIT_RADIUS?contextInput:va("%.3f",(double)s->pushSpeed/JAR_MP_CRYSTAL_CRATE_MILLI),CONTEXT_CRYSTAL_HIT_RADIUS);
  ContextFieldDraw(226,"Blast damage",contextEditing==CONTEXT_CRYSTAL_BLAST_DAMAGE?contextInput:va("%d",s->damage),CONTEXT_CRYSTAL_BLAST_DAMAGE);
  ContextFieldDraw(252,"Blast radius",contextEditing==CONTEXT_CRYSTAL_BLAST_RADIUS?contextInput:va("%d",s->speed),CONTEXT_CRYSTAL_BLAST_RADIUS);
  ContextButton(402,278,220,s->flags&JAR_MP_CRYSTAL_CRATE_SHOT_ONLY?"Collision: Shot only":"Collision: Solid");
  ContextButton(402,304,104,s->inactive?"Disabled":"Enabled");
  if(!s->inactive)ContextButton(514,304,104,"Detonate");
  ContextText(402,329,va("Health %d / %d",s->count,s->initial),.43f,qfalse);
  return;
 }
 if(JAR_ControllerRack(kind)){
  const char*const guns[JAR_MP_GUN_RACK_SLOTS]={"E-11 blaster","Heavy repeater","Rocket launcher"};
  const char*const ammo[JAR_MP_AMMO_RACK_SLOTS]={"Blaster ammo","Metallic bolts","Power cell","E-11 blaster","Instant medpack"};
  const char*const*names=kind==JAR_AMMO_RACK_CONTROLLER?ammo:guns;
  const int slots=JAR_MP_RackSlotCount(kind);
  for(int i=0;i<slots;++i)
   ContextText(402,148+i*28,va("%s - %s",names[i],s->flags&(1<<i)?"Collected":"Stocked"),.43f,qfalse);
  return;
 }
 if(kind==JAR_MUSIC_CONTROLLER){MusicDraw();return;}
 if(JAR_ControllerMedia(kind)){MediaDraw();return;}
 if(JAR_ControllerTrigger(kind)){TriggerDraw();return;}
 if(JAR_ControllerStation(kind)){
  ContextFieldDraw(148,"Capacity",contextEditing==203?contextInput:va("%d",s->initial),203);
  ContextFieldDraw(176,"Charge remaining",contextEditing==210?contextInput:va("%d",s->count),210);
  ContextFieldDraw(204,"Recharge ms",contextEditing==201?contextInput:va("%d",s->waitMs),201);
  ContextButton(402,232,220,s->flags?"Unlimited supply: On":"Unlimited supply: Off");ContextButton(402,260,220,"Refill station");
  ContextButton(402,288,220,s->inactive?"Enabled: Off":"Enabled: On");ContextText(402,316,"Hold USE nearby, or Activate.",.4f,qfalse);return;
 }
 if(kind==29){
  ContextButton(402,148,220,s->flags&8?"Mode: reusable button":"Mode: toggle object");
  if(s->flags&8)ContextFieldDraw(176,"Reuse ms (0 = once)",contextEditing==201?contextInput:va("%d",s->waitMs),201);
  else ContextButton(402,176,220,s->active?"Visible / solid: On":"Visible / solid: Off");
  ContextFieldDraw(204,"Health (0 = invulnerable)",contextEditing==203?contextInput:va("%d",s->initial),203);
  ContextButton(402,232,104,s->flags&64?"USE: On":"USE: Off");ContextButton(514,232,104,s->flags&32?"NPCs only":"Any actor");
  ContextButton(402,260,220,s->flags&16?"Check space: On":"Check space: Off");
  ContextButton(402,288,104,"Rearm / heal");ContextButton(514,288,104,s->inactive?"Disabled":"Enabled");
  ContextText(402,316,s->repeats==1?"Waiting for clear space":s->repeats==2?"Cooling down":s->repeats==3?"Spent - Rearm to reuse":va("Ready - health %d / %d",s->count,s->initial),.38f,qfalse);return;
 }
 if(JAR_ControllerAnimatedTag(kind)){
  const qboolean bomb=kind==JAR_BOMB_CONTROLLER;
  const char*const bombPhases[]={"Armed","Disarming","Disarmed"};
  const char*const beaconPhases[]={"Hidden","Deploying","Signalling"};
  ContextText(402,148,bomb?bombPhases[s->active]:beaconPhases[s->active],.5f,qfalse);
  if(!s->inactive&&(!bomb||s->active==JAR_MP_BOMB_ARMED))
   ContextButton(402,176,220,bomb?"Disarm":s->active==JAR_MP_BEACON_HIDDEN?"Deploy":"Hide");
  ContextButton(402,204,220,s->inactive?"Disabled":"Enabled");
  if(bomb)ContextText(402,232,"Link Primary to run on disarm.",.4f,qfalse);
  return;
 }
 if(JAR_ControllerShooter(kind)){
  ContextFieldDraw(148,"Repeat interval (ms)",contextEditing==201?contextInput:va("%d",s->waitMs),201);
  ContextFieldDraw(176,"Added random delay (ms)",contextEditing==202?contextInput:va("%d",s->randomMs),202);
  ContextButton(402,204,220,s->flags&JAR_SHOOTER_ALT?"Fire mode: Alternate":"Fire mode: Primary");
  ContextButton(402,232,220,s->flags&JAR_SHOOTER_REPEAT?"Repeat: On":"Repeat: Off");
  if(!s->inactive)ContextButton(402,260,220,s->active?"Stop firing":s->flags&JAR_SHOOTER_REPEAT?"Start firing":"Fire once");
  ContextButton(402,288,220,s->inactive?"Disabled":"Enabled");
  ContextText(402,316,"Link Primary to aim at an entity.",.4f,qfalse);
  return;
 }
 if(JAR_ControllerEmplaced(kind)){
  const int actionY=kind==JAR_EWEB_CONTROLLER?232:204;
  if(s->active==1)ContextFieldDraw(148,"Maximum health",contextEditing==203?contextInput:va("%d",s->initial),203);
  else ContextText(402,155,s->active==2?"Destruction pending":"Destroyed",.43f,qfalse);
  ContextFieldDraw(176,"Yaw limit",contextEditing==213?contextInput:va("%d",s->speed),213);
  if(kind==JAR_EWEB_CONTROLLER)ContextFieldDraw(204,"Ammunition",contextEditing==214?contextInput:va("%d",s->pushSpeed),214);
  if(s->active==0)ContextButton(402,actionY,220,"Repair gun");
  else if(s->active==1&&!s->inactive)ContextButton(402,actionY,220,"Mount gun");
  ContextButton(402,actionY+28,220,s->inactive?"Disabled":"Enabled");
  if(s->active==1)ContextText(402,actionY+56,va("Health %d / %d",s->count,s->initial),.4f,qfalse);
  return;
 }
 if(JAR_ControllerTerminalBreakable(kind)){
  const qboolean smallCargo=kind==JAR_SMALL_CARGO_CONTROLLER;
  if(s->active==1)ContextFieldDraw(148,"Maximum health",contextEditing==203?contextInput:va("%d",s->initial),203);
  else ContextText(402,155,smallCargo?"Destruction pending":"Fuse burning",.43f,qfalse);
  ContextFieldDraw(176,"Splash damage",contextEditing==206?contextInput:va("%d",s->damage),206);
  ContextFieldDraw(204,"Splash radius",contextEditing==212?contextInput:va("%d",s->speed),212);
  if(ControllerCanDetonate(kind,s))ContextButton(402,232,220,"Detonate");
  if(smallCargo)ContextText(402,238,"Heavy weapons only",.4f,qfalse);
  ContextButton(402,260,220,s->inactive?"Disabled":"Enabled");
  if(s->active==1)ContextText(402,288,va("Health %d / %d",s->count,s->initial),.4f,qfalse);
  if(s->active==1&&kind==JAR_GAS_TANK_CONTROLLER)ContextText(402,316,s->flags&JAR_GAS_TANK_PULSE_SPENT?"Flame pulse used":"Flame pulse ready",.4f,qfalse);
  if(smallCargo){ContextText(402,316,"Drops one instant medpack.",.4f,qfalse);ContextText(402,332,"Link Primary to run on destruction.",.38f,qfalse);}
  return;
 }
 if(JAR_ControllerDebris(kind)){
  if(!breakPage){
   ContextFieldDraw(148,"Maximum health",contextEditing==203?contextInput:va("%d",s->initial),203);
   ContextFieldDraw(176,"Break delay ms",contextEditing==201?contextInput:va("%d",s->waitMs),201);
   ContextButton(402,204,220,s->flags&1?"Damage: invulnerable":s->flags&16?"Damage: saber only":s->flags&32?"Damage: heavy weapons":"Damage: any weapon");
   ContextButton(402,232,104,s->flags&128?"USE: On":"USE: Off");ContextButton(514,232,104,s->flags&64?(kind==31?"Use: no break":"Use: signal"):"Use: break");
   ContextButton(402,260,104,"Repair / heal");ContextButton(514,260,104,s->inactive?"Disabled":"Enabled");
   ContextText(402,288,s->active==0?"Destroyed":s->active==2?"Destruction pending":va("Health %d / %d",s->count,s->initial),.4f,qfalse);
  }else{
   const char*material[]={"Metal","Glass","Electrical","Electrical metal","Dark stone","Light stone","Glass + metal","Metal 2","None","Grey stone","Metal 3","Crate","Grate","Rope","Crate 2","White metal","Snowy rock"};
   ContextButton(402,148,220,va("Debris: %s",material[s->repeats]));
   ContextFieldDraw(176,"Chunks %",contextEditing==211?contextInput:va("%d",s->pushSpeed),211);
   ContextFieldDraw(204,"Splash damage",contextEditing==206?contextInput:va("%d",s->damage),206);
   ContextFieldDraw(232,"Splash radius",contextEditing==212?contextInput:va("%d",s->speed),212);
   ContextFieldDraw(260,"Pain output cooldown ms",contextEditing==202?contextInput:va("%d",s->randomMs),202);
   ContextButton(402,288,220,kind==31?(s->flags&2048?"Explosion + splash: Off":"Explosion + splash: On"):(s->flags&2048?"Explosion effect: Off":"Explosion effect: On"));
  }
  ContextButton(402,316,220,breakPage?"< Behavior":"Effects and damage >");return;
 }
 if(!kind){ContextText(402,148,"Waiting for controller settings",.43f,qfalse);return;}
 if(kind==1||kind==5||kind==8){ContextFieldDraw(148,kind==1?"Message":"Label",contextEditing==200?contextInput:s->text,200);ContextText(402,174,"Click, type text, Enter to apply.",.4f,qfalse);}
 if(kind==1){const char*audience[]={"Everyone","Red team","Blue team","Both teams","Activator"};ContextButton(402,204,220,va("Audience: %s",audience[s->flags]));}
 if(kind==2){const char*team[]={"Everyone","Red team","Blue team"};ContextButton(402,148,220,va("Accept: %s",team[s->flags]));}
 if(kind==3){
  ContextFieldDraw(148,"Delay ms",contextEditing==201?contextInput:va("%d",s->waitMs),201);
  ContextFieldDraw(176,"Random +/- ms",contextEditing==202?contextInput:va("%d",s->randomMs),202);
  ContextButton(402,204,220,s->flags?"While waiting: ignore input":"While waiting: restart delay");
  ContextText(402,258,"Random varies the delay around its base.",.39f,qfalse);
  ContextText(402,280,"Changes affect future activations.",.39f,qfalse);
 }
 if(kind==4){
  ContextFieldDraw(148,"Activations",contextEditing==203?contextInput:va("%d",s->initial),203);
  ContextFieldDraw(176,"Rearms left",contextEditing==204?contextInput:va("%d",s->repeats),204);
  ContextText(402,204,va("Remaining: %d / %d",s->count,s->initial),.43f,qfalse);
  ContextButton(402,232,220,"Reset remaining to full count");
  ContextText(402,280,"Rearms: -1 means unlimited.",.38f,qfalse);
  ContextText(402,302,"-1 repeats forever; 0 runs once.",.4f,qfalse);
 }
 if(kind==6){
  ContextFieldDraw(148,"Speed",contextEditing==205?contextInput:va("%d",s->speed),205);
  ContextButton(402,176,220,s->flags&2?"Direction: constant speed":"Direction: arc to destination");
  ContextButton(402,204,220,s->flags&1?"Jump sound: On":"Jump sound: Off");
  ContextText(402,260,"Speed is in units per second.",.38f,qfalse);
  ContextText(402,282,"Link Primary to a destination marker.",.38f,qfalse);
  ContextText(402,304,"No link: rotate to choose direction.",.38f,qfalse);
 }
 if(kind==7){ContextText(402,148,"Activate teleports its activator.",.4f,qfalse);ContextText(402,176,"Link Primary to a destination marker.",.38f,qfalse);ContextText(402,204,"Move / rotate the marker to place exit.",.38f,qfalse);ContextText(402,264,"Vehicles travel with consenting riders.",.4f,qfalse);}
 if(kind==13){ContextFieldDraw(148,"Damage per tick",contextEditing==206?contextInput:va("%d",s->damage),206);ContextButton(402,176,220,s->active?"Firing: On":"Firing: Off");ContextText(402,204,"Rotate to aim, or Link Primary.",.4f,qfalse);ContextText(402,280,"Range: 2048. Damage 0: visual only.",.38f,qfalse);ContextText(402,302,"Damage respects tool permissions.",.38f,qfalse);}
 if(kind==33){ContextFieldDraw(148,"Radius",contextEditing==212?contextInput:va("%d",s->speed),212);ContextButton(402,176,220,s->active?"Light: On":"Light: Off");ContextText(402,204,"Radius: 4-1020, in steps of 4.",.4f,qfalse);}
 if(kind==14){ContextButton(402,148,220,s->flags?"Use once: On":"Use once: Off");ContextButton(402,176,220,s->active?"Armed: On":"Armed: Off");ContextText(402,204,"Chooses one eligible linked target.",.38f,qfalse);ContextText(402,280,"Link Primary / Secondary to choices.",.38f,qfalse);}
 if(kind==15){ContextFieldDraw(148,"Interval ms",contextEditing==201?contextInput:va("%d",s->waitMs),201);ContextFieldDraw(176,"Random +/- ms",contextEditing==202?contextInput:va("%d",s->randomMs),202);ContextButton(402,204,220,s->active?"Running: On":"Running: Off");ContextText(402,280,"Link Primary; Activate starts/stops.",.38f,qfalse);ContextText(402,302,"Random varies the interval around its base.",.38f,qfalse);}
 if(kind==16){ContextText(402,148,"Activate collects linked pickups.",.4f,qfalse);ContextText(402,176,"Link Primary / Secondary to items.",.38f,qfalse);ContextText(402,204,"Full / refused pickups are retained.",.38f,qfalse);ContextText(402,280,"Collected items stay consumed in saves.",.38f,qfalse);}
 if(kind==17){ContextFieldDraw(148,"Points",contextEditing==207?contextInput:va("%d",s->count),207);ContextText(402,176,"Negative values subtract points.",.38f,qfalse);ContextText(402,204,"Adds points to the activator.",.38f,qfalse);}
 if(kind==18){const char*modes[]={"Normal","Falling","Electrical"};ContextButton(402,148,220,va("Death: %s",modes[s->flags]));ContextText(402,176,"Activate kills its activator.",.4f,qfalse);ContextText(402,204,"Respects player tool consent.",.38f,qfalse);}
 if(JAR_ControllerSwitch(kind)){ContextText(402,148,kind==11?"Enables linked controllers.":"Disables linked controllers.",.4f,qfalse);ContextText(402,176,"Link Primary / Secondary to targets.",.38f,qfalse);ContextText(402,204,"Changes Enabled; does not fire them.",.38f,qfalse);ContextText(402,280,"Pending delays keep their timing.",.38f,qfalse);}
 if(kind==JAR_STATIC_BRUSH_CONTROLLER)ContextText(402,148,"Use fires the Primary link.",.4f,qfalse);
 if(kind!=5&&kind!=8)ContextButton(402,kind==4?258:232,220,s->inactive?"Enabled: Off":"Enabled: On");
}
static qboolean ControllerClick(void){
 int kind=contextControllerKind;if(!kind)return qfalse;if(kind==JAR_SECTION_CONTROLLER){if(!ContextBox(402,148,220,20))return qfalse;ContextProperty("activate");return qtrue;}if(kind==JAR_MUSIC_CONTROLLER)return MusicClick();if(JAR_ControllerMedia(kind))return MediaClick();if(JAR_ControllerTrigger(kind))return TriggerClick();
 if(JAR_ControllerPortal(kind)){
  if(kind==JAR_PORTAL_SURFACE_CONTROLLER)for(int i=0;i<2;++i)
   if(ContextBox(402,148+28*i,220,20)){contextEditing=CONTEXT_PORTAL_WIDTH+i;contextInput[0]=0;return qtrue;}
  if(ContextBox(402,204,220,20)){ContextProperty("controller_portal_mode");return qtrue;}
  if(ContextBox(402,232,104,20)){ContextProperty("controller_portal_power");return qtrue;}
  if(ContextBox(514,232,104,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(kind==JAR_SEARCHLIGHT_CONTROLLER){
  for(int i=0;i<4;++i)if(ContextBox(402,148+28*i,220,20)){contextEditing=i==3?CONTEXT_SEARCHLIGHT_COLOR:CONTEXT_SEARCHLIGHT_RANGE+i;contextInput[0]=0;return qtrue;}
  if(ContextBox(402,260,220,20)){ContextProperty("controller_searchlight_track");return qtrue;}
  if(ContextBox(402,288,104,20)){ContextProperty("controller_searchlight_power");return qtrue;}
  if(ContextBox(514,288,104,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(kind==JAR_DIRECTIONAL_CONTROLLER){
  if(ContextBox(402,148,220,20)||ContextBox(402,176,220,20)||ContextBox(402,204,220,20)){
   contextEditing=cursorY<176?208:cursorY<204?CONTEXT_DIRECTIONAL_WAIT:CONTEXT_DIRECTIONAL_USES;
   contextInput[0]=0;return qtrue;
  }
  if(ContextBox(402,232,220,20)){ContextProperty("controller_directional_players");return qtrue;}
  if(ContextBox(402,260,220,20)){ContextProperty("controller_playing");return qtrue;}
  if(ContextBox(402,288,220,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(JAR_ControllerMovement(kind)){
  if(ContextBox(402,148,220,20)){contextEditing=kind==JAR_GRAVITY_CONTROLLER?CONTEXT_MARKER_GRAVITY:CONTEXT_MARKER_FRICTION;contextInput[0]=0;return qtrue;}
  if(ContextBox(402,176,220,20)){ContextProperty("controller_enabled");return qtrue;}
  if(!contextController.inactive&&ContextBox(402,204,220,20)){ContextProperty("activate");return qtrue;}
  return qfalse;
 }
 if(kind==JAR_DISTANCE_CONTROLLER)return DistanceClick();
 if(JAR_ControllerSlide(kind))return SlidingDoorClick();
 if(kind==JAR_BOBBING_CONTROLLER)return BobbingClick();
 if(JAR_ControllerAngular(kind))return AngularClick();
 if(kind==JAR_BUTTON_CONTROLLER)return ButtonClick();
 if(kind==JAR_REFLECTION_CONTROLLER)return ReflectionClick();
 if(kind==JAR_WALL_CONTROLLER){
  if(ContextBox(402,148,220,20)){ContextProperty("controller_wall_visible");return qtrue;}
  if(ContextBox(402,176,220,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(JAR_ControllerStart(kind)){
  if(ContextBox(402,148,220,20)){contextEditing=200;contextInput[0]=0;return qtrue;}
  if(ContextBox(402,176,220,20)){ContextProperty("controller_start_spawn");return qtrue;}
  if(!contextController.inactive&&ContextBox(402,204,220,20)){ContextProperty("activate");return qtrue;}
  return qfalse;
 }
 if(kind==JAR_ALWAYS_CONTROLLER){
  if(ContextBox(402,148,220,20)){contextEditing=201;contextInput[0]=0;return qtrue;}
  if(ContextBox(402,232,220,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(kind==JAR_VISIBLE_CONTROLLER){
  if(ContextBox(402,148,220,20)||ContextBox(402,176,220,20)){
   contextEditing=cursorY<176?CONTEXT_VISIBLE_RANGE:CONTEXT_VISIBLE_CONE;
   contextInput[0]=0;return qtrue;
  }
  if(ContextBox(402,232,220,20)){ContextProperty("controller_visible_walls");return qtrue;}
  if(ContextBox(402,260,220,20)){ContextProperty("controller_visible_sight");return qtrue;}
  if(ContextBox(402,288,220,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(JAR_ControllerLocation(kind)){
  if(ContextBox(402,148,220,20)||(kind==JAR_LOCATION_VOLUME_CONTROLLER&&ContextBox(402,204,220,20))){
   contextEditing=cursorY<204?200:208;contextInput[0]=0;return qtrue;
  }
  if(ContextBox(402,176,220,20)){ContextProperty("controller_location_color");return qtrue;}
  if(ContextBox(402,232,220,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(kind==JAR_DOOR_CAMERA_CONTROLLER){
  if(ContextBox(402,148,220,20)||ContextBox(402,176,220,20)){
   contextEditing=cursorY<176?CONTEXT_CAMERA_TRAVEL:CONTEXT_CAMERA_SPEED;contextInput[0]=0;return qtrue;
  }
  if(!contextController.inactive&&ContextBox(402,204,220,20)){ContextProperty("activate");return qtrue;}
  if(ContextBox(402,232,220,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(kind==JAR_WELDER_CONTROLLER){
  const int fields[]={CONTEXT_WELDER_REACH,CONTEXT_WELDER_RADIUS,CONTEXT_WELDER_INTERVAL,206};
  for(int i=0;i<ARRAY_LEN(fields);++i)if(ContextBox(402,148+i*28,220,20)){
   contextEditing=fields[i];contextInput[0]=0;return qtrue;
  }
  if(!contextController.inactive&&ContextBox(402,260,220,20)){ContextProperty("activate");return qtrue;}
  if(ContextBox(402,288,220,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(kind==JAR_CRYSTAL_CRATE_CONTROLLER){
  const int fields[]={CONTEXT_CRYSTAL_HEALTH,CONTEXT_CRYSTAL_HIT_DAMAGE,CONTEXT_CRYSTAL_HIT_RADIUS,CONTEXT_CRYSTAL_BLAST_DAMAGE,CONTEXT_CRYSTAL_BLAST_RADIUS};
  for(int i=0;i<ARRAY_LEN(fields);++i)if(ContextBox(402,148+i*26,220,20)){
   contextEditing=fields[i];contextInput[0]=0;return qtrue;
  }
  if(ContextBox(402,278,220,20)){ContextProperty("controller_crystal_solid");return qtrue;}
  if(ContextBox(402,304,104,20)){ContextProperty("controller_enabled");return qtrue;}
  if(!contextController.inactive&&ContextBox(514,304,104,20)){ContextProperty("activate");return qtrue;}
  return qfalse;
 }
 if(JAR_ControllerRack(kind))return qfalse;
 if(JAR_ControllerStation(kind)){
  if(ContextBox(402,148,220,20)||ContextBox(402,176,220,20)||ContextBox(402,204,220,20)){contextEditing=cursorY<176?203:cursorY<204?210:201;contextInput[0]=0;return qtrue;}
  if(ContextBox(402,232,220,20)){ContextProperty("controller_nodrain");return qtrue;}if(ContextBox(402,260,220,20)){ContextProperty("controller_reset");return qtrue;}if(ContextBox(402,288,220,20)){ContextProperty("controller_enabled");return qtrue;}return qfalse;
 }
 if(kind==29){
  if(ContextBox(402,148,220,20)){ContextProperty("controller_usablemode");return qtrue;}
  if(ContextBox(402,176,220,20)){if(contextController.flags&8){contextEditing=201;contextInput[0]=0;}else ContextProperty("controller_visible");return qtrue;}
  if(ContextBox(402,204,220,20)){contextEditing=203;contextInput[0]=0;return qtrue;}
  if(ContextBox(402,232,104,20)){ContextProperty("controller_playeruse");return qtrue;}if(ContextBox(514,232,104,20)){ContextProperty("controller_npconly");return qtrue;}
  if(ContextBox(402,260,220,20)){ContextProperty("controller_blockcheck");return qtrue;}
  if(ContextBox(402,288,104,20)){ContextProperty("controller_rearm");return qtrue;}if(ContextBox(514,288,104,20)){ContextProperty("controller_enabled");return qtrue;}return qfalse;
 }
 if(JAR_ControllerAnimatedTag(kind)){
  const qboolean actionable=kind==JAR_BEACON_CONTROLLER||contextController.active==JAR_MP_BOMB_ARMED;
  if(!contextController.inactive&&actionable&&ContextBox(402,176,220,20)){ContextProperty("activate");return qtrue;}
  if(ContextBox(402,204,220,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(JAR_ControllerShooter(kind)){
  if(ContextBox(402,148,220,20)||ContextBox(402,176,220,20)){
   contextEditing=cursorY<176?201:202;contextInput[0]=0;return qtrue;
  }
  if(ContextBox(402,204,220,20)){ContextProperty("controller_shooter_alt");return qtrue;}
  if(ContextBox(402,232,220,20)){ContextProperty("controller_shooter_repeat");return qtrue;}
  if(!contextController.inactive&&ContextBox(402,260,220,20)){ContextProperty("activate");return qtrue;}
  if(ContextBox(402,288,220,20)){ContextProperty("controller_enabled");return qtrue;}
  return qfalse;
 }
 if(JAR_ControllerEmplaced(kind)){
  const int actionY=kind==JAR_EWEB_CONTROLLER?232:204;
  if(contextController.active==1&&ContextBox(402,148,220,20))contextEditing=203;
  else if(ContextBox(402,176,220,20))contextEditing=213;
  else if(kind==JAR_EWEB_CONTROLLER&&ContextBox(402,204,220,20))contextEditing=214;
  else if(contextController.active==0&&ContextBox(402,actionY,220,20)){ContextProperty("controller_repair");return qtrue;}
  else if(contextController.active==1&&!contextController.inactive&&ContextBox(402,actionY,220,20)){ContextProperty("activate");return qtrue;}
  else if(ContextBox(402,actionY+28,220,20)){ContextProperty("controller_enabled");return qtrue;}
  else return qfalse;
  contextInput[0]=0;return qtrue;
 }
 if(JAR_ControllerTerminalBreakable(kind)){
  if(contextController.active==1&&ContextBox(402,148,220,20))contextEditing=203;
  else if(ContextBox(402,176,220,20))contextEditing=206;
  else if(ContextBox(402,204,220,20))contextEditing=212;
  else if(ControllerCanDetonate(kind,&contextController)&&ContextBox(402,232,220,20)){ContextProperty("activate");return qtrue;}
  else if(ContextBox(402,260,220,20)){ContextProperty("controller_enabled");return qtrue;}
  else return qfalse;
  contextInput[0]=0;return qtrue;
 }
 if(JAR_ControllerDebris(kind)){
  if(ContextBox(402,316,220,20)){breakPage=!breakPage;contextEditing=0;return qtrue;}
  if(!breakPage){
   if(ContextBox(402,148,220,20)||ContextBox(402,176,220,20)){contextEditing=cursorY<176?203:201;contextInput[0]=0;return qtrue;}
   if(ContextBox(402,204,220,20)){ContextProperty("controller_damagepolicy");return qtrue;}
   if(ContextBox(402,232,104,20)){ContextProperty("controller_playeruse");return qtrue;}if(ContextBox(514,232,104,20)){ContextProperty("controller_usemode");return qtrue;}
   if(ContextBox(402,260,104,20)){ContextProperty("controller_repair");return qtrue;}if(ContextBox(514,260,104,20)){ContextProperty("controller_enabled");return qtrue;}
  }else{
   if(ContextBox(402,148,220,20)){ContextProperty("controller_material");return qtrue;}
   int fields[]={211,206,212,202};for(int i=0;i<4;++i)if(ContextBox(402,176+i*28,220,20)){contextEditing=fields[i];contextInput[0]=0;return qtrue;}
   if(ContextBox(402,288,220,20)){ContextProperty("controller_explosion");return qtrue;}
  }return qfalse;
 }
 if((kind==1||kind==5||kind==8)&&ContextBox(402,148,220,20)){contextEditing=200;contextInput[0]=0;return qtrue;}
 if((kind==3||kind==4||kind==15)&&(ContextBox(402,148,220,20)||ContextBox(402,176,220,20))){contextEditing=(kind==4?203:201)+(cursorY>=176);contextInput[0]=0;return qtrue;}
 if((kind==1||kind==3)&&ContextBox(402,204,220,20)){ContextProperty(va("controller_flags_%d",(contextController.flags+1)%(kind==1?5:2)));return qtrue;}
 if(kind==2&&ContextBox(402,148,220,20)){ContextProperty(va("controller_flags_%d",(contextController.flags+1)%3));return qtrue;}
 if(kind==4&&ContextBox(402,232,220,20)){ContextProperty("controller_reset");return qtrue;}
 if(kind==6){
  if(ContextBox(402,148,220,20)){contextEditing=205;contextInput[0]=0;return qtrue;}
  if(ContextBox(402,176,220,20)){ContextProperty(va("controller_flags_%d",contextController.flags^2));return qtrue;}
  if(ContextBox(402,204,220,20)){ContextProperty(va("controller_flags_%d",contextController.flags^1));return qtrue;}
 }
 if(kind==14){if(ContextBox(402,148,220,20)){ContextProperty(va("controller_flags_%d",!contextController.flags));return qtrue;}if(ContextBox(402,176,220,20)){ContextProperty("controller_playing");return qtrue;}}
 if(kind==17&&ContextBox(402,148,220,20)){contextEditing=207;contextInput[0]=0;return qtrue;}
 if(kind==18&&ContextBox(402,148,220,20)){ContextProperty(va("controller_flags_%d",(contextController.flags+1)%3));return qtrue;}
 if(kind==15&&ContextBox(402,204,220,20)){ContextProperty("controller_playing");return qtrue;}
 if(kind==33){if(ContextBox(402,148,220,20)){contextEditing=212;contextInput[0]=0;return qtrue;}if(ContextBox(402,176,220,20)){ContextProperty("controller_playing");return qtrue;}}
 if(kind==13){if(ContextBox(402,148,220,20)){contextEditing=206;contextInput[0]=0;return qtrue;}if(ContextBox(402,176,220,20)){ContextProperty("controller_playing");return qtrue;}}
 if(kind!=5&&kind!=8&&ContextBox(402,kind==4?258:232,220,20)){ContextProperty("controller_enabled");return qtrue;}
 return qfalse;
}
static void ContextPick(void){
 if(cg.refdef.width<=0||cg.refdef.height<=0)return;
 float sx=cursorX*cgs.screenXScale,sy=cursorY*cgs.screenYScale;
 if(sx<cg.refdef.x||sx>=cg.refdef.x+cg.refdef.width||sy<cg.refdef.y||sy>=cg.refdef.y+cg.refdef.height)return;
 vec3_t direction;float x=((sx-cg.refdef.x)*2/cg.refdef.width-1)*tanf(cg.refdef.fov_x*(float)M_PI/360),y=(1-(sy-cg.refdef.y)*2/cg.refdef.height)*tanf(cg.refdef.fov_y*(float)M_PI/360);
 VectorMA(cg.refdef.viewaxis[0],-x,cg.refdef.viewaxis[1],direction);VectorMA(direction,y,cg.refdef.viewaxis[2],direction);VectorNormalize(direction);
 trap->SendClientCommand(va("jar_select ray_%.3f_%.3f_%.3f_%.6f_%.6f_%.6f",cg.refdef.vieworg[0],cg.refdef.vieworg[1],cg.refdef.vieworg[2],direction[0],direction[1],direction[2]));
}
static qboolean ContextActionAvailable(int action){
 if(contextKind==JAR_CONTEXT_PLAYER){
  if(selectedEntity==cg.clientNum)return action==1&&(contextCaps&JAR_CONTEXT_RELEASE);
  if(action==2||action==3||action==4)return qfalse;
 }
 // These selections identify their passive controller before the type echo arrives.
 const int kind=contextControllerKind;
 const qboolean canDisableAction=kind==JAR_DOOR_CAMERA_CONTROLLER||kind==JAR_WELDER_CONTROLLER||kind==JAR_CRYSTAL_CRATE_CONTROLLER;
 return action!=2||(contextCatalog!=JAR_MP_SMALL_CARGO_ID&&contextControllerKind!=JAR_SMALL_CARGO_CONTROLLER&&
                   contextCatalog!=JAR_MP_GUN_RACK_ID&&contextCatalog!=JAR_MP_AMMO_RACK_ID&&
                   !JAR_ControllerRack(kind)&&(!canDisableAction||!contextController.inactive));
}
static void ContextClick(void){
 if(JAR_ContextHit(JC_CLOSE,0,cursorX,cursorY)){MenuClose();return;}
 if(ContextBox(8,418,200,20)){contextEditing=0;trap->SendClientCommand("jar_release");return;}
 if(ContextBox(8,442,170,20)||ContextBox(184,442,190,20)){contextEditing=0;trap->SendClientCommand(cursorX<180?"jar_allowTools 1":"jar_allowTools 0");return;}
 if(JAR_ContextHit(JC_WORLD,0,cursorX,cursorY)){contextEditing=0;ContextPick();return;}
 if(JAR_ContextHit(JC_SELF,0,cursorX,cursorY)){contextEditing=0;trap->SendClientCommand("jar_select self");return;}
 if(JAR_ContextHit(JC_REFRESH,0,cursorX,cursorY)){ContextTransformReload();ContextProperty("refresh");return;}
 for(int i=0;i<3;++i)if(JAR_ContextHit(JC_TAB,i,cursorX,cursorY)){contextEditing=0;contextPage=i;return;}
 if(JAR_ContextHit(JC_UNDO,0,cursorX,cursorY)){contextEditing=0;if(contextCaps&JAR_CONTEXT_UNDO)ContextProperty("property_undo");return;}
 if(selectedEntity<0)return;
 const char*actions[]={"freeze","release","activate","remove","copy","paste","rope","thruster","weld","hinge","detach"};
 for(int i=0;i<11;++i)if(ContextActionAvailable(i)&&JAR_ContextHit(JC_ACTION,i,cursorX,cursorY)){contextEditing=0;ContextProperty(actions[i]);return;}
 if(contextPage==1){
  contextEditing=0;
  if(contextCaps&JAR_CONTEXT_MOVEMENT){
   if(ContextBox(402,148,220,20)||ContextBox(402,176,220,20)){
    contextEditing=cursorY<176?CONTEXT_ACTOR_GRAVITY:CONTEXT_ACTOR_FRICTION;contextInput[0]=0;
   }else if(ContextBox(402,204,220,20))ContextProperty("actor_map_gravity");
   else if(ContextBox(402,232,220,20))ContextProperty("actor_map_friction");
  }else if((contextCaps&JAR_CONTEXT_GRAVITY)&&ContextBox(402,346,220,20))ContextProperty("gravity");
  else if(contextCaps&JAR_CONTEXT_CONTROLLER)ControllerClick();
  else if((contextCaps&JAR_CONTEXT_GRAVITY)&&ContextBox(402,148,220,20))ContextProperty("gravity");
  return;
 }
 if(contextPage==2){contextEditing=0;if(!poseFrames)return;
  if(ContextBox(402,148,220,20)){poseEditing=qtrue;poseInput[0]=0;return;}
  if(ContextBox(402,214,104,20))ContextProperty("pose_prev");else if(ContextBox(514,214,104,20))ContextProperty("pose_next");
  else if(ContextBox(402,246,220,20))ContextProperty("pose_undo");else if(ContextBox(402,278,220,20))ContextProperty("pose_reset");return;
 }
 if(contextCaps&JAR_CONTEXT_TRANSFORM){
  if(ContextBox(402,210,220,20)){ContextTransformApplyDraft(0);return;}
  if(ContextBox(402,296,220,20)){ContextTransformApplyDraft(3);return;}
  for(int i=0;i<6;++i){float y=i<3?148+20.f*i:234+20.f*(i-3);if(ContextBox(402,y,220,20)){if(!ContextTransformCommit())return;contextEditing=100+i;contextInput[0]=0;contextTransformPage=1;return;}}
 }
 if(ContextField()&&ContextBox(402,322,220,20)){
  contextEditing=(contextCaps&JAR_CONTEXT_ARMOR)&&cursorX>=514?JAR_PROPERTY_ARMOR:ContextField();
  contextInput[0]=0;return;
 }
 if(ContextField()&&ContextBox(402,346,220,20)){ContextInputKey(A_ENTER);return;}
 contextEditing=0;
}
static void DrawContext(void){
 jarContextRect_t panel=JAR_ContextRect(JC_PANEL,0);vec4_t color={.04f,.04f,.04f,.95f};CG_FillRect(panel.x,panel.y,panel.w,panel.h,color);
 ContextWidget(JC_TAB,0,"Properties");if(contextCaps&(JAR_CONTEXT_CONTROLLER|JAR_CONTEXT_GRAVITY|JAR_CONTEXT_MOVEMENT))ContextWidget(JC_TAB,1,contextCaps&JAR_CONTEXT_CONTROLLER?"Controller":contextCaps&JAR_CONTEXT_MOVEMENT?"Movement":"Physics");if(poseFrames)ContextWidget(JC_TAB,2,"Pose");
 const char*name=contextKinds[contextKind];char shortName[35];
 if(contextKind==JAR_CONTEXT_PLAYER&&selectedEntity>=0&&selectedEntity<MAX_CLIENTS){
  if(selectedEntity==cg.clientNum)name="Your player";
  else if(cgs.clientinfo[selectedEntity].infoValid)name=cgs.clientinfo[selectedEntity].cleanname;
 }else{
  for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(jarMPCatalog[i].id==contextCatalog){name=JAR_MPCatalogName(&jarMPCatalog[i]);break;}
  const char*base=strrchr(name,'/');if(base)name=base+1;
 }
 Q_strncpyz(shortName,name,sizeof(shortName));ContextText(402,76,shortName,.43f,qfalse);
 ContextWidget(JC_INFO,0,selectedEntity<0?"Click an entity in the world":va("%s #%d",contextKinds[contextKind],selectedEntity));
 ContextWidget(JC_SELF,0,"Select myself");ContextWidget(JC_REFRESH,0,"Refresh");
 if(selectedEntity>=0){
  if(contextPage==1){
   if(contextCaps&JAR_CONTEXT_MOVEMENT){
    ContextFieldDraw(148,"Gravity",contextEditing==CONTEXT_ACTOR_GRAVITY?contextInput:contextActorGravity<0?"Map":va("%d",contextActorGravity),CONTEXT_ACTOR_GRAVITY);
    ContextFieldDraw(176,"Ground friction",contextEditing==CONTEXT_ACTOR_FRICTION?contextInput:contextActorFriction<0?"Default":va("%d",contextActorFriction),CONTEXT_ACTOR_FRICTION);
    ContextButton(402,204,220,"Use map gravity");
    ContextButton(402,232,220,"Use default friction");
   }else if(contextCaps&JAR_CONTEXT_CONTROLLER){ControllerDraw();if(contextCaps&JAR_CONTEXT_GRAVITY)ContextButton(402,346,220,contextGravity?"Gravity: Off":"Gravity: On");}
   else if(contextCaps&JAR_CONTEXT_GRAVITY){ContextButton(402,148,220,contextGravity?"Gravity: Off":"Gravity: On");ContextText(402,176,contextFrozen?"Frozen":"Released / held",.43f,qfalse);}
  }
  else if(contextPage==2){if(poseFrames){ContextButton(402,148,220,poseEditing?va("Frame: %s_",poseInput):va("Frame: %d",poseFrame));ContextText(402,180,va("0 to %d; type and press Enter",poseFrames-1),.43f,qfalse);ContextButton(402,214,104,"Previous");ContextButton(514,214,104,"Next");ContextButton(402,246,220,"Revert pose");ContextButton(402,278,220,"Reset to frame 0");ContextText(402,310,"Copy and world saves keep this pose.",.38f,qfalse);}}
  else {
   if(contextCaps&JAR_CONTEXT_TRANSFORM){for(int i=0;i<6;++i)ContextFieldDraw(i<3?148+20.f*i:234+20.f*(i-3),contextTransformLabels[i],contextEditing==100+i?contextInput:contextTransformDraft[i],100+i);ContextButton(402,210,220,"Apply position");ContextButton(402,296,220,"Apply angles");}
   else if(contextKind!=JAR_CONTEXT_PLAYER||selectedEntity!=cg.clientNum)ContextText(402,148,"Freeze actors to edit their placement.",.38f,qfalse);
   int field=ContextField();
   if(field){
    const int width=contextCaps&JAR_CONTEXT_ARMOR?108:220;
    ContextFieldAt(402,322,width,field==JAR_PROPERTY_MASS?"Mass kg":field==JAR_PROPERTY_HEALTH?"Health":"Quantity",
     contextEditing==field?contextInput:field==JAR_PROPERTY_MASS?va("%.9g",contextMass):va("%d",field==JAR_PROPERTY_HEALTH?contextHealth:contextQuantity),field);
    if(contextCaps&JAR_CONTEXT_ARMOR)
     ContextFieldAt(514,322,108,"Armor",contextEditing==JAR_PROPERTY_ARMOR?contextInput:va("%d",contextArmor),JAR_PROPERTY_ARMOR);
    ContextButton(402,346,220,"Apply value");
   }
  }
  for(int i=0;i<11;++i)if(ContextActionAvailable(i))ContextWidget(JC_ACTION,i,jarContextActions[i]);
 }
 if(contextCaps&JAR_CONTEXT_UNDO)ContextWidget(JC_UNDO,0,"Revert property");ContextWidget(JC_CLOSE,0,"Close");
 ContextButton(8,418,200,"Release myself");ContextButton(8,442,170,"Allow tools on me");ContextButton(184,442,190,"Block tools on me");
 const char*hint=menuPinned?"Click an entity or field. Typing unlocked / Esc closes.":"Hold C. Click an entity or field; release C after clicking a field to type.";
 if(ContextBox(402,322,220,20)&&!contextPage)hint=ContextField()==JAR_PROPERTY_MASS?"Mass in kilograms.":ContextField()==JAR_PROPERTY_HEALTH?"Type a whole number and press Enter.":"Zero uses the pickup's native default.";
 if(contextPage==0&&ContextBox(402,148,220,148))hint="Apply position and angles separately. Refresh discards staged edits.";
 if(contextPage==1&&contextControllerKind==JAR_MUSIC_CONTROLLER)hint="One stock track, or intro and loop separated by a space.";
 if(contextPage==1&&JAR_ControllerMedia(contextControllerKind))hint=contextControllerKind==9?"Stock sound/...wav or .mp3. Repeat 0 plays on demand.":"Effect path without effects/ or .efx. Rotate or Link Primary to aim.";
 Widget(JM_STATUS,0,menuStatusUntil>cg.time?menuStatus:hint,qfalse);
}
