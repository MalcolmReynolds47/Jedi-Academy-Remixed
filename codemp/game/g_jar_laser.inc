// Portable target_laser: native trace/rendering, owned body and derived beam.
// Damage is authoritative and rechecks current editing/actor consent policy.
// GPL-2.0-only.
extern void JAR_MP_NativeLaserTrace(gentity_t*,int,trace_t*);
extern void target_laser_off(gentity_t*);
static int laserParent[MAX_GENTITIES];
static void LaserReset(void){memset(laserParent,0,sizeof(laserParent));}
static gentity_t*LaserHelper(int n){
 int h=objects[n].laserHelper-1;
 return h>=MAX_CLIENTS&&h<ENTITYNUM_WORLD&&g_entities[h].inuse&&laserParent[h]==n+1?&g_entities[h]:NULL;
}
static void LaserForget(int n){
 int parent=laserParent[n]-1;laserParent[n]=0;
 if(parent>=MAX_CLIENTS&&parent<ENTITYNUM_WORLD&&objects[parent].laserHelper==n+1){objects[parent].laserHelper=0;objects[parent].laserActive=0;++objects[parent].propertyRevision;}
 gentity_t*h=LaserHelper(n);objects[n].laserHelper=0;
 if(h){laserParent[h->s.number]=0;G_FreeEntity(h);}
}
static qboolean LaserVector(const vec3_t v){for(int a=0;a<3;++a)if(!isfinite(v[a])||fabsf(v[a])>131072)return qfalse;return qtrue;}
static qboolean LaserSync(int n,trace_t*out){
 gentity_t*e=&g_entities[n],*h=LaserHelper(n);if(!h)return qfalse;
 jarObject_t*o=&objects[n];vec3_t origin,direction;trace_t tr;
 if(!sandbox.integer||!o->laserActive||(e->flags&FL_INACTIVE)||o->copyPending)goto off;
 VectorCopy(e->r.currentOrigin,origin);origin[2]+=e->r.maxs[2]+2;
 if(!LaserVector(origin)||!LaserVector(e->r.currentAngles))goto off;
 if(o->output[0]){
  int target=o->output[0]-1;
  if(target==n||target<MAX_CLIENTS||target>=ENTITYNUM_WORLD||!ForcePermission(o->owner,target)||!objects[target].catalog||objects[target].copyPending||(g_entities[target].flags&FL_INACTIVE))goto off;
  gentity_t*t=&g_entities[target];vec3_t center;for(int a=0;a<3;++a)center[a]=t->r.currentOrigin[a]+.5f*(t->r.mins[a]+t->r.maxs[a]);
  if(!LaserVector(center))goto off;VectorSubtract(center,origin,direction);if(VectorNormalize(direction)<.001f)goto off;
 }else AngleVectors(e->r.currentAngles,direction,NULL,NULL);
 G_SetOrigin(h,origin);VectorCopy(origin,h->s.origin);VectorCopy(direction,h->movedir);h->enemy=NULL;
 JAR_MP_NativeLaserTrace(h,n,&tr);
 if(tr.startsolid||tr.allsolid||!LaserVector(tr.endpos))goto off;
 VectorCopy(tr.endpos,h->s.origin2);
 // The full beam bounds reach both endpoints' visibility clusters.
 for(int a=0;a<3;++a){float delta=tr.endpos[a]-origin[a];h->r.mins[a]=fminf(0,delta)-4;h->r.maxs[a]=fmaxf(0,delta)+4;}
 trap->LinkEntity((sharedEntity_t*)h);if(h->nextthink<=0)h->nextthink=level.time+FRAMETIME;
 if(out)*out=tr;return qtrue;
off:
 target_laser_off(h);return qfalse;
}
static qboolean HazardPlayerAllowed(int owner,int n){
 // A stationary hazard can hit its creator, unlike a Physgun self-grab.
 // Persisted server-owned hazards still honor each player's current consent.
 if(owner!=MAX_CLIENTS+1&&(owner<1||owner>MAX_CLIENTS||!g_entities[owner-1].inuse||!g_entities[owner-1].client||g_entities[owner-1].client->pers.connected!=CON_CONNECTED))return qfalse;
 return playerTools.integer&&!players[n].noTools&&level.time>=players[n].noGrabUntil;
}
static qboolean LaserDamageAllowed(int owner,int n){
 if(n<0||n>=ENTITYNUM_WORLD)return qfalse;
 if(n>=MAX_CLIENTS)return ForcePermission(owner,n);
 return HazardPlayerAllowed(owner,n)&&ActorAliveMode(&g_entities[n],qtrue);
}
static void LaserThink(gentity_t*h){
 int n=laserParent[h->s.number]-1;
 if(n<MAX_CLIENTS||n>=ENTITYNUM_WORLD||ControllerKind(n)!=13||LaserHelper(n)!=h)return;
 trace_t tr;if(!LaserSync(n,&tr))return;
 int hit=tr.entityNum;gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];
 if(!e->damage||tr.fraction>=1||hit<0||hit>=ENTITYNUM_WORLD||hit==n||!g_entities[hit].takedamage||!LaserDamageAllowed(o->owner,hit)||objects[hit].copyPending)return;
 gentity_t*attacker=e;int c=o->activationClient-1;
 if(c>=0&&c<MAX_CLIENTS&&o->activationEpoch==controllerClientEpoch[c]&&g_entities[c].inuse&&g_entities[c].client&&g_entities[c].client->pers.connected==CON_CONNECTED)attacker=&g_entities[c];
 // Damage may destroy this body/helper through chained explosions. Publish
 // and schedule first, then perform no further access after the native call.
 G_Damage(&g_entities[hit],e,attacker,h->movedir,tr.endpos,e->damage,DAMAGE_NO_KNOCKBACK,MOD_TARGET_LASER);
}
static void LaserState(int n,jarControllerState_t*s){s->damage=g_entities[n].damage;s->active=objects[n].laserActive;s->inactive=!!(g_entities[n].flags&FL_INACTIVE);}
static qboolean LaserApply(int n,const jarControllerState_t*s){
 if(!JAR_ControllerStateValid(13,s))return qfalse;
 gentity_t*h=LaserHelper(n);
 if(!h){if(!Room())return qfalse;h=G_TrySpawn();if(!h)return qfalse;h->classname="jar_laser_helper";laserParent[h->s.number]=n+1;objects[n].laserHelper=h->s.number+1;
  h->s.eType=ET_BEAM;h->s.pos.trType=TR_STATIONARY;h->r.contents=h->clipmask=0;h->think=LaserThink;}
 gentity_t*e=&g_entities[n];e->damage=s->damage;objects[n].laserActive=s->active;
 if(s->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
 LaserSync(n,NULL);return qtrue;
}
static void LaserUse(gentity_t*e,gentity_t*other,gentity_t*activator){
 jarControllerState_t s={0};LaserState(e->s.number,&s);s.active=!s.active;
 if(LaserApply(e->s.number,&s))SelectionChanged(e->s.number);
}
static qboolean LaserInit(gentity_t*e){jarControllerState_t s={0};s.damage=1;s.active=1;e->use=LaserUse;return LaserApply(e->s.number,&s);}
static void LaserFrame(void){for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==13)LaserSync(n,NULL);}
