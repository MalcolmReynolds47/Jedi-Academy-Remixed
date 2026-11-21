// Model-backed func_usable. Native use/pain/death decisions stay shared with
// map entities; only brush geometry, owned state and lifetime are adapted.
// GPL-2.0-only.
extern void func_usable_use(gentity_t*,gentity_t*,gentity_t*);
extern void func_usable_think(gentity_t*);
extern void func_usable_pain(gentity_t*,gentity_t*,int);
extern void func_usable_die(gentity_t*,gentity_t*,gentity_t*,int,int);
static void UsableUse(gentity_t*,gentity_t*,gentity_t*);
static void UsableThink(gentity_t*);
static qboolean UsableClear(gentity_t*e){
 if(!(e->spawnflags&16))return qtrue;
 trace_t tr;trap->Trace(&tr,e->r.currentOrigin,e->r.mins,e->r.maxs,e->r.currentOrigin,e->s.number,MASK_PLAYERSOLID,qfalse,0,0);
 return !tr.startsolid&&!tr.allsolid;
}
static void UsableVisibility(gentity_t*e,qboolean visible){
 int n=e->s.number;qboolean changed=visible==!!(e->s.eFlags&EF_NODRAW);
 if(changed){if(objects[n].heldBy)Drop(objects[n].heldBy-1,qfalse);PhysicsForget(n);}
 e->count=visible?1:0;
 if(visible){e->s.eFlags&=~EF_NODRAW;e->r.svFlags&=~SVF_NOCLIENT;e->r.contents=CONTENTS_SOLID;e->clipmask=MASK_PLAYERSOLID;}
 else {e->s.eFlags|=EF_NODRAW;e->r.svFlags|=SVF_NOCLIENT;e->s.solid=0;e->r.contents=e->clipmask=0;
  // Hidden bodies retain their saved transform and attachments, not an
  // invisible collider or an accumulating gravity/thruster impulse.
  if(objects[n].heldBy)Drop(objects[n].heldBy-1,qfalse);JAR_RigidForget(n);
  VectorClear(objects[n].velocity);VectorClear(objects[n].angular);VectorClear(e->s.pos.trDelta);VectorClear(e->s.apos.trDelta);
 }
 e->use=UsableUse;if((e->spawnflags&64)&&!e->genericValue11)e->r.svFlags|=SVF_PLAYER_USABLE;else e->r.svFlags&=~SVF_PLAYER_USABLE;
 PhysicsPublish(e);trap->LinkEntity((sharedEntity_t*)e);
}
qboolean JAR_MP_UsableReturn(gentity_t*e){
 if(!e||ControllerKind(e->s.number)!=29)return qfalse;
 if(!sandbox.integer||(e->flags&FL_INACTIVE)||objects[e->s.number].copyPending||!UsableClear(e)){
  e->count=0;e->genericValue11=1;e->think=UsableThink;e->nextthink=level.time+FRAMETIME;return qtrue;
 }
 e->genericValue11=0;e->think=NULL;e->nextthink=0;UsableVisibility(e,qtrue);
 ++objects[e->s.number].propertyRevision;SelectionChanged(e->s.number);
 G_UseTargets2(e,ControllerActivator(e),e->target2);return qtrue;
}
static void UsableThink(gentity_t*e){
 if(!sandbox.integer||(e->flags&FL_INACTIVE)||objects[e->s.number].copyPending){e->nextthink=level.time+FRAMETIME;return;}
 if(e->genericValue11==1){JAR_MP_UsableReturn(e);return;}
 if(e->genericValue11==2){func_usable_think(e);e->genericValue11=0;e->nextthink=0;UsableVisibility(e,qtrue);++objects[e->s.number].propertyRevision;SelectionChanged(e->s.number);}
}
static void UsableUse(gentity_t*e,gentity_t*other,gentity_t*a){
 if(!sandbox.integer||(e->flags&FL_INACTIVE)||objects[e->s.number].copyPending||e->genericValue11)return;
 if(a&&a->client&&!ActionActivator(e,a))return;
 if(other==a&&(e->spawnflags&32)&&(!a||!a->NPC))return;
 int n=e->s.number;unsigned epoch=controllerEntityEpoch[n];ControllerCaptureActivator(e,a);e->activator=a;
 func_usable_use(e,other,a);
 if(!e->inuse||controllerEntityEpoch[n]!=epoch)return;
 if(e->spawnflags&8){e->genericValue11=e->nextthink>level.time?2:3;e->think=e->genericValue11==2?UsableThink:NULL;}
 UsableVisibility(e,!(e->s.eFlags&EF_NODRAW));++objects[n].propertyRevision;SelectionChanged(n);
}
static void UsablePain(gentity_t*e,gentity_t*a,int damage){
 e->health=Com_Clampi(0,e->genericValue4,e->health);++objects[e->s.number].propertyRevision;func_usable_pain(e,a,damage);SelectionChanged(e->s.number);
}
static void UsableDie(gentity_t*e,gentity_t*i,gentity_t*a,int damage,int mod){
 e->health=0;++objects[e->s.number].propertyRevision;func_usable_die(e,i,a,damage,mod);SelectionChanged(e->s.number);
}
static void UsableState(int n,jarControllerState_t*s){
 gentity_t*e=&g_entities[n];s->flags=e->spawnflags;s->waitMs=JAR_NumberMillis((double)e->wait*1000);s->active=!(e->s.eFlags&EF_NODRAW);s->repeats=e->genericValue11;
 s->count=Com_Clampi(0,e->genericValue4,e->health);s->initial=e->genericValue4;s->inactive=!!(e->flags&FL_INACTIVE);
}
static qboolean UsableApply(int n,const jarControllerState_t*s){
 if(!JAR_ControllerStateValid(29,s))return qfalse;gentity_t*e=&g_entities[n];
 if(s->active&&BodyDormant(n)&&(s->flags&16)){int old=e->spawnflags;e->spawnflags=s->flags;qboolean clear=UsableClear(e);e->spawnflags=old;if(!clear)return qfalse;}
 int pending=e->genericValue11;e->spawnflags=s->flags;e->wait=s->waitMs*.001f;e->genericValue4=s->initial;e->health=s->count;e->takedamage=e->health>0;e->pain=UsablePain;e->die=UsableDie;
 e->genericValue11=s->repeats;if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
 if(s->repeats==1||s->repeats==2){e->think=UsableThink;if(pending!=s->repeats||!e->nextthink)e->nextthink=JAR_NumberDeadline(level.time,s->repeats==1?FRAMETIME:s->waitMs);}
 else {e->think=NULL;e->nextthink=0;}
 UsableVisibility(e,s->active);return qtrue;
}
static qboolean UsableInit(gentity_t*e){jarControllerState_t s={0};s.flags=16|64;s.active=1;return UsableApply(e->s.number,&s);}
