// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
static void InitBBrush ( gentity_t *ent )
{
	float		light;
	vec3_t		color;
	qboolean	lightSet, colorSet;

	VectorCopy( ent->s.origin, ent->pos1 );

	G_JAR_SetBrushModel( ent, ent->model );

	ent->e_DieFunc = dieF_funcBBrushDie;

	ent->svFlags |= SVF_BBRUSH;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet )
	{
		int		r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	if(ent->spawnflags & 128)
	{//Can be used by the player's BUTTON_USE
		ent->svFlags |= SVF_PLAYER_USABLE;
	}

	ent->s.eType = ET_MOVER;
	gi.linkentity (ent);

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->s.pos.trBase );
}

void funcBBrushTouch( gentity_t *ent, gentity_t *other, trace_t *trace )
{
}

/*QUAKED func_breakable (0 .8 .5) ? INVINCIBLE IMPACT CRUSHER THIN SABERONLY HEAVY_WEAP USE_NOT_BREAK PLAYER_USE NO_EXPLOSION
INVINCIBLE - can only be broken by being used
IMPACT - does damage on impact
CRUSHER - won't reverse movement when hit an obstacle
THIN - can be broken by impact damage, like glass
SABERONLY - only takes damage from sabers
HEAVY_WEAP - only takes damage by a heavy weapon, like an emplaced gun or AT-ST gun.
USE_NOT_BREAK - Using it doesn't make it break, still can be destroyed by damage
PLAYER_USE - Player can use it with the use button
NO_EXPLOSION - Does not play an explosion effect, though will still create chunks if specified

When destroyed, fires it's trigger and chunks and plays sound "noise" or sound for type if no noise specified

"targetname" entities with matching target will fire it
"paintarget" target to fire when hit (but not destroyed)
"wait"		how long minimum to wait between firing paintarget each time hit
"delay"		When killed or used, how long (in seconds) to wait before blowing up (none by default)
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"target"	all entities with a matching targetname will be used when this is destoryed
"health"	default is 10
"radius"  Chunk code tries to pick a good volume of chunks, but you can alter this to scale the number of spawned chunks. (default 1)  (.5) is half as many chunks, (2) is twice as many chunks
"NPC_targetname" - Only the NPC with this name can damage this
"forcevisible" - When you turn on force sight (any level), you can see these draw through the entire level...
"redCrosshair" - crosshair turns red when you look at this

Damage: default is none
"splashDamage" - damage to do
"splashRadius" - radius for above damage

"team" - This cannot take damage from members of this team:
	"player"
	"neutral"
	"enemy"

Don't know if these work:
"color"		constantLight color
"light"		constantLight radius

"material" - default is "0 - MAT_METAL" - choose from this list:
0 = MAT_METAL		(basic blue-grey scorched-DEFAULT)
1 = MAT_GLASS
2 = MAT_ELECTRICAL	(sparks only)
3 = MAT_ELEC_METAL	(METAL2 chunks and sparks)
4 =	MAT_DRK_STONE	(brown stone chunks)
5 =	MAT_LT_STONE	(tan stone chunks)
6 =	MAT_GLASS_METAL (glass and METAL2 chunks)
7 = MAT_METAL2		(electronic type of metal)
8 = MAT_NONE		(no chunks)
9 = MAT_GREY_STONE	(grey colored stone)
10 = MAT_METAL3		(METAL and METAL2 chunk combo)
11 = MAT_CRATE1		(yellow multi-colored crate chunks)
12 = MAT_GRATE1		(grate chunks--looks horrible right now)
13 = MAT_ROPE		(for yavin_trial, no chunks, just wispy bits )
14 = MAT_CRATE2		(red multi-colored crate chunks)
15 = MAT_WHITE_METAL (white angular chunks for Stu, NS_hideout )

*/
void SP_func_breakable( gentity_t *self )
{
	if ( G_SiegeKorribanRoom(G_SubBSPOwner(self)) )
	{
		int team;
		G_SpawnInt("teamnodmg", "0", &team);
		self->noDamageTeam = team == 2 ? TEAM_ENEMY : team == 1 ? TEAM_PLAYER : TEAM_FREE;
	}
	if(!(self->spawnflags & 1))
	{
		if(!self->health)
		{
			self->health = 10;
		}
	}

	if ( self->spawnflags & 16 ) // saber only
	{
		self->flags |= FL_DMG_BY_SABER_ONLY;
	}
	else if ( self->spawnflags & 32 ) // heavy weap
	{
		self->flags |= FL_DMG_BY_HEAVY_WEAP_ONLY;
	}

	if (self->health)
	{
		self->takedamage = qtrue;
	}

	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");//precaching
	G_SpawnFloat( "radius", "1", &self->radius ); // used to scale chunk code if desired by a designer
	G_SpawnInt( "material", "0", (int*)&self->material );
	CacheChunkEffects( self->material );

	self->e_UseFunc = useF_funcBBrushUse;

	//if ( self->paintarget )
	{
		self->e_PainFunc = painF_funcBBrushPain;
	}

	self->e_TouchFunc = touchF_funcBBrushTouch;

	if ( self->team && self->team[0] )
	{
		self->noDamageTeam = (team_t)GetIDForString( TeamTable, self->team );
		if(self->noDamageTeam == TEAM_FREE)
		{
			G_Error("team name %s not recognized\n", self->team);
		}
	}
	self->team = NULL;
	if (!self->model) {
		G_Error("func_breakable with NULL model\n");
	}
	InitBBrush( self );

	char	buffer[MAX_QPATH];
	char	*s;
	if ( G_SpawnString( "noise", "*NOSOUND*", &s ) )
	{
		Q_strncpyz( buffer, s, sizeof(buffer) );
		COM_DefaultExtension( buffer, sizeof(buffer), ".wav");
		self->noise_index = G_SoundIndex(buffer);
	}

	int forceVisible = 0;
	G_SpawnInt( "forcevisible", "0", &forceVisible );
	if ( forceVisible )
	{//can see these through walls with force sight, so must be broadcast
		if ( VectorCompare( self->s.origin, vec3_origin ) )
		{//no origin brush
			self->svFlags |= SVF_BROADCAST;
		}
		self->s.eFlags |= EF_FORCE_VISIBLE;
	}

	int redCrosshair = 0;
	G_SpawnInt( "redCrosshair", "0", &redCrosshair );
	if ( redCrosshair )
	{//can see these through walls with force sight, so must be broadcast
		self->flags |= FL_RED_CROSSHAIR;
	}
}


