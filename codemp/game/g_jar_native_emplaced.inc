// Native emplaced chair and Sandbox backend. GPL-2.0-only.
#include "jar_mp_emplaced.h"
#include "g_jar_native_eweb.h"

#define EMPLACED_CANRESPAWN 1
#define EMPLACED_GUN_HEALTH 800

static gentity_t *PortableEmplacedOwner(const gentity_t *gun)
{
	const int client = gun->s.emplacedOwner;
	if (client < 0 || client >= MAX_CLIENTS ||
		gun->s.activeForcePass != NUM_FORCE_POWERS+1 || gun->activator != &g_entities[client])
		return NULL;

	gentity_t *user = &g_entities[client];
	if (!user->inuse || !user->client || user->s.number != client ||
		user->client->ps.clientNum != client || user->client->pers.connected != CON_CONNECTED ||
		user->r.ownerNum != gun->s.number)
		return NULL;
	return user;
}

qboolean JAR_MP_NativeEmplacedOccupied(const gentity_t *gun)
{
	if (!gun || !gun->inuse || !JAR_MP_EmplacedState(&gun->s))
		return qfalse;
	const gentity_t *user = PortableEmplacedOwner(gun);
	return user && user->health > 0 && user->client->ps.stats[STAT_HEALTH] > 0 &&
		user->client->ps.emplacedIndex == gun->s.number &&
		user->client->ps.pm_type == PM_NORMAL && !user->client->ps.trueJedi;
}

void JAR_MP_NativeEmplacedDetach(gentity_t *gun)
{
	if (!gun || !gun->inuse || !JAR_MP_EmplacedState(&gun->s))
		return;
	gentity_t *user = PortableEmplacedOwner(gun);
	const qboolean hadSeat = gun->activator || gun->s.activeForcePass == NUM_FORCE_POWERS+1;
	const qboolean eweb = JAR_MP_EWebState(&gun->s);
	if (user)
	{
		playerState_t *ps = &user->client->ps;
		// Pmove can clear the seat before the server receives the dismount.
		if (ps->emplacedIndex == gun->s.number || !ps->emplacedIndex)
		{
			if (!gun->genericValue7)
				ps->stats[STAT_WEAPONS] &= ~(1u << WP_EMPLACED_GUN);
			if (ps->weapon == WP_EMPLACED_GUN)
			{
				const int previous = eweb ? gun->genericValue11 : gun->s.weapon;
				ps->weapon = previous > WP_NONE && previous < WP_NUM_WEAPONS && previous < 32 &&
					((unsigned)ps->stats[STAT_WEAPONS] & (1u << previous)) ? previous : WP_NONE;
				ps->weaponstate = WEAPON_READY;
				ps->weaponTime = 0;
			}
			if (user->s.weapon == WP_EMPLACED_GUN)
				user->s.weapon = ps->weapon;
			ps->emplacedTime = level.time + 1000;
			ps->emplacedIndex = 0;
			ps->saberHolstered = gun->genericValue6;
			user->r.ownerNum = ENTITYNUM_NONE;
		}
	}
	gun->activator = NULL;
	gun->genericValue1 = 0;
	gun->genericValue6 = gun->genericValue7 = 0;
	gun->s.emplacedOwner = ENTITYNUM_NONE;
	gun->s.activeForcePass = 0;
	gun->s.weapon = eweb ? WP_NONE : WP_EMPLACED_GUN;
	if (eweb)
	{
		gun->genericValue11 = 0;
		if (hadSeat)
			JAR_MP_NativeEWebDismounted(gun);
	}
	if (hadSeat)
		JAR_MP_EmplacedChanged(gun);
}

static void PortableEmplacedAttach(gentity_t *gun, gentity_t *user)
{
	playerState_t *ps = &user->client->ps;
	gun->genericValue1 = 1;
	gun->genericValue6 = ps->saberHolstered;
	gun->genericValue7 = !!((unsigned)ps->stats[STAT_WEAPONS] & (1u << WP_EMPLACED_GUN));
	if (JAR_MP_EWebState(&gun->s))
		gun->genericValue11 = ps->weapon;
	else
		gun->s.weapon = ps->weapon;
	ps->weapon = WP_EMPLACED_GUN;
	ps->weaponstate = WEAPON_READY;
	ps->stats[STAT_WEAPONS] |= 1u << WP_EMPLACED_GUN;
	ps->emplacedIndex = gun->s.number;
	gun->s.emplacedOwner = user->s.number;
	gun->s.activeForcePass = NUM_FORCE_POWERS+1;
	user->s.weapon = WP_EMPLACED_GUN;
	user->r.ownerNum = gun->s.number;
	gun->activator = user;
	JAR_MP_EmplacedChanged(gun);
}

