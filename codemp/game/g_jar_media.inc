// Visible, manipulable bodies retain native sound/FX snapshots in private helpers.
// Helpers are derived state: never separate menu cards, saved objects or owners.
// GPL-2.0-only.
extern void Use_Target_Speaker(gentity_t*,gentity_t*,gentity_t*);
extern void fx_runner_use(gentity_t*,gentity_t*,gentity_t*);
extern void fx_runner_think(gentity_t*);
static int mediaParent[MAX_GENTITIES];
static void MediaReset(void){memset(mediaParent,0,sizeof(mediaParent));}
static gentity_t*MediaHelper(int n){
 int h=objects[n].mediaHelper-1;
 return h>=MAX_CLIENTS&&h<ENTITYNUM_WORLD&&g_entities[h].inuse&&mediaParent[h]==n+1?&g_entities[h]:NULL;
}
static void MediaForget(int n){
 int parent=mediaParent[n]-1;mediaParent[n]=0;
 if(parent>=MAX_CLIENTS&&parent<ENTITYNUM_WORLD&&objects[parent].mediaHelper==n+1){objects[parent].mediaHelper=0;objects[parent].mediaActive=0;++objects[parent].propertyRevision;}
 gentity_t*h=MediaHelper(n);objects[n].mediaHelper=0;
 if(h){mediaParent[h->s.number]=0;G_FreeEntity(h);}
}
static qboolean MediaValidate(int kind,const char*path){
 if(!JAR_MediaPath(kind,path))return qfalse;
 char file[MAX_QPATH+16],value[MAX_QPATH];fileHandle_t handle=0;
 Com_sprintf(file,sizeof(file),kind==9?"%s":"effects/%s.efx",path);
 int size=trap->FS_Open(file,&handle,FS_READ);if(handle)trap->FS_Close(handle);if(size<=0||!handle)return qfalse;
 int start=kind==9?CS_SOUNDS:CS_EFFECTS,count=kind==9?MAX_SOUNDS:MAX_FX,free=0;
 for(int i=1;i<count;++i){trap->GetConfigstring(start+i,value,sizeof(value));if(!strcmp(value,path))return qtrue;if(!*value)++free;}
 return free>=(kind==9?64:16);
}
static void MediaState(int n,jarControllerState_t*s){
 gentity_t*e=&g_entities[n];s->inactive=(e->flags&FL_INACTIVE)?1:0;s->flags=e->spawnflags;
 s->waitMs=e->delay;s->randomMs=JAR_NumberMillis(e->random);s->active=objects[n].mediaActive;
 Q_strncpyz(s->text,objects[n].controllerText,sizeof(s->text));
}
static qboolean MediaPosition(int n,gentity_t*h){
 gentity_t*e=&g_entities[n];vec3_t origin,angles;VectorCopy(e->r.currentOrigin,origin);origin[2]+=e->r.maxs[2]+2;VectorCopy(e->r.currentAngles,angles);
 if(ControllerKind(n)==10&&objects[n].output[0]){
  int d=objects[n].output[0]-1;
  if((ControllerKind(d)!=5&&ControllerKind(d)!=8)||!ForcePermission(objects[n].owner,d)||objects[d].copyPending||(g_entities[d].flags&FL_INACTIVE))return qfalse;
  vec3_t direction;VectorSubtract(g_entities[d].r.currentOrigin,origin,direction);if(VectorNormalize(direction)<.001f)return qfalse;vectoangles(direction,angles);
 }
 G_SetOrigin(h,origin);VectorCopy(origin,h->s.origin);h->s.pos.trType=TR_STATIONARY;
 VectorCopy(angles,h->r.currentAngles);VectorCopy(angles,h->s.angles);VectorCopy(angles,h->s.apos.trBase);h->s.apos.trType=TR_STATIONARY;
 trap->LinkEntity((sharedEntity_t*)h);return qtrue;
}
static void MediaThink(gentity_t*h);
static qboolean MediaSync(int n){
 gentity_t*e=&g_entities[n],*h=MediaHelper(n);if(!h)return qfalse;
 qboolean ready=sandbox.integer&&!(e->flags&FL_INACTIVE)&&!objects[n].copyPending&&MediaPosition(n,h);
 if(!ready){h->s.loopSound=0;h->s.modelindex2=FX_STATE_OFF;h->nextthink=0;return qfalse;}
 int kind=ControllerKind(n);jarObject_t*o=&objects[n];
 if(kind==9){
  h->s.loopSound=e->spawnflags==2&&o->mediaActive?h->noise_index:0;h->s.loopIsSoundset=qfalse;h->s.trickedentindex=!h->s.loopSound;
 }else if(e->spawnflags!=2)h->s.modelindex2=o->mediaActive?FX_STATE_CONTINUOUS:FX_STATE_OFF;
 if(o->mediaActive&&e->delay){if(h->nextthink<=0)h->nextthink=JAR_NumberDeadline(level.time,e->delay);h->think=MediaThink;}
 else h->nextthink=0;
 return qtrue;
}
static void MediaOutput(int n){
 if(objects[n].output[1])JAR_MP_UseTargets(&g_entities[n],&g_entities[n],"@jar_secondary");
}
static void MediaThink(gentity_t*h){
 int n=mediaParent[h->s.number]-1;
 if(n<MAX_CLIENTS||n>=ENTITYNUM_WORLD||!g_entities[n].inuse||MediaHelper(n)!=h){G_FreeEntity(h);return;}
 if(!MediaSync(n)||!objects[n].mediaActive)return;
 gentity_t*e=&g_entities[n];
 if(ControllerKind(n)==10){fx_runner_think(h);h->think=MediaThink;MediaOutput(n);}
 else {Use_Target_Speaker(h,e,e);h->nextthink=JAR_NumberDeadline(level.time,e->delay+(double)e->random*Q_flrand(-1,1));}
}
static qboolean MediaApply(int n,const jarControllerState_t*s){
 int kind=ControllerKind(n);gentity_t*e=&g_entities[n],*h=MediaHelper(n);
 if(!JAR_ControllerStateValid(kind,s)||!MediaValidate(kind,s->text))return qfalse;
 if(!h){if(!Room())return qfalse;h=G_TrySpawn();if(!h)return qfalse;h->classname="jar_media_helper";mediaParent[h->s.number]=n+1;objects[n].mediaHelper=h->s.number+1;}
 int index=kind==9?G_SoundIndex(s->text):G_EffectIndex(s->text);if(!index)return qfalse;
 Q_strncpyz(objects[n].controllerText,s->text,64);e->message=objects[n].controllerText;e->spawnflags=s->flags;e->delay=s->waitMs;e->random=(float)s->randomMs;objects[n].mediaActive=s->active;
 if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
 h->r.svFlags=kind==9&&s->flags==4?SVF_BROADCAST:0;h->r.contents=0;h->clipmask=0;
 h->s.eType=kind==9?ET_SPEAKER:ET_FX;h->noise_index=index;h->s.modelindex=kind==10?index:0;h->s.eventParm=kind==9?index:0;
 h->spawnflags=s->flags;h->delay=s->waitMs;h->random=(float)s->randomMs;h->s.speed=(float)s->waitMs;h->s.time=s->randomMs;
 h->s.modelindex2=FX_STATE_OFF;h->s.frame=h->s.clientNum=0;h->s.loopSound=0;h->nextthink=0;h->think=MediaThink;
 MediaSync(n);return qtrue;
}
static void MediaUse(gentity_t*e,gentity_t*other,gentity_t*activator){
 int n=e->s.number,kind=ControllerKind(n);gentity_t*h=MediaHelper(n);
 if(!h){jarControllerState_t s={0};MediaState(n,&s);if(!MediaApply(n,&s))return;h=MediaHelper(n);}
 if(!MediaSync(n))return;
 if(kind==9){
  if(e->spawnflags==8&&(!activator||!activator->inuse||!activator->client))return;
  if(e->spawnflags==2){Use_Target_Speaker(h,other,activator);objects[n].mediaActive=h->s.loopSound!=0;}
  else if(e->delay){objects[n].mediaActive=!objects[n].mediaActive;if(objects[n].mediaActive)Use_Target_Speaker(h,other,activator);}
  else Use_Target_Speaker(h,other,activator);
  MediaSync(n);
 }else{
  h->nextthink=objects[n].mediaActive?JAR_NumberDeadline(level.time,e->delay):-1;
  fx_runner_use(h,other,activator);h->think=MediaThink;
  if(e->spawnflags==2){objects[n].mediaActive=0;if(h->s.modelindex2>=FX_STATE_ONE_SHOT_LIMIT)h->s.modelindex2=FX_STATE_ONE_SHOT;}
  else objects[n].mediaActive=h->nextthink>0;
  if(e->spawnflags==2||objects[n].mediaActive)MediaOutput(n);
 }
 SelectionChanged(n);
}
static qboolean MediaInit(gentity_t*e){
 int kind=ControllerKind(e->s.number);jarControllerState_t s={0};
 Q_strncpyz(s.text,kind==9?jarSpeakerChoices[0]:jarEffectChoices[0],64);if(kind==10){s.waitMs=500;s.active=1;}
 e->use=MediaUse;return MediaApply(e->s.number,&s);
}
static void MediaFrame(void){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&JAR_ControllerMedia(ControllerKind(n)))MediaSync(n);
}
