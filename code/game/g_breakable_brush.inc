// Native func_breakable damage callbacks. GPL-2.0-only.
#include "g_sandbox_breakable.inc"
#include "g_siege_item.h"
#include "g_subbsp_ownership.h"
static void BreakableDebrisCenter(const vec3_t mins, const vec3_t maxs,
								 const BreakableDebrisFrame *frame, vec3_t center)
{
	VectorAdd(mins, maxs, center);
	VectorScale(center, 0.5f, center);
	if (frame)
	{
		frame->ToWorld(center);
	}
}

static const BreakableDebrisFrame *BreakableSectionFrame(const gentity_t &brush, BreakableDebrisFrame &frame)
{
	if (!G_BSPSectionBreakable(brush))
	{
		return nullptr;
	}
	VectorCopy(brush.currentOrigin, frame.origin);
	AnglesToAxis(brush.currentAngles, frame.axis);
	return &frame;
}

//--------------------------------------
void funcBBrushDieGo (gentity_t *self)
{
	if(G_SandboxBreakable(*self)){sandboxBreakableDetonate(*self);return;}
	vec3_t		org, dir, up;
	gentity_t	*attacker = self->enemy;
	float		scale;
	int			numChunks, size = 0;
	material_t	chunkType = self->material;

	// if a missile is stuck to us, blow it up so we don't look dumb
	// FIXME: flag me so I should know to do this check!
	for ( int i = 0; i < MAX_GENTITIES; i++ )
	{
		if ( g_entities[i].s.groundEntityNum == self->s.number && ( g_entities[i].s.eFlags & EF_MISSILE_STICK ))
		{
			G_Damage( &g_entities[i], self, self, NULL, NULL, 99999, 0, MOD_CRUSH ); //?? MOD?
		}
	}

	BreakableDebrisFrame sectionFrame;
	const auto *frame = BreakableSectionFrame(*self, sectionFrame);
	if (!frame)
	{
		// Native map brushes may open a host area portal before losing contents.
		gi.AdjustAreaPortalState(self, qtrue);
	}

	// Remove collision before spawning chunks and firing linked targets.
	self->s.solid = 0;
	self->contents = 0;
	self->clipmask = 0;
	if (frame)
	{
		// Relinking a bmodel would restore its solid flag. Keep this entity
		// until the native free timer runs, without drawing or clipping it.
		gi.unlinkentity(self);
		self->bmodel = qfalse;
		self->s.eFlags |= EF_NODRAW;
		self->s.modelindex = self->s.modelindex2 = 0;
		self->e_UseFunc = useF_NULL;
	}
	else
	{
		gi.linkentity(self);
	}

	const float *mins = frame ? self->mins : self->absmin;
	const float *maxs = frame ? self->maxs : self->absmax;

	VectorSet(up, 0, 0, 1);

	if ( self->target && attacker != NULL )
	{
		const auto lifetime = G_EntityLifetime(self);
		G_UseTargets(self, attacker);
		if ( !self->inuse || G_EntityLifetime(self) != lifetime ) return;
	}

	VectorSubtract( maxs, mins, org );// size

	numChunks = Q_flrand(0.0f, 1.0f) * 6 + 18;

	// This formula really has no logical basis other than the fact that it seemed to be the closest to yielding the results that I wanted.
	// Volume is length * width * height...then break that volume down based on how many chunks we have
	scale = sqrt( sqrt( org[0] * org[1] * org[2] )) * 1.75f;

	if ( scale > 48 )
	{
		size = 2;
	}
	else if ( scale > 24 )
	{
		size = 1;
	}

	scale = scale / numChunks;

	if ( self->radius > 0.0f )
	{
		// designer wants to scale number of chunks, helpful because the above scale code is far from perfect
		//	I do this after the scale calculation because it seems that the chunk size generally seems to be very close, it's just the number of chunks is a bit weak
		numChunks *= self->radius;
	}

	BreakableDebrisCenter(mins, maxs, frame, org);

	if ( attacker != NULL && attacker->client )
	{
		VectorSubtract( org, attacker->currentOrigin, dir );
		VectorNormalize( dir );
	}
	else
	{
		VectorCopy(up, dir);
	}

	if ( !(self->spawnflags & 2048) ) // NO_EXPLOSION
	{
		// we are allowed to explode
		CG_MiscModelExplosion(mins, maxs, size, chunkType, frame);
	}

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{
		//explode
		AddSightEvent( attacker, org, 256, AEL_DISCOVERED, 100 );
		AddSoundEvent( attacker, org, 128, AEL_DISCOVERED, qfalse, qtrue );//FIXME: am I on ground or not?
		G_RadiusDamage( org, self, self->splashDamage, self->splashRadius, self, MOD_UNKNOWN );

		gentity_t *te = G_TempEntity( org, EV_GENERAL_SOUND );
		if (te)
		{
			te->s.eventParm = G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
		}
	}
	else
	{//just break
		AddSightEvent( attacker, org, 128, AEL_DISCOVERED );
		AddSoundEvent( attacker, org, 64, AEL_SUSPICIOUS, qfalse, qtrue );//FIXME: am I on ground or not?
	}

	//FIXME: base numChunks off size?
	CG_Chunks(self->s.number, org, dir, mins, maxs, 300, numChunks, chunkType, 0, scale, self->noise_index, frame);

	self->e_ThinkFunc = thinkF_G_FreeEntity;
	self->nextthink = level.time + 50;
	//G_FreeEntity( self );
}

