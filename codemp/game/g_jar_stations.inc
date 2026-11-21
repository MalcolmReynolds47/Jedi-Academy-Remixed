// Portable stations retain native dispensing/recharge; map stations are unchanged.
// GPL-2.0-only.
extern void check_recharge(gentity_t*);
extern void shield_power_converter_use(gentity_t*,gentity_t*,gentity_t*);
extern void ammo_generic_power_converter_use(gentity_t*,gentity_t*,gentity_t*);
extern void ammo_power_converter_use(gentity_t*,gentity_t*,gentity_t*);
extern void health_power_converter_use(gentity_t*,gentity_t*,gentity_t*);
static void StationStop(gentity_t*e){if(e->s.loopSound)G_Sound(e,CHAN_AUTO,e->genericValue7);e->activator=NULL;e->s.loopSound=0;e->s.loopIsSoundset=qfalse;e->fly_sound_debounce_time=0;}
static void StationPublish(gentity_t* e){
 const int capacity=e->genericValue4;
 e->s.maxhealth=capacity<JAR_MP_ENTITY_HEALTH_MAX?capacity:JAR_MP_ENTITY_HEALTH_MAX;
 e->s.health=(int)((long long)e->count*e->s.maxhealth/capacity);
 if(e->count>0&&!e->s.health)e->s.health=1;
}
static void StationThink(gentity_t*e){
 int n=e->s.number;if(!sandbox.integer||(e->flags&FL_INACTIVE)||objects[n].copyPending){StationStop(e);e->nextthink=level.time+50;return;}
 if(e->activator&&(ControllerActivator(e)!=e->activator||!ActionActivator(e,e->activator)))StationStop(e);
 int charge=e->count;check_recharge(e);StationPublish(e);e->nextthink=level.time+50;
 if(charge!=e->count){++objects[n].propertyRevision;SelectionChanged(n);}
}
static void StationUse(gentity_t*e,gentity_t*other,gentity_t*a){
 if(!ActionActivator(e,a)||e->setTime>=level.time)return;
 int n=e->s.number,kind=ControllerKind(n),before=e->count,health=a->health;
 if(!e->count&&!e->genericValue12){StationStop(e);G_Sound(e,CHAN_AUTO,G_SoundIndex(kind==24||kind==27?"sound/interface/ammocon_empty":"sound/interface/shieldcon_empty"));e->setTime=level.time+100;return;}
 if(e->genericValue12)e->count=e->genericValue4;
 // Native shield limits must use the recipient's Siege class, including when
 // a Relay or Delay, rather than that player, supplied the 'other' argument.
 if(kind==24)ammo_generic_power_converter_use(e,a,a);
 else if(kind==27)ammo_power_converter_use(e,a,a);
 else if(kind==28){health_power_converter_use(e,a,a);if(!e->genericValue12)e->count-=a->health-health;a->client->ps.stats[STAT_HEALTH]=a->health;}
 else shield_power_converter_use(e,a,a);
 if(e->genericValue12)e->count=before;e->count=Com_Clampi(0,e->genericValue4,e->count);StationPublish(e);
 // Floor ammo's native successful route omits its rate gate. Owned stations
 // dispense at most once per 100 ms, independent of server/client frame rate.
 if(e->setTime<level.time+100)e->setTime=level.time+100;
 if(e->activator==a)ControllerCaptureActivator(e,a);
 ++objects[n].propertyRevision;if(before!=e->count)SelectionChanged(n);
}
static void StationState(int n,jarControllerState_t*s){gentity_t*e=&g_entities[n];s->count=e->count;s->initial=e->genericValue4;s->waitMs=e->genericValue5;s->flags=e->genericValue12;s->inactive=!!(e->flags&FL_INACTIVE);}
static qboolean StationApply(int n,const jarControllerState_t*s){
 if(!JAR_ControllerStateValid(ControllerKind(n),s))return qfalse;gentity_t*e=&g_entities[n];
 e->count=s->count;e->genericValue4=s->initial;e->genericValue5=s->waitMs;e->genericValue12=s->flags;
 StationPublish(e);e->use=StationUse;e->think=StationThink;
 if(s->inactive){e->flags|=FL_INACTIVE;StationStop(e);}else e->flags&=~FL_INACTIVE;
 return qtrue;
}
static qboolean StationInit(gentity_t*e){
 e->r.svFlags|=SVF_PLAYER_USABLE;e->s.shouldtarget=qtrue;e->s.owner=ENTITYNUM_NONE;
 int kind=ControllerKind(e->s.number);e->genericValue7=G_SoundIndex(kind==24||kind==27?"sound/interface/ammocon_done":"sound/interface/shieldcon_done");
 e->nextthink=level.time+50;e->genericValue8=level.time+100;
 jarControllerState_t s={0};s.count=s.initial=200;s.waitMs=100;return StationApply(e->s.number,&s);
}
