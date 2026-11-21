// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
#include "g_logic_settings.h"
#include "g_subbsp_ownership.h"
#include "g_workshop.h"
#include <algorithm>
#include <limits>

#include "g_target_delay.inc"

/*QUAKED target_counter (1.0 0 0) (-4 -4 -4) (4 4 4) x x x x x x x INACTIVE
Acts as an intermediary for an action that takes multiple inputs.

INACTIVE cannot be used until used by a target_activate

target2 - what the counter should fire each time it's incremented and does NOT reach it's count

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.

bounceCount - number of times the counter should reset to it's full count when it's done
*/

void target_counter_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
 if(G_LogicKind(*self)==jar::logic::Counter&&G_JAR_WorkshopOwns(self)){
  jar::logic::State value;
  if(!G_LogicRead(*self,value)||!value.enabled||!value.remaining)return;
  if(--self->count){
   G_UseTargets2(self,G_LogicActivator(*self,activator),self->target2);
   return;
  }
  if(value.repeat)self->count=value.hits;
  const auto lifetime=G_EntityLifetime(self);
  const auto activatorLifetime=activator?G_EntityLifetime(activator):0;
  G_ActivateBehavior(self,BSET_USE);
  if(self->inuse&&G_EntityLifetime(self)==lifetime){
   if(activator&&(!activator->inuse||G_EntityLifetime(activator)!=activatorLifetime))activator=nullptr;
   G_UseTargets(self,G_LogicActivator(*self,activator));
  }
  return;
 }
	if ( self->count == 0 )
	{
		return;
	}

	//gi.Printf("target_counter %s used by %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
	self->count--;

	if ( activator )
	{
		Quake3Game()->DebugPrint( IGameInterface::WL_VERBOSE, "target_counter %s used by %s (%d/%d)\n", self->targetname, activator->targetname, (self->max_health-self->count), self->max_health );
	}

	if ( self->count )
	{
		if ( self->target2 )
		{
			//gi.Printf("target_counter %s firing target2 from %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
			G_UseTargets2( self, activator, self->target2 );
		}
		return;
	}

	G_ActivateBehavior( self,BSET_USE );

	if ( self->spawnflags & 128 )
	{
		self->svFlags |= SVF_INACTIVE;
	}

	self->activator = activator;
	G_UseTargets( self, activator );

	if ( self->count == 0 )
	{
		if ( self->bounceCount == 0 )
		{
			return;
		}
		self->count = self->max_health;
		if ( self->bounceCount > 0 )
		{//-1 means bounce back forever
			self->bounceCount--;
		}
	}
}

void SP_target_counter (gentity_t *self)
{
	self->wait = -1;
	if (!self->count)
	{
		self->count = 2;
	}
	//if ( self->bounceCount > 0 )//let's always set this anyway
	{//we will reset when we use up our count, remember our initial count
		self->max_health = self->count;
	}

	self->e_UseFunc = useF_target_counter_use;
}


#include "g_target_random.inc"

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score (gentity_t *ent, gentity_t *other, gentity_t *activator)
{
 if(G_LogicKind(*ent)==jar::logic::Score&&G_JAR_WorkshopOwns(ent)){
  jar::logic::State settings;
  if(!G_LogicRead(*ent,settings)||!settings.enabled||!G_LogicLiveActor(activator))return;
  const auto actorLife=G_EntityLifetime(activator);
  G_ActivateBehavior(ent,BSET_USE);
  if(!G_LogicLiveActor(activator)||G_EntityLifetime(activator)!=actorLife)return;
  const int current=activator->client->ps.persistant[PERS_SCORE];
  const int64_t total=int64_t(current)+settings.points;
  // SP snapshots encode persistent player stats as signed shorts.
  const int bounded=int(std::min<int64_t>(std::numeric_limits<int16_t>::max(),
                         std::max<int64_t>(std::numeric_limits<int16_t>::min(),total)));
  AddScore(activator,bounded-current);
  if(activator->s.number==0)gi.SendServerCommand(0,"cp \"Score: %d\"",bounded);
  return;
 }
	G_ActivateBehavior(ent,BSET_USE);

	AddScore( activator, ent->count );
}

void SP_target_score( gentity_t *ent ) {
	if ( !ent->count ) {
		ent->count = 1;
	}
	ent->e_UseFunc = useF_Use_Target_Score;
}


/*QUAKED target_relay (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM x x x x INACTIVE
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them

INACTIVE  Can't be used until activated

  "delay" - Will actually fire this many seconds after being used
  "wait" - Cannot be fired again until this many seconds after the last time it was used
*/
void target_relay_use_go (gentity_t *self )
{
 if(G_LogicKind(*self)==jar::logic::Relay&&G_JAR_WorkshopOwns(self)){
  jar::logic::State settings;
  auto* actor=self->activator;
  self->activator=nullptr;
  if(!G_LogicRead(*self,settings)||!settings.enabled||!settings.remaining)return;
  self->painDebounceTime=level.time+settings.cooldownMs;
  if(!settings.repeat)self->count=1;
  const auto sourceLife=G_EntityLifetime(self),actorLife=G_EntityLifetime(actor);
  G_ActivateBehavior(self,BSET_USE);
  if(!self->inuse||G_EntityLifetime(self)!=sourceLife)return;
  if(G_EntityLifetime(actor)!=actorLife)actor=nullptr;
  G_UseTargets(self,G_LogicActivator(*self,actor));
  return;
 }
	G_ActivateBehavior( self, BSET_USE );

	if ( self->spawnflags & 4 )
	{
		gentity_t	*ent;

		ent = G_PickTarget( self->target );
		if ( ent && (ent->e_UseFunc != useF_NULL) )
		{	// e_UseFunc check can be omitted
			GEntity_UseFunc( ent, self, self->activator );
		}
		return;
	}

	G_UseTargets( self, self->activator );
}

