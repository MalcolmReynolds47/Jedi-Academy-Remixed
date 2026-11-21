// Server-owned immutable clipboards. Entity slots are translated only at capture
// and paste; source deletion/reuse never changes a saved assembly. GPL-2.0-only.
#include "../../shared/game/jar_world_anchor.h"
typedef struct {
 int stationUseRemaining;int itemConsumed;int output[2],controllerCount,controllerInitial,controllerRemaining,controllerInactive;
 jarControllerState_t controller;
 byte cityDamage[JAR_MP_CITY_DAMAGE_BYTES];
 unsigned id;int poseFrame;int frozen,thruster,noGravity,health,count;
 float mass;vec3_t offset,angles,mins,maxs;int thrustMounted;float thrustForce;vec3_t thrustAnchor,thrustAxis;
 int vehicleMotion;vec3_t vehicleDrift;float vehicleSpeed;
 int actorGravity,actorFriction;
} jarCopyNode_t;
typedef struct {int first,second;vec3_t a,b;float length;int kind;vec4_t ra,rb;jarRopePath_t path;vec3_t copyWorldNormal;} jarCopyRope_t;
typedef struct {int count,ropes;jarCopyNode_t node[MAX_GENTITIES];jarCopyRope_t rope[JAR_MP_ATTACHMENT_LIMIT];vec3_t copyOrigin;} jarCopy_t;
static qboolean CopyHiddenBeacon(const jarCopyNode_t*n){return n->id==JAR_MP_BEACON_ID&&n->controller.active==JAR_MP_BEACON_HIDDEN;}
static int CopyPlacementMask(const jarCopyNode_t*n){
 return n->id==JAR_MP_CRYSTAL_CRATE_ID&&(n->controller.flags&JAR_MP_CRYSTAL_CRATE_SHOT_ONLY)?MASK_SOLID:MASK_PLAYERSOLID;
}
// The existing two controller integer slots store local node indices for
// distance inputs. They are independent of the two activation outputs.
static qboolean CopyDistanceValid(const jarCopy_t*c,int index){
 const jarCopyNode_t*n=&c->node[index];
 const int reference=n->controllerCount,subject=n->controllerInitial;
 if(n->controller.count||n->controller.initial||
    reference<JAR_DISTANCE_LINK_MISSING||reference>c->count||reference==index+1||
    subject<JAR_DISTANCE_LINK_MISSING||subject>c->count||subject==index+1)return qfalse;
 if(subject>0){
  int catalog;
  const jarMPEntry_t*entry=Entry(c->node[subject-1].id,&catalog);
  if(!entry||(entry->kind!=2&&entry->kind!=3))return qfalse;
 }
 return qtrue;
}
static void CopyDistanceCapture(jarCopyNode_t*node,int source,const int*indices){
 if(node->id!=JAR_MP_DISTANCE_ID)return;
 int*saved[]={&node->controllerCount,&node->controllerInitial};
 for(int i=0;i<2;++i){
  const int link=objects[source].distanceInput[i];
  if(!link)continue;
  *saved[i]=link>0&&DistanceEndpoint(source,link-1)&&indices[link-1]>=0?
            indices[link-1]+1:JAR_DISTANCE_LINK_MISSING;
 }
}
static void CopyDistanceRestore(const jarCopyNode_t*node,int destination,const int*mapped){
 if(node->id!=JAR_MP_DISTANCE_ID)return;
 const int saved[]={node->controllerCount,node->controllerInitial};
 for(int i=0;i<2;++i)objects[destination].distanceInput[i]=saved[i]>0?mapped[saved[i]-1]+1:saved[i];
}
static qboolean CopyControllersValid(const jarCopy_t*c){
 int incoming[MAX_GENTITIES]={0},queue[MAX_GENTITIES],head=0,tail=0;
 byte musicRanks[MAX_GENTITIES]={0};int musicRanksUsed=0,highestRank=0;
 for(int i=0;i<c->count;++i){const jarCopyNode_t*n=&c->node[i];int index;const jarMPEntry_t*entry=Entry(n->id,&index);if(!entry)return qfalse;int kind=entry->kind==5?ControllerClass(entry->name):0;
  if(kind==JAR_SECTION_CONTROLLER&&(!n->frozen||n->thruster||n->thrustMounted||n->noGravity||n->poseFrame))return qfalse;
  if(kind==JAR_DOOR_CAMERA_CONTROLLER&&(n->poseFrame||n->mass!=JAR_MP_DOOR_CAMERA_MASS))return qfalse;
  if(kind==JAR_WELDER_CONTROLLER&&(n->poseFrame))return qfalse;
  if(kind==JAR_CRYSTAL_CRATE_CONTROLLER&&(n->poseFrame||n->controllerRemaining||n->stationUseRemaining||n->output[1]))return qfalse;
  if(!SectionCityDamageValid(n->id,n->cityDamage))return qfalse;
  if(n->actorGravity<0||n->actorGravity>JAR_MP_ACTOR_MOVEMENT_MAX+1||
     n->actorFriction<0||n->actorFriction>JAR_MP_ACTOR_MOVEMENT_MAX+1||
     (entry->kind!=2&&(n->actorGravity||n->actorFriction)))return qfalse;
  if(!JAR_ControllerStateValid(kind,&n->controller)||n->controller.inactive!=n->controllerInactive)return qfalse;
  if(kind==JAR_DISTANCE_CONTROLLER){if(!CopyDistanceValid(c,i))return qfalse;}
  else if(n->controller.count!=n->controllerCount||n->controller.initial!=n->controllerInitial)return qfalse;
  if(n->itemConsumed<0||n->itemConsumed>1||(entry->kind!=4&&n->itemConsumed))return qfalse;
  if(n->controllerInactive<0||n->controllerInactive>1||n->controllerRemaining<0)return qfalse;
  if(!JAR_ControllerStation(kind)&&!JAR_ControllerBreakable(kind)&&!JAR_ControllerEmplaced(kind)&&kind!=29&&kind!=4&&kind!=17&&kind!=19&&kind!=20&&kind!=JAR_DISTANCE_CONTROLLER&&kind!=JAR_DIRECTIONAL_CONTROLLER&&(n->controllerCount||n->controllerInitial))return qfalse;if(kind==17&&n->controllerInitial)return qfalse;if(kind==4&&(!n->controllerInitial||n->controllerCount>n->controllerInitial))return qfalse;
  if(kind==JAR_MUSIC_CONTROLLER){
   const int rank=n->controllerRemaining;
   if(rank>c->count||!!rank!=n->controller.active||(rank&&musicRanks[rank-1]))return qfalse;
   if(rank){musicRanks[rank-1]=1;++musicRanksUsed;if(rank>highestRank)highestRank=rank;}
  }
  const jarMPSectionKind_t section=SectionKind(entry);
  if(section==JAR_MP_SECTION_TASPIR){if(n->controllerRemaining>SECTION_HURT_MAX_REMAINING)return qfalse;}
  else if(section==JAR_MP_SECTION_BESPIN){if(n->controllerRemaining>=SECTION_BOBBER_PERIOD)return qfalse;}
  else if(kind==JAR_SEARCHLIGHT_CONTROLLER){if(n->controllerRemaining>JAR_SEARCHLIGHT_REARM_MS)return qfalse;}
  else if(kind==JAR_DIRECTIONAL_CONTROLLER){if(n->controllerRemaining>(n->controller.waitMs>0?n->controller.waitMs:1))return qfalse;}
  else if(kind!=JAR_MUSIC_CONTROLLER&&kind!=JAR_WELDER_CONTROLLER&&kind!=JAR_ALWAYS_CONTROLLER&&!JAR_ControllerSlide(kind)&&kind!=JAR_BUTTON_CONTROLLER&&!JAR_ControllerStation(kind)&&!JAR_ControllerBreakable(kind)&&!JAR_ControllerEmplaced(kind)&&!JAR_ControllerShooter(kind)&&!JAR_ControllerAnimatedTag(kind)&&kind!=29&&kind!=3&&kind!=15&&kind!=19&&kind!=20&&kind!=21&&kind!=22&&n->controllerRemaining)return qfalse;
  if(!kind&&n->controllerInactive)return qfalse;
  if(JAR_ControllerBreakable(kind)){
   const qboolean pending=n->controller.active==2;
   if(!!n->controllerRemaining!=(pending||kind==JAR_GAS_TANK_CONTROLLER))return qfalse;
   if(JAR_ControllerTerminalBreakable(kind)){
    const int limit=kind==JAR_CARGO_BARREL_CONTROLLER?500:pending?JAR_STOCK_EXPLOSIVE_FUSE:JAR_GAS_TANK_IDLE_MAX;
    if(n->controllerRemaining>limit||n->stationUseRemaining||n->output[1])return qfalse;
   }
  }
  if(JAR_ControllerEmplaced(kind)&&(n->poseFrame||n->controllerRemaining>(kind==JAR_EWEB_CONTROLLER?50:3000)||!!n->controllerRemaining!=(n->controller.active==2)))return qfalse;
  if(JAR_ControllerShooter(kind)&&(n->output[1]||!!n->controllerRemaining!=n->controller.active||
     n->controllerRemaining>n->controller.waitMs+n->controller.randomMs))return qfalse;
  const int animated=JAR_ControllerAnimatedTag(kind);
  if(animated&&(!JAR_MP_AnimatedFrameValid(animated,n->controller.active,n->poseFrame)||
     !!n->controllerRemaining!=JAR_MP_AnimatedFrameRuns(animated,n->controller.active)||
     n->controllerRemaining>JAR_MP_ANIM_TICK_MS||n->output[1]||(kind==JAR_BEACON_CONTROLLER&&n->output[0])))return qfalse;
  if(kind==29&&((!!n->controllerRemaining!=(n->controller.repeats==1||n->controller.repeats==2))||(n->controller.repeats==1&&n->controllerRemaining>FRAMETIME)))return qfalse;
  if(kind==15&&(!!n->controllerRemaining!=n->controller.active))return qfalse;
  if(kind==JAR_ALWAYS_CONTROLLER&&((!!n->controllerRemaining!=n->controller.active)||n->output[1]))return qfalse;
  if(kind==JAR_VISIBLE_CONTROLLER&&n->output[1])return qfalse;
  if((kind==JAR_BOBBING_CONTROLLER||JAR_ControllerAngular(kind))&&(n->poseFrame||n->controller.repeats))return qfalse;
  if(JAR_ControllerSlide(kind)&&(n->poseFrame||n->controller.repeats||
     (n->controllerRemaining&&(!n->controller.active||n->controllerRemaining>n->controller.waitMs||
      n->controller.pushSpeed!=n->controller.damage*JAR_SLIDE_MILLI))))return qfalse;
  if(kind==JAR_BUTTON_CONTROLLER){
   const jarButtonState_t button={n->controller.waitMs,n->controller.active,n->poseFrame,n->controllerRemaining};
   if(!JAR_MP_ButtonValid(button))return qfalse;
  }
  if((kind==19||kind==20)&&(!!n->controllerRemaining!=!!n->controllerInitial))return qfalse;
  if(kind==22&&(n->controllerRemaining>n->controller.waitMs&&n->controllerRemaining))return qfalse;
  if(kind==21&&n->controllerRemaining>1000)return qfalse;
  if(n->stationUseRemaining<0||(!JAR_ControllerStation(kind)&&!JAR_ControllerBreakable(kind)&&n->stationUseRemaining))return qfalse;
  for(int o=0;o<2;++o){int target=n->output[o]-1;if(target< -1||target>=c->count||target==i)return qfalse;
   if(target>=0){const jarMPEntry_t*dest=Entry(c->node[target].id,&index);if(!dest)return qfalse;int destKind=dest->kind==5?ControllerClass(dest->name):0;
    if((kind==15||kind==JAR_STATIC_BRUSH_CONTROLLER)&&o)return qfalse;
    if(JAR_ControllerPortal(kind)){
     if(o||(kind==JAR_PORTAL_SURFACE_CONTROLLER&&destKind!=JAR_PORTAL_CAMERA_CONTROLLER))return qfalse;
    }
    else if(JAR_ControllerRack(destKind)&&kind!=13&&!JAR_ControllerShooter(kind)&&!(kind==JAR_SEARCHLIGHT_CONTROLLER&&!o))return qfalse;
    else if(JAR_ControllerEmplaced(destKind)&&!JAR_ControllerSwitch(kind)&&!JAR_ControllerShooter(kind)&&!(kind==JAR_SEARCHLIGHT_CONTROLLER&&!o))return qfalse;
    else if(kind==JAR_SEARCHLIGHT_CONTROLLER&&!o){ /* Aim does not invoke the target. */ }
    else if(kind==13||JAR_ControllerShooter(kind)){if(o)return qfalse;}
    else if(kind==6||kind==7||kind==22||kind==23||(kind==10&&!o)){if(o||!JAR_ControllerPosition(destKind))return qfalse;}
    else if(kind==16){if(dest->kind!=4)return qfalse;}
    else if(JAR_ControllerSwitch(kind)){if(!destKind)return qfalse;}
    else if(((kind<2||kind>4)&&kind!=10&&kind!=14&&kind!=15&&kind!=19&&kind!=20&&kind!=29&&kind!=JAR_BOMB_CONTROLLER&&kind!=JAR_ALWAYS_CONTROLLER&&kind!=JAR_VISIBLE_CONTROLLER&&kind!=JAR_DISTANCE_CONTROLLER&&kind!=JAR_DIRECTIONAL_CONTROLLER&&kind!=JAR_SEARCHLIGHT_CONTROLLER&&kind!=JAR_STATIC_BRUSH_CONTROLLER&&!JAR_ControllerSlide(kind)&&kind!=JAR_BUTTON_CONTROLLER&&!JAR_ControllerBreakable(kind))||!destKind||destKind==5||destKind==8||destKind==JAR_SECTION_CONTROLLER||destKind==JAR_SMALL_CARGO_CONTROLLER)return qfalse;++incoming[target];}
  }
 }
 for(int i=0;i<c->count;++i)if(!incoming[i])queue[tail++]=i;
 while(head<tail){const jarCopyNode_t*n=&c->node[queue[head++]];for(int o=0;o<2;++o)if(n->output[o]&&!--incoming[n->output[o]-1])queue[tail++]=n->output[o]-1;}
 return tail==c->count&&musicRanksUsed==highestRank;
}
static void CopyControllerCapture(jarCopyNode_t*n,const gentity_t*e){
 if(e->client&&e->NPC&&!ActorVehicle(e)){
  n->actorGravity=e->client->ps.stats[STAT_JAR_ACTOR_GRAVITY];
  n->actorFriction=e->client->ps.stats[STAT_JAR_ACTOR_FRICTION];
 }
 int kind=ControllerKind(e->s.number);if(!kind)return;ControllerState(e->s.number,&n->controller);
 if(kind==JAR_DIRECTIONAL_CONTROLLER){n->controllerCount=n->controller.count;n->controllerInitial=n->controller.initial;n->controllerRemaining=e->painDebounceTime>level.time?e->painDebounceTime-level.time:0;}
 if(kind==JAR_SEARCHLIGHT_CONTROLLER)n->controllerRemaining=Com_Clampi(0,JAR_SEARCHLIGHT_REARM_MS,e->painDebounceTime-level.time);
 if(kind==JAR_DISTANCE_CONTROLLER)n->controller.count=n->controller.initial=0; // Live input status is derived after restore.
 if(JAR_ControllerSlide(kind)){n->controller.repeats=0;n->controllerRemaining=e->genericValue4;}
 if(kind==JAR_BOBBING_CONTROLLER||JAR_ControllerAngular(kind))n->controller.repeats=0;
 if(kind==JAR_BUTTON_CONTROLLER)n->controllerRemaining=e->genericValue8;
 if(kind==JAR_MUSIC_CONTROLLER)n->controller.active=0; // Pasting never starts global playback.
 n->controllerInactive=(e->flags&FL_INACTIVE)?1:0;
 if(JAR_ControllerStation(kind)){n->controllerCount=e->count;n->controllerInitial=e->genericValue4;n->controllerRemaining=e->genericValue8>level.time?e->genericValue8-level.time:0;n->stationUseRemaining=e->setTime>level.time?e->setTime-level.time:0;}
 if(JAR_ControllerBreakable(kind)){
  n->controllerCount=n->controller.count;n->controllerInitial=n->controller.initial;
  if(kind!=JAR_CRYSTAL_CRATE_CONTROLLER)n->stationUseRemaining=e->painDebounceTime>level.time?e->painDebounceTime-level.time:0;
  if(n->controller.active==2||kind==JAR_GAS_TANK_CONTROLLER)n->controllerRemaining=e->nextthink>level.time?e->nextthink-level.time:1;
 }
 if(JAR_ControllerEmplaced(kind)){n->controllerCount=n->controller.count;n->controllerInitial=n->controller.initial;if(n->controller.active==2)n->controllerRemaining=e->s.time>level.time?e->s.time-level.time:1;}
 if(JAR_ControllerShooter(kind)&&n->controller.active)n->controllerRemaining=e->nextthink>level.time?e->nextthink-level.time:1;
 if(JAR_ControllerAnimatedTag(kind))n->controllerRemaining=AnimatedRemaining(e);
 if(kind==29){n->controllerCount=n->controller.count;n->controllerInitial=n->controller.initial;if(e->genericValue11==1||e->genericValue11==2)n->controllerRemaining=e->nextthink>level.time?e->nextthink-level.time:1;}
 if(kind==17)n->controllerCount=e->count;
 if(kind==19||kind==20){n->controllerInitial=n->controller.initial;if(e->nextthink>0)n->controllerRemaining=e->nextthink>level.time?e->nextthink-level.time:1;}
 if(kind==22&&e->wait>0&&JAR_NumberDeadline(e->painDebounceTime,JAR_NumberMillis(e->wait))>level.time)n->controllerRemaining=JAR_NumberDeadline(e->painDebounceTime,JAR_NumberMillis(e->wait))-level.time;
 if(kind==21&&e->timestamp>level.time)n->controllerRemaining=e->timestamp-level.time;
 if(kind==JAR_SECTION_CONTROLLER)n->controllerRemaining=SectionBobber(e->s.number)?SectionBobberPhase(e->s.number):SectionHurtRemaining(e->s.number);
 if(kind==4){n->controllerCount=e->count;n->controllerInitial=e->genericValue1;}
 if((kind==3||kind==15)&&e->nextthink>0&&e->think==(kind==15?func_timer_think:Think_Target_Delay))n->controllerRemaining=e->nextthink>level.time?e->nextthink-level.time:1;
 if(kind==JAR_ALWAYS_CONTROLLER&&n->controller.active)n->controllerRemaining=e->nextthink>level.time?e->nextthink-level.time:1;
}
static qboolean CopyControllerApply(gentity_t*e,const jarCopyNode_t*n){
 int kind=ControllerKind(e->s.number);if(!kind)return qtrue;
 jarControllerState_t state=n->controller;
 if(kind==JAR_MUSIC_CONTROLLER)state.active=0; // Publish only after all placements succeed.
 if(!ControllerStateApply(e->s.number,&state))return qfalse;if(n->controllerInactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
 if(JAR_ControllerStation(kind)){e->genericValue8=JAR_NumberDeadline(level.time,n->controllerRemaining);e->setTime=n->stationUseRemaining?JAR_NumberDeadline(level.time,n->stationUseRemaining):0;e->activator=NULL;e->fly_sound_debounce_time=0;e->s.loopSound=0;}
 if(JAR_ControllerBreakable(kind)){if(n->controllerRemaining)e->nextthink=JAR_NumberDeadline(level.time,n->controllerRemaining);e->painDebounceTime=n->stationUseRemaining?JAR_NumberDeadline(level.time,n->stationUseRemaining):0;ControllerCaptureActivator(e,NULL);}
 if(JAR_ControllerEmplaced(kind)&&n->controllerRemaining)e->s.time=JAR_NumberDeadline(level.time,n->controllerRemaining);
 if(JAR_ControllerShooter(kind)&&n->controllerRemaining)e->nextthink=JAR_NumberDeadline(level.time,n->controllerRemaining);
 if(JAR_ControllerAnimatedTag(kind))AnimatedFrame(e,n->poseFrame,n->controllerRemaining);
 if(kind==JAR_BUTTON_CONTROLLER){
  const jarButtonState_t button={state.waitMs,state.active,n->poseFrame,n->controllerRemaining};
  ButtonWrite(e,&button);
 }
 if(kind==JAR_DIRECTIONAL_CONTROLLER)e->painDebounceTime=n->controllerRemaining?JAR_NumberDeadline(level.time,n->controllerRemaining):0;
 if(kind==JAR_SEARCHLIGHT_CONTROLLER)e->painDebounceTime=JAR_NumberDeadline(level.time,n->controllerRemaining);
 if(kind==JAR_WELDER_CONTROLLER)WelderRestoreDeadline(e,n->controllerRemaining);
 if(kind==29){if(n->controllerRemaining)e->nextthink=JAR_NumberDeadline(level.time,n->controllerRemaining);e->activator=NULL;}
 if(kind==19||kind==20){e->nextthink=n->controllerRemaining?JAR_NumberDeadline(level.time,n->controllerRemaining):0;e->think=n->controllerInitial==2?trigger_cleared_fire:NULL;e->activator=e;}
 if(kind==22)e->painDebounceTime=n->controllerRemaining?JAR_NumberDeadline(level.time,n->controllerRemaining)-JAR_NumberMillis(e->wait):level.time-JAR_NumberMillis(fmaxf(0,e->wait));
 if(kind==21)e->timestamp=n->controllerRemaining?JAR_NumberDeadline(level.time,n->controllerRemaining):0;
 if(kind==JAR_SECTION_CONTROLLER){
  if(SectionBobber(e->s.number))SectionBobberRestore(e->s.number,n->controllerRemaining);
  else SectionHurtRestore(e->s.number,n->controllerRemaining);
 }
 if(kind==4){e->count=n->controllerCount;e->genericValue1=n->controllerInitial;}
 if((kind==3||kind==15)&&n->controllerRemaining){e->think=kind==15?func_timer_think:Think_Target_Delay;e->nextthink=JAR_NumberDeadline(level.time,n->controllerRemaining);e->activator=e;}
 if(kind==JAR_ALWAYS_CONTROLLER)AlwaysSchedule(e,n->controllerRemaining);
 if(JAR_ControllerSlide(kind))e->genericValue4=n->controllerRemaining;
 return qtrue;
}
static jarCopy_t copies[MAX_CLIENTS];
static void CopyReset(int client){if(client>=0&&client<MAX_CLIENTS)memset(&copies[client],0,sizeof(copies[client]));}
static qboolean CopyOccupied(gentity_t*e){return (e->client&&e->client->ps.m_iVehicleNum)||VehicleOccupied(e->m_pVehicle)||JAR_MP_NativeEmplacedOccupied(e);}
static qboolean RemovalOccupied(gentity_t*e){return CopyOccupied(e)||SectionSupportsActors(e,-1);}
static qboolean CopyBusyExcept(int owner,unsigned group,int departingClient){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  if(!g_entities[n].inuse||objects[n].owner!=owner||objects[n].order!=group)continue;
  if(CopyOccupied(&g_entities[n])||SectionSupportsActors(&g_entities[n],departingClient))return qtrue;
 }
 return qfalse;
}
static qboolean CopyBusy(int owner,unsigned group){return CopyBusyExcept(owner,group,-1);}
static qboolean CopyVector(const vec3_t v,float limit){for(int i=0;i<3;++i)if(!isfinite(v[i])||fabsf(v[i])>limit)return qfalse;return qtrue;}
static qboolean CopyRotation(const float*q){float norm=0;for(int a=0;a<4;++a){if(!isfinite(q[a]))return qfalse;norm+=q[a]*q[a];}return fabsf(norm-1)<.001f;}
static qboolean CopyLinkValid(const jarCopyRope_t*r,int count){
 if(r->first< -1||r->second< -1||r->first>=count||r->second>=count||r->first==r->second||!CopyVector(r->a,r->first<0?65535:8192)||!CopyVector(r->b,r->second<0?65535:8192)||!isfinite(r->length)||r->length<0||r->length>131072||r->kind<0||r->kind>2)return qfalse;
 if(!JAR_RopePathValid(&r->path)||(r->kind&&r->path.count))return qfalse;
 for(int i=r->path.count;i<JAR_ROPE_PATH_BENDS;++i)for(int a=0;a<3;++a)if(r->path.bend[i][a]!=0)return qfalse;
 if(!r->kind)for(int i=0;i<2;++i){const float*n=i?r->rb:r->ra;if(!CopyVector(n,1.01f)||!isfinite(n[3])||(n[3]!=0&&n[3]!=1)||(n[3]&&fabsf(VectorLengthSquared(n)-1)>.001f)||(!n[3]&&VectorLengthSquared(n)))return qfalse;}
 return !r->kind||(r->length==0&&CopyRotation(r->ra)&&CopyRotation(r->rb));
}
static qboolean CopyLinkEndpoints(const jarCopy_t *copy, const jarCopyRope_t *link) {
 for (int side = 0; side < 2; ++side) {
  const int node = side ? link->second : link->first;
  int index;
  if (node >= 0 && SectionCard(Entry(copy->node[node].id, &index))) return qfalse;
 }
 return qtrue;
}
static void CopyLinkCapture(jarCopyRope_t*r,const jarObject_t*o){VectorCopy(o->anchorA,r->a);VectorCopy(o->anchorB,r->b);r->length=o->length;r->kind=o->jointKind;r->path=o->ropePath;memcpy(r->ra,o->jointA,sizeof(r->ra));memcpy(r->rb,o->jointB,sizeof(r->rb));}
static void CopyLinkApply(jarObject_t*o,const jarCopyRope_t*r){VectorCopy(r->a,o->anchorA);VectorCopy(r->b,o->anchorB);o->length=r->length;o->jointKind=r->kind;o->ropePath=r->path;memcpy(o->jointA,r->ra,sizeof(o->jointA));memcpy(o->jointB,r->rb,sizeof(o->jointB));}
static void CopyWorldTrace(trace_t*hit,const vec3_t start,const vec3_t end){
 trap->Trace(hit,start,NULL,NULL,end,ENTITYNUM_NONE,MASK_SOLID,qfalse,0,0);
}
static qboolean CopyWorldNormal(jarCopyRope_t* link){
 const qboolean first=link->first<0;
 const float* point=first?link->a:link->b;
 const float* frame=first?link->ra:link->rb;
 return JAR_WorldAnchorSurface(point,link->kind||!frame[3]?vec3_origin:frame,CopyWorldTrace,link->copyWorldNormal);
}
static qboolean CopyPlaceWorldLink(jarCopyRope_t* link,const vec3_t offset){
 float* point=link->first<0?link->a:link->b;
 vec3_t normal;
 VectorAdd(point,offset,point);
 return CopyVector(point,65535)&&JAR_WorldAnchorSurface(point,link->copyWorldNormal,CopyWorldTrace,normal);
}
static qboolean CopyThrustValid(const jarCopyNode_t*n){
 if(n->thrustMounted<0||n->thrustMounted>1||!isfinite(n->thrustForce)||!CopyVector(n->thrustAnchor,8192)||!CopyVector(n->thrustAxis,1.01f))return qfalse;
 return !n->thrustMounted||fabsf(VectorLengthSquared(n->thrustAxis)-1)<.001f;
}
static qboolean CopyMotionValid(const jarCopyNode_t*n,int kind){
 if(!n->vehicleMotion)return VectorCompare(n->vehicleDrift,vec3_origin)&&n->vehicleSpeed==0;
 return n->vehicleMotion==1&&kind==3&&CopyVector(n->vehicleDrift,8192)&&n->vehicleDrift[2]==0&&
  isfinite(n->vehicleSpeed)&&fabsf(n->vehicleSpeed)<=131072&&
  (!n->frozen||(VectorCompare(n->vehicleDrift,vec3_origin)&&n->vehicleSpeed==0));
}
static void CopyThrustCapture(jarCopyNode_t*n,const jarObject_t*o){n->thrustMounted=o->thrustMounted;n->thrustForce=o->thrustForce;VectorCopy(o->thrustAnchor,n->thrustAnchor);VectorCopy(o->thrustAxis,n->thrustAxis);}
static void CopyThrustApply(jarObject_t*o,const jarCopyNode_t*n){o->thrustMounted=n->thrustMounted;o->thrustForce=n->thrustForce;VectorCopy(n->thrustAnchor,o->thrustAnchor);VectorCopy(n->thrustAxis,o->thrustAxis);}
static void CopyAssembly(gentity_t*p,int root){
 static jarCopy_t candidate;int slots[MAX_GENTITIES],indices[MAX_GENTITIES],external=0;
 memset(&candidate,0,sizeof(candidate));for(int n=0;n<MAX_GENTITIES;++n)indices[n]=-1;
 if(!Edit(p,root)||!objects[root].catalog){Tell(p,"Copy needs a spawned catalog object you may edit.");return;}
 VectorCopy(g_entities[root].r.currentOrigin,candidate.copyOrigin);
 slots[0]=root;indices[root]=0;candidate.count=1;
 for(int at=0;at<candidate.count;++at){
  int n=slots[at];gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];jarCopyNode_t*node=&candidate.node[at];
  if(!Edit(p,n)||o->catalog<1||o->catalog>JAR_MP_CATALOG_COUNT||CopyOccupied(e)||o->copyPending||
     (e->client&&(!e->NPC||e->health<=0||(e->s.eFlags&EF_NODRAW)))){Tell(p,"Assembly has an unavailable, occupied, initializing or protected member; clipboard unchanged.");return;}
  if((jarMPCatalog[o->catalog-1].kind==2||jarMPCatalog[o->catalog-1].kind==3)&&!e->client){Tell(p,"Native actor has not initialized; clipboard unchanged.");return;}
  node->poseFrame=PhysicsProp(e->s.number)?e->s.frame:0;node->id=jarMPCatalog[o->catalog-1].id;node->frozen=o->frozen;node->thruster=o->thruster;node->noGravity=o->noGravity;node->mass=o->mass;CopyThrustCapture(node,o);
  node->health=e->client?e->health:0;node->count=e->item?e->count:0;node->itemConsumed=o->itemConsumed;CopyControllerCapture(node,e);
  VectorSubtract(e->r.currentOrigin,g_entities[root].r.currentOrigin,node->offset);VectorCopy(e->r.currentAngles,node->angles);VectorCopy(e->r.mins,node->mins);VectorCopy(e->r.maxs,node->maxs);
  const int boundsLimit=SectionRoot(n)||ControllerKind(n)?WORLD_SIZE:8192;
  if(node->poseFrame<0||node->poseFrame>65535||!CopyThrustValid(node)||!CopyVector(node->offset,65535)||!CopyVector(node->angles,100000)||!CopyVector(node->mins,boundsLimit)||!CopyVector(node->maxs,boundsLimit)||
     !JAR_ContextMassValid(node->mass)||node->health<0||node->health>100000||node->count<0){Tell(p,"Assembly properties exceed supported bounds; clipboard unchanged.");return;}
  for(int a=0;a<3;++a)if(node->mins[a]>node->maxs[a]){Tell(p,"Invalid collision bounds; clipboard unchanged.");return;}
  // Controller links and distance inputs belong to the same assembly in either direction.
  for(int source=MAX_CLIENTS;source<level.num_entities;++source){
   if(!g_entities[source].inuse)continue;
   const int roles=(objects[source].distanceInput[0]||objects[source].distanceInput[1])?JAR_DISTANCE_LINKS:2;
   for(int role=0;role<roles;++role){
    const int target=*DistanceLinkField(source,role)-1;
    if(target<0||(source!=n&&target!=n))continue;
    if(role>=JAR_DISTANCE_REFERENCE&&!DistanceEndpoint(source,target))continue;
    const int other=source==n?target:source;
    if(!Edit(p,other)||!objects[other].catalog){Tell(p,"A connected controller is protected; clipboard unchanged.");return;}
    if(indices[other]<0){if(candidate.count==MAX_GENTITIES)return;indices[other]=candidate.count;slots[candidate.count++]=other;}
   }
  }
  for(int r=MAX_CLIENTS;r<level.num_entities;++r){jarObject_t*link=&objects[r];if(!g_entities[r].inuse||!link->owner||g_entities[r].s.generic1!=251)continue;
   int a=link->first-1,b=link->second-1;if(a!=n&&b!=n)continue;int other=a==n?b:a;
   if(other==ENTITYNUM_WORLD)continue;
   if(!Edit(p,r)||!Edit(p,other)||!objects[other].catalog){Tell(p,"A connected member or attachment is protected; clipboard unchanged.");return;}
   if(indices[other]<0){if(candidate.count==MAX_GENTITIES)return;indices[other]=candidate.count;slots[candidate.count++]=other;}
  }
 }
 for(int r=MAX_CLIENTS;r<level.num_entities;++r){jarObject_t*o=&objects[r];if(!g_entities[r].inuse||!o->owner||g_entities[r].s.generic1!=251)continue;
  int a=o->first-1,b=o->second-1;
  if(a<0||b<0||a>ENTITYNUM_WORLD||b>ENTITYNUM_WORLD)continue;
  const qboolean worldA=a==ENTITYNUM_WORLD,worldB=b==ENTITYNUM_WORLD;
  if((worldA&&worldB)||(!worldA&&indices[a]<0)||(!worldB&&indices[b]<0))continue;
  jarCopyRope_t link={0};link.first=worldA?-1:indices[a];link.second=worldB?-1:indices[b];CopyLinkCapture(&link,o);
  if(!CopyLinkValid(&link,candidate.count)){Tell(p,"Invalid attachment frame; clipboard unchanged.");return;}
  if(worldA||worldB){
   if(!CopyWorldNormal(&link)){++external;continue;}
   if(!Edit(p,r)){Tell(p,"A connected world attachment is protected; clipboard unchanged.");return;}
   if(!link.kind){vec4_t* frame=worldA?&link.ra:&link.rb;VectorCopy(link.copyWorldNormal,*frame);(*frame)[3]=1;}
  }
  if(candidate.ropes==JAR_MP_ATTACHMENT_LIMIT){Tell(p,"Attachment capacity reached; clipboard unchanged.");return;}
  candidate.rope[candidate.ropes++]=link;
 }
 for(int i=0;i<candidate.count;++i){
  for(int o=0;o<2;++o){int target=objects[slots[i]].output[o]-1;if(target>=0)candidate.node[i].output[o]=indices[target]+1;}
  CopyDistanceCapture(&candidate.node[i],slots[i],indices);
 }
 if(!CopyControllersValid(&candidate)){Tell(p,"Controller graph or state is invalid; clipboard unchanged.");return;}
 copies[p->s.number]=candidate;
 Tell(p,va("Copied %d objects and %d attachments. Left click pastes. %d unsupported world anchor(s) excluded.",candidate.count,candidate.ropes,external));
}
// Remove the whole still-initializing group if an NPC disappears or times out.
// Clear every pending flag first, so nested native delete callbacks cannot recur.
static void CopyForgot(int entity,int owner,unsigned group){
 if(!owner||!group)return;qboolean pending=qfalse;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].owner==owner&&objects[n].order==group&&objects[n].copyPending)pending=qtrue;
 if(!pending)return;
 if(owner==MAX_CLIENTS+1)WorldRestoreFailed();
 if(CopyBusy(owner,group)){
  for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(n!=entity&&objects[n].owner==owner&&objects[n].order==group){objects[n].copyPending=-1;objects[n].copyDeadline=level.time;}
  objects[entity].copyPending=0;return;
 }
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].owner==owner&&objects[n].order==group)objects[n].copyPending=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(n!=entity&&g_entities[n].inuse&&objects[n].owner==owner&&objects[n].order==group)G_FreeEntity(&g_entities[n]);
 if(owner<=MAX_CLIENTS&&g_entities[owner-1].inuse&&g_entities[owner-1].client)Tell(&g_entities[owner-1],"Pending assembly removed: a native member failed, was removed, or could not initialize.");
 else if(owner==MAX_CLIENTS+1)Tell(NULL,"Pending server world removed: a native member failed, was removed, or could not initialize.");
}
static void CopyFrame(void){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].copyPending&&(!sandbox.integer||level.time>=objects[n].copyDeadline)&&!CopyBusy(objects[n].owner,objects[n].order))G_FreeEntity(&g_entities[n]);
}
void JAR_MP_CopyNPCReady(gentity_t*e){
 if(!e||!e->inuse||e->s.number<MAX_CLIENTS||e->s.number>=ENTITYNUM_WORLD)return;jarObject_t*o=&objects[e->s.number];
 if(o->copyPending<=0)return;
 if(!e->client||e->health<=0||o->owner<1||o->owner>MAX_CLIENTS+1){G_FreeEntity(e);return;}
 // Native initialization settles actors toward the floor. Reapply the saved
 // transform only after checking that the destination is still unoccupied.
 vec3_t lo,hi;if(!VehicleBounds(e,o->copyAngles,lo,hi)){G_FreeEntity(e);return;}
 trace_t tr;trap->Trace(&tr,o->copyOrigin,lo,hi,o->copyOrigin,e->s.number,MASK_PLAYERSOLID,qfalse,JAR_TRACE_SPAWN_OVERLAP,0);
 if(tr.startsolid||tr.allsolid){G_FreeEntity(e);return;}
 VectorCopy(lo,e->r.mins);VectorCopy(hi,e->r.maxs);
 Move(e,o->copyOrigin);VectorCopy(o->copyAngles,e->r.currentAngles);VectorCopy(o->copyAngles,e->s.angles);VectorCopy(o->copyAngles,e->s.apos.trBase);SetClientViewAngle(e,o->copyAngles);
 e->health=e->client->ps.stats[STAT_HEALTH]=o->copyHealth;o->copyPending=0;VectorCopy(o->velocity,e->client->ps.velocity);
 if(e->m_pVehicle){
  e->m_pVehicle->m_iArmor=e->health;VehicleOrientation(e);
  if(o->copyVehicleMotion){VectorCopy(o->copyVehicleDrift,e->client->ps.jarVehicleDrift);e->client->ps.speed=o->copyVehicleSpeed;}
  else{VectorClear(e->client->ps.jarVehicleDrift);vec3_t forward;AngleVectors(e->r.currentAngles,forward,NULL,NULL);e->client->ps.speed=DotProduct(e->client->ps.velocity,forward);}
 }
 if(o->copyFrozen&&ActorAlive(e)){BeginActorAs(o->owner,e);o->frozen=1;JAR_MP_ActorThink(e,NULL);}
}
static qboolean PlaceCopyState(gentity_t*p,const jarCopy_t*copy,const vec3_t anchor,const vec3_t*velocity,const vec3_t*angular,
                              const jarSectionStates_t *timers){
 int mapped[MAX_GENTITIES],index,free=0,ropeCount=0,newActors=0,propCount=0,newProps=0,newMedia=0;
 trace_t tr;static jarMPSectionAdmission_t sections[JAR_MP_SECTION_COUNT];qboolean sectionRead[JAR_MP_SECTION_COUNT]={0};
 if(copy->count<0||copy->count>MAX_GENTITIES||copy->ropes<0||copy->ropes>JAR_MP_ATTACHMENT_LIMIT||!CopyControllersValid(copy))return qfalse;
 for(int r=0;r<copy->ropes;++r)if(!CopyLinkValid(&copy->rope[r],copy->count)||!CopyLinkEndpoints(copy,&copy->rope[r]))return qfalse;
 if(!copy->count){Tell(p,"Right click a spawned object to copy its connected assembly first.");return qfalse;}
 jarCopyRope_t placedLinks[JAR_MP_ATTACHMENT_LIMIT];
 vec3_t offset;VectorSubtract(anchor,copy->copyOrigin,offset);
 for(int r=0;r<copy->ropes;++r){
  placedLinks[r]=copy->rope[r];
  if(p&&VectorLengthSquared(placedLinks[r].copyWorldNormal)>.5f&&!CopyPlaceWorldLink(&placedLinks[r],offset)){
   Tell(p,"A copied world anchor needs matching scenery. Nothing pasted.");return qfalse;
  }
 }
 for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD;++n){if(!g_entities[n].inuse)++free;else if(objects[n].owner){if(PhysicsProp(n))++propCount;if(g_entities[n].s.generic1==251)++ropeCount;}}
 int uniqueModels=0,availableModels=0;char model[MAX_QPATH];
 for(int m=1;m<MAX_MODELS;++m){trap->GetConfigstring(CS_MODELS+m,model,sizeof(model));if(!*model)++availableModels;}
 for(int i=0;i<copy->count;++i){const jarCopyNode_t*n=&copy->node[i];const jarMPEntry_t*entry=Entry(n->id,&index);if(!entry||!CopyMotionValid(n,entry->kind))return qfalse;
  const jarMPSectionKind_t section=SectionKind(entry);
  if(section){
   if(!sectionRead[section]){
    if(!SectionRead(p,section,&sections[section]))return qfalse;
    sectionRead[section]=qtrue;
   }
   newMedia+=sections[section].childCount;
  }
  if(entry->kind==2||entry->kind==3)++newActors;
  int media=entry->kind==5?ControllerClass(entry->name):0;if(media==13||JAR_ControllerTrigger(media)||media==JAR_DIRECTIONAL_CONTROLLER)++newMedia;
  if(JAR_ControllerRack(media))newMedia+=JAR_MP_RackSlotsRemaining(media,n->controller.flags);
  if(media==JAR_DOOR_CAMERA_CONTROLLER)newMedia+=2;
  if(media==JAR_CRYSTAL_CRATE_CONTROLLER)++newMedia;
  if(media==JAR_MUSIC_CONTROLLER){
   if(!p&&n->controller.active&&musicCount){Tell(NULL,"Clear active Sandbox music before restoring a world.");return qfalse;}
   if(!MusicValidate(n->controller.text)){Tell(p,"A saved music track is unavailable. Nothing placed.");return qfalse;}
  }
  if(JAR_ControllerMedia(media)){++newMedia;if(!MediaValidate(media,n->controller.text)){Tell(p,"A copied sound/effect is missing or its resource reserve is reached.");return qfalse;}}
  if(CatalogBody(entry)){++newProps;qboolean known=qfalse;for(int j=0;j<i;++j)if(copy->node[j].id==n->id)known=qtrue;
   for(int m=1;!known&&m<MAX_MODELS;++m){trap->GetConfigstring(CS_MODELS+m,model,sizeof(model));if(!Q_stricmp(model,entry->model))known=qtrue;}
   if(!known){++uniqueModels;fileHandle_t f;int size=trap->FS_Open(entry->model,&f,FS_READ);if(f)trap->FS_Close(f);if(size<=0){Tell(p,"A copied model is unavailable; nothing pasted.");return qfalse;}}
  }
 }
 // Extra actor slots cover spawners, goals, sabers and vehicle droids; the
 // ordinary native reserve remains available during every member allocation.
 if(free<copy->count+copy->ropes+newActors*8+newMedia+JAR_MP_ENTITY_RESERVE+16||ropeCount+copy->ropes>JAR_MP_ATTACHMENT_LIMIT||propCount+newProps>JAR_MP_PROP_LIMIT||availableModels<uniqueModels+64){Tell(p,"Not enough entity, NPC, attachment or model capacity for the whole assembly.");return qfalse;}
 for(int i=0;i<copy->count;++i){vec3_t point;VectorAdd(anchor,copy->node[i].offset,point);if(!CopyVector(point,65535)){Tell(p,"Assembly extends outside the world bounds.");return qfalse;}
  vec3_t low,high;VectorCopy(copy->node[i].mins,low);VectorCopy(copy->node[i].maxs,high);
  const jarCopyNode_t*node=&copy->node[i];const jarMPEntry_t*item=Entry(node->id,&index);
  if(node->poseFrame<0||node->poseFrame>65535||(!item)||(CatalogBody(item)?!PhysicsPoseHull(index+1,node->poseFrame,node->angles,low,high):node->poseFrame!=0)){Tell(p,"A copied pose is unavailable; nothing pasted.");return qfalse;}
  const jarMPSectionKind_t section=SectionKind(item);
  if(section)SectionHull(sections[section].mins,sections[section].maxs,node->angles,low,high);
  if(node->id==JAR_MP_DOOR_CAMERA_ID)
   DoorCameraBounds(node->angles,node->controller.pushSpeed/(float)JAR_MP_DOOR_CAMERA_MILLI,low,high);
  if(node->id==JAR_MP_PORTAL_SURFACE_ID)
   JAR_MP_PortalHull(node->controller.speed,node->controller.pushSpeed,node->angles,low,high);
  if(node->id==JAR_WALL_CATALOG_ID&&!node->controller.active)WallMarkerHull(low,high);
  if(!CopyHiddenBeacon(node)){
   trap->Trace(&tr,point,low,high,point,ENTITYNUM_NONE,CopyPlacementMask(node),qfalse,section?0:JAR_TRACE_SPAWN_OVERLAP,0);
   if(tr.startsolid||tr.allsolid){Tell(p,"Not enough clear space for the whole assembly. Nothing pasted.");return qfalse;}
  }
 }
 unsigned group=++order;int made=0,sentinel=0,shaft=0,rift=0,korriban=0,yavin=0;byte before[MAX_GENTITIES];for(int n=0;n<MAX_GENTITIES;++n)before[n]=(byte)g_entities[n].inuse;
 for(int i=0;i<copy->count;++i){const jarCopyNode_t*node=&copy->node[i];vec3_t point;VectorAdd(anchor,node->offset,point);
  const jarMPEntry_t*entry=Entry(node->id,&index);
  const jarMPSectionKind_t section=SectionKind(entry);
  const qboolean hiddenBeacon=CopyHiddenBeacon(node);
  gentity_t*e=section?SectionSpawn(p,index,point,node->angles,&sections[section]):SpawnAtPoseState(p,node->id,point,node->angles,node->poseFrame,&node->controller);
  if(!e)goto fail;
  mapped[made++]=e->s.number;jarObject_t*o=&objects[e->s.number];o->order=group;o->mass=node->mass;o->thruster=node->thruster;o->thrustBy=node->thruster?o->owner:0;o->noGravity=node->noGravity;CopyThrustApply(o,node);o->frozen=e->client?0:node->frozen;if(velocity)VectorCopy(velocity[i],o->velocity);if(angular&&PhysicsProp(e->s.number))VectorCopy(angular[i],o->angular);
  if(e->item)e->count=node->count;if(node->itemConsumed)ConsumePickup(e);if(!CopyControllerApply(e,node))goto fail;
  if(section==JAR_MP_SECTION_CITY&&!p)SectionCityRestore(e->s.number,node->cityDamage);
  if(timers&&section==JAR_MP_SECTION_SENTINEL){
   const jarSentinelState_t*s=&timers->sentinels[sentinel++];
   if(!SectionTimersRestore(e->s.number,s->lifts,SENTINEL_LIFTS,s->pads,SENTINEL_PADS,s->pickups,SENTINEL_PICKUPS))goto fail;
  }else if(timers&&section==JAR_MP_SECTION_SHAFT){
   const jarShaftState_t*s=&timers->shafts[shaft++];
   if(!SectionTimersRestore(e->s.number,s->lifts,SHAFT_LIFTS,s->pads,SHAFT_PADS,NULL,0))goto fail;
  }else if(timers&&section==JAR_MP_SECTION_RIFT){
   if(!SectionTimersRestore(e->s.number,NULL,0,NULL,0,timers->rifts[rift++].pickups,RIFT_PICKUPS))goto fail;
  }else if(timers&&section==JAR_MP_SECTION_KORRIBAN){
   if(!SectionTimersRestore(e->s.number,NULL,0,NULL,0,timers->korriban[korriban++],JAR_MP_KORRIBAN_PICKUPS))goto fail;
  }else if(timers&&section==JAR_MP_SECTION_YAVIN){
   SectionYavinRestore(e->s.number,timers->yavin[yavin++]);
  }
  if(e->client){o->copyPending=1;o->copyHealth=node->health;o->copyFrozen=node->frozen;o->copyDeadline=level.time+3000;VectorCopy(point,o->copyOrigin);VectorCopy(node->angles,o->copyAngles);o->copyVehicleMotion=node->vehicleMotion;VectorCopy(node->vehicleDrift,o->copyVehicleDrift);o->copyVehicleSpeed=node->vehicleSpeed;
   e->client->ps.stats[STAT_JAR_ACTOR_GRAVITY]=node->actorGravity;e->client->ps.stats[STAT_JAR_ACTOR_FRICTION]=node->actorFriction;
  }
  trap->UnlinkEntity((sharedEntity_t*)e);
  vec3_t low,high;
  if(SectionRoot(e->s.number)){
   SectionSolidChildrenLink(e->s.number,qfalse);
   SectionHull(e->r.mins,e->r.maxs,e->r.currentAngles,low,high);
  }
  else {VectorCopy(e->r.mins,low);VectorCopy(e->r.maxs,high);}
  if(!hiddenBeacon){trap->Trace(&tr,point,low,high,point,ENTITYNUM_NONE,CopyPlacementMask(node),qfalse,section?0:JAR_TRACE_SPAWN_OVERLAP,0);if(tr.startsolid||tr.allsolid)goto fail;}
 }
 for(int i=0;i<copy->ropes;++i){const jarCopyRope_t*link=&placedLinks[i];gentity_t*e=G_TrySpawn();if(!e)goto fail;Track(e,p?p->s.number:MAX_CLIENTS,-1);jarObject_t*o=&objects[e->s.number];o->order=group;
  o->first=(link->first<0?ENTITYNUM_WORLD:mapped[link->first])+1;o->second=(link->second<0?ENTITYNUM_WORLD:mapped[link->second])+1;CopyLinkApply(o,link);
  if(p)memset(&o->ropePath,0,sizeof(o->ropePath)); // Copies reroute; world restores retain their side.
  const qboolean hiddenEndpoint=(link->first>=0&&CopyHiddenBeacon(&copy->node[link->first]))||
                                (link->second>=0&&CopyHiddenBeacon(&copy->node[link->second]));
  // Dormant links resume normal route admission after the beacon deploys.
  if(!hiddenEndpoint&&!RopeAdmission(e->s.number,NULL))goto fail;
  if(!o->jointKind)e->s.jarRopeState=1;
  e->classname=o->jointKind==1?"jar_weld":o->jointKind==2?"jar_hinge":"jar_rope";e->s.eType=ET_GENERAL;e->s.generic1=251;e->s.weapon=o->jointKind;e->r.svFlags|=o->jointKind?SVF_NOCLIENT:SVF_BROADCAST;e->s.time2=(int)o->length;
  vec3_t a,b;Endpoint(o->first-1,o->anchorA,a);Endpoint(o->second-1,o->anchorB,b);Move(e,a);VectorCopy(b,e->s.origin2);
 }
 for(int i=0;i<made;++i){
  for(int o=0;o<2;++o)objects[mapped[i]].output[o]=copy->node[i].output[o]?mapped[copy->node[i].output[o]-1]+1:0;
  CopyDistanceRestore(&copy->node[i],mapped[i],mapped);
  if(JAR_MP_PortalSurface(&g_entities[mapped[i]].s))PortalSync(&g_entities[mapped[i]]);
 }
 for(int i=0;i<made;++i){if(!objects[mapped[i]].itemConsumed)trap->LinkEntity((sharedEntity_t*)&g_entities[mapped[i]]);if(SectionRoot(mapped[i]))SectionSolidChildrenLink(mapped[i],qtrue);if(JAR_ControllerMedia(ControllerKind(mapped[i])))MediaSync(mapped[i]);else if(ControllerKind(mapped[i])==13)LaserSync(mapped[i],NULL);else if(JAR_ControllerTrigger(ControllerKind(mapped[i]))||ControllerKind(mapped[i])==JAR_DIRECTIONAL_CONTROLLER)TriggerSync(mapped[i]);else if(ControllerKind(mapped[i])==33)LightSync(mapped[i]);}
 if(!p){
  // Saved ranks are contiguous and unique, validated before placing anything.
  int restored=0;
  for(int i=0;i<made;++i)if(ControllerKind(mapped[i])==JAR_MUSIC_CONTROLLER&&copy->node[i].controller.active){
   const int rank=copy->node[i].controllerRemaining;
   musicSlots[rank-1]=mapped[i];musicRank[mapped[i]]=rank;++restored;
  }
  if(restored){
   if(!musicDirty)trap->GetConfigstring(CS_MUSIC,musicBaseline,sizeof(musicBaseline));
   musicCount=restored;musicDirty=qtrue;
  }
 }
 if(p)players[p->s.number].selected=mapped[0];
 if(p)Tell(p,va("Pasted %d objects and %d attachments. Undo removes this group.%s",copy->count,copy->ropes,newActors?" Native actors are initializing.":""));return qtrue;
