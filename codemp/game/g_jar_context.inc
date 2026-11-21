// Typed, identity-bound multiplayer context edits. GPL-2.0-only.
#include "g_jar_transform.inc"
static unsigned contextSequence;
static qboolean ContextEditable(gentity_t*p,int n){
 return sandbox.integer&&!level.intermissiontime&&(n==p->s.number?ActorAlive(p):Manipulate(p,n));
}
static unsigned ContextIdentity(int n){
 if(!objects[n].contextIdentity){if(!++contextSequence)++contextSequence;objects[n].contextIdentity=contextSequence;}
 return objects[n].contextIdentity;
}
static int ContextKind(int n){
 gentity_t*e=&g_entities[n];if(e->client)return n<MAX_CLIENTS?JAR_CONTEXT_PLAYER:e->m_pVehicle?JAR_CONTEXT_VEHICLE:JAR_CONTEXT_NPC;
 return PhysicsProp(n)?JAR_CONTEXT_PROP:e->item?JAR_CONTEXT_PICKUP:JAR_CONTEXT_OTHER;
}
static double ContextValue(int n,int field){
 switch(field){case JAR_PROPERTY_MASS:return objects[n].mass;case JAR_PROPERTY_GRAVITY:return (float)objects[n].noGravity;
  case JAR_PROPERTY_ACTOR_GRAVITY:case JAR_PROPERTY_ACTOR_FRICTION:return g_entities[n].client->ps.stats[MovementPropertyStat(field)]-1;
  case JAR_PROPERTY_HEALTH:return g_entities[n].health;case JAR_PROPERTY_QUANTITY:return g_entities[n].count;
  case JAR_PROPERTY_ARMOR:return g_entities[n].client->ps.stats[STAT_ARMOR];
  default:return 0;}
}
static qboolean ContextControllerCurrent(int n,const jarControllerState_t*expected){
 jarControllerState_t actual;
 ControllerState(n,&actual);
 // Actuator progress does not supersede a travel or speed edit.
 if(ControllerKind(n)==JAR_DOOR_CAMERA_CONTROLLER)actual.pushSpeed=expected->pushSpeed;
 if(JAR_ControllerSlide(ControllerKind(n))){actual.pushSpeed=expected->pushSpeed;actual.repeats=expected->repeats;}
 if(ControllerKind(n)==JAR_BOBBING_CONTROLLER||JAR_ControllerAngular(ControllerKind(n))){actual.pushSpeed=expected->pushSpeed;actual.repeats=expected->repeats;}
 if(ControllerKind(n)==JAR_BUTTON_CONTROLLER)actual.active=expected->active;
 return !memcmp(&actual,expected,sizeof(actual))?qtrue:qfalse;
}
static qboolean ContextUndoReady(gentity_t*p,int n){
 const jarPropertyUndo_t*u=&players[p->s.number].propertyUndo;
 return u->field&&u->entity==n&&u->identity==ContextIdentity(n)&&u->revision==objects[n].propertyRevision&&
  (u->field==JAR_PROPERTY_DISTANCE_LINK?ControllerKind(n)==JAR_DISTANCE_CONTROLLER:
   u->field==JAR_PROPERTY_CONTROLLER?ContextControllerCurrent(n,&u->controllerAfter):u->field==JAR_PROPERTY_TRANSFORM?ContextTransformCurrent(n,u):ContextValue(n,u->field)==u->after);
}
static int ContextCaps(gentity_t*p,int n){
 gentity_t*e=&g_entities[n];int kind=ContextKind(n),caps=JAR_CONTEXT_DETACH;
 if(objects[n].copyPending||(e->client&&(e->s.eFlags&EF_NODRAW)))return 0;
 if(MovementActorEligible(e))caps|=JAR_CONTEXT_MOVEMENT;
 if(kind==JAR_CONTEXT_PLAYER)caps|=JAR_CONTEXT_ARMOR;
 if(n==p->s.number)return caps|JAR_CONTEXT_HEALTH|JAR_CONTEXT_RELEASE|(ContextUndoReady(p,n)?JAR_CONTEXT_UNDO:0);
 if(Edit(p,n)&&!RemovalOccupied(e))caps|=JAR_CONTEXT_REMOVE;
 if(kind==JAR_CONTEXT_PROP&&!JAR_MP_NativeEmplacedOccupied(e))caps|=JAR_CONTEXT_FREEZE|JAR_CONTEXT_RELEASE|JAR_CONTEXT_GRAVITY|JAR_CONTEXT_MASS|JAR_CONTEXT_ROTATE|JAR_CONTEXT_POSE;
 else if(e->client){caps|=JAR_CONTEXT_FREEZE|JAR_CONTEXT_RELEASE|JAR_CONTEXT_HEALTH;if(JAR_MP_ActorControlled(e))caps|=JAR_CONTEXT_ROTATE;}
 else if(kind==JAR_CONTEXT_PICKUP&&(e->item->giType==IT_WEAPON||e->item->giType==IT_AMMO||e->item->giType==IT_HEALTH||e->item->giType==IT_POWERUP))caps|=JAR_CONTEXT_QUANTITY;
 if(ControllerKind(n))caps|=JAR_CONTEXT_CONTROLLER;
 if(JAR_MP_EmplacedState(&e->s)||JAR_MP_AnimatedTag(&e->s))caps&=~JAR_CONTEXT_POSE;
 if(JAR_MP_DoorCameraRootState(&e->s))caps&=~(JAR_CONTEXT_POSE|JAR_CONTEXT_MASS);
 if(JAR_MP_WelderRootState(&e->s)||JAR_MP_CrystalCrateRootState(&e->s))caps&=~JAR_CONTEXT_POSE;
 if(JAR_ControllerPortal(ControllerKind(n)))caps&=~JAR_CONTEXT_POSE;
 if(ControllerKind(n)==JAR_WALL_CONTROLLER||JAR_ControllerSlide(ControllerKind(n))||
    ControllerKind(n)==JAR_BUTTON_CONTROLLER||ControllerKind(n)==JAR_BOBBING_CONTROLLER||JAR_ControllerAngular(ControllerKind(n)))caps&=~JAR_CONTEXT_POSE;
 if(JAR_MP_BeaconState(&e->s)&&BodyDormant(n))caps&=~(JAR_CONTEXT_FREEZE|JAR_CONTEXT_RELEASE|JAR_CONTEXT_ROTATE);
 if(ContextTransformAllowed(n))caps|=JAR_CONTEXT_TRANSFORM;
 if(ContextUndoReady(p,n))caps|=JAR_CONTEXT_UNDO;
 return caps;
}
static void SelectionEcho(gentity_t*p){
 int n=players[p->s.number].selected;
 if(!ContextEditable(p,n)){players[p->s.number].selected=ENTITYNUM_NONE;trap->SendServerCommand(p->s.number,"jarselection -1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");return;}
 jarObject_t*o=&objects[n];gentity_t*e=&g_entities[n];unsigned id=o->catalog?jarMPCatalog[o->catalog-1].id:0;
 const int caps=ContextCaps(p,n);
 const qboolean movement=(caps&JAR_CONTEXT_MOVEMENT)!=0;
 trap->SendServerCommand(p->s.number,va("jarselection %d %d %d %u %d %u %d %d %d %.9g %d %d %.9g %.9g %.9g %.9g %.9g %.9g %d %d %d",n,
  PhysicsProp(n)?e->s.frame:0,PhysicsPoseCount(n),ContextIdentity(n),ContextKind(n),id,caps&~(JAR_CONTEXT_MOVEMENT|JAR_CONTEXT_ARMOR),o->frozen,o->noGravity,o->mass,e->client?e->health:0,e->item?e->count:0,
  e->r.currentOrigin[0],e->r.currentOrigin[1],e->r.currentOrigin[2],AngleNormalize360(e->r.currentAngles[0]),AngleNormalize360(e->r.currentAngles[1]),AngleNormalize360(e->r.currentAngles[2]),
  movement?e->client->ps.stats[STAT_JAR_ACTOR_GRAVITY]-1:-2,movement?e->client->ps.stats[STAT_JAR_ACTOR_FRICTION]-1:-2,
  (caps&JAR_CONTEXT_ARMOR)?e->client->ps.stats[STAT_ARMOR]:-1));
 if(ControllerKind(n)){jarControllerState_t s;char hex[127];ControllerState(n,&s);JAR_ControllerHex(s.text,hex);
  trap->SendServerCommand(p->s.number,va("jarcontroller %u %d %d %d %d %d %d %d %d %s %d %d %d %d",ContextIdentity(n),ControllerKind(n),s.waitMs,s.randomMs,s.flags,s.repeats,s.count,s.initial,s.inactive,*hex?hex:"-",s.speed,s.active,s.damage,s.pushSpeed));}

}
static void SelectionChanged(int n){for(int c=0;c<MAX_CLIENTS;++c)if(players[c].selected==n&&g_entities[c].inuse&&g_entities[c].client&&g_entities[c].client->pers.connected==CON_CONNECTED)SelectionEcho(&g_entities[c]);}
static void ContextApply(int n,int field,double value){
 gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];
 switch(field){
  case JAR_PROPERTY_MASS:o->mass=(float)value;break;
  case JAR_PROPERTY_GRAVITY:o->noGravity=(int)value;break;
  case JAR_PROPERTY_HEALTH:
   e->health=e->client->ps.stats[STAT_HEALTH]=(int)value;
   if(e->m_pVehicle)e->m_pVehicle->m_iArmor=(int)value;
   if(n>=MAX_CLIENTS)ActorSnapshot(e); // Native player frames already publish their playerState.
   break;
  case JAR_PROPERTY_ARMOR:e->client->ps.stats[STAT_ARMOR]=(int)value;break;
  case JAR_PROPERTY_QUANTITY:e->count=(int)value;break;
  case JAR_PROPERTY_ACTOR_GRAVITY:case JAR_PROPERTY_ACTOR_FRICTION:
   e->client->ps.stats[MovementPropertyStat(field)]=(int)value+1;
   if(field==JAR_PROPERTY_ACTOR_GRAVITY)JAR_MP_ApplyMovementGravity(e);
   break;
 }
 ++o->propertyRevision;
}
static qboolean ContextController(gentity_t*p,const char*arg){
 if(strncmp(arg,"controller_",11))return qfalse;int n=players[p->s.number].selected,kind=ControllerKind(n);
 if(!kind){Tell(p,"Controller settings apply to spawned controllers.");return qtrue;}
 jarControllerState_t before,next;ControllerState(n,&before);next=before;const char*field=arg+11;const char*number=NULL;int*value=NULL;
 if(kind==JAR_REFLECTION_CONTROLLER&&!strcmp(field,"probe_refresh")){
  gentity_t* e=&g_entities[n];
  e->genericValue3=e->genericValue3>=INT_MAX?1:e->genericValue3+1;
  ReflectionSync(e);Tell(p,"Reflection capture requested.");return qtrue;
 }
 if(kind==JAR_DISTANCE_CONTROLLER&&(!strcmp(field,"distance_clear_reference")||!strcmp(field,"distance_clear_subject"))){
  const int role=!strcmp(field,"distance_clear_reference")?JAR_DISTANCE_REFERENCE:JAR_DISTANCE_SUBJECT;
  DistanceLinkChange(p,n,role,0);
  Tell(p,role==JAR_DISTANCE_REFERENCE?"This marker is now the distance reference.":"NPC link cleared.");
  return qtrue;
 }
 if(!strncmp(field,"text_",5)&&(kind==1||JAR_ControllerPosition(kind)||kind==JAR_MUSIC_CONTROLLER||JAR_ControllerMedia(kind)||JAR_ControllerLocation(kind))){if(!JAR_ControllerUnhex(field+5,next.text)){Tell(p,"Text must be 1-63 printable characters without quotes, semicolons or backslashes.");return qtrue;}}
 else if(!strcmp(field,"start_spawn")&&JAR_ControllerStart(kind))next.flags^=1;
 else if(kind==JAR_REFLECTION_CONTROLLER&&!strcmp(field,"probe_active"))next.active=!next.active;
 else if(kind==JAR_REFLECTION_CONTROLLER&&!strncmp(field,"probe_radius_",13)){number=field+13;value=&next.speed;}
 else if(kind==JAR_BOBBING_CONTROLLER&&!strcmp(field,"bob_running"))next.active=!next.active;
 else if(kind==JAR_BOBBING_CONTROLLER&&!strncmp(field,"bob_amplitude_",14)){number=field+14;value=&next.speed;}
 else if(kind==JAR_BOBBING_CONTROLLER&&!strncmp(field,"bob_period_",11)){number=field+11;value=&next.waitMs;}
 else if(JAR_ControllerAngular(kind)&&!strcmp(field,"angular_running"))next.active=!next.active;
 else if(JAR_ControllerAngular(kind)&&!strcmp(field,"angular_axis"))next.flags=(next.flags+1)%JAR_ANGULAR_AXES;
 else if(JAR_ControllerAngular(kind)&&!strncmp(field,"angular_rate_",13)){number=field+13;value=&next.speed;}
 else if(kind==JAR_PENDULUM_CONTROLLER&&!strncmp(field,"angular_period_",15)){number=field+15;value=&next.waitMs;}
 else if(!strcmp(field,"location_color")&&JAR_ControllerLocation(kind))next.flags=(next.flags+1)%8;
 else if((!strcmp(field,"asset_prev")||!strcmp(field,"asset_next"))&&JAR_ControllerMedia(kind))Q_strncpyz(next.text,JAR_MediaCycle(kind,before.text,!strcmp(field,"asset_prev")?-1:1),64);
 else if((!strcmp(field,"asset_prev")||!strcmp(field,"asset_next"))&&kind==JAR_MUSIC_CONTROLLER)JAR_MusicCycle(before.text,!strcmp(field,"asset_prev")?-1:1,next.text);
 else if(!strcmp(field,"shooter_alt")&&JAR_ControllerShooter(kind))next.flags^=JAR_SHOOTER_ALT;
 else if(!strcmp(field,"shooter_repeat")&&JAR_ControllerShooter(kind))next.flags^=JAR_SHOOTER_REPEAT;
 else if(!strcmp(field,"playing")&&(JAR_ControllerMedia(kind)||kind==13||kind==14||kind==15||kind==33||JAR_ControllerTrigger(kind)||kind==JAR_DIRECTIONAL_CONTROLLER)){
  next.active=!next.active;
  if(kind==JAR_DIRECTIONAL_CONTROLLER&&next.active&&next.repeats){next.count=next.initial;next.repeats=0;}
 }
 else if(!strcmp(field,"mode")&&JAR_ControllerMedia(kind)){
  if(kind==9){next.flags=next.flags==0?2:next.flags==2?4:next.flags==4?8:0;if(next.flags==2||next.flags==8)next.waitMs=next.randomMs=0;}
  else next.flags=next.flags?0:2;
  next.active=0;
 }
 else if(!strcmp(field,"pushmode")&&kind==22){int mode=next.flags&~2048;next.flags=(next.flags&2048)|(mode==4?20:mode==20?0:4);if(!(next.flags&4)){next.flags=0;next.waitMs=0;}}
 else if(!strcmp(field,"multiple")&&kind==22&&(next.flags&4))next.flags^=2048;
 else if(!strcmp(field,"spectators")&&kind==23)next.flags^=1;
 else if(!strncmp(field,"pushspeed_",10)&&kind==22){number=field+10;value=&next.pushSpeed;}
 else if(!strcmp(field,"subjects")&&(kind==19||kind==20)){int filter=next.flags&17;next.flags=(next.flags&~17)|(filter==0?1:filter==1?16:0);}
 else if(!strcmp(field,"input")&&(kind==19||kind==20))next.flags=(next.flags&~12)|(((next.flags&12)+4)%16);
 else if(!strcmp(field,"facing")&&(kind==19||kind==20))next.flags^=2;
 else if(!strcmp(field,"slow")&&kind==21)next.flags^=16;
 else if(kind==JAR_SPACE_CONTROLLER&&(!strncmp(field,"space_width_",12)||!strncmp(field,"space_depth_",12)||!strncmp(field,"space_height_",13))){
  int size[3];JAR_SpaceDimensions(&before,size);
  next.speed=size[0];next.damage=size[1];next.pushSpeed=size[2];
  if(field[6]=='w'){number=field+12;value=&next.speed;}
  else if(field[6]=='d'){number=field+12;value=&next.damage;}
  else {number=field+13;value=&next.pushSpeed;}
 }
 else if(!strncmp(field,"extent_",7)&&kind!=JAR_SPACE_CONTROLLER&&(JAR_ControllerTrigger(kind)||kind==JAR_LOCATION_VOLUME_CONTROLLER||kind==JAR_DIRECTIONAL_CONTROLLER)){number=field+7;value=&next.speed;}
 else if(!strcmp(field,"nodrain")&&JAR_ControllerStation(kind))next.flags^=1;
 else if(!strncmp(field,"charge_",7)&&JAR_ControllerStation(kind)){number=field+7;value=&next.count;}
 else if(JAR_ControllerDebris(kind)&&!strcmp(field,"repair")){next.active=1;next.count=next.initial;}
 else if(JAR_ControllerEmplaced(kind)&&!strcmp(field,"repair")&&next.active==0){next.active=1;next.count=next.initial;}
 else if(JAR_ControllerEmplaced(kind)&&!strncmp(field,"yaw_",4)){number=field+4;value=&next.speed;}
 else if(kind==JAR_EWEB_CONTROLLER&&!strncmp(field,"ammo_",5)){number=field+5;value=&next.pushSpeed;}
 else if(JAR_ControllerDebris(kind)&&!strcmp(field,"playeruse"))next.flags^=128;
 else if(JAR_ControllerDebris(kind)&&!strcmp(field,"usemode"))next.flags^=64;
 else if(JAR_ControllerDebris(kind)&&!strcmp(field,"explosion"))next.flags^=2048;
 else if(JAR_ControllerDebris(kind)&&!strcmp(field,"material"))next.repeats=(next.repeats+1)%17;
 else if(JAR_ControllerDebris(kind)&&!strcmp(field,"damagepolicy")){int f=next.flags&49;next.flags=(next.flags&~49)|(f==0?1:f==1?16:f==16?32:0);}
 else if(JAR_ControllerDebris(kind)&&!strncmp(field,"chunks_",7)){number=field+7;value=&next.pushSpeed;}
 else if((JAR_ControllerBreakable(kind)||kind==33)&&!strncmp(field,"radius_",7)){number=field+7;value=&next.speed;}
 else if(kind==29&&!strcmp(field,"visible")){next.active=!next.active;next.repeats=0;}
 else if(kind==29&&!strcmp(field,"usablemode")){next.flags^=8;next.repeats=0;next.waitMs=(next.flags&8)?1000:0;if(next.flags&8)next.active=1;}
 else if(kind==29&&!strcmp(field,"playeruse"))next.flags^=64;
 else if(kind==29&&!strcmp(field,"npconly"))next.flags^=32;
 else if(kind==29&&!strcmp(field,"blockcheck"))next.flags^=16;
 else if(kind==29&&!strcmp(field,"rearm")){next.repeats=0;next.count=next.initial;}
 else if(kind==JAR_DOOR_CAMERA_CONTROLLER&&!strncmp(field,"camera_travel_",14)){number=field+14;value=&next.damage;}
 else if(kind==JAR_DOOR_CAMERA_CONTROLLER&&!strncmp(field,"camera_speed_",13)){number=field+13;value=&next.speed;}
 else if(kind==JAR_WELDER_CONTROLLER&&!strncmp(field,"welder_reach_",13)){number=field+13;value=&next.speed;}
 else if(kind==JAR_WELDER_CONTROLLER&&!strncmp(field,"welder_radius_",14)){number=field+14;value=&next.pushSpeed;}
 else if(kind==JAR_WELDER_CONTROLLER&&!strncmp(field,"welder_interval_",16)){number=field+16;value=&next.waitMs;}
 else if(kind==JAR_CRYSTAL_CRATE_CONTROLLER&&!strncmp(field,"crystal_hit_damage_",19)){number=field+19;value=&next.repeats;}
 else if(kind==JAR_CRYSTAL_CRATE_CONTROLLER&&!strncmp(field,"crystal_hit_radius_",19)){number=field+19;value=&next.pushSpeed;}
 else if(kind==JAR_CRYSTAL_CRATE_CONTROLLER&&!strncmp(field,"crystal_blast_radius_",21)){number=field+21;value=&next.speed;}
 else if(kind==JAR_CRYSTAL_CRATE_CONTROLLER&&!strcmp(field,"crystal_solid"))next.flags^=JAR_MP_CRYSTAL_CRATE_SHOT_ONLY;
 else if(kind==JAR_VISIBLE_CONTROLLER&&!strncmp(field,"visible_range_",14)){number=field+14;value=&next.speed;}
 else if(kind==JAR_VISIBLE_CONTROLLER&&!strncmp(field,"visible_cone_",13)){number=field+13;value=&next.damage;}
 else if(kind==JAR_VISIBLE_CONTROLLER&&!strcmp(field,"visible_walls"))next.flags^=JAR_VISIBLE_IGNORE_WALLS;
 else if(kind==JAR_VISIBLE_CONTROLLER&&!strcmp(field,"visible_sight"))next.flags^=JAR_VISIBLE_FORCE_SIGHT;
 else if(kind==JAR_DISTANCE_CONTROLLER&&!strncmp(field,"distance_range_",15)){number=field+15;value=&next.speed;}
 else if(JAR_ControllerMovement(kind)&&!strncmp(field,"movement_value_",15)){number=field+15;value=&next.speed;}
 else if(kind==JAR_DIRECTIONAL_CONTROLLER&&!strncmp(field,"directional_wait_",17)){number=field+17;value=&next.waitMs;}
 else if(kind==JAR_DIRECTIONAL_CONTROLLER&&!strncmp(field,"directional_uses_",17)){number=field+17;value=&next.initial;}
 else if(kind==JAR_DIRECTIONAL_CONTROLLER&&!strcmp(field,"directional_players"))next.flags^=JAR_DIRECTIONAL_PLAYER_ONLY;
 else if(kind==JAR_SEARCHLIGHT_CONTROLLER&&!strcmp(field,"searchlight_power"))next.active=!next.active;
 else if(kind==JAR_SEARCHLIGHT_CONTROLLER&&!strcmp(field,"searchlight_track"))next.flags^=JAR_SEARCHLIGHT_TRACK;
 else if(JAR_ControllerPortal(kind)&&!strcmp(field,"portal_power"))next.active=!next.active;
 else if(JAR_ControllerPortal(kind)&&!strcmp(field,"portal_mode"))next.flags^=1;
 else if(kind==JAR_PORTAL_SURFACE_CONTROLLER&&!strncmp(field,"portal_width_",13)){number=field+13;value=&next.speed;}
 else if(kind==JAR_PORTAL_SURFACE_CONTROLLER&&!strncmp(field,"portal_height_",14)){number=field+14;value=&next.pushSpeed;}
 else if(kind==JAR_SEARCHLIGHT_CONTROLLER&&!strncmp(field,"searchlight_range_",18)){number=field+18;value=&next.speed;}
 else if(kind==JAR_SEARCHLIGHT_CONTROLLER&&!strncmp(field,"searchlight_angle_",18)){number=field+18;value=&next.damage;}
 else if(kind==JAR_SEARCHLIGHT_CONTROLLER&&!strncmp(field,"searchlight_light_",18)){number=field+18;value=&next.pushSpeed;}
 else if(kind==JAR_SEARCHLIGHT_CONTROLLER&&!strncmp(field,"searchlight_color_",18)){
  const char* rgb=field+18;
  if(strlen(rgb)!=6||strspn(rgb,"0123456789abcdefABCDEF")!=6){Tell(p,"Enter a six-digit RGB color.");return qtrue;}
  next.flags=(next.flags&JAR_SEARCHLIGHT_TRACK)|(int)strtoul(rgb,NULL,16);
 }
 else if(kind==JAR_DISTANCE_CONTROLLER&&!strcmp(field,"distance_subjects"))next.flags=next.flags%3+1;
 else if(kind==JAR_WALL_CONTROLLER&&!strcmp(field,"wall_visible"))next.active=!next.active;
 else if(kind==JAR_BUTTON_CONTROLLER&&!strncmp(field,"button_wait_",12)){number=field+12;value=&next.waitMs;}
 else if(JAR_ControllerSlide(kind)&&!strncmp(field,"slide_travel_",13)){number=field+13;value=&next.damage;}
 else if(JAR_ControllerSlide(kind)&&!strncmp(field,"slide_speed_",12)){number=field+12;value=&next.speed;}
 else if(JAR_ControllerSlide(kind)&&!strncmp(field,"slide_wait_",11)){number=field+11;value=&next.waitMs;}
 else if(kind==JAR_SLIDING_DOOR_CONTROLLER&&!strcmp(field,"slide_direction")){
  if(next.pushSpeed){Tell(p,"Close the door before changing its direction.");return qtrue;}
  next.flags=(next.flags+1)%JAR_SLIDE_DIRECTIONS;
 }
 else if(!strcmp(field,"enabled"))next.inactive=!next.inactive;
 else if(!strcmp(field,"reset")&&(kind==4||JAR_ControllerStation(kind)))next.count=next.initial;
 else if(!strncmp(field,"damage_",7)&&(kind==13||kind==21||kind==JAR_WELDER_CONTROLLER||JAR_ControllerBreakable(kind))){number=field+7;value=&next.damage;}
 else if(!strncmp(field,"speed_",6)&&kind==6){number=field+6;value=&next.speed;}
 else if(!strncmp(field,"delay_",6)&&(kind==JAR_ALWAYS_CONTROLLER||JAR_ControllerShooter(kind)||JAR_ControllerDebris(kind)||kind==3||kind==15||kind==19||kind==22||(kind==29&&(next.flags&8))||JAR_ControllerStation(kind)||JAR_ControllerMedia(kind))){number=field+6;value=&next.waitMs;}
 else if(!strncmp(field,"random_",7)&&(JAR_ControllerShooter(kind)||JAR_ControllerDebris(kind)||kind==3||kind==15||kind==19||JAR_ControllerMedia(kind))){number=field+7;value=&next.randomMs;}
 else if(!strncmp(field,"points_",7)&&kind==17){number=field+7;value=&next.count;}
 else if(!strncmp(field,"count_",6)&&(JAR_ControllerDebris(kind)||((JAR_ControllerTerminalBreakable(kind)||JAR_ControllerEmplaced(kind)||kind==JAR_CRYSTAL_CRATE_CONTROLLER)&&next.active==1)||kind==4||kind==29||JAR_ControllerStation(kind))){number=field+6;value=&next.initial;}
 else if(!strncmp(field,"repeats_",8)&&kind==4){number=field+8;value=&next.repeats;}
 else if(!strncmp(field,"flags_",6)&&((kind>=1&&kind<=3)||kind==6||kind==14||kind==18)){number=field+6;value=&next.flags;}
 else {Tell(p,"That setting does not apply to this controller.");return qtrue;}
 if(number){
  if(!JAR_ParseInt(number,value)){Tell(p,"Enter a valid whole number.");return qtrue;}
  if(value==&next.initial){next.count=next.initial;if(JAR_ControllerBreakable(kind))next.active=1;}
  if(kind==JAR_DIRECTIONAL_CONTROLLER&&(value==&next.initial||value==&next.waitMs)&&next.repeats){next.repeats=0;next.count=next.initial;}
  if(kind==JAR_DOOR_CAMERA_CONTROLLER&&value==&next.damage&&next.pushSpeed>next.damage)next.pushSpeed=next.damage;
  if(kind==JAR_PENDULUM_CONTROLLER&&value==&next.waitMs&&next.waitMs>0)
   next.pushSpeed=(int)((long long)before.pushSpeed*next.waitMs/before.waitMs);
 }
 if(kind==JAR_MUSIC_CONTROLLER&&next.inactive)next.active=0;
 if(kind==JAR_ALWAYS_CONTROLLER&&next.inactive)next.active=0;
 if(JAR_ControllerShooter(kind)&&(next.inactive||!(next.flags&JAR_SHOOTER_REPEAT)))next.active=0;
 if(kind==9&&!next.waitMs&&next.flags!=2)next.active=0;
 if(!JAR_ControllerStateValid(kind,&next)){
  Tell(p,"This value is outside what the selected entity can use.");
  return qtrue;
 }
 if(kind==JAR_MUSIC_CONTROLLER&&!MusicValidate(next.text)){Tell(p,"A music track is missing on the server.");return qtrue;}
 if(JAR_ControllerMedia(kind)&&!MediaValidate(kind,next.text)){Tell(p,"Sound/effect file is missing or its network resource reserve is reached.");return qtrue;}
 if(!memcmp(&before,&next,sizeof(next))){SelectionEcho(p);return qtrue;}
 if(!ControllerStateApply(n,&next)){Tell(p,"The requested state is obstructed or unavailable; properties unchanged.");return qtrue;}
 if((kind==13||kind==15)&&!before.active&&next.active){ControllerCaptureActivator(&g_entities[n],p);}
 jarPropertyUndo_t*u=&players[p->s.number].propertyUndo;memset(u,0,sizeof(*u));
 u->entity=n;u->field=JAR_PROPERTY_CONTROLLER;u->identity=ContextIdentity(n);u->revision=++objects[n].propertyRevision;u->controllerBefore=before;u->controllerAfter=next;
 SelectionChanged(n);Tell(p,"Controller properties applied.");return qtrue;
}
static qboolean ContextScalar(gentity_t*p,const char*arg){
 int n=players[p->s.number].selected,field=0,required=0;double value=0;const char*number=NULL;
 jarPropertyUndo_t*u=&players[p->s.number].propertyUndo;
 if(ContextController(p,arg))return qtrue;
 if(!strcmp(arg,"property_undo")){
  if(!ContextUndoReady(p,n)){Tell(p,"Revert requires an unchanged property on the same entity.");SelectionEcho(p);return qtrue;}
  if(u->field==JAR_PROPERTY_CONTROLLER){if(!ControllerStateApply(n,&u->controllerBefore)){Tell(p,"Previous state is obstructed or unavailable; You can retry Revert property.");return qtrue;}if((ControllerKind(n)==13||ControllerKind(n)==15)&&u->controllerBefore.active&&!u->controllerAfter.active){ControllerCaptureActivator(&g_entities[n],p);}++objects[n].propertyRevision;memset(u,0,sizeof(*u));SelectionChanged(n);Tell(p,"Previous controller properties restored.");return qtrue;}
  if(u->field==JAR_PROPERTY_DISTANCE_LINK){
   if(!DistanceLinkUndo(n,u)){Tell(p,"The previous output would create an activation cycle.");return qtrue;}
   memset(u,0,sizeof(*u));SelectionChanged(n);Tell(p,"Previous distance link restored.");return qtrue;
  }
  if(u->field==JAR_PROPERTY_TRANSFORM){if(!ContextTransformApply(n,u->oldOrigin,u->oldAngles)){Tell(p,"Previous placement is obstructed or no longer editable; You can retry Revert property.");SelectionEcho(p);return qtrue;}memset(u,0,sizeof(*u));SelectionChanged(n);Tell(p,"Previous properties restored.");return qtrue;}
  int caps=ContextCaps(p,n),cap=u->field==JAR_PROPERTY_MASS?JAR_CONTEXT_MASS:u->field==JAR_PROPERTY_GRAVITY?JAR_CONTEXT_GRAVITY:u->field==JAR_PROPERTY_HEALTH?JAR_CONTEXT_HEALTH:u->field==JAR_PROPERTY_QUANTITY?JAR_CONTEXT_QUANTITY:u->field==JAR_PROPERTY_ARMOR?JAR_CONTEXT_ARMOR:JAR_CONTEXT_MOVEMENT;
  if(!(caps&cap)){Tell(p,"Property is no longer editable.");SelectionEcho(p);return qtrue;}
  ContextApply(n,u->field,u->before);memset(u,0,sizeof(*u));SelectionChanged(n);Tell(p,"Previous properties restored.");return qtrue;
 }
 if(!strncmp(arg,"transform_",10)){
  vec3_t origin,angles;if(!(ContextCaps(p,n)&JAR_CONTEXT_TRANSFORM)||!ContextTransformParse(arg+10,origin,angles)){Tell(p,"Placement requires finite angles and coordinates inside the map. Release the Physgun first and freeze actors.");SelectionEcho(p);return qtrue;}
  gentity_t*e=&g_entities[n];if(ContextNear(e->r.currentOrigin,origin)&&ContextNear(e->r.currentAngles,angles)){SelectionEcho(p);return qtrue;}
  jarPropertyUndo_t next={0};next.entity=n;next.field=JAR_PROPERTY_TRANSFORM;next.identity=ContextIdentity(n);next.poseRevision=objects[n].poseRevision;next.frozen=objects[n].frozen;
  VectorCopy(e->r.currentOrigin,next.oldOrigin);VectorCopy(e->r.currentAngles,next.oldAngles);VectorCopy(origin,next.newOrigin);VectorCopy(angles,next.newAngles);
  if(!ContextTransformApply(n,origin,angles)){Tell(p,"Placement intersects the world, an entity or an occupied seat; nothing changed.");SelectionEcho(p);return qtrue;}
  next.revision=objects[n].propertyRevision;*u=next;SelectionChanged(n);return qtrue;
 }
 if(!strncmp(arg,"mass_",5)){field=JAR_PROPERTY_MASS;required=JAR_CONTEXT_MASS;number=arg+5;}
 else if(!strncmp(arg,"health_",7)){field=JAR_PROPERTY_HEALTH;required=JAR_CONTEXT_HEALTH;number=arg+7;}
 else if(!strncmp(arg,"armor_",6)){field=JAR_PROPERTY_ARMOR;required=JAR_CONTEXT_ARMOR;number=arg+6;}
 else if(!strncmp(arg,"quantity_",9)){field=JAR_PROPERTY_QUANTITY;required=JAR_CONTEXT_QUANTITY;number=arg+9;}
 else if(!strncmp(arg,"actor_gravity_",14)){field=JAR_PROPERTY_ACTOR_GRAVITY;required=JAR_CONTEXT_MOVEMENT;number=arg+14;}
 else if(!strncmp(arg,"actor_friction_",15)){field=JAR_PROPERTY_ACTOR_FRICTION;required=JAR_CONTEXT_MOVEMENT;number=arg+15;}
 else if(!strcmp(arg,"actor_map_gravity")){field=JAR_PROPERTY_ACTOR_GRAVITY;required=JAR_CONTEXT_MOVEMENT;value=-1;}
 else if(!strcmp(arg,"actor_map_friction")){field=JAR_PROPERTY_ACTOR_FRICTION;required=JAR_CONTEXT_MOVEMENT;value=-1;}
 else if(!strcmp(arg,"light")||!strcmp(arg,"normal")||!strcmp(arg,"heavy")){field=JAR_PROPERTY_MASS;required=JAR_CONTEXT_MASS;value=!strcmp(arg,"light")?20:!strcmp(arg,"normal")?80:200;}
 else if(!strcmp(arg,"gravity")){field=JAR_PROPERTY_GRAVITY;required=JAR_CONTEXT_GRAVITY;value=(float)!objects[n].noGravity;}
 else return qfalse;
 if(!(ContextCaps(p,n)&required)){Tell(p,"That property does not apply to this selection.");SelectionEcho(p);return qtrue;}
 if(number){
  if(field==JAR_PROPERTY_MASS){
   float mass;
   if(!JAR_ParseFloat(number,&mass)||!JAR_ContextMassValid(mass)){
    Tell(p,"Mass needs a positive, representable value.");return qtrue;
   }
   value=mass;
  }else{
   int whole;
   if(!JAR_ParseInt(number,&whole)){Tell(p,"Enter a valid whole number.");return qtrue;}
   if((field==JAR_PROPERTY_HEALTH&&(whole<=0||whole>SHRT_MAX))||
      (field==JAR_PROPERTY_ARMOR&&(whole<0||whole>SHRT_MAX))||
      (field==JAR_PROPERTY_QUANTITY&&whole<0)||
      ((field==JAR_PROPERTY_ACTOR_GRAVITY||field==JAR_PROPERTY_ACTOR_FRICTION)&&
       (whole<0||whole>JAR_MP_ACTOR_MOVEMENT_MAX))){
    Tell(p,"This value is outside what the selected entity can use.");return qtrue;
   }
   value=whole;
  }
 }
 const double before=ContextValue(n,field);if(before==value){SelectionEcho(p);return qtrue;}
 if((field==JAR_PROPERTY_HEALTH&&(before<=0||before>SHRT_MAX))||
    (field==JAR_PROPERTY_ARMOR&&(before<0||before>SHRT_MAX)))memset(u,0,sizeof(*u));
 else *u=(jarPropertyUndo_t){n,field,ContextIdentity(n),objects[n].propertyRevision+1,before,value};
 ContextApply(n,field,value);SelectionChanged(n);return qtrue;
}
static qboolean ContextRequest(gentity_t*p){
 char text[32];unsigned token;int n=players[p->s.number].selected;trap->Argv(2,text,sizeof(text));
 if(!JAR_MP_ParseId(text,&token)||!token||!ContextEditable(p,n)||ContextIdentity(n)!=token){Tell(p,"Selection changed; select the entity again.");SelectionEcho(p);return qfalse;}
 if(!ContextCaps(p,n)){Tell(p,"Wait for the selected entity to initialize.");SelectionEcho(p);return qfalse;}
 return qtrue;
}
static void ContextFrame(void){
 for(int c=0;c<MAX_CLIENTS;++c){jarPlayer_t*p=&players[c];if(!p->contextOpen||p->selected==ENTITYNUM_NONE||level.time<p->contextEchoAt)continue;
  p->contextEchoAt=level.time+500;
  if(g_entities[c].inuse&&g_entities[c].client&&g_entities[c].client->pers.connected==CON_CONNECTED)SelectionEcho(&g_entities[c]);
 }
}