void JAR_MP_NativeEmplacedDetachUser(gentity_t *user)
{
	if (!user || !user->client)
		return;
	const int index = user->client->ps.emplacedIndex ? user->client->ps.emplacedIndex : user->r.ownerNum;
	if (index >= MAX_CLIENTS && index < ENTITYNUM_WORLD && g_entities[index].activator == user)
		JAR_MP_NativeEmplacedDetach(&g_entities[index]);
}

//----------------------------------------------------------

/*QUAKED emplaced_gun (0 0 1) (-30 -20 8) (30 20 60) CANRESPAWN

 count - if CANRESPAWN spawnflag, decides how long it is before gun respawns (in ms)
 constraint - number of degrees gun is constrained from base angles on each side (default 60.0)

 showhealth - set to 1 to show health bar on this entity when crosshair is over it

  teamowner - crosshair shows green for this team, red for opposite team
	0 - none
	1 - red
	2 - blue

  alliedTeam - team that can use this
	0 - any
	1 - red
	2 - blue

  teamnodmg - team that turret does not take damage from or do damage to
	0 - none
	1 - red
	2 - blue
*/

//----------------------------------------------------------
extern qboolean TryHeal(gentity_t *ent, gentity_t *target); //g_utils.c
void emplaced_gun_use( gentity_t *self, gentity_t *other, trace_t *trace )
{
	vec3_t fwd1, fwd2;
	float dot;
	int oldWeapon;
	gentity_t *activator = other;
	float zoffset = 50;
	vec3_t anglesToOwner;
	vec3_t vLen;
	float ownLen;
	const qboolean portable = JAR_MP_EmplacedState(&self->s);
	const float *origin = portable ? self->r.currentOrigin : self->s.origin;

	if ( self->health <= 0 )
	{ //gun is destroyed
		return;
	}

	if (self->activator)
	{ //someone is already using me
		return;
	}

	if (!activator->client)
	{
		return;
	}

	if (activator->client->ps.emplacedTime > level.time)
	{ //last use attempt still too recent
		return;
	}

	if (activator->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{ //don't use if busy doing something else
		return;
	}

	if (activator->client->ps.origin[2] > origin[2]+zoffset-8)
	{ //can't use it from the top
		return;
	}

	if (activator->client->ps.pm_flags & PMF_DUCKED)
	{ //must be standing
		return;
	}

	if (activator->client->ps.isJediMaster)
	{ //jm can't use weapons
		return;
	}

	VectorSubtract(origin, activator->client->ps.origin, vLen);
	ownLen = VectorLength(vLen);

	if (ownLen > 64.0f)
	{ //must be within 64 units of the gun to use at all
		return;
	}

	// Let's get some direction vectors for the user
	AngleVectors( activator->client->ps.viewangles, fwd1, NULL, NULL );

	// Get the guns direction vector
	AngleVectors( self->pos1, fwd2, NULL, NULL );

	dot = DotProduct( fwd1, fwd2 );

	// Must be reasonably facing the way the gun points ( 110 degrees or so ), otherwise we don't allow to use it.
	if ( dot < -0.2f )
	{
		goto tryHeal;
	}

	VectorSubtract(origin, activator->client->ps.origin, fwd1);
	VectorNormalize(fwd1);

	dot = DotProduct( fwd1, fwd2 );

	//check the positioning in relation to the gun as well
	if ( dot < 0.6f )
	{
		goto tryHeal;
	}

	if (portable)
	{
		PortableEmplacedAttach(self, activator);
		return;
	}
	self->genericValue1 = 1;
	oldWeapon = activator->s.weapon;

	// swap the users weapon with the emplaced gun
	activator->client->ps.weapon = self->s.weapon;
	activator->client->ps.weaponstate = WEAPON_READY;
	activator->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_EMPLACED_GUN );

	activator->client->ps.emplacedIndex = self->s.number;

	self->s.emplacedOwner = activator->s.number;
	self->s.activeForcePass = NUM_FORCE_POWERS+1;

	// the gun will track which weapon we used to have
	self->s.weapon = oldWeapon;

	//user's new owner becomes the gun ent
	activator->r.ownerNum = self->s.number;
	self->activator = activator;

	VectorSubtract(self->r.currentOrigin, activator->client->ps.origin, anglesToOwner);
	vectoangles(anglesToOwner, anglesToOwner);
	return;

tryHeal: //well, not in the right dir, try healing it instead...
	TryHeal(activator, self);
}

