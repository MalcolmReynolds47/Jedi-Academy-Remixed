#include "qcommon/jar_angular_trajectory.h"
#include "g_mover_motion.h"
#include "g_bsp_section.h"
#include "g_bsp_section_motion.h"
#include "g_sandbox_angular.h"
#include "g_angular_cargo.h"
#include "jar_physics.h"
#include <algorithm>
#include <cmath>
typedef struct {
	gentity_t	*ent;
	vec3_t	origin;
	vec3_t	angles;
	float	deltayaw;
	int ground;
	trType_t positionType;
} pushed_t;
pushed_t	pushed[MAX_GENTITIES], *pushed_p;

static void G_RestorePushedEntities(){
 while(pushed_p>pushed){
  const auto& saved=*--pushed_p;
  auto& entity=*saved.ent;
  VectorCopy(saved.origin,entity.s.pos.trBase);
  VectorCopy(saved.angles,entity.s.apos.trBase);
  if(entity.client){
   entity.client->ps.delta_angles[YAW]=saved.deltayaw;
   VectorCopy(saved.origin,entity.client->ps.origin);
  }
  VectorCopy(saved.origin,entity.currentOrigin);
  entity.s.pos.trType=saved.positionType;
  entity.s.groundEntityNum=saved.ground;
  gi.linkentity(&entity);
 }
}
// Id Software / Raven / Activision / OpenJK native mover pushing. GPL-2.0-only.
gentity_t	*G_TestEntityPosition( gentity_t *ent ) {
	trace_t	tr;
	int		mask;

	if ( (ent->client && ent->health <= 0) || !ent->clipmask )
	{//corpse or something with no clipmask
		mask = MASK_SOLID;
	}
	else
	{
		mask = ent->clipmask;
	}
	if ( ent->client )
	{
		gi.trace( &tr, ent->client->ps.origin, ent->mins, ent->maxs, ent->client->ps.origin, ent->s.number, mask, (EG2_Collision)0, 0 );
	}
	else
	{
		if ( ent->s.eFlags & EF_MISSILE_STICK )//Arggh, this is dumb...but when it used the bbox, it was pretty much always in solid when it is riding something..which is wrong..so I changed it to basically be a point contents check
		{
			gi.trace( &tr, ent->s.pos.trBase, vec3_origin, vec3_origin, ent->s.pos.trBase, ent->s.number, mask, (EG2_Collision)0, 0 );
		}
		else
		{
			gi.trace( &tr, ent->s.pos.trBase, ent->mins, ent->maxs, ent->s.pos.trBase, ent->s.number, mask, (EG2_Collision)0, 0 );
		}
	}

	if (tr.startsolid)
		return &g_entities[ tr.entityNum ];

	return NULL;
}


/*
==================
G_TryPushingEntity

Returns qfalse if the move is blocked
==================
*/
extern qboolean G_OkayToRemoveCorpse( gentity_t *self );