fail:
 // Spawn callbacks execute synchronously. Remove only slots allocated during
 // this transaction, including native spawners/helpers that failed before Track.
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(!before[n])objects[n].copyPending=0;
 for(int n=level.num_entities-1;n>=MAX_CLIENTS;--n)if(!before[n]&&g_entities[n].inuse)G_FreeEntity(&g_entities[n]);
 Tell(p,"Assembly placement failed and was rolled back. Existing objects preserved.");return qfalse;
}

static qboolean PlaceCopy(gentity_t*p,const jarCopy_t*copy,const vec3_t anchor,const vec3_t*velocity,const vec3_t*angular){
 return PlaceCopyState(p,copy,anchor,velocity,angular,NULL);
}

static void PasteAssembly(gentity_t*p){
 const jarCopy_t*copy=&copies[p->s.number];vec3_t low={65536,65536,65536},high={-65536,-65536,-65536},anchor;trace_t aim;
 if(!copy->count){Tell(p,"Right click a spawned object to copy its connected assembly first.");return;}
 for(int i=0;i<copy->count;++i){
  const jarCopyNode_t*node=&copy->node[i];int index;
  vec3_t mins,maxs;
  if(SectionCard(Entry(node->id,&index)))SectionHull(node->mins,node->maxs,node->angles,mins,maxs);
  else{VectorCopy(node->mins,mins);VectorCopy(node->maxs,maxs);}
  for(int a=0;a<3;++a){low[a]=fminf(low[a],node->offset[a]+mins[a]);high[a]=fmaxf(high[a],node->offset[a]+maxs[a]);}
  if(node->id==JAR_LIFT_ID)low[2]=fminf(low[2],node->offset[2]+mins[2]-node->controller.pushSpeed/(float)JAR_SLIDE_MILLI);
  if(node->id==JAR_MP_BOBBING_ID){
   const float amplitude=(float)node->controller.speed;
   const float center=node->offset[2]-amplitude*(float)JAR_BobWave(node->controller.pushSpeed);
   low[2]=fminf(low[2],center+mins[2]-amplitude);
   high[2]=fmaxf(high[2],center+maxs[2]+amplitude);
  }
 }
 Aim(p,&aim,2048);if(aim.startsolid||aim.allsolid||aim.fraction>=1){Tell(p,"Aim at a surface within 2048 units to paste.");return;}
 float offset=2;for(int a=0;a<3;++a)offset-=aim.plane.normal[a]*(aim.plane.normal[a]>=0?low[a]:high[a]);VectorMA(aim.endpos,offset,aim.plane.normal,anchor);
 for(int i=0;i<copy->ropes;++i){
  const jarCopyRope_t* link=&copy->rope[i];
  if(DotProduct(link->copyWorldNormal,aim.plane.normal)<.98f)continue;
  vec3_t projected;
  VectorSubtract(link->first<0?link->a:link->b,copy->copyOrigin,projected);
  VectorAdd(anchor,projected,projected);VectorSubtract(aim.endpos,projected,projected);
  VectorMA(anchor,DotProduct(projected,aim.plane.normal),aim.plane.normal,anchor);
  break;
 }
 PlaceCopy(p,copy,anchor,NULL,NULL);
}
