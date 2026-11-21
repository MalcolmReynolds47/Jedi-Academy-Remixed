// Portable native trigger callbacks with derived, oriented sensor volumes.
// Bodies use ordinary stock models, physics and tool selection. GPL-2.0-only.
extern void SP_trigger_multiple(gentity_t*);
extern void SP_trigger_once(gentity_t*);
extern void SP_trigger_hurt(gentity_t*);
extern void SP_trigger_push(gentity_t*);
extern void SP_trigger_teleport(gentity_t*);
extern void Touch_Multi(gentity_t*,gentity_t*,trace_t*);
extern void Use_Multi(gentity_t*,gentity_t*,gentity_t*);
extern void trigger_cleared_fire(gentity_t*);
extern void hurt_touch(gentity_t*,gentity_t*,trace_t*);
extern qboolean G_PointInBounds(vec3_t,vec3_t,vec3_t);
static int triggerParent[MAX_GENTITIES];
static int directionalSensors;
static byte directionalSensor[MAX_GENTITIES];
static int teleportGrantTime,teleportGrantBudget,teleportGrantRoom;
static void TriggerPublishTeleport(gentity_t*,gentity_t*);
static void TriggerState(int n,jarControllerState_t*s);
qboolean JAR_MP_PortableTrigger(gentity_t*e){return e&&JAR_ControllerTrigger(ControllerKind(e->s.number));}
void JAR_MP_TriggerActivation(gentity_t*e,gentity_t*a){if(JAR_MP_PortableTrigger(e)){ControllerCaptureActivator(e,a);++objects[e->s.number].propertyRevision;}}
static void TriggerReset(void){memset(triggerParent,0,sizeof(triggerParent));memset(directionalSensor,0,sizeof(directionalSensor));directionalSensors=0;TriggerPropReset();teleportGrantTime=-1;teleportGrantBudget=0;teleportGrantRoom=-1;}
static gentity_t*TriggerHelper(int n){int h=objects[n].triggerHelper-1;return h>=MAX_CLIENTS&&h<ENTITYNUM_WORLD&&g_entities[h].inuse&&triggerParent[h]==n+1?&g_entities[h]:NULL;}
static qboolean SpaceRiderProtected(const gentity_t* actor){
 const int seat=actor->client->ps.m_iVehicleNum;
 if(seat<MAX_CLIENTS||seat>=ENTITYNUM_WORLD)return qfalse;
 const gentity_t*vehicle=&g_entities[seat];
 return vehicle->inuse&&vehicle->m_pVehicle&&vehicle->m_pVehicle->m_pVehicleInfo&&vehicle->m_pVehicle->m_pVehicleInfo->hideRider;
}
static qboolean PortableSpaceContains(int n,const gentity_t* actor){
 const gentity_t*volume=&g_entities[n];
 const jarObject_t*o=&objects[n];
 if(!sandbox.integer||!o->triggerActive||o->copyPending||(volume->flags&FL_INACTIVE))return qfalse;
 if(actor->s.number<MAX_CLIENTS&&actor->client->ps.m_iVehicleNum){
  if(!HazardPlayerAllowed(o->owner,actor->s.number)||
     actor->client->pers.connected!=CON_CONNECTED||actor->client->sess.sessionTeam==TEAM_SPECTATOR||
     actor->client->ps.duelInProgress||actor->client->ps.fd.forceGripBeingGripped>level.time)return qfalse;
 }else if(!LaserDamageAllowed(o->owner,actor->s.number))return qfalse;
 jarControllerState_t state={0};
 TriggerState(n,&state);
 vec3_t lo,hi,delta,axis[3];
 JAR_SpaceBounds(&state,lo,hi);
 AnglesToAxis(volume->r.currentAngles,axis);
 VectorSubtract(actor->client->ps.origin,volume->r.currentOrigin,delta);
 for(int a=0;a<3;++a){
  const float local=DotProduct(delta,axis[a]);
  if(local<lo[a]-.001f||local>hi[a]+.001f)return qfalse;
 }
 return qtrue;
}
static gentity_t *SpaceSourceAt(int slot,const gentity_t*actor){
 if(slot<MAX_CLIENTS||slot>=ENTITYNUM_WORLD)return NULL;
 gentity_t*sensor=&g_entities[slot];
 if(!sensor->inuse||!sensor->r.linked||!(sensor->r.contents&CONTENTS_TRIGGER))return NULL;
 const int n=triggerParent[slot]-1;
 if(ControllerKind(n)==JAR_SPACE_CONTROLLER&&TriggerHelper(n)==sensor){
  if(PortableSpaceContains(n,actor))return &g_entities[n];
 }else if(!objects[slot].owner&&sensor->classname&&!strcmp(sensor->classname,"trigger_space")&&
          G_PointInBounds(actor->client->ps.origin,sensor->r.absmin,sensor->r.absmax))return sensor;
 return NULL;
}
static gentity_t *SpaceSourceExcept(gentity_t *actor,int excluded){
 if(!actor||!actor->client)return NULL;
 gclient_t*client=actor->client;
 const int slot=client->inSpaceIndex;
 if(!slot||slot==ENTITYNUM_NONE)return NULL;
 if(slot>=MAX_CLIENTS&&slot<ENTITYNUM_WORLD&&actor->inuse&&actor->health>0&&!client->noclip&&!SpaceRiderProtected(actor)){
  gentity_t*source=slot==excluded?NULL:SpaceSourceAt(slot,actor);
  if(source)return source;
  const float*point=client->ps.origin;
  int nearby[MAX_GENTITIES];
  const int count=trap->EntitiesInBox(point,point,nearby,MAX_GENTITIES);
  for(int i=0;i<count;++i){
   const int candidate=nearby[i];
   if(candidate==excluded||candidate==slot)continue;
   source=SpaceSourceAt(candidate,actor);
   if(source){client->inSpaceIndex=candidate;return source;}
  }
 }
 client->inSpaceIndex=client->inSpaceSuffocation=0;
 return NULL;
}
static gentity_t *SpaceSource(gentity_t*actor){return SpaceSourceExcept(actor,ENTITYNUM_NONE);}
int JAR_MP_SpaceGravity(gentity_t *actor){
 const gentity_t*source=SpaceSource(actor);
 if(!source)return -1;
 return ControllerKind(source->s.number)==JAR_SPACE_CONTROLLER?0:1;
}
static qboolean SpaceBreathes(const gentity_t*actor){
 switch(actor->client->NPC_class){
 case CLASS_ATST:case CLASS_GONK:case CLASS_INTERROGATOR:case CLASS_MARK1:case CLASS_MARK2:case CLASS_GALAKMECH:
 case CLASS_MOUSE:case CLASS_PROBE:case CLASS_PROTOCOL:case CLASS_R2D2:case CLASS_R5D2:case CLASS_REMOTE:
 case CLASS_SEEKER:case CLASS_SENTRY:case CLASS_VEHICLE:return qfalse;
 default:return qtrue;
 }
}
void JAR_MP_SpaceFrame(gentity_t *actor){
 gentity_t*source=SpaceSource(actor);
 if(!source)return;
 gclient_t*client=actor->client;
 const qboolean portable=ControllerKind(source->s.number)==JAR_SPACE_CONTROLLER;
 if(!actor->takedamage||client->inSpaceSuffocation>level.time||
    (portable?!SpaceBreathes(actor):actor->s.number>=MAX_CLIENTS))return;
 client->inSpaceSuffocation=level.time+(portable?Q_irand(1000,2000):Q_irand(100,200));
 const int damage=portable?Q_irand(20,40):Q_irand(50,70);
 G_Damage(actor,source,source,NULL,client->ps.origin,damage,DAMAGE_NO_ARMOR,MOD_SUICIDE);
 if(!actor->inuse||actor->client!=client||actor->health<=0||!SpaceSource(actor))return;
 G_EntitySound(actor,CHAN_VOICE,G_SoundIndex(va("*choke%d.wav",Q_irand(1,3))));
 client->ps.forceHandExtend=HANDEXTEND_CHOKE;
 client->ps.forceHandExtendTime=level.time+2000;
}
static void TriggerForget(int n){
 const gentity_t*entity=&g_entities[n];
 if((triggerParent[n]&&ControllerKind(triggerParent[n]-1)==JAR_SPACE_CONTROLLER)||
    (!objects[n].owner&&entity->classname&&!strcmp(entity->classname,"trigger_space"))){
  for(int actor=0;actor<level.num_entities;++actor){
   gclient_t*client=g_entities[actor].client;
   if(g_entities[actor].inuse&&client&&client->inSpaceIndex==n)SpaceSourceExcept(&g_entities[actor],n);
  }
 }
 if(directionalSensor[n]){directionalSensor[n]=0;--directionalSensors;}
 int parent=triggerParent[n]-1;triggerParent[n]=0;
 if(parent>=MAX_CLIENTS&&parent<ENTITYNUM_WORLD&&objects[parent].triggerHelper==n+1){objects[parent].triggerHelper=0;objects[parent].triggerActive=0;++objects[parent].propertyRevision;}
 gentity_t*h=TriggerHelper(n);objects[n].triggerHelper=0;if(h)G_FreeEntity(h);
}
// The engine broad phase uses the helper's enclosing AABB. Reject the extra
// corners with SAT so rotation does not enlarge the actual trigger volume.
static qboolean TriggerContact(const gentity_t*e,const gentity_t*other){
 return JAR_VolumeContact(e->r.currentOrigin,e->r.currentAngles,(float)objects[e->s.number].triggerExtent,other->client?other->client->ps.origin:other->r.currentOrigin,other->r.mins,other->r.maxs);
}
static void TriggerSync(int n){
 gentity_t*e=&g_entities[n],*h=TriggerHelper(n);if(!h)return;jarObject_t*o=&objects[n];int kind=ControllerKind(n);
 if(kind==JAR_DIRECTIONAL_CONTROLLER){DirectionalSync(n);return;}
 AngleVectors(e->r.currentAngles,e->movedir,NULL,NULL);
 if(kind==19)e->target2=objects[n].output[1]?"@jar_secondary":NULL;
 if(!sandbox.integer||!o->triggerActive||((kind==19||kind==20)&&!e->use)||(e->flags&FL_INACTIVE)||o->copyPending){h->r.svFlags|=SVF_NOCLIENT;h->s.trickedentindex=h->s.trickedentindex2=h->s.trickedentindex3=h->s.trickedentindex4=0;trap->UnlinkEntity((sharedEntity_t*)h);return;}
 vec3_t axes[3];AnglesToAxis(e->r.currentAngles,axes);
 if(kind==JAR_SPACE_CONTROLLER){
  jarControllerState_t state={0};
  TriggerState(n,&state);
  vec3_t lo,hi;
  JAR_SpaceBounds(&state,lo,hi);
  for(int world=0;world<3;++world){
   float min=0,max=0;
   for(int local=0;local<3;++local){
    const float a=axes[local][world]*lo[local],b=axes[local][world]*hi[local];
    min+=fminf(a,b);max+=fmaxf(a,b);
   }
   h->r.mins[world]=min;h->r.maxs[world]=max;
  }
 }else for(int a=0;a<3;++a){float span=o->triggerExtent*(fabsf(axes[0][a])+fabsf(axes[1][a])+fabsf(axes[2][a]));h->r.mins[a]=-span;h->r.maxs[a]=span;}
 G_SetOrigin(h,e->r.currentOrigin);if(kind==22)TriggerPublishPush(e,h);else if(kind==23)TriggerPublishTeleport(e,h);trap->LinkEntity((sharedEntity_t*)h);
}
static void TriggerTouch(gentity_t*h,gentity_t*other,trace_t*trace){
 int n=triggerParent[h->s.number]-1;if(n<MAX_CLIENTS||n>=ENTITYNUM_WORLD||TriggerHelper(n)!=h||!JAR_ControllerTrigger(ControllerKind(n)))return;
 gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];int kind=ControllerKind(n);
 if(kind==JAR_SPACE_CONTROLLER){
  if(!other||!other->inuse||!other->client||other->health<=0||other->client->noclip||
     SpaceRiderProtected(other)||!h->r.linked||!PortableSpaceContains(n,other))return;
  if(!SpaceSource(other))other->client->inSpaceSuffocation=level.time+5000;
  other->client->inSpaceIndex=h->s.number;
  return;
 }
 if(!other||!other->inuse||!sandbox.integer||!o->triggerActive||(e->flags&FL_INACTIVE)||o->copyPending||!TriggerContact(e,other)||(kind<22&&!LaserDamageAllowed(o->owner,other->s.number)))return;
 jarControllerState_t before,after;ControllerState(n,&before);
 if(kind>=22)TriggerTransportTouch(e,h,other,trace);
 else if(kind==21){if(!e->damage||!other->takedamage)return;e->activator=ControllerActivator(e);hurt_touch(e,other,trace);}
 else {if(!e->use)return;Touch_Multi(e,other,trace);}
 if(g_entities[n].inuse&&TriggerHelper(n)==h){TriggerSync(n);ControllerState(n,&after);if(memcmp(&before,&after,sizeof(before)))SelectionChanged(n);}
}
static void TriggerToggleUse(gentity_t*e,gentity_t*other,gentity_t*activator){
 if(ControllerKind(e->s.number)==JAR_SPACE_CONTROLLER&&(!sandbox.integer||objects[e->s.number].copyPending||(e->flags&FL_INACTIVE)))return;
 objects[e->s.number].triggerActive=!objects[e->s.number].triggerActive;
 TriggerSync(e->s.number);SelectionChanged(e->s.number);
}
static void TriggerState(int n,jarControllerState_t*s){
 gentity_t*e=&g_entities[n];int kind=ControllerKind(n);s->flags=e->spawnflags;s->speed=objects[n].triggerExtent;s->inactive=!!(e->flags&FL_INACTIVE);s->active=objects[n].triggerActive&&(kind>=21||e->use!=NULL);
 if(kind==19){s->waitMs=JAR_NumberMillis((double)e->wait*1000);s->randomMs=JAR_NumberMillis((double)e->random*1000);}
 if(kind==22){s->pushSpeed=(int)e->speed;s->waitMs=JAR_NumberMillis(e->wait);}
 if(kind==21)s->damage=e->damage;else if((kind==19||kind==20)&&e->nextthink>0)s->initial=e->think==trigger_cleared_fire?2:1;
 if(kind==JAR_SPACE_CONTROLLER){s->damage=e->genericValue1;s->pushSpeed=e->genericValue2;}
}
static qboolean TriggerApply(int n,const jarControllerState_t*s){
 gentity_t*e=&g_entities[n];int kind=ControllerKind(n);if(!JAR_ControllerStateValid(kind,s))return qfalse;gentity_t*h=TriggerHelper(n);
 if(!h){if(!Room())return qfalse;h=G_TrySpawn();if(!h)return qfalse;h->classname="jar_trigger_volume";h->s.eType=kind==23?ET_TELEPORT_TRIGGER:kind==22?ET_PUSH_TRIGGER:ET_GENERAL;h->r.svFlags=SVF_NOCLIENT;h->r.contents=CONTENTS_TRIGGER;h->touch=TriggerTouch;triggerParent[h->s.number]=n+1;objects[n].triggerHelper=h->s.number+1;}
 e->spawnflags=s->flags;objects[n].triggerExtent=s->speed;objects[n].triggerActive=s->active;
 if(kind==JAR_SPACE_CONTROLLER){e->genericValue1=s->damage;e->genericValue2=s->pushSpeed;}
 if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
 if(kind>=21){e->use=TriggerToggleUse;if(kind==21)e->damage=s->damage;if(kind==22){e->speed=(float)s->pushSpeed;e->wait=(float)s->waitMs;}}else {e->use=s->active?Use_Multi:NULL;if(kind==19){e->wait=s->waitMs*.001f;e->random=s->randomMs*.001f;}}
 TriggerSync(n);return qtrue;
}
static qboolean TriggerInit(gentity_t*e){
 if(ControllerKind(e->s.number)==JAR_SPACE_CONTROLLER){
  jarControllerState_t space={0};space.speed=space.damage=space.pushSpeed=128;
  space.flags=JAR_SPACE_FLOOR_ANCHOR;space.active=1;
  return TriggerApply(e->s.number,&space);
 }
 int oldCount=level.numSpawnVars;qboolean oldSpawning=level.spawning;level.numSpawnVars=0;level.spawning=qtrue;
 int kind=ControllerKind(e->s.number);if(kind==19){e->wait=1;SP_trigger_multiple(e);}else if(kind==20)SP_trigger_once(e);else if(kind==22)SP_trigger_push(e);else if(kind==23)SP_trigger_teleport(e);else {e->damage=5;SP_trigger_hurt(e);}
 level.numSpawnVars=oldCount;level.spawning=oldSpawning;
 if(kind>=22){e->s.eType=ET_GENERAL;e->think=NULL;e->nextthink=0;e->painDebounceTime=level.time-60000;}
 e->touch=NULL;e->r.contents=CONTENTS_SOLID;e->r.svFlags&=~SVF_NOCLIENT;
 jarControllerState_t state={0};state.speed=64;state.active=1;if(kind==19)state.waitMs=1000;if(kind==21)state.damage=5;if(kind==22){state.pushSpeed=1000;state.flags=4;}return TriggerApply(e->s.number,&state);
}
static void TriggerFrame(void){for(int n=MAX_CLIENTS;n<level.num_entities;++n){const int kind=ControllerKind(n);if(JAR_ControllerTrigger(kind)||kind==JAR_DIRECTIONAL_CONTROLLER)TriggerSync(n);}TriggerPropFrame();}
