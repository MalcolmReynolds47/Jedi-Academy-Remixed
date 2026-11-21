// Exercise actual MP server command/lifecycle code with deterministic engine imports.
#include "game/g_jar_sandbox.c"
#include "game/bg_jar_vehicle_bounds.inc"
#include <assert.h>
#include <stdarg.h>

gentity_t g_entities[MAX_GENTITIES];
level_locals_t level;
vmCvar_t g_gravity,g_weaponDisable,g_duelWeaponDisable,g_knockback;
weaponData_t weaponData[WP_NUM_WEAPONS];
static int registeredItems,ammoGrants;
static int actionKillCalls,actionKillVictim,actionKillMod;
static int hurtCalls,hurtVictim,hurtAttacker,hurtAmount;
static int spaceDamage,spaceSounds;
static int laserDamageCalls,laserVictim,laserAttacker,laserAmount,laserDestroySource;
void G_Damage(gentity_t*t,gentity_t*i,gentity_t*a,vec3_t direction,vec3_t point,int damage,int flags,int mod){
 if(mod==MOD_TRIGGER_HURT){assert(flags==DAMAGE_NO_PROTECTION&&i==a&&damage>=0&&damage<=1000);++hurtCalls;hurtVictim=t->s.number;hurtAttacker=a->s.number;hurtAmount=damage;return;}
 if(mod==MOD_SUICIDE&&flags==DAMAGE_NO_ARMOR){assert(i==a&&damage>=20&&damage<=70);spaceDamage+=damage;return;}
 if(flags==DAMAGE_NO_PROTECTION){assert(!i&&!a&&damage==100000);++actionKillCalls;actionKillVictim=t->s.number;actionKillMod=mod;return;}
 assert(flags==DAMAGE_NO_KNOCKBACK&&mod==MOD_TARGET_LASER);++laserDamageCalls;laserVictim=t->s.number;laserAttacker=a->s.number;laserAmount=damage;
 if(laserDestroySource){int slot=i->s.number;G_FreeEntity(i);gentity_t*reuse=G_Spawn();assert(reuse->s.number==slot);reuse->classname="replacement";}
}
int G_ItemDisabled(gitem_t*);
gitem_t bg_itemlist[6];
int bg_numItems=1;
static gameImport_t imports;
gameImport_t *trap=&imports;
static gclient_t clients[3];
static char argument[160],contextTokenArgument[32];
static int snapshotCalls;static char actorSpawnName[64];static qboolean actorSpawnVehicle;
static int aimed=ENTITYNUM_WORLD,blocked,itemDisabled,traceCalls,blockAt,missingModel;
static gclient_t copyClients[MAX_GENTITIES];static gNPC_t copyNpcs[MAX_GENTITIES];
static Vehicle_t copyVehicles[MAX_GENTITIES];
static vehicleInfo_t copyVehicleInfo;
static void Register(vmCvar_t*c,const char*n,const char*v,uint32_t f){if(!c){assert(!strcmp(n,"jar_mpVersion")&&(f&CVAR_SERVERINFO)&&(f&CVAR_ROM));return;}c->integer=atoi(v);c->value=(float)atof(v);Q_strncpyz(c->string,v,sizeof(c->string));}
int G_ItemDisabled(gitem_t*i){return itemDisabled;}
void RegisterItem(gitem_t*i){++registeredItems;}
void SaveRegisteredItems(void){}
void Add_Ammo(gentity_t*p,int ammo,int count){++ammoGrants;p->client->ps.ammo[ammo]+=count;}
static void Set(const char*n,const char*v){}
static void Update(vmCvar_t*c){}
static void Argv(int n,char*b,int size){Q_strncpyz(b,n==2?contextTokenArgument:argument,size);}
static void Config(int n,char*b,int size){*b=0;}
static void Link(sharedEntity_t*e){e->r.linked=qtrue;}
static void Unlink(sharedEntity_t*e){e->r.linked=qfalse;}
static int FixtureEntitiesInBox(const vec3_t low,const vec3_t high,int* list,int capacity){
 int count=0;
 for(int n=0;n<level.num_entities&&count<capacity;++n){
  const gentity_t* e=&g_entities[n];if(!e->inuse||!e->r.linked)continue;
  qboolean overlaps=qtrue;
  for(int k=0;k<3;++k)overlaps&=low[k]<=e->r.currentOrigin[k]+e->r.maxs[k]&&high[k]>=e->r.currentOrigin[k]+e->r.mins[k];
  if(overlaps)list[count++]=n;
 }
 return count;
}
static int teleportEvents,teleportKillBoxes;
gentity_t*G_TempEntity(vec3_t point,int event){static gentity_t temp[2];assert(event==EV_PLAYER_TELEPORT_IN||event==EV_PLAYER_TELEPORT_OUT);return &temp[(teleportEvents++)&1];}
void G_KillBox(gentity_t*e){assert(!blocked);++teleportKillBoxes;}
static int controllerPrints;static char weaponEcho[64];static int weaponEchoClient;
static int undoSounds;
static char spawnStatsEcho[MAX_CLIENTS][64];
static void Send(int n,const char*s){if(!strcmp(s,"jarundo"))++undoSounds;if(!strncmp(s,"cp ",3))++controllerPrints;if(!strncmp(s,"jarequip ",9)){Q_strncpyz(weaponEcho,s,sizeof(weaponEcho));weaponEchoClient=n;}if(n>=0&&n<MAX_CLIENTS&&!strncmp(s,"jarstats ",9))Q_strncpyz(spawnStatsEcho[n],s,sizeof(spawnStatsEcho[n]));}
static byte defaultModel[sizeof(md3Header_t)+sizeof(md3Frame_t)];static int defaultReadAt;
static int Open(const char*p,fileHandle_t*f,fsMode_t m){
 *f=missingModel?0:1;defaultReadAt=0;memset(defaultModel,0,sizeof(defaultModel));
 md3Header_t*h=(md3Header_t*)defaultModel;md3Frame_t*b=(md3Frame_t*)(h+1);h->ident=MD3_IDENT;h->version=MD3_VERSION;h->numFrames=1;h->ofsFrames=sizeof(*h);h->ofsEnd=sizeof(defaultModel);
 VectorSet(b->bounds[0],-12,-8,-4);VectorSet(b->bounds[1],20,8,12);return missingModel?-1:sizeof(defaultModel);
}
static int Read(void*out,int bytes,fileHandle_t f){assert(f==1&&bytes>=0&&defaultReadAt+bytes<=sizeof(defaultModel));memcpy(out,defaultModel+defaultReadAt,bytes);defaultReadAt+=bytes;return bytes;}
static void Close(fileHandle_t f){}
static void Trace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 memset(t,0,sizeof(*t));t->fraction=mins?1:.5f;t->entityNum=aimed;t->plane.normal[2]=1;VectorCopy(end,t->endpos);t->startsolid=blocked||(++traceCalls==blockAt);
}
static void PanelTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 Trace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 const vec3_t obstacle={-12.6f,0,4};
 qboolean inside=qtrue;
 for(int a=0;a<3;++a)if(obstacle[a]<start[a]+mins[a]||obstacle[a]>start[a]+maxs[a])inside=qfalse;
 if(inside)t->startsolid=qtrue;
}
static vec3_t physgunFixtureHit;
static void PhysgunFixtureTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 Trace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(!mins&&aimed>=0&&aimed<ENTITYNUM_WORLD){VectorCopy(physgunFixtureHit,t->endpos);t->fraction=Distance(start,t->endpos)/Distance(start,end);}
}
static float physgunWorldDistance;
static qboolean PhysgunModelPick(const char* model,int frame,const vec3_t origin,const vec3_t angles,
 const vec3_t start,const vec3_t end,float* fraction,vec3_t normal){
 if(origin[0]<0)return qfalse;
 assert(model&&frame==0&&angles[YAW]==45);
 // The visible surface sits inside the oriented collision bounds.
 const float hit=(194-start[0])/(end[0]-start[0]);
 if(start[2]<100&&hit<*fraction){*fraction=hit;VectorSet(normal,-1,0,0);}
 return qtrue;
}
static void PhysgunWorldTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 Trace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(flags&JAR_TRACE_PHYS_GUN){
  assert(!mins&&mask==JAR_PHYSGUN_MP_AIM_MASK);
  vec3_t delta;VectorSubtract(end,start,delta);
  t->entityNum=ENTITYNUM_WORLD;t->fraction=physgunWorldDistance/VectorLength(delta);
  VectorMA(start,t->fraction,delta,t->endpos);
 }
}
gentity_t*G_Spawn(void){for(int i=MAX_CLIENTS;i<ENTITYNUM_WORLD;++i)if(!g_entities[i].inuse){gentity_t*e=&g_entities[i];memset(e,0,sizeof(*e));e->s.number=i;e->inuse=qtrue;if(level.num_entities<=i)level.num_entities=i+1;return e;}assert(0);return NULL;}
static int breakAllocationFail,rackAllocationFailureCountdown;
gitem_t*BG_FindItem(const char*classname){
 for(int i=1;i<bg_numItems;++i)if(bg_itemlist[i].classname&&!strcmp(bg_itemlist[i].classname,classname))return &bg_itemlist[i];
 return NULL;
}
gentity_t*G_TrySpawn(void){
 if(breakAllocationFail||(rackAllocationFailureCountdown&&!--rackAllocationFailureCountdown))return NULL;
 return G_Spawn();
}
gentity_t*JAR_MP_TryLaunchItem(gitem_t*item,const vec3_t origin,const vec3_t velocity){
 assert(item&&VectorCompare(velocity,vec3_origin));
 gentity_t*drop=G_TrySpawn();
 if(!drop)return NULL;
 drop->item=item;drop->classname=item->classname;drop->s.eType=ET_ITEM;drop->flags=FL_DROPPED_ITEM;
 G_SetOrigin(drop,(float*)origin);trap->LinkEntity((sharedEntity_t*)drop);
 return drop;
}
void G_FreeEntity(gentity_t*e){JAR_MP_Forget(e->s.number);e->inuse=qfalse;}
void G_SetOrigin(gentity_t*e,vec3_t p){VectorCopy(p,e->r.currentOrigin);VectorCopy(p,e->s.pos.trBase);}
qboolean G_SpawnVector(const char*key,const char*fallback,float*out){
 char *text;return G_SpawnString(key,fallback,&text)&&sscanf(text,"%f %f %f",out,out+1,out+2)==3;
}
void G_SpawnItem(gentity_t*e,gitem_t*i){assert(e->classname==i->classname&&level.numSpawnVars==0&&level.spawning);if(itemDisabled==1)return;if(itemDisabled==2){G_FreeEntity(e);return;}e->item=i;e->s.eType=ET_ITEM;e->s.modelindex=i-bg_itemlist;e->touch=Touch_Item;e->r.contents=CONTENTS_TRIGGER;}
gentity_t*NPC_SpawnTypeAt(gentity_t*p,char*n,char*t,qboolean vehicle,const vec3_t point,const vec3_t angles){
 Q_strncpyz(actorSpawnName,n,sizeof(actorSpawnName));actorSpawnVehicle=vehicle;
 gentity_t*e=G_Spawn();int slot=e->s.number;e->client=&copyClients[slot];e->NPC=&copyNpcs[slot];memset(e->client,0,sizeof(*e->client));memset(e->NPC,0,sizeof(*e->NPC));
 e->playerState=&e->client->ps;e->NPC->tempGoal=G_Spawn();e->NPC->tempGoal->parent=e;e->NPC->tempGoal->classname="NPC_goal";
 e->s.eType=ET_NPC;e->s.eFlags=EF_NODRAW;e->client->ps.clientNum=slot;G_SetOrigin(e,(float*)point);VectorCopy(angles,e->r.currentAngles);VectorSet(e->r.mins,-16,-16,-24);VectorSet(e->r.maxs,16,16,40);
 if(vehicle){e->client->NPC_class=CLASS_VEHICLE;e->m_pVehicle=&copyVehicles[slot];memset(e->m_pVehicle,0,sizeof(*e->m_pVehicle));e->m_pVehicle->m_pVehicleInfo=&copyVehicleInfo;e->m_pVehicle->m_pParentEntity=(bgEntity_t*)e;e->m_pVehicle->m_vOrientation=e->client->ps.vehOrientation;}
 return e;
}
enum { FIXTURE_GAS_JET_EFFECT=17,FIXTURE_GAS_FLAME_EFFECT=18 };
enum { FIXTURE_BOMB_MODEL=21,FIXTURE_BEACON_MODEL=22,FIXTURE_ANIMATED_LOOP=23,FIXTURE_BOMB_DISARM=24,FIXTURE_CARGO_MEDPACK_MODEL=25 };
enum {FIXTURE_DOOR_CAMERA_BASE=26,FIXTURE_DOOR_CAMERA_SHAFT=27,FIXTURE_DOOR_CAMERA_HEAD=28,FIXTURE_DOOR_CAMERA_SERVO=29};
enum {FIXTURE_WELDER_MODEL=30,FIXTURE_WELDER_LOOP=31,FIXTURE_WELDER_EFFECT=32};
enum {FIXTURE_CRYSTAL_PART=34,FIXTURE_CRYSTAL_BLAST=1,FIXTURE_CRYSTAL_DEBRIS=36};
static int cargoModelRegistrations,cargoEffectRegistrations,crateModelRegistrations,tankModelRegistrations;
static int gasJetRegistrations,gasFlameRegistrations,stockExplosionSounds;
int G_ModelIndex(const char*n){
 if(n&&!strcmp(n,"models/map_objects/imperial/barrels.md3"))++cargoModelRegistrations;
 if(n&&!strcmp(n,"models/map_objects/nar_shaddar/crate_xplode.md3"))++crateModelRegistrations;
 if(n&&!strcmp(n,"models/map_objects/imp_mine/tank.md3"))++tankModelRegistrations;
 if(n&&!strcmp(n,"models/map_objects/factory/bomb_new_deact.md3"))return FIXTURE_BOMB_MODEL;
 if(n&&!strcmp(n,"models/map_objects/wedge/beacon.md3"))return FIXTURE_BEACON_MODEL;
 if(n&&!strcmp(n,JAR_MP_CARGO_DROP_MODEL))return FIXTURE_CARGO_MEDPACK_MODEL;
 if(n&&!strcmp(n,JAR_MP_DOOR_CAMERA_BASE_MODEL))return FIXTURE_DOOR_CAMERA_BASE;
 if(n&&!strcmp(n,JAR_MP_DOOR_CAMERA_SHAFT_MODEL))return FIXTURE_DOOR_CAMERA_SHAFT;
 if(n&&!strcmp(n,JAR_MP_DOOR_CAMERA_HEAD_MODEL))return FIXTURE_DOOR_CAMERA_HEAD;
 if(n&&!strcmp(n,JAR_MP_WELDER_MODEL))return FIXTURE_WELDER_MODEL;
 if(n&&!strcmp(n,JAR_MP_CRYSTAL_CRATE_PART_MODEL))return FIXTURE_CRYSTAL_PART;
 return 1;
}
int G_SoundIndex(const char*n){
 if(n&&!strcmp(n,"sound/weapons/explosions/cargoexplode.wav"))++stockExplosionSounds;
 if(n&&!strcmp(n,"sound/interface/ammocon_run.wav"))return FIXTURE_ANIMATED_LOOP;
 if(n&&!strcmp(n,"sound/weapons/overchargeend.wav"))return FIXTURE_BOMB_DISARM;
 if(n&&!strcmp(n,JAR_MP_DOOR_CAMERA_SERVO))return FIXTURE_DOOR_CAMERA_SERVO;
 if(n&&!strcmp(n,JAR_MP_WELDER_LOOP))return FIXTURE_WELDER_LOOP;
 return 1;
}
int G_EffectIndex(const char*n){
 if(n&&!strcmp(n,"thermal/explosion")){++cargoEffectRegistrations;return FIXTURE_CRYSTAL_BLAST;}
 if(n&&!strcmp(n,"env/mini_gasjet")){++gasJetRegistrations;return FIXTURE_GAS_JET_EFFECT;}
 if(n&&!strcmp(n,"env/mini_flamejet")){++gasFlameRegistrations;return FIXTURE_GAS_FLAME_EFFECT;}
 if(n&&!strcmp(n,JAR_MP_WELDER_EFFECT))return FIXTURE_WELDER_EFFECT;
 if(n&&!strcmp(n,JAR_MP_CRYSTAL_CRATE_DEBRIS_EFFECT))return FIXTURE_CRYSTAL_DEBRIS;
 return 1;
}
static int mediaMissing,mediaFull,mediaEvents,mediaEventEntity,mediaEventType,mediaEventParm;
void G_AddEvent(gentity_t*e,int event,int parm){++mediaEvents;mediaEventEntity=e->s.number;mediaEventType=event;mediaEventParm=parm;}
int BMS_START=1,BMS_MID=2,BMS_END=3;
int G_SoundSetIndex(const char*name){assert(0);return 0;}
static int breakRadiusCalls, sectionFireRadiusCalls;
static int welderRadiusCalls,welderRecycleSource;
static vec3_t welderRadiusOrigin;
static float welderDamage,welderRadius;
static void CrystalFixtureRadius(const vec3_t,gentity_t*,float,float);
static void CargoBarrelRadius(const vec3_t origin,gentity_t*source,int damage,int radius);
static void StockExplosiveRadius(const vec3_t origin,gentity_t*source,int damage,int radius);
qboolean G_RadiusDamage(vec3_t origin,gentity_t*attacker,float damage,float radius,gentity_t*ignore,gentity_t*missile,int mod){
 if(JAR_MP_SectionFireSource(attacker)){
  assert(attacker==ignore&&attacker==missile&&mod==MOD_UNKNOWN&&damage==5&&radius==16);
  assert(VectorCompare(origin,attacker->r.currentOrigin));
  ++sectionFireRadiusCalls;
 }else if(ControllerKind(attacker->s.number)==JAR_WELDER_CONTROLLER){
  assert(attacker==ignore&&!missile&&mod==MOD_UNKNOWN&&JAR_MP_BreakableRadiusSource(attacker,NULL));
  ++welderRadiusCalls;VectorCopy(origin,welderRadiusOrigin);welderDamage=damage;welderRadius=radius;
  if(welderRecycleSource){
   const int slot=attacker->s.number;
   G_FreeEntity(attacker);
   gentity_t*replacement=G_Spawn();
   assert(replacement->s.number==slot);replacement->health=73;
  }
 }else if(ControllerKind(attacker->s.number)==JAR_CRYSTAL_CRATE_CONTROLLER){
  assert(attacker==ignore&&!missile&&mod==MOD_UNKNOWN&&JAR_MP_BreakableRadiusSource(attacker,NULL));
  CrystalFixtureRadius(origin,attacker,damage,radius);
 }else{
  assert(attacker==ignore&&!missile&&JAR_ControllerBreakable(ControllerKind(attacker->s.number))&&mod==MOD_UNKNOWN&&damage>0&&damage<=1000&&radius>0&&radius<=2048);
  if(ControllerKind(attacker->s.number)==JAR_CARGO_BARREL_CONTROLLER)CargoBarrelRadius(origin,attacker,damage,radius);
  if(JAR_ControllerStockExplosive(ControllerKind(attacker->s.number)))StockExplosiveRadius(origin,attacker,damage,radius);
  ++breakRadiusCalls;
 }
 return qfalse;
}
void BG_EvaluateTrajectory(const trajectory_t*tr,int time,vec3_t result){
 if(tr->trType==TR_SINE){assert(tr->trDuration==SECTION_BOBBER_PERIOD);VectorMA(tr->trBase,sinf((time-tr->trTime)*2*M_PI/tr->trDuration),tr->trDelta,result);return;}
 assert(tr->trType==TR_STATIONARY);VectorCopy(tr->trBase,result);
}
void G_Sound(gentity_t*e,int c,int s){}
void G_SetAnim(gentity_t*e,usercmd_t*u,int parts,int anim,int flags,int blend){e->client->ps.legsAnim=e->client->ps.torsoAnim=anim;}
void SetClientViewAngle(gentity_t*e,vec3_t angles){
 for(int a=0;a<3;++a)e->client->ps.delta_angles[a]=ANGLE2SHORT(angles[a])-e->client->pers.cmd.angles[a];
 VectorCopy(angles,e->s.angles);
 VectorCopy(angles,e->client->ps.viewangles);
}
void BG_PlayerStateToEntityState(playerState_t*p,entityState_t*s,qboolean snap){++snapshotCalls;s->number=p->clientNum;s->eType=ET_PLAYER;VectorCopy(p->origin,s->pos.trBase);VectorCopy(p->viewangles,s->apos.trBase);s->legsAnim=p->legsAnim;s->torsoAnim=p->torsoAnim;}
char*QDECL va(const char*fmt,...){static char b[4096];va_list args;va_start(args,fmt);vsnprintf(b,sizeof(b),fmt,args);va_end(args);return b;}
int QDECL Com_sprintf(char*dst,int size,const char*fmt,...){va_list args;va_start(args,fmt);int length=vsnprintf(dst,size,fmt,args);va_end(args);return length;}
static void Error(int code,const char*fmt,...){abort();}
static void Print(const char*fmt,...){}
void (*Com_Error)(int,const char*,...)=Error;
void (*Com_Printf)(const char*,...)=Print;
static void Command(int c,const char*cmd,const char*arg){level.time+=110;Q_strncpyz(argument,arg,sizeof(argument));int selected=players[c].selected;snprintf(contextTokenArgument,sizeof(contextTokenArgument),"%u",selected>=0&&selected<ENTITYNUM_WORLD?ContextIdentity(selected):0);assert(JAR_MP_Command(&g_entities[c],cmd));}
static int Last(int client){unsigned newest=0;int result=-1;for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(g_entities[i].inuse&&objects[i].owner==client+1&&objects[i].order>newest){newest=objects[i].order;result=i;}return result;}
#include "mp_native_controllers.inc"
#include "mp_actor.inc"
#include "mp_duplicate.inc"
#include "mp_vehicle.inc"
#include "mp_world.inc"
#include "mp_forces.inc"
#include "mp_autosave.inc"
#include "mp_rigid.inc"
#include "mp_attachments.inc"
#include "mp_poses.inc"
#include "mp_skeletal.inc"
#include "mp_joints.inc"
#include "mp_aliases.inc"
#include "mp_context.inc"
#include "mp_vehicle_seats.inc"
#include "mp_vehicle_motion.inc"
#include "mp_transform.inc"
#include "mp_capacity_sandbox.inc"
#include "mp_controllers.inc"
#include "mp_tool_parity.inc"
#include "mp_controller_properties.inc"
#include "mp_transport_targets.inc"
#include "mp_vehicle_teleport.inc"
#include "mp_vehicle_push.inc"
#include "mp_media.inc"
#include "mp_controller_switches.inc"
#include "mp_lights.inc"
#include "mp_music.inc"
#include "mp_section.inc"
#include "game/g_jar_native_mover_state.inc"
#include "mp_laser.inc"
#include "mp_feedback.inc"
#include "mp_logic_controllers.inc"
#include "mp_action_controllers.inc"
#include "mp_trigger_controllers.inc"
#include "mp_trigger_transport.inc"
#include "mp_trigger_props.inc"
#include "mp_stations.inc"
#include "mp_usable.inc"
#include "mp_breakable.inc"
#include "mp_cargo_barrel.inc"
#include "mp_stock_explosives.inc"
#include "mp_small_cargo.inc"
#include "mp_gun_rack.inc"
#include "mp_ammo_rack.inc"
#include "mp_door_camera.inc"
#include "mp_welder.inc"
#include "mp_searchlight.inc"
#include "mp_portal.inc"
#include "mp_crystal_crate.inc"
#include "mp_shooters.inc"
#include "mp_emplaced.inc"
#include "mp_animated.inc"
#include "mp_push_publication.inc"
#include "mp_rope_paths.inc"
#include "mp_location.inc"
int main(void){
 imports.Print=Print;imports.Cvar_Register=Register;imports.Cvar_Set=Set;imports.Cvar_Update=Update;imports.Argv=Argv;imports.GetConfigstring=Config;imports.LinkEntity=Link;imports.UnlinkEntity=Unlink;imports.SendServerCommand=Send;imports.FS_Open=Open;imports.FS_Read=Read;imports.FS_Close=Close;imports.Trace=Trace;
 imports.EntitiesInBox=FixtureEntitiesInBox;
 level.num_entities=MAX_CLIENTS;level.time=1000;g_gravity.value=800;JAR_MP_Init();assert(sharedEditing.integer==1&&!cleanupDisconnect.integer&&worldEditing.integer==1);sharedEditing.integer=worldEditing.integer=0;cleanupDisconnect.integer=1;
 for(int c=0;c<2;++c){g_entities[c].s.number=c;g_entities[c].inuse=qtrue;g_entities[c].health=100;g_entities[c].client=&clients[c];clients[c].pers.connected=CON_CONNECTED;clients[c].sess.sessionTeam=TEAM_FREE;clients[c].ps.viewheight=24;}
 {
  gentity_t* wall=G_Spawn();
  for(int i=0;i<JAR_MP_CATALOG_COUNT;++i)
   if(jarMPCatalog[i].id==JAR_WALL_CATALOG_ID)objects[wall->s.number].catalog=i+1;
  objects[wall->s.number].owner=1;
  wall->classname="func_wall";
  wall->r.contents=CONTENTS_SOLID;
  PhysicsHull(wall,vec3_origin,wall->r.mins,wall->r.maxs);
  const vec3_t contact={20,0,4},turn={0,90,0};
  imports.Trace=PanelTrace;
  assert(!ToolRotate(wall->s.number,turn,contact));
  assert(VectorCompare(wall->r.currentOrigin,vec3_origin)&&VectorCompare(wall->r.currentAngles,vec3_origin));
  imports.Trace=Trace;
  assert(ToolRotate(wall->s.number,turn,contact));
  assert(Distance(wall->r.currentOrigin,((vec3_t){20,-20,0}))<.01f);
  G_FreeEntity(wall);
 }
 // Native baton item route: identity, spawn-key isolation, ownership, Undo and
 // both native disabled-item outcomes (early return or freeing the entity).
 char key[32];unsigned baton=0;for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(!strcmp(jarMPCatalog[i].name,"weapon_stun_baton")){baton=jarMPCatalog[i].id;assert(jarMPCatalog[i].kind==4);break;}assert(baton);
 bg_numItems=2;bg_itemlist[1].classname="weapon_stun_baton";bg_itemlist[1].giType=IT_WEAPON;bg_itemlist[1].giTag=WP_STUN_BATON;
 snprintf(key,sizeof(key),"%u",baton);clients[0].ps.weapon=WP_BLASTER;clients[0].ps.weaponTime=1000;
 Command(0,"jar_equip",key);assert(clients[0].ps.weapon==WP_STUN_BATON&&clients[0].ps.weaponTime==0&&(clients[0].ps.stats[STAT_WEAPONS]&(1<<WP_STUN_BATON))&&Last(0)==-1&&registeredItems==1);
 assert(!strcmp(weaponEcho,"jarequip 1")&&weaponEchoClient==0&&clients[1].ps.weapon==WP_NONE);
 for(int refusal=0;refusal<7;++refusal){
  clients[0].ps.weapon=WP_BLASTER;g_weaponDisable.integer=refusal==0?1<<WP_STUN_BATON:0;itemDisabled=refusal==1;
  clients[0].ps.saberInFlight=refusal==2;clients[0].ps.emplacedIndex=refusal==3;clients[0].ps.forceHandExtend=refusal==4?HANDEXTEND_CHOKE:HANDEXTEND_NONE;
  sandbox.integer=refusal==5?0:1;g_entities[0].health=refusal==6?0:100;
  Command(0,"jar_equip",key);assert(clients[0].ps.weapon==WP_BLASTER&&registeredItems==1);
 }
 g_weaponDisable.integer=itemDisabled=clients[0].ps.saberInFlight=clients[0].ps.emplacedIndex=0;clients[0].ps.forceHandExtend=HANDEXTEND_NONE;sandbox.integer=1;g_entities[0].health=100;
 Command(0,"jar_equip","123;quit");assert(registeredItems==1);
 level.numSpawnVars=1;level.spawning=qfalse;Command(0,"jar_spawn",key);int batonEntity=Last(0);assert(batonEntity>=MAX_CLIENTS&&g_entities[batonEntity].item==&bg_itemlist[1]&&level.numSpawnVars==1&&!level.spawning);Command(0,"jar_undo","");assert(!g_entities[batonEntity].inuse);
 for(itemDisabled=1;itemDisabled<=2;++itemDisabled){Command(0,"jar_spawn",key);assert(Last(0)==-1&&level.numSpawnVars==1&&!level.spawning);for(int i=MAX_CLIENTS;i<level.num_entities;++i)assert(!g_entities[i].inuse);}itemDisabled=0;level.numSpawnVars=0;
 unsigned id=0;for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(jarMPCatalog[i].kind==1){id=jarMPCatalog[i].id;break;}assert(id);snprintf(key,sizeof(key),"%u",id);
 Command(0,"jar_spawn",key);int a=Last(0);assert(a>=MAX_CLIENTS);
 // A second request in the same server tick cannot spawn another object.
 assert(JAR_MP_Command(&g_entities[0],"jar_spawn"));assert(Last(0)==a);
 Command(1,"jar_spawn",key);int b=Last(1);assert(b!=a);
 aimed=a;players[1].tool=1;Tool(&g_entities[1],qfalse);assert(g_entities[a].inuse);
 sharedEditing.integer=1;assert(Edit(&g_entities[1],a));sharedEditing.integer=0;
 players[0].tool=0;Tool(&g_entities[0],qfalse);assert(objects[a].frozen);
 Tool(&g_entities[0],qtrue);assert(!objects[a].frozen);
 vec3_t pickupOrigin;VectorCopy(g_entities[a].r.currentOrigin,pickupOrigin);
 VectorAdd(pickupOrigin,((vec3_t){-12,5,6}),physgunFixtureHit);
 VectorSet(clients[0].ps.origin,physgunFixtureHit[0]-37,physgunFixtureHit[1],physgunFixtureHit[2]-24);
 imports.Trace=PhysgunFixtureTrace;
 clients[0].ps.weapon=WP_PHYSGUN;clients[0].pers.cmd.buttons=BUTTON_ATTACK;JAR_MP_Fire(&g_entities[0],qfalse);assert(players[0].held==a&&players[0].distance==37);
 imports.Trace=Trace;
 level.previousTime=level.time;level.time+=25;JAR_MP_Frame();assert(players[0].beam!=ENTITYNUM_NONE);
 assert(Distance(g_entities[a].r.currentOrigin,pickupOrigin)<.01f);
 assert(Distance(g_entities[players[0].beam].s.origin2,physgunFixtureHit)<.01f);
 assert(clients[0].ps.stats[STAT_JAR_PHYSGUN_HOLD]==a+1);
 assert(JAR_PhysgunTurnActive(&clients[0].ps,BUTTON_ATTACK|BUTTON_USE,0));
 assert(!JAR_PhysgunTurnActive(&clients[0].ps,BUTTON_ATTACK|BUTTON_USE,KEYCATCH_CGAME));
 vec3_t view,propAngles;VectorCopy(clients[0].ps.viewangles,view);VectorCopy(g_entities[a].r.currentAngles,propAngles);
 float yawRemainder=0,pitchRemainder=0;
 for(int n=0;n<2;++n){
  clients[0].pers.cmd.buttons=BUTTON_ATTACK|BUTTON_USE|BUTTON_JAR_PHYSGUN_TURN;
  clients[0].pers.cmd.angles[ROLL]=JAR_PackPhysgunTurn(6.25f,-3.5f,&yawRemainder,&pitchRemainder);
  JAR_MP_ReceivePhysgunInput(&g_entities[0],&clients[0].pers.cmd,qtrue);
  assert(clients[0].pers.cmd.buttons==BUTTON_ATTACK);
  assert(clients[0].pers.cmd.angles[ROLL]==((ANGLE2SHORT(view[ROLL])-clients[0].ps.delta_angles[ROLL])&65535));
 }
 assert(players[0].turnInput&&players[0].turnYaw==12.5f&&players[0].turnPitch==-7);
 JAR_MP_ReceivePhysgunInput(&g_entities[0],&clients[0].pers.cmd,qtrue);
 assert(!players[0].turnInput&&players[0].turnYaw==12.5f);
 level.previousTime=level.time;level.time+=25;JAR_MP_Frame();
 level.previousTime=level.time;level.time+=25;JAR_MP_Frame();
 assert(VectorCompare(view,clients[0].ps.viewangles));
 assert(fabsf(AngleSubtract(g_entities[a].r.currentAngles[YAW],propAngles[YAW])-12.5f)<.01f);
 assert(fabsf(AngleSubtract(g_entities[a].r.currentAngles[PITCH],propAngles[PITCH])+7)<.01f);
 assert(Distance(g_entities[players[0].beam].s.origin2,physgunFixtureHit)<.01f);
 assert(Distance(g_entities[a].r.currentOrigin,pickupOrigin)>1);
 JAR_MP_ReceivePhysgunInput(&g_entities[0],&clients[0].pers.cmd,qtrue);
 assert(!players[0].turnInput&&!players[0].turnYaw&&!players[0].turnPitch);
 clients[0].ps.viewangles[YAW]=30;clients[0].ps.viewangles[PITCH]=-30;
 for(int i=0;i<6;++i){level.previousTime=level.time;level.time+=25;JAR_MP_Frame();}
 assert(fabsf(AngleSubtract(g_entities[a].r.currentAngles[YAW],propAngles[YAW])-42.5f)<.01f);
 assert(fabsf(AngleSubtract(g_entities[a].r.currentAngles[PITCH],propAngles[PITCH])+7)<.01f);
 assert(fabsf(g_entities[players[0].beam].s.origin2[2]-physgunFixtureHit[2]-18.5f)<.01f);
 JAR_MP_Fire(&g_entities[0],qtrue);assert(objects[a].frozen&&players[0].held==ENTITYNUM_NONE);JAR_MP_Fire(&g_entities[0],qfalse);assert(players[0].held==ENTITYNUM_NONE);
 assert(!clients[0].ps.stats[STAT_JAR_PHYSGUN_HOLD]);
 // An actual aim ray must hit the rotated model face and keep its local
 // point. The world trace alone never returns the broad-phase prop box.
 {
  const vec3_t origin={200,0,96},angle={0,45,0};
  Move(&g_entities[a],origin);assert(PhysicsSetAngles(&g_entities[a],angle));
  Move(&g_entities[b],((vec3_t){-100,0,96}));
  VectorSet(clients[0].ps.origin,0,0,78);VectorClear(clients[0].ps.viewangles);
  imports.Trace=PhysgunWorldTrace;physgunWorldDistance=100;
  trace_t hit;PhysgunAim(&g_entities[0],&hit);assert(hit.entityNum==ENTITYNUM_WORLD);
  physgunWorldDistance=512;
  PhysgunAim(&g_entities[0],&hit);assert(hit.entityNum==a&&fabsf(hit.endpos[0]-(200-8*sqrtf(2)))<.01f);
  players[0].freezeLatch=0;JAR_MP_Fire(&g_entities[0],qfalse);
  assert(players[0].held==a&&fabsf(players[0].grip.localPoint[0]+8)<.01f&&
         fabsf(players[0].grip.localPoint[1]-8)<.01f&&fabsf(players[0].grip.localPoint[2]-6)<.01f);
  Drop(0,qtrue);
  imports.JAR_PickPropModel=PhysgunModelPick;
  PhysgunAim(&g_entities[0],&hit);assert(hit.entityNum==ENTITYNUM_WORLD);
  clients[0].ps.origin[2]=74;
  PhysgunAim(&g_entities[0],&hit);assert(hit.entityNum==a&&fabsf(hit.endpos[0]-194)<.01f);
  players[0].freezeLatch=0;JAR_MP_Fire(&g_entities[0],qfalse);
  assert(players[0].held==a&&fabsf(players[0].distance-194)<.01f);
  vec3_t contact;
  JAR_GripPoint(&players[0].grip,g_entities[a].r.currentOrigin,g_entities[a].r.currentAngles,contact);
  assert(Distance(contact,hit.endpos)<.01f);
  Drop(0,qtrue);players[0].freezeLatch=1;imports.Trace=Trace;imports.JAR_PickPropModel=NULL;
 }
 VectorClear(clients[0].ps.viewangles);VectorClear(clients[0].ps.origin);
 clients[0].pers.cmd.buttons=BUTTON_ALT_ATTACK;level.previousTime=level.time;level.time+=25;JAR_MP_Frame();assert(players[0].freezeLatch);JAR_MP_Fire(&g_entities[0],qtrue);assert(objects[a].frozen);
 clients[0].pers.cmd.buttons=0;level.previousTime=level.time;level.time+=25;JAR_MP_Frame();assert(players[0].held==ENTITYNUM_NONE&&!players[0].freezeLatch);
 aimed=a;JAR_MP_Fire(&g_entities[0],qtrue);assert(!objects[a].frozen&&players[0].held==ENTITYNUM_NONE);players[0].freezeLatch=0;
 players[0].distance=144;Command(0,"jar_distance","out");assert(players[0].distance==168);Command(0,"jar_distance","in");assert(players[0].distance==144);
 players[0].distance=2040;Command(0,"jar_distance","out");assert(players[0].distance==2048);players[0].distance=48;Command(0,"jar_distance","in");assert(players[0].distance==48);
 // A rope references endpoint generations through delete callbacks, not reused slots.
 players[0].tool=4;aimed=a;Tool(&g_entities[0],qfalse);aimed=ENTITYNUM_WORLD;Tool(&g_entities[0],qfalse);int rope=Last(0);assert(rope!=a&&g_entities[rope].s.generic1==251);
 G_FreeEntity(&g_entities[a]);assert(!g_entities[rope].inuse);assert(players[0].first==ENTITYNUM_NONE);
 Command(0,"jar_spawn",key);a=Last(0);assert(a>=MAX_CLIENTS);objects[a].velocity[2]=0;float z=g_entities[a].r.currentOrigin[2];level.previousTime=level.time;level.time+=25;JAR_MP_Frame();assert(g_entities[a].r.currentOrigin[2]<z);
 objects[a].frozen=1;z=g_entities[a].r.currentOrigin[2];level.previousTime=level.time;level.time+=25;JAR_MP_Frame();assert(g_entities[a].r.currentOrigin[2]==z);
 blocked=1;Command(0,"jar_spawn",key);assert(Last(0)==a);blocked=0;
 aimed=a;Command(1,"jar_select","");assert(players[1].selected==ENTITYNUM_NONE);
 Command(0,"jar_select","");assert(players[0].selected==a);Command(0,"jar_property","heavy");assert(objects[a].mass==200);
 Command(0,"jar_property","gravity");assert(objects[a].noGravity);Command(0,"jar_property","release");assert(!objects[a].frozen);
 VectorClear(objects[a].velocity);z=g_entities[a].r.currentOrigin[2];level.previousTime=level.time;level.time+=25;JAR_MP_Frame();assert(g_entities[a].r.currentOrigin[2]==z);
 Command(0,"jar_spawn","123;quit");assert(Last(0)==a);
 JAR_MP_Disconnect(0);assert(!g_entities[a].inuse&&g_entities[b].inuse);assert(players[0].held==ENTITYNUM_NONE&&players[0].beam==ENTITYNUM_NONE&&players[0].selected==ENTITYNUM_NONE);
 Command(1,"jar_undo","");assert(!g_entities[b].inuse);
 Command(1,"jar_spawn",key);b=Last(1);cleanupDisconnect.integer=0;JAR_MP_Disconnect(1);assert(g_entities[b].inuse&&objects[b].owner==MAX_CLIENTS+1);assert(!Edit(&g_entities[1],b));
 ActorCases();DuplicateCases();VehicleCases();ForceCases();WorldCases();AutoSaveCases();RigidCases();FallenObjectCases();AttachmentCases();PoseCases();LargePlacementCases();SkeletalCases();JointCases();AliasCases();ContextCases();VehicleSeatCases();VehicleMotionCases();TransformCases();RopePathCases();
 MovementContextCases();PlayerContextCases();NPCThrowCases();
 ConstraintLoadCases();
 RopeLengthCases();
 CapacitySandboxCases();ToolParityCases();ControllerCases();StaticBrushCases();ControllerPropertyCases();ContextWorldCases();TransportTargetCases();VehicleTeleportCases();VehiclePushCases();MediaCases();ControllerSwitchCases();LightCases();MusicCases();SectionCases();LaserCases();FeedbackCases();LogicControllerCases();HoldableCatalogCases();ActionControllerCases();
 TriggerControllerCases();SpaceControllerCases();DirectionalCases();PortableMapModelCases();LocationCases();AlwaysControllerCases();VisibleControllerCases();DistanceControllerCases();WallControllerCases();SlidingDoorCases();LiftCases();VehicleLiftCases();BobbingCases();AngularMoverCases();PlayerStartCases();ReflectionCases();
 SearchlightCases();
 PortalCases();
 TriggerTransportCases();TriggerVehicleTransportCases();TriggerPropCases();StationCases();UsableCases();BreakableCases();CargoBarrelCases();StockExplosiveCases();SmallCargoCases();GunRackCases();AmmoRackCases();DoorCameraCases();WelderCases();CrystalCrateCases();ShooterCases();EmplacedCases();PushPublicationCases();AnimatedCases();
 puts("MP production commands: ownership, rate limits, collision rejection, hold/release, rope deletion, physics, disconnect and fixed capacity persistence passed.");return 0;
}