void emplaced_gun_realuse( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	emplaced_gun_use(self, other, NULL);
}

qboolean JAR_MP_NativeEmplacedUse(gentity_t *gun, gentity_t *user)
{
	if (!gun || !gun->inuse || !JAR_MP_EmplacedState(&gun->s) || !user || !user->inuse ||
		!user->client || user->s.number < 0 || user->s.number >= MAX_CLIENTS ||
		user != &g_entities[user->s.number] || user->client->ps.clientNum != user->s.number ||
		user->client->pers.connected != CON_CONNECTED || user->client->ps.emplacedIndex ||
		user->client->ps.pm_type != PM_NORMAL || user->client->ps.trueJedi ||
		user->client->ewebIndex || user->client->ps.weapon == WP_EMPLACED_GUN ||
		!JAR_MP_EmplacedAllowsSeat(gun, user))
		return qfalse;
	if (gun->activator && !JAR_MP_NativeEmplacedOccupied(gun))
		JAR_MP_NativeEmplacedDetach(gun);
	JAR_MP_NativeEmplacedDetachUser(user);
	if (JAR_MP_EWebState(&gun->s))
	{
		if (!gun->activator && JAR_MP_NativeEWebSeat(gun, user))
			PortableEmplacedAttach(gun, user);
	}
	else
		emplaced_gun_use(gun, user, NULL);
	return gun->activator == user && JAR_MP_NativeEmplacedOccupied(gun);
}

//----------------------------------------------------------
void emplaced_gun_pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if (JAR_MP_EmplacedState(&self->s))
	{
		G_ScaleNetHealth(self);
		JAR_MP_EmplacedChanged(self);
		return;
	}
	self->s.health = self->health;

	if ( self->health <= 0 )
	{
		//death effect.. for now taken care of on cgame
	}
	else
	{
		//if we have a pain behavior set then use it I guess
		G_ActivateBehavior( self, BSET_PAIN );
	}
}

gentity_t *JAR_MP_NativeEmplacedUpdateSeat(gentity_t *gun)
{
	gentity_t *user = gun->activator;
	const qboolean eweb = JAR_MP_EWebState(&gun->s);
	const float maxDistance = eweb ? 80.0f : 64.0f;
	if (!user)
		return NULL;
	if (!JAR_MP_NativeEmplacedOccupied(gun) || !JAR_MP_EmplacedAllowsSeat(gun, user) ||
		DistanceSquared(gun->r.currentOrigin, user->client->ps.origin) > maxDistance * maxDistance)
	{
		JAR_MP_NativeEmplacedDetach(gun);
		return NULL;
	}
	if (!(user->client->pers.cmd.buttons & BUTTON_USE))
		gun->genericValue1 = 0;
	else if (!gun->genericValue1 && (!eweb || level.time >= gun->genericValue8))
	{
		JAR_MP_NativeEmplacedDetach(gun);
		return NULL;
	}
	user->client->ps.weapon = WP_EMPLACED_GUN;
	user->client->ps.weaponstate = WEAPON_READY;
	return user;
}

static void EmplacedEffect(qboolean portable, int effect, vec3_t origin, vec3_t direction)
{
	if (!portable)
	{
		G_PlayEffect(effect, origin, direction);
		return;
	}
	gentity_t *event = G_TrySpawn();
	if (!event)
		return;
	event->classname = "jar_emplaced_effect";
	event->s.eType = ET_EVENTS + EV_PLAY_EFFECT;
	event->eventTime = level.time;
	event->freeAfterEvent = qtrue;
	event->s.eventParm = effect;
	VectorCopy(origin, event->s.origin);
	VectorCopy(direction, event->s.angles);
	vec3_t snapped;
	VectorCopy(origin, snapped);
	SnapVector(snapped);
	G_SetOrigin(event, snapped);
	trap->LinkEntity((sharedEntity_t *)event);
}