qboolean	G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove, bool callbacks=true, const float (*rotation)[3]=nullptr, bool exact=false ) {
	vec3_t		forward, right, up;
	vec3_t		org, org2, move2;
	gentity_t	*block;

	/*
	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->s.eFlags & EF_MOVER_STOP ) &&
		check->s.groundEntityNum != pusher->s.number ) {
		return qfalse;
	}
	*/

	// save off the old position
	if (pushed_p >= &pushed[MAX_GENTITIES]) {
		G_Error( "pushed_p > &pushed[MAX_GENTITIES]" );
	}
	pushed_p->ent = check;
	pushed_p->ground = check->s.groundEntityNum;
	pushed_p->positionType = check->s.pos.trType;
	VectorCopy (check->s.pos.trBase, pushed_p->origin);
	VectorCopy (check->s.apos.trBase, pushed_p->angles);
	if ( check->client ) {
		pushed_p->deltayaw = check->client->ps.delta_angles[YAW];
		VectorCopy (check->client->ps.origin, pushed_p->origin);
	}
	pushed_p++;
	const bool supported = (pushed_p-1)->ground==pusher->s.number ||
		(check->client && check->client->ps.groundEntityNum==pusher->s.number);

	// we need this for pushing things later
	VectorSubtract (vec3_origin, amove, org);
	AngleVectors (org, forward, right, up);

	// try moving the contacted entity
	VectorAdd (check->s.pos.trBase, move, check->s.pos.trBase);
	if (check->client) {
		// make sure the client's view rotates when on a rotating mover
		float yaw=amove[YAW];
        if(rotation){const float a=check->client->ps.viewangles[YAW]*M_PI/180.f;vec3_t facing={std::cos(a),std::sin(a),0};const float x=DotProduct(facing,rotation[0]),y=DotProduct(facing,rotation[1]);
            yaw=x*x+y*y>1e-10f?AngleSubtract(std::atan2(y,x)*180.f/M_PI,check->client->ps.viewangles[YAW]):0;}
        check->client->ps.delta_angles[YAW] += ANGLE2SHORT(yaw);
	}

	// figure movement due to the pusher's amove
	VectorSubtract (check->s.pos.trBase, pusher->currentOrigin, org);
	if(rotation){for(int a=0;a<3;++a)org2[a]=DotProduct(org,rotation[a]);}
    else {org2[0] = DotProduct (org, forward);
        org2[1] = -DotProduct (org, right);
        org2[2] = DotProduct (org, up);}
	VectorSubtract (org2, org, move2);
	VectorAdd (check->s.pos.trBase, move2, check->s.pos.trBase);
	if ( check->client ) {
		VectorAdd (check->client->ps.origin, move, check->client->ps.origin);
		VectorAdd (check->client->ps.origin, move2, check->client->ps.origin);
	}
	const bool portableDeck=G_JAR_PortableDeck(*pusher);
	const bool angularRider=(G_SandboxAngular(*pusher)||portableDeck)&&
		(check->client?check->client->ps.groundEntityNum:check->s.groundEntityNum)==pusher->s.number;
	if(angularRider&&(!portableDeck||VectorLengthSquared(amove))){
		// Match the server's projected box top as the crate tilts. The
		// native push above already carried the rider around the pivot.
		check->s.pos.trBase[2]=pusher->currentOrigin[2]+pusher->maxs[2]-check->mins[2]+.125f;
		if(check->client)check->client->ps.origin[2]=check->s.pos.trBase[2];
	}

	// may have pushed them off an edge
	if (angularRider) {
		check->s.groundEntityNum=pusher->s.number;
	} else if ( check->s.groundEntityNum != pusher->s.number ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
	}

	/*
	if ( check->client && check->health <= 0 && check->contents == CONTENTS_CORPSE )
	{//sigh... allow pushing corpses into walls... problem is, they'll be stuck in there... maybe just remove them?
		return qtrue;
	}
	*/
	block = G_TestEntityPosition( check );
	if (!block && supported && (portableDeck||(exact&&!VectorLengthSquared(amove)))) {
		// Test the rider's path with the supporting deck excluded. Pad turns
		// by the arc between the rider's old and new positions.
		trace_t path{};
		vec3_t lo,hi;VectorCopy(check->mins,lo);VectorCopy(check->maxs,hi);
		if(portableDeck&&VectorLengthSquared(amove)){
			const auto spin=jar::physics::AngularVelocity(
				{pusher->currentAngles[0]-amove[0],pusher->currentAngles[1]-amove[1],pusher->currentAngles[2]-amove[2]},
				{pusher->currentAngles[0],pusher->currentAngles[1],pusher->currentAngles[2]},1);
			vec3_t axis={spin.x,spin.y,spin.z},oldCenter;VectorSubtract(pusher->currentOrigin,move,oldCenter);
			const float angle=VectorNormalize(axis);
			const float sag=Distance((pushed_p-1)->origin,oldCenter)*(1-std::cos(.5f*angle));
			for(int i=0;i<3;++i){const float pad=sag*std::sqrt(std::max(0.f,1-axis[i]*axis[i]));lo[i]-=pad;hi[i]+=pad;}
		}
		gi.unlinkentity(pusher);
		gi.trace(&path, (pushed_p-1)->origin, lo, hi,
			check->client ? check->client->ps.origin : check->s.pos.trBase,
			check->s.number, MASK_SOLID, G2_NOCOLLIDE, 0);
		gi.linkentity(pusher);
		if (path.startsolid || path.allsolid || path.fraction<1) return qfalse;
	}
	if (!block) {
		// pushed ok
		if ( check->client ) {
			VectorCopy( check->client->ps.origin, check->currentOrigin );
		} else {
			VectorCopy( check->s.pos.trBase, check->currentOrigin );
		}
		gi.linkentity (check);
		return qtrue;
	}

	// if it is ok to leave in the old position, do it
	// this is only relevent for riding entities, not pushed
	// Sliding trapdoors can cause this.
	VectorCopy( (pushed_p-1)->origin, check->s.pos.trBase);
	if ( check->client ) {
		VectorCopy( (pushed_p-1)->origin, check->client->ps.origin);
	}
	VectorCopy( (pushed_p-1)->angles, check->s.apos.trBase );
	block = G_TestEntityPosition (check);
	if ( !block && !angularRider && (!exact || !supported) ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
        if(!callbacks&&check->client)check->client->ps.delta_angles[YAW]=(pushed_p-1)->deltayaw;
		pushed_p--;
		return qtrue;
	}

	// blocked
	if ( callbacks && pusher->damage )
	{//Do damage
		if ( (pusher->spawnflags&MOVER_CRUSHER)//a crusher
 			&& check->s.clientNum >= MAX_CLIENTS//not the player
			&& check->client //NPC
			&& check->health <= 0 //dead
			&& G_OkayToRemoveCorpse( check ) )//okay to remove him
		{//crusher stuck on a non->player corpse that does not have a key and is not running a script
			G_FreeEntity( check );
		}
		else
		{
			G_Damage(check, pusher, pusher->activator, move, check->currentOrigin, pusher->damage, 0, MOD_CRUSH );
		}
	}

	return qfalse;
}


