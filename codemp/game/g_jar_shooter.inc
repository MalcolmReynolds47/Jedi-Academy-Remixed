// Owned Sandbox shooters share native projectiles and resolve their aim each shot.
// GPL-2.0-only.
static int ShooterWeapon(int kind){
 switch(kind){
  case JAR_PLASMA_SHOOTER_CONTROLLER:return WP_BRYAR_PISTOL;
  case JAR_GRENADE_SHOOTER_CONTROLLER:return WP_THERMAL;
  case JAR_ROCKET_SHOOTER_CONTROLLER:return WP_ROCKET_LAUNCHER;
  case JAR_BLASTER_SHOOTER_CONTROLLER:return WP_BLASTER;
  default:return WP_NONE;
 }
}

unsigned JAR_MP_EntityEpoch(int n){
 return n>=0&&n<MAX_GENTITIES?controllerEntityEpoch[n]:0;
}

qboolean JAR_MP_ShooterSource(const gentity_t*source,unsigned*epoch){
 if(!source||!source->inuse||!sandbox.integer||(source->flags&FL_INACTIVE))return qfalse;
 const int n=source->s.number;
 if(!JAR_ControllerShooter(ControllerKind(n))||objects[n].copyPending)return qfalse;
 const int owner=objects[n].owner;
 if(owner<1||owner>MAX_CLIENTS+1)return qfalse;
 if(owner<=MAX_CLIENTS){
  const gentity_t*player=&g_entities[owner-1];
  if(!player->inuse||!player->client||player->client->pers.connected!=CON_CONNECTED)return qfalse;
 }
 if(epoch)*epoch=controllerEntityEpoch[n];
 return qtrue;
}

qboolean JAR_MP_ShooterDamageDenied(const gentity_t*missile,const gentity_t*target){
 if(!missile||!missile->jarShooterSource)return qfalse;
 if(missile->jarShooterSource<=MAX_CLIENTS||missile->jarShooterSource>ENTITYNUM_WORLD||
    !target||target->s.number<0||target->s.number>=ENTITYNUM_WORLD)return qtrue;
 const int n=missile->jarShooterSource-1;
 unsigned epoch;
 return !JAR_MP_ShooterSource(&g_entities[n],&epoch)||epoch!=missile->jarShooterEpoch||
  target->s.number==n||objects[target->s.number].copyPending||!LaserDamageAllowed(objects[n].owner,target->s.number);
}

static void ShooterStop(gentity_t*e){
 e->genericValue1=0;
 e->nextthink=0;
 e->think=NULL;
}

static void ShooterShot(gentity_t*e){
 const int n=e->s.number;
 vec3_t direction,target;
 qboolean linked=qfalse;
 const int destination=objects[n].output[0]-1;
 if(destination>=MAX_CLIENTS&&destination<ENTITYNUM_WORLD&&destination!=n){
  if(objects[destination].copyPending)return; // Wait for a pasted native aim target to initialize.
  if(ForcePermission(objects[n].owner,destination)){
   VectorCopy(g_entities[destination].r.currentOrigin,target);
   VectorSubtract(target,e->r.currentOrigin,direction);
   linked=VectorNormalize(direction)>.001f;
  }
 }
 if(!linked)AngleVectors(e->r.currentAngles,direction,NULL,NULL);
 JAR_MP_FireShooter(e,ShooterWeapon(ControllerKind(n)),(e->spawnflags&JAR_SHOOTER_ALT)!=0,
                   e->r.currentOrigin,direction,linked?target:NULL);
}

static void ShooterThink(gentity_t*e){
 if(!JAR_MP_ShooterSource(e,NULL)||!(e->spawnflags&JAR_SHOOTER_REPEAT)){
  ShooterStop(e);
  SelectionChanged(e->s.number);
  return;
 }
 const int jitter=e->genericValue3;
 const double offset=jitter?(double)Q_flrand(0,1)*(jitter-1):0;
 e->nextthink=JAR_NumberDeadline(level.time,e->genericValue2+offset);
 ShooterShot(e);
}

static void ShooterUse(gentity_t*e,gentity_t*other,gentity_t*activator){
 if(!JAR_MP_ShooterSource(e,NULL))return;
 if(e->genericValue1)ShooterStop(e);
 else if(e->spawnflags&JAR_SHOOTER_REPEAT){
  e->genericValue1=1;
  e->think=ShooterThink;
  ShooterThink(e);
 }else ShooterShot(e);
 SelectionChanged(e->s.number);
}

static void ShooterState(int n,jarControllerState_t*s){
 const gentity_t*e=&g_entities[n];
 s->waitMs=e->genericValue2;
 s->randomMs=e->genericValue3;
 s->flags=e->spawnflags;
 s->active=e->genericValue1;
 s->inactive=(e->flags&FL_INACTIVE)!=0;
}

static qboolean ShooterApply(int n,const jarControllerState_t*s){
 if(!JAR_ControllerStateValid(ControllerKind(n),s))return qfalse;
 gentity_t*e=&g_entities[n];
 e->genericValue2=s->waitMs;
 e->genericValue3=s->randomMs;
 e->spawnflags=s->flags;
 if(s->inactive)e->flags|=FL_INACTIVE;
 else e->flags&=~FL_INACTIVE;
 if(!s->active)ShooterStop(e);
 else {
  e->genericValue1=1;
  e->think=ShooterThink;
  if(!e->nextthink)e->nextthink=JAR_NumberDeadline(level.time,s->waitMs);
  const int latest=JAR_NumberDeadline(level.time,s->waitMs+s->randomMs);
  if(e->nextthink>latest)e->nextthink=latest;
 }
 return qtrue;
}

static qboolean ShooterInit(gentity_t*e,int kind){
 if(!JAR_MP_ShooterAssets(ShooterWeapon(kind)))return qfalse;
 jarControllerState_t defaults={0};
 defaults.waitMs=500;
 e->use=ShooterUse;
 return ShooterApply(e->s.number,&defaults);
}
