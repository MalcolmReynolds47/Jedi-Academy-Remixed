// Native magnetic-lock callbacks, extracted from g_misc.cpp.
// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
#include "g_maglock.h"
extern void G_JAR_WorkshopDetachChild(gentity_t*,gentity_t*);

void G_MaglockDetach(gentity_t*self){
 auto*door=self->activator;self->activator=nullptr;
 if(door&&door->inuse&&door->lockCount>0){--door->lockCount;if(!door->lockCount)door->svFlags&=~SVF_INACTIVE;}
}
bool G_MaglockAttach(gentity_t*self,gentity_t*door){
 if(!self||!self->inuse||!door||!door->inuse||door==self)return false;
 if(self->activator!=door){G_MaglockDetach(self);self->activator=door;++door->lockCount;}
 door->svFlags|=SVF_INACTIVE;
 self->e_ThinkFunc=thinkF_NULL;self->nextthink=0;
 const float radius=self->model&&!strcmp(self->model,"models/map_objects/kejim/sec_panel.md3")?12.0f:8.0f;
 VectorSet(self->mins,-radius,-radius,-radius);VectorSet(self->maxs,radius,radius,radius);self->contents=CONTENTS_CORPSE;
 self->flags|=FL_SHIELDED;self->takedamage=qtrue;self->health=10;self->e_DieFunc=dieF_maglock_die;
 gi.linkentity(self);return true;
}
void G_MaglockForget(gentity_t*ent){
 if(ent->classname&&!strcmp(ent->classname,"misc_maglock"))G_MaglockDetach(ent);
 if(ent->lockCount>0){
  for(int i=0;i<globals.num_entities;++i){auto&lock=g_entities[i];
   if(lock.inuse&&lock.classname&&!strcmp(lock.classname,"misc_maglock")&&lock.activator==ent)lock.activator=nullptr;
  }ent->lockCount=0;
 }
}
/*QUAKED misc_maglock (0 .5 .8) (-8 -8 -8) (8 8 8) x x x x x x x x
Place facing a door (using the angle, not a targetname) and it will lock that door.  Can only be destroyed by lightsaber and will automatically unlock the door it's attached to

NOTE: place these half-way in the door to make it flush with the door's surface.

"target"	thing to use when destoryed (not doors - it automatically unlocks the door it was angled at)
"health"	default is 10
*/
void maglock_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc )
{
	// Destruction unlocks and leaves the portable door; manual removal still
	// removes the complete owned setup. WP_Explode fires targets exactly once.
	G_JAR_WorkshopDetachChild(self,self->activator);
	G_MaglockDetach(self);
	self->e_DieFunc=dieF_NULL;self->owner=attacker;
	WP_Explode( self );
}
void SP_misc_maglock ( gentity_t *self )
{
	//NOTE: May have to make these only work on doors that are either untargeted
	//		or are targeted by a trigger, not doors fired off by scripts, counters
	//		or other such things?
	const bool portable=self->model&&!strcmp(self->model,"models/map_objects/kejim/sec_panel.md3");
	self->s.modelindex = G_ModelIndex(portable?self->model:"models/map_objects/imp_detention/door_lock.md3");
	self->fxID = G_EffectIndex(portable?"thermal/explosion":"maglock/explosion");

	G_SetOrigin( self, self->s.origin );

	self->e_ThinkFunc = thinkF_maglock_link;
	//FIXME: for some reason, when you re-load a level, these fail to find their doors...?  Random?  Testing an additional 200ms after the START_TIME_FIND_LINKS
	self->nextthink = level.time + START_TIME_FIND_LINKS+200;//START_TIME_FIND_LINKS;//because we need to let the doors link up and spawn their triggers first!
}
void maglock_link( gentity_t *self )
{
	//find what we're supposed to be attached to
	vec3_t	forward, start, end;
	trace_t	trace;

	AngleVectors( self->s.angles, forward, NULL, NULL );
	VectorMA( self->s.origin, 128, forward, end );
	VectorMA( self->s.origin, -4, forward, start );

	gi.trace( &trace, start, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

	if ( trace.allsolid || trace.startsolid )
	{
		gi.Printf("misc_maglock at %s in solid; optional lock removed\n", vtos(self->s.origin));
		G_FreeEntity( self );
		return;
	}
	if ( trace.fraction == 1.0 )
	{
		self->e_ThinkFunc = thinkF_maglock_link;
		self->nextthink = level.time + 100;
		return;
	}
	gentity_t *traceEnt = trace.entityNum>=0&&trace.entityNum<ENTITYNUM_WORLD?&g_entities[trace.entityNum]:nullptr;
	if ( !traceEnt || !traceEnt->inuse || !traceEnt->classname || Q_stricmp( "func_door", traceEnt->classname ) )
	{
		self->e_ThinkFunc = thinkF_maglock_link;
		self->nextthink = level.time + 100;
		return;
	}

	auto*door=G_FindDoorTrigger(traceEnt);if(!door)door=traceEnt;
	if(!G_MaglockAttach(self,door))return;

	//now position and orient it
	vectoangles( trace.plane.normal, end );
	G_SetOrigin( self, trace.endpos );
	G_SetAngles( self, end );

	gi.linkentity( self );
}

