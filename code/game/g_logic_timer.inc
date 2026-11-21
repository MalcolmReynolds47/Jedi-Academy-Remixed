// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
#include "g_logic_settings.h"
#include "g_subbsp_ownership.h"
#include "g_workshop.h"

/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
This should be renamed trigger_timer...
Repeatedly fires its targets.
Can be turned on or off by using.

"wait"			base time between triggering all targets, default is 1
"random"		wait variance, default is 0
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

*/
void func_timer_think( gentity_t *self ) {
 if(G_LogicKind(*self)==jar::logic::Timer&&G_JAR_WorkshopOwns(self)){
  jar::logic::State settings;
  if(!G_LogicRead(*self,settings))return;
  const int variation=settings.jitterMs?Q_irand(-settings.jitterMs,settings.jitterMs):0;
  self->nextthink=level.time+settings.delayMs+variation;
  if(settings.enabled)G_UseTargets(self,G_LogicActivator(*self,self->activator));
  return;
 }
	G_UseTargets (self, self->activator);
	// set time before next firing
	self->nextthink = level.time + 1000 * ( self->wait + Q_flrand(-1.0f, 1.0f) * self->random );
}

void func_timer_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
 if(G_LogicKind(*self)==jar::logic::Timer&&G_JAR_WorkshopOwns(self)){
  if(self->nextthink){
   G_LogicTimerStop(*self);
   return;
  }
  const auto lifetime=G_EntityLifetime(self);
  self->activator=activator&&activator->inuse?activator:nullptr;
  G_ActivateBehavior(self,BSET_USE);
  if(self->inuse&&lifetime==G_EntityLifetime(self))func_timer_think(self);
  return;
 }
	self->activator = activator;

	G_ActivateBehavior(self,BSET_USE);


	// if on, turn it off
	if ( self->nextthink ) {
		self->nextthink = 0;
		return;
	}

	// turn it on
	func_timer_think (self);
}

void SP_func_timer( gentity_t *self ) {
	G_SpawnFloat( "random", "1", &self->random);
	G_SpawnFloat( "wait", "1", &self->wait );

	self->e_UseFunc   = useF_func_timer_use;
	self->e_ThinkFunc = thinkF_func_timer_think;

	if ( self->random >= self->wait ) {
		self->random = self->wait - 1;//NOTE: was - FRAMETIME, but FRAMETIME is in msec (100) and these numbers are in *seconds*!
		gi.Printf( "func_timer at %s has random >= wait\n", vtos( self->s.origin ) );
	}

	if ( self->spawnflags & 1 ) {
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	self->svFlags = SVF_NOCLIENT;
}