void target_relay_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
 if(G_LogicKind(*self)==jar::logic::Relay&&G_JAR_WorkshopOwns(self)){
  jar::logic::State settings;
  if(!G_LogicRead(*self,settings)||!settings.enabled||!settings.remaining||
     self->nextthink>0||self->painDebounceTime>level.time)return;
  self->activator=activator&&activator->inuse?activator:nullptr;
  if(settings.delayMs){
   self->e_ThinkFunc=thinkF_target_relay_use_go;
   self->nextthink=level.time+settings.delayMs;
  }else target_relay_use_go(self);
  return;
 }
	if ( ( self->spawnflags & 1 ) && activator && activator->client )
	{//&& activator->client->ps.persistant[PERS_TEAM] != TEAM_RED ) {
		return;
	}

	if ( ( self->spawnflags & 2 ) && activator && activator->client )
	{//&& activator->client->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return;
	}

	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if ( self->painDebounceTime > level.time )
	{
		return;
	}

	G_SetEnemy( self, other );
	self->activator = activator;

	if ( self->delay )
	{
		self->e_ThinkFunc = thinkF_target_relay_use_go;
		self->nextthink = level.time + self->delay;
		return;
	}

	target_relay_use_go( self );

	if ( self->wait < 0 )
	{
		self->e_UseFunc = useF_NULL;
	}
	else
	{
		self->painDebounceTime = level.time + self->wait;
	}
}

void SP_target_relay (gentity_t *self)
{
	self->e_UseFunc = useF_target_relay_use;
	self->wait *= 1000;
	self->delay *= 1000;
	if ( self->spawnflags&128 )
	{
		self->svFlags |= SVF_INACTIVE;
	}
}


//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8) FALLING ELECTRICAL
Kills the activator.
*/
void target_kill_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if (!activator) return;

 const bool portable=G_LogicKind(*self)==jar::logic::Kill&&G_JAR_WorkshopOwns(self);
 const auto actorLife=portable?G_EntityLifetime(activator):0;
 const int mode=self->spawnflags;
 if(portable&&((self->svFlags&SVF_INACTIVE)||!G_LogicLiveActor(activator)))return;
	G_ActivateBehavior(self,BSET_USE);
 if(portable&&(!G_LogicLiveActor(activator)||G_EntityLifetime(activator)!=actorLife))return;
 const int flags=portable?mode:self->spawnflags;

	if ( flags & 1 )
	{//falling death
		G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_FALLING );
		if ( (!portable||(activator->inuse&&G_EntityLifetime(activator)==actorLife))&&
       !activator->s.number && activator->health <= 0 )
		{
			extern void CGCam_Fade( vec4_t source, vec4_t dest, float duration );
			float	src[4] = {0,0,0,0},dst[4]={0,0,0,1};
			CGCam_Fade( src, dst, 10000 );
		}
	}
	else if ( flags & 2 ) // electrical
	{
		G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_ELECTROCUTE );

		if ( (!portable||(activator->inuse&&G_EntityLifetime(activator)==actorLife))&&activator->client )
		{
			activator->s.powerups |= ( 1 << PW_SHOCKED );
			activator->client->ps.powerups[PW_SHOCKED] = level.time + 4000;
		}
	}
	else
	{
		G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_UNKNOWN);
	}
}

void SP_target_kill( gentity_t *self )
{
	self->e_UseFunc = useF_target_kill_use;
}


/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_Give( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
 if(G_LogicKind(*ent)==jar::logic::Give&&G_JAR_WorkshopOwns(ent)){
  jar::logic::State settings;
  if(!G_LogicRead(*ent,settings)||!settings.enabled||!G_LogicLiveActor(activator))return;
  const auto sourceLife=G_EntityLifetime(ent),actorLife=G_EntityLifetime(activator);
  G_ActivateBehavior(ent,BSET_USE);
  trace_t trace{};
  for(int slot=0;slot<jar::logic::outputCount;++slot){
   if(!ent->inuse||G_EntityLifetime(ent)!=sourceLife||!G_LogicLiveActor(activator)||
      G_EntityLifetime(activator)!=actorLife)return;
   const char* name=G_LogicOutput(*ent,slot);
   auto* pickup=name?G_Find(nullptr,FOFS(targetname),name):nullptr;
   if(!pickup||!G_LogicPickup(*pickup)||!G_JAR_WorkshopOwns(pickup))continue;
   bool duplicate=false;
   for(int previous=0;previous<slot;++previous){
    const char* earlier=G_LogicOutput(*ent,previous);
    if(earlier&&!strcmp(earlier,name)){duplicate=true;break;}
   }
   if(!duplicate)Touch_Item(pickup,activator,&trace);
   // Native pickup can reject or free the entity. Its callback owns disposal.
  }
  return;
 }
	gentity_t	*t;
	trace_t		trace;

	if ( !activator || !activator->client ) {
		return;
	}

	if ( !ent->target ) {
		return;
	}

	G_ActivateBehavior(ent,BSET_USE);

	memset( &trace, 0, sizeof( trace ) );
	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), ent->target)) != NULL ) {
		if ( !t->item ) {
			continue;
		}
		Touch_Item( t, activator, &trace );

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		gi.unlinkentity( t );
	}
}

void SP_target_give( gentity_t *ent ) {
	ent->e_UseFunc = useF_Use_Target_Give;
}