//----------------------------------------------------------
void emplaced_gun_update(gentity_t *self)
{
	vec3_t	smokeOrg, puffAngle;
	int oldWeap;
	float ownLen = 0;
	const qboolean portable = JAR_MP_EmplacedState(&self->s);
	const int identity = self->s.time2;

	if (portable)
		JAR_MP_NativeEmplacedUpdateSeat(self);

	if (self->health < 1 && !self->genericValue5)
	{ //we are dead, set our respawn delay if we have one
		if (self->spawnflags & EMPLACED_CANRESPAWN)
		{
			self->genericValue5 = level.time + 4000 + self->count;
		}
	}
	else if (self->health < 1 && self->genericValue5 < level.time)
	{ //we are dead, see if it's time to respawn
		self->s.time = 0;
		self->genericValue4 = 0;
		self->genericValue3 = 0;
		self->health = EMPLACED_GUN_HEALTH*0.4;
		self->s.health = self->health;
	}

	if (self->genericValue4 && self->genericValue4 < 2 &&
		(self->s.time < level.time || (portable && self->s.time == level.time)))
	{ //we have finished our warning (red flashing) effect, it's time to finish dying
		vec3_t explOrg, blastOrg;

		VectorSet( puffAngle, 0, 0, 1 );

		VectorCopy(self->r.currentOrigin, explOrg);
		explOrg[2] += 16;

		//just use the detpack explosion effect
		EmplacedEffect(portable, EFFECT_EXPLOSION_DETPACK, explOrg, puffAngle);

		self->genericValue3 = level.time + Q_irand(2500, 3500);
		if (portable)
		{
			// Publish the terminal state before radius callbacks can touch this gun.
			VectorCopy(self->r.currentOrigin, blastOrg);
			self->s.time = -1;
			self->genericValue4 = 2;
			self->nextthink = level.time + 50;
		}

		G_RadiusDamage(portable ? blastOrg : self->r.currentOrigin, self,
			self->splashDamage, self->splashRadius, self, NULL, MOD_UNKNOWN);
		if (portable && (!self->inuse || !JAR_MP_EmplacedState(&self->s) || self->s.time2 != identity))
			return;

		self->s.time = -1;

		self->genericValue4 = 2;
		if (portable)
			JAR_MP_EmplacedChanged(self);
	}

	if (self->genericValue3 > level.time)
	{ //see if we are freshly dead and should be smoking
		if (self->genericValue2 < level.time)
		{ //is it time yet to spawn another smoke puff?
			VectorSet( puffAngle, 0, 0, 1 );
			VectorCopy(self->r.currentOrigin, smokeOrg);

			smokeOrg[2] += 60;

			EmplacedEffect(portable, EFFECT_SMOKE, smokeOrg, puffAngle);
			self->genericValue2 = level.time + Q_irand(250, 400);
		}
	}

	if (portable)
	{
		self->nextthink = level.time + 50;
		return;
	}

	if (self->activator && self->activator->client && self->activator->inuse)
	{ //handle updating current user
		vec3_t vLen;
		VectorSubtract(self->s.origin, self->activator->client->ps.origin, vLen);
		ownLen = VectorLength(vLen);

		if (!(self->activator->client->pers.cmd.buttons & BUTTON_USE) && self->genericValue1)
		{
			self->genericValue1 = 0;
		}

		if ((self->activator->client->pers.cmd.buttons & BUTTON_USE) && !self->genericValue1)
		{
			self->activator->client->ps.emplacedIndex = 0;
			self->activator->client->ps.saberHolstered = 0;
			self->nextthink = level.time + 50;
			return;
		}
	}

	if ((self->activator && self->activator->client) &&
		(!self->activator->inuse || self->activator->client->ps.emplacedIndex != self->s.number || self->genericValue4 || ownLen > 64))
	{ //get the user off of me then
		self->activator->client->ps.stats[STAT_WEAPONS] &= ~(1<<WP_EMPLACED_GUN);

		oldWeap = self->activator->client->ps.weapon;
		self->activator->client->ps.weapon = self->s.weapon;
		self->s.weapon = oldWeap;
		self->activator->r.ownerNum = ENTITYNUM_NONE;
		self->activator->client->ps.emplacedTime = level.time + 1000;
		self->activator->client->ps.emplacedIndex = 0;
		self->activator->client->ps.saberHolstered = 0;
		self->activator = NULL;

		self->s.activeForcePass = 0;
	}
	else if (self->activator && self->activator->client)
	{ //make sure the user is using the emplaced gun weapon
		self->activator->client->ps.weapon = WP_EMPLACED_GUN;
		self->activator->client->ps.weaponstate = WEAPON_READY;
	}
	self->nextthink = level.time + 50;
}