/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
============
*/
qboolean G_MoverPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle, bool callbacks=true, const float (*rotation)[3]=nullptr, bool exact=false ) {
	qboolean	notMoving;
	int			i, e;
	int			listedEntities;
	vec3_t		mins, maxs;
	vec3_t		pusherMins, pusherMaxs, totalMins, totalMaxs;
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*check;

	*obstacle = NULL;
	const bool portableAngular=G_SandboxAngular(*pusher);
	const bool portableDeck=!callbacks&&G_JAR_PortableDeck(*pusher);
	const bool projectedHull=portableAngular||portableDeck;


	if ( !pusher->bmodel )
	{//misc_model_breakable
		VectorAdd( pusher->currentOrigin, pusher->mins, pusherMins );
		VectorAdd( pusher->currentOrigin, pusher->maxs, pusherMaxs );
	}

	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->currentAngles[0] || pusher->currentAngles[1] || pusher->currentAngles[2]
		|| amove[0] || amove[1] || amove[2] )
	{
		float		radius;

		radius = RadiusFromBounds( pusher->mins, pusher->maxs );
		for ( i = 0 ; i < 3 ; i++ )
		{
			mins[i] = pusher->currentOrigin[i] + move[i] - radius;
			maxs[i] = pusher->currentOrigin[i] + move[i] + radius;
			totalMins[i] = mins[i] - move[i];
			totalMaxs[i] = maxs[i] - move[i];
			if(!callbacks){
				totalMins[i]=std::min(totalMins[i],mins[i]);
				totalMaxs[i]=std::max(totalMaxs[i],maxs[i]);
			}
		}
	}
	else
	{
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = pusher->absmin[i] + move[i];
			maxs[i] = pusher->absmax[i] + move[i];
		}

		VectorCopy( pusher->absmin, totalMins );
		VectorCopy( pusher->absmax, totalMaxs );
		for (i=0 ; i<3 ; i++)
		{
			if ( move[i] > 0 )
			{
				totalMaxs[i] += move[i];
			}
			else
			{
				totalMins[i] += move[i];
			}
		}
	}

	// unlink the pusher so we don't get it in the entityList
	gi.unlinkentity( pusher );

	listedEntities = gi.EntitiesInBox( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to it's final position
	VectorAdd( pusher->currentOrigin, move, pusher->currentOrigin );
	VectorAdd( pusher->currentAngles, amove, pusher->currentAngles );
	if(portableAngular)G_SandboxAngularHull(pusher->currentAngles,pusher->mins,pusher->maxs);
	else if(portableDeck)G_JAR_ProjectedDeckHull(*pusher,pusher->currentAngles,pusher->mins,pusher->maxs);
	gi.linkentity( pusher );

	notMoving = (qboolean)(VectorCompare( vec3_origin, move )&&VectorCompare( vec3_origin, amove ));

	// see if any solid entities are inside the final position
	for ( e = 0 ; e < listedEntities ; e++ ) {
		check = entityList[ e ];

		if (callbacks && ( check->s.eFlags & EF_MISSILE_STICK ) && (notMoving || check->s.groundEntityNum < 0 || check->s.groundEntityNum >= ENTITYNUM_NONE ))
		{
			// special case hack for sticky things, destroy it if we aren't attached to the thing that is moving, but the moving thing is pushing us
			G_Damage( check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH );
			continue;
		}

		// only push items and players
		if ( check->s.eType != ET_ITEM )
		{
			//FIXME: however it allows items to be pushed through stuff, do same for corpses?
			if ( check->s.eType != ET_PLAYER )
			{
				if ( !( check->s.eFlags & EF_MISSILE_STICK ))
				{
					// cannot be pushed by this mover
					continue;
				}
			}
			/*
			else if ( check->health <= 0 )
			{//For now, don't push on dead players
				continue;
			}
			*/
			else if ( !pusher->bmodel && !projectedHull )
			{
				vec3_t	checkMins, checkMaxs;

				VectorAdd( check->currentOrigin, check->mins, checkMins );
				VectorAdd( check->currentOrigin, check->maxs, checkMaxs );

				if ( G_BoundsOverlap( checkMins, checkMaxs, pusherMins, pusherMaxs ) )
				{//They're inside me already, no push - FIXME: we're testing a moves spot, aren't we, so we could have just moved inside them?
					continue;
				}
			}
		}


		if ( check->maxs[0] - check->mins[0] <= 0 &&
				check->maxs[1] - check->mins[1] <= 0 &&
				check->maxs[2] - check->mins[2] <= 0 )
		{//no size, don't push
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		const int ground=projectedHull&&check->client?check->client->ps.groundEntityNum:check->s.groundEntityNum;
		const bool supported=ground==pusher->s.number ||
			(exact&&check->client&&check->client->ps.groundEntityNum==pusher->s.number);
		if ( !supported ) {
			// see if the ent needs to be tested
			if ( check->absmin[0] >= maxs[0]
			|| check->absmin[1] >= maxs[1]
			|| check->absmin[2] >= maxs[2]
			|| check->absmax[0] <= mins[0]
			|| check->absmax[1] <= mins[1]
			|| check->absmax[2] <= mins[2] ) {
				continue;
			}
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if ( G_TestEntityPosition( check ) != pusher )
			{
				continue;
			}
		}

		if ( ((pusher->spawnflags&2)&&!Q_stricmp("func_breakable",pusher->classname))
			||((pusher->spawnflags&16)&&!Q_stricmp("func_static",pusher->classname)) )
		{//ugh, avoid stricmp with a unique flag
			//Damage on impact
			if ( callbacks && pusher->damage )
			{//Do damage
				G_Damage( check, pusher, pusher->activator, move, check->currentOrigin, pusher->damage, 0, MOD_CRUSH );
				if ( pusher->health >= 0 && pusher->takedamage && !(pusher->spawnflags&1) )
				{//do some damage to me, too
					G_Damage( pusher, check, pusher->activator, move, pusher->s.pos.trBase, floor(pusher->damage/4.0f), 0, MOD_CRUSH );
				}
			}
		}
		// really need a flag like MOVER_TOUCH that calls the ent's touch function here, instead of this stricmp crap
		else if ( callbacks && (pusher->spawnflags&2) && !Q_stricmp( "func_rotating", pusher->classname ) )
		{
			GEntity_TouchFunc( pusher, check, NULL );
			continue;	// don't want it blocking so skip past it
		}

		vec3_t oldOrg;

		VectorCopy( check->s.pos.trBase, oldOrg );

		// the entity needs to be pushed
		// Vehicle seats and separately controlled bodies cannot be moved
		// independently by this pusher.
		const bool independent=(!exact&&!projectedHull)||(!check->m_pVehicle&&
			(!check->client||check->s.m_iVehicleNum<=0||check->s.m_iVehicleNum>=ENTITYNUM_WORLD)&&
			!((check->client?check->client->ps.eFlags:check->s.eFlags)&EF_FORCE_GRIPPED));
		if ( independent && G_TryPushingEntity( check, pusher, move, amove, callbacks, rotation, exact ) )
		{
			// the mover wasn't blocked
			if ( check->s.eFlags & EF_MISSILE_STICK )
			{
				if ( !VectorCompare( oldOrg, check->s.pos.trBase ))
				{
					// and the rider was actually pushed, so interpolate position change to smooth out the ride
					check->s.pos.trType = TR_INTERPOLATE;
					continue;
				}
				//else..the mover wasn't blocked & we are riding the mover & but the rider did not move even though the mover itself did...so
				//	drop through and let it blow up the rider up
			}
			else
			{
				continue;
			}
		}

		// the move was blocked even after pushing this rider
		if ( callbacks && (check->s.eFlags & EF_MISSILE_STICK) )
		{
			// so nuke 'em so they don't block us anymore
			G_Damage( check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH );
			continue;
		}

		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		G_RestorePushedEntities();
		return qfalse;
	}

	return qtrue;
}


static bool G_SandboxMoverPush(gentity_t& deck,vec3_t move,vec3_t angleDelta,const float (*rotation)[3],bool exact=false,const AngularCargo* staged=nullptr){
 pushed_p=pushed;
 gentity_t* obstacle=nullptr;
 if(staged){
  if(!G_MoverPush(&deck,move,angleDelta,&obstacle,false,rotation,exact))return false;
  if(G_AngularCargoCommit(deck,*staged))return true;
  G_RestorePushedEntities();
  return false;
 }
 if(!G_SandboxAngular(deck))return G_MoverPush(&deck,move,angleDelta,&obstacle,false,rotation,exact)!=qfalse;

 vec3_t origin,angles;
 VectorAdd(deck.currentOrigin,move,origin);VectorAdd(deck.currentAngles,angleDelta,angles);
 AngularCargo cargo;
 if(!G_AngularCargoPlan(deck,origin,angles,cargo))return false;
 {
  AngularCargoUnlinked unlinked(cargo);
  if(!G_MoverPush(&deck,move,angleDelta,&obstacle,false,rotation,exact))return false;
  if(!G_AngularCargoCommit(deck,cargo)){
   G_RestorePushedEntities();
   return false;
  }
 }
 G_SandboxChargesRefresh(cargo.charges);
 return true;
}

static void G_JAR_OffsetMoverPosition(gentity_t&e,const vec3_t move){
 VectorAdd(e.s.pos.trBase,move,e.s.pos.trBase);VectorAdd(e.s.origin,move,e.s.origin);
 if(e.next_roff_time>0){VectorAdd(e.pos1,move,e.pos1);} // ROFF pos2 holds angles.
 else if(e.s.eType==ET_MOVER&&!jar::bspSection::Portable(e)){VectorAdd(e.pos1,move,e.pos1);VectorAdd(e.pos2,move,e.pos2);}
}

static bool G_JAR_TranslatePortableDeck(gentity_t& e,const vec3_t destination,const vec3_t move){
 AngularCargo cargo;
 if(!G_AngularCargoPlan(e,destination,e.currentAngles,cargo))return false;
 {
  AngularCargoUnlinked riders(cargo);
  trace_t hit{};
  gi.trace(&hit,e.currentOrigin,e.mins,e.maxs,destination,e.s.number,MASK_SOLID,G2_NOCOLLIDE,0);
  if(hit.startsolid||hit.allsolid||hit.fraction<1)return false;
  vec3_t step,rotation{};VectorCopy(move,step);
  vec3_t previous;VectorCopy(e.currentOrigin,previous);
  if(!G_SandboxMoverPush(e,step,rotation,nullptr,true,&cargo)){
   VectorCopy(previous,e.currentOrigin);gi.linkentity(&e);return false;
  }
 }
 G_SandboxChargesRefresh(cargo.charges);
 G_JAR_OffsetMoverPosition(e,move);
 return true;
}

static bool G_JAR_MoverTranslateImpl(gentity_t&e,const vec3_t requested,vec3_t actual,bool exact){
 VectorClear(actual);vec3_t destination,lo,hi;VectorAdd(e.currentOrigin,requested,destination);
 for(float v:destination)if(!std::isfinite(v)||std::fabs(v)>65535)return false;
 if(G_JAR_PortableDeck(e)){
  if(!VectorLengthSquared(requested)||!G_JAR_TranslatePortableDeck(e,destination,requested))return false;
  VectorCopy(requested,actual);return true;
 }
 VectorCopy(e.mins,lo);VectorCopy(e.maxs,hi);
 if(e.bmodel&&(e.currentAngles[0]||e.currentAngles[1]||e.currentAngles[2])){
  // BSP bounds are local; sweep the enclosing rotated hull against scenery.
  vec3_t axis[3];AnglesToAxis(e.currentAngles,axis);VectorSet(lo,1e9f,1e9f,1e9f);VectorSet(hi,-1e9f,-1e9f,-1e9f);
  for(int corner=0;corner<8;++corner){vec3_t p={0,0,0};for(int a=0;a<3;++a)VectorMA(p,corner&(1<<a)?e.maxs[a]:e.mins[a],axis[a],p);for(int a=0;a<3;++a){lo[a]=std::min(lo[a],p[a]);hi[a]=std::max(hi[a],p[a]);}}
 }
 BSPSectionMotionScope sectionMotion(e);if(!sectionMotion)return false;
 // Physical contacts with actors are resolved by the pusher transaction below.
 trace_t hit{};gi.trace(&hit,e.currentOrigin,lo,hi,destination,e.s.number,sectionMotion.Mask(MASK_SOLID),(EG2_Collision)0,0);
 if(hit.startsolid||hit.allsolid||(exact&&hit.fraction<1))return false;
 vec3_t move,old,rotation={0,0,0};VectorSubtract(hit.endpos,e.currentOrigin,move);VectorCopy(e.currentOrigin,old);
 if(VectorLengthSquared(move)<1e-12f)return false;
 // The ordinary native push path carries riders and rolls them back on a
 // blocked push. This mode suppresses crushing/touch/sticky-missile callbacks.
 if(e.contents&(CONTENTS_SOLID|CONTENTS_BODY)){
  if(!G_SandboxMoverPush(e,move,rotation,nullptr,exact)){VectorCopy(old,e.currentOrigin);gi.linkentity(&e);return false;}
 }else {VectorAdd(e.currentOrigin,move,e.currentOrigin);gi.linkentity(&e);}
 VectorCopy(move,actual);G_JAR_OffsetMoverPosition(e,move);
 return hit.fraction>=1;
}

bool G_JAR_MoverTranslate(gentity_t&e,const vec3_t requested,vec3_t actual){
 return G_JAR_MoverTranslateImpl(e,requested,actual,false);
}

bool G_JAR_MoverRotateAt(gentity_t&e,const vec3_t angles,const vec3_t origin,const vec3_t pivot){
 for(int a=0;a<3;++a)if(!std::isfinite(angles[a])||std::fabs(angles[a])>100000)return false;
 vec3_t old,oldOrigin,move={0,0,0},delta,lo,hi;VectorCopy(e.currentAngles,old);VectorCopy(e.currentOrigin,oldOrigin);
 if(origin){
  for(int a=0;a<3;++a)if(!std::isfinite(origin[a])||std::fabs(origin[a])>65535)return false;
  VectorSubtract(origin,oldOrigin,move);
 }
 for(int a=0;a<3;++a)delta[a]=AngleSubtract(angles[a],old[a]);
 if(origin&&!VectorLengthSquared(delta)&&VectorLengthSquared(move)){
  vec3_t actual;
  return G_JAR_MoverTranslateImpl(e,move,actual,true);
 }
 vec3_t before[3],after[3];AnglesToAxis(old,before);AnglesToAxis(angles,after);float rotation[3][3];
 for(int i=0;i<3;++i)for(int j=0;j<3;++j){rotation[i][j]=0;for(int a=0;a<3;++a)rotation[i][j]+=after[a][i]*before[a][j];}
 VectorCopy(e.mins,lo);VectorCopy(e.maxs,hi);
 const bool portableAngular=G_SandboxAngular(e);
 const bool portableDeck=G_JAR_PortableDeck(e);
 if(e.bmodel||portableAngular||portableDeck){
  if(portableAngular||portableDeck){
   vec3_t nextMin,nextMax;
   if(portableDeck)G_JAR_ProjectedDeckHull(e,angles,nextMin,nextMax);
   else G_SandboxAngularHull(angles,nextMin,nextMax);
   for(int a=0;a<3;++a){lo[a]=std::min(lo[a],move[a]+nextMin[a]);hi[a]=std::max(hi[a],move[a]+nextMax[a]);}
  }else{
   VectorSet(lo,1e9f,1e9f,1e9f);VectorSet(hi,-1e9f,-1e9f,-1e9f);
   for(int end=0;end<2;++end)for(int corner=0;corner<8;++corner){
    vec3_t p;VectorCopy(end?move:vec3_origin,p);
    for(int a=0;a<3;++a)VectorMA(p,corner&(1<<a)?e.maxs[a]:e.mins[a],end?after[a]:before[a],p);
    for(int a=0;a<3;++a){lo[a]=std::min(lo[a],p[a]);hi[a]=std::max(hi[a],p[a]);}
   }
  }
  // Enclose the arc between corner endpoints, not just its final hull.
  const auto spin=jar::physics::AngularVelocity({old[0],old[1],old[2]},{angles[0],angles[1],angles[2]},1);vec3_t axis={spin.x,spin.y,spin.z};const float angle=VectorNormalize(axis);
  const float radius=RadiusFromBounds(e.mins,e.maxs)+(pivot?Distance(pivot,oldOrigin):0);
  const float sag=radius*(1-std::cos(angle*.5f));
  for(int a=0;a<3;++a){const float pad=sag*std::sqrt(std::max(0.f,1-axis[a]*axis[a]));lo[a]-=pad;hi[a]+=pad;}
 }
 BSPSectionMotionScope sectionMotion(e);if(!sectionMotion)return false;
 AngularCargo cargo;
 if(portableDeck&&!G_AngularCargoPlan(e,origin?origin:oldOrigin,angles,cargo))return false;
 {
  AngularCargoUnlinked riders(cargo);
  trace_t hit{};gi.trace(&hit,e.currentOrigin,lo,hi,e.currentOrigin,e.s.number,sectionMotion.Mask(MASK_SOLID),(EG2_Collision)0,0);if(hit.startsolid||hit.allsolid)return false;
  if(e.contents&(CONTENTS_SOLID|CONTENTS_BODY)){
   if(!G_SandboxMoverPush(e,move,delta,rotation,origin&&!pivot,portableDeck?&cargo:nullptr)){
    VectorCopy(oldOrigin,e.currentOrigin);
    VectorCopy(old,e.currentAngles);
    if(portableAngular)G_SandboxAngularHull(old,e.mins,e.maxs);
    else if(portableDeck)G_JAR_ProjectedDeckHull(e,old,e.mins,e.maxs);
    gi.linkentity(&e);return false;
   }
  }
 }
 if(portableDeck)G_SandboxChargesRefresh(cargo.charges);
 if(origin){VectorCopy(origin,e.currentOrigin);G_JAR_OffsetMoverPosition(e,move);}
 const bool angular=jar::angular::Rebase(e.s.apos,old,angles);
 VectorCopy(angles,e.currentAngles);for(int a=0;a<3;++a){if(!angular)e.s.apos.trBase[a]=AngleSubtract(e.s.apos.trBase[a]+delta[a],0);e.s.angles[a]=AngleSubtract(e.s.angles[a]+delta[a],0);if(e.next_roff_time>0)e.pos2[a]=AngleSubtract(e.pos2[a]+delta[a],0);}
 if(portableAngular)G_SandboxAngularMoved(e);
 gi.linkentity(&e);return true;
}
bool G_JAR_MoverRotate(gentity_t&e,const vec3_t angles){
 return G_JAR_MoverRotateAt(e,angles,nullptr,nullptr);
}
