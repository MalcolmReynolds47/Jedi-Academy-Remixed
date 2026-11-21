// Portable item and activator actions. Authored map callbacks remain native.
// GPL-2.0-only.
extern int G_ItemDisabled(gitem_t*);
static qboolean ActionActivator(gentity_t*e,gentity_t*a){
 if(!sandbox.integer||(e->flags&FL_INACTIVE)||objects[e->s.number].copyPending||!a||!a->inuse||!a->client||a->health<=0)return qfalse;
 int n=a->s.number;if(n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD)return ActorAlive(a)&&ForcePermission(objects[e->s.number].owner,n);
 if(n<0||n>=MAX_CLIENTS||a->client->pers.connected!=CON_CONNECTED||a->client->sess.sessionTeam==TEAM_SPECTATOR)return qfalse;
 // Use the same player consent rules as portable hazards, including seated players.
 return LaserDamageAllowed(objects[e->s.number].owner,n);
}
static void ConsumePickup(gentity_t*e){
 if(objects[e->s.number].heldBy)Drop(objects[e->s.number].heldBy-1,qfalse);
 objects[e->s.number].itemConsumed=1;++objects[e->s.number].propertyRevision;
 e->nextthink=0;e->think=NULL;e->touch=NULL;e->use=NULL;e->r.contents=0;e->r.svFlags|=SVF_NOCLIENT;e->s.eFlags|=EF_NODRAW;e->s.eFlags&=~EF_ITEMPLACEHOLDER;
 e->s.pos.trType=TR_STATIONARY;e->s.groundEntityNum=ENTITYNUM_WORLD;VectorClear(e->s.pos.trDelta);
 e->freeAfterEvent=e->unlinkAfterEvent=qfalse;trap->UnlinkEntity((sharedEntity_t*)e);
}
qboolean JAR_MP_GiveTarget(gentity_t*e,gentity_t*a){
 if(!e||ControllerKind(e->s.number)!=16)return qfalse;
 if(!ActionActivator(e,a))return qtrue;
 int source=e->s.number;unsigned identity=objects[source].order;jarObject_t*o=&objects[source];
 for(int output=0;output<2;++output){
  int n=o->output[output]-1;
  if(n<MAX_CLIENTS||n>=ENTITYNUM_WORLD||(output&&o->output[0]==o->output[1])||!ForcePermission(o->owner,n)||objects[n].copyPending||objects[n].itemConsumed)continue;
  gentity_t*t=&g_entities[n];gitem_t*item=t->item;
  if(!objects[n].catalog||jarMPCatalog[objects[n].catalog-1].kind!=4||!item||t->touch!=Touch_Item||(t->s.eFlags&(EF_NODRAW|EF_ITEMPLACEHOLDER))||G_ItemDisabled(item))continue;
  if(item->giType==IT_WEAPON){int weapon=item->giTag,disabled=(level.gametype==GT_DUEL||level.gametype==GT_POWERDUEL)?g_duelWeaponDisable.integer:g_weaponDisable.integer;
   if(weapon<=WP_NONE||weapon>=WP_NUM_WEAPONS||(level.gametype!=GT_JEDIMASTER&&(disabled&(1<<weapon))))continue;}
  unsigned targetOrder=objects[n].order;trace_t trace={0};Touch_Item(t,a,&trace);
  // Native pickup eligibility/events run first. A refused/full pickup stays available.
  if(t->inuse&&objects[n].order==targetOrder&&(t->s.eFlags&(EF_NODRAW|EF_ITEMPLACEHOLDER)))ConsumePickup(t);
  if(!e->inuse||objects[source].order!=identity||!ActionActivator(e,a))return qtrue;
 }
 return qtrue;
}
qboolean JAR_MP_ActionTarget(gentity_t*e,gentity_t*a){
 int kind=e?ControllerKind(e->s.number):0;if(kind!=17&&kind!=18)return qfalse;
 if(!ActionActivator(e,a))return qtrue;
 if(kind==17){
  int points=e->count,score=a->client->ps.persistant[PERS_SCORE];
  if(points< -1000||points>1000||(points>0&&score>INT_MAX-points)||(points<0&&score<INT_MIN-points))return qtrue;
  if(level.gametype==GT_TEAM){int team=a->client->ps.persistant[PERS_TEAM];if(team<0||team>=TEAM_NUM_TEAMS)return qtrue;int total=level.teamScores[team];if((points>0&&total>INT_MAX-points)||(points<0&&total<INT_MIN-points))return qtrue;}
  AddScore(a,e->r.currentOrigin,points);return qtrue;
 }
 // MP has its own electrical damage enum and replicated electrify timer.
 int mod=e->spawnflags==1?MOD_FALLING:e->spawnflags==2?MOD_DEMP2:MOD_UNKNOWN;
 if(e->spawnflags==2)a->client->ps.electrifyTime=level.time+4000;
 G_Damage(a,NULL,NULL,NULL,NULL,100000,DAMAGE_NO_PROTECTION,mod);return qtrue;
}