//----------------------------------------------------------
void emplaced_gun_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod )
{ //set us up to flash and then explode
	if (self->genericValue4)
	{
		return;
	}

	self->genericValue4 = 1;

	self->s.time = level.time + 3000;

	self->genericValue5 = 0;
	if (JAR_MP_EmplacedState(&self->s))
	{
		self->health = 0;
		self->takedamage = qfalse;
		self->r.svFlags &= ~SVF_PLAYER_USABLE;
		G_ScaleNetHealth(self);
		const qboolean hadSeat = self->activator || self->s.activeForcePass == NUM_FORCE_POWERS+1;
		JAR_MP_NativeEmplacedDetach(self);
		if (!hadSeat)
			JAR_MP_EmplacedChanged(self);
	}
}

static void NativeEmplacedInitCommon(gentity_t *ent)
{
	ent->r.contents = CONTENTS_SOLID;
	ent->s.solid = SOLID_BBOX;
	ent->genericValue5 = 0;
	ent->spawnflags |= 4; // deadsolid

	ent->health = EMPLACED_GUN_HEALTH;

	if (ent->spawnflags & EMPLACED_CANRESPAWN)
	{ //make it somewhat easier to kill if it can respawn
		ent->health *= 0.4;
	}

	ent->maxHealth = ent->health;
	G_ScaleNetHealth(ent);

	ent->genericValue4 = 0;

	ent->takedamage = qtrue;
	ent->pain = emplaced_gun_pain;
	ent->die = emplaced_gun_die;

	// being caught in this thing when it blows would be really bad.
	ent->splashDamage = 80;
	ent->splashRadius = 128;

	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 110;

	//so the cgame knows for sure that we're an emplaced weapon
	ent->s.weapon = WP_EMPLACED_GUN;

	ent->think = emplaced_gun_update;
	ent->nextthink = level.time + 50;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.owner = MAX_CLIENTS+1;
	ent->s.shouldtarget = qtrue;
}

void JAR_MP_NativeEmplacedInit(gentity_t *gun)
{
	RegisterItem(BG_FindItemForWeapon(WP_EMPLACED_GUN));
	SaveRegisteredItems();
	NativeEmplacedInitCommon(gun);
	gun->s.eType = ET_GENERAL;
	gun->s.generic1 = 250;
	gun->s.bolt2 = 1;
	gun->s.speed = 60;
	gun->s.emplacedOwner = ENTITYNUM_NONE;
	gun->count = 600;
}

void SP_emplaced_gun( gentity_t *ent )
{
	const char *name = "models/map_objects/mp/turret_chair.glm";
	vec3_t down;
	trace_t tr;

	RegisterItem( BG_FindItemForWeapon(WP_EMPLACED_GUN) );
	VectorSet( ent->r.mins, -30, -20, 8 );
	VectorSet( ent->r.maxs, 30, 20, 60 );
	VectorCopy(ent->s.origin, down);
	down[2] -= 1024;

	trap->Trace(&tr, ent->s.origin, ent->r.mins, ent->r.maxs, down, ent->s.number, MASK_SOLID, qfalse, 0, 0);
	if (tr.fraction != 1 && !tr.allsolid && !tr.startsolid)
		VectorCopy(tr.endpos, ent->s.origin);

	NativeEmplacedInitCommon(ent);
	G_SpawnInt( "count", "600", &ent->count );
	G_SpawnFloat( "constraint", "60", &ent->s.origin2[0] );
	ent->s.modelindex = G_ModelIndex( (char *)name );
	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->pos1 );
	VectorCopy( ent->s.angles, ent->r.currentAngles );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	ent->use = emplaced_gun_realuse;

	trap->LinkEntity((sharedEntity_t *)ent);
}