void funcBBrushDie (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc)
{
	if(G_SandboxBreakable(*self)){sandboxBreakableBegin(*self,attacker);return;}
	const bool siege = G_SiegeKorribanRoom(G_SubBSPOwner(self));
	if ( siege && (self->e_ThinkFunc == thinkF_funcBBrushDieGo || !self->bmodel) ) return;
	self->takedamage = qfalse;//stop chain reaction runaway loops

	if ( siege ) self->enemy = attacker && attacker->inuse ? attacker : self;
	else G_SetEnemy(self, attacker);

	if(self->delay)
	{
		self->e_ThinkFunc = thinkF_funcBBrushDieGo;
		self->nextthink = level.time + floor(self->delay * 1000.0f);
		return;
	}

	funcBBrushDieGo(self);
}

void funcBBrushUse (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if(G_SandboxBreakable(*self)){
		if(self->count!=jar::sandboxBreakable::Intact||(self->svFlags&SVF_INACTIVE))return;
		const auto life=G_EntityLifetime(self),actorLife=G_EntityLifetime(activator);
		G_ActivateBehavior(self,BSET_USE);
		if(G_EntityLifetime(self)==life)
			sandboxBreakableBegin(*self,actorLife&&G_EntityLifetime(activator)==actorLife?activator:nullptr);
		return;
	}
	G_ActivateBehavior( self, BSET_USE );
	if(self->spawnflags & 64)
	{//Using it doesn't break it, makes it use it's targets
		if(self->target && self->target[0])
		{
			G_UseTargets(self, activator);
		}
	}
	else
	{
		funcBBrushDie(self, other, activator, self->health, MOD_UNKNOWN);
	}
}

void funcBBrushPain(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, const vec3_t point, int damage, int mod,int hitLoc)
{
	if ( self->painDebounceTime > level.time )
	{
		return;
	}

	if ( self->paintarget )
	{
		const auto lifetime = G_EntityLifetime(self);
		auto* activator = self->activator;
		if ( !activator && attacker && attacker->inuse && attacker->client &&
		     G_SiegeKorribanRoom(G_SubBSPOwner(self)) ) activator = attacker;
		G_UseTargets2 (self, activator, self->paintarget);
		if ( !self->inuse || G_EntityLifetime(self) != lifetime ) return;
	}

	G_ActivateBehavior( self, BSET_PAIN );

	if ( self->material == MAT_DRK_STONE
		|| self->material == MAT_LT_STONE
		|| self->material == MAT_GREY_STONE )
	{
		vec3_t	org, dir;
		float	scale;
		BreakableDebrisFrame sectionFrame;
		const auto *frame = BreakableSectionFrame(*self, sectionFrame);
		const float *mins = frame ? self->mins : self->absmin;
		const float *maxs = frame ? self->maxs : self->absmax;
		VectorSubtract( maxs, mins, org );// size
		// This formula really has no logical basis other than the fact that it seemed to be the closest to yielding the results that I wanted.
		// Volume is length * width * height...then break that volume down based on how many chunks we have
		scale = VectorLength( org ) / 100.0f;
		BreakableDebrisCenter(mins, maxs, frame, org);
		if ( attacker != NULL && attacker->client )
		{
			VectorSubtract( attacker->currentOrigin, org, dir );
			VectorNormalize( dir );
		}
		else
		{
			VectorSet( dir, 0, 0, 1 );
		}
		CG_Chunks(self->s.number, org, dir, mins, maxs, 300, Q_irand(1, 3), self->material, 0, scale, 0, frame);
	}

	if ( self->wait == -1 )
	{
		self->e_PainFunc = painF_NULL;
		return;
	}

	self->painDebounceTime = level.time + self->wait;
}

