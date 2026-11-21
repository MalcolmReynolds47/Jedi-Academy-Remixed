// Native explosive barrel spawn, extracted from g_misc.cpp.
// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void SP_object_cargo_barrel1(gentity_t *ent)
{
	const bool portable=ent->model&&!strcmp(ent->model,"models/map_objects/imperial/barrels.md3");
	if(portable){ent->s.modelindex=G_ModelIndex(ent->model);ent->fxID=G_EffectIndex("thermal/explosion");}
	else if(ent->spawnflags & 8)
	{
		ent->s.modelindex = G_ModelIndex( "/models/mapobjects/cargo/barrel_wood2.md3" );
//		ent->sounds = G_SoundIndex("sound/weapons/explosions/explode3.wav");
	}
	else if(ent->spawnflags & 2)
	{
		ent->s.modelindex = G_ModelIndex( "/models/mapobjects/scavenger/k_barrel.md3" );
//		ent->sounds = G_SoundIndex("sound/weapons/explosions/explode4.wav");
	}
	else
	{
		ent->s.modelindex = G_ModelIndex( va("/models/mapobjects/cargo/barrel%i.md3", Q_irand( 0, 2 )) );
//		ent->sounds = G_SoundIndex("sound/weapons/explosions/explode1.wav");
	}

	ent->contents = CONTENTS_SOLID|CONTENTS_OPAQUE;

	if(portable){/* Keep the qualified stock model bounds from the spawn transaction. */}
	else if ( ent->spawnflags & 1 )
	{
		VectorSet (ent->mins, -8, -8, -16);
		VectorSet (ent->maxs, 8, 8, 8);
	}
	else
	{
		VectorSet (ent->mins, -16, -16, -16);
		VectorSet (ent->maxs, 16, 16, 29);
	}

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	if(!ent->health)
		ent->health = 20;

	if(!ent->splashDamage)
		ent->splashDamage = 100;

	if(!ent->splashRadius)
		ent->splashRadius = 200;

	ent->takedamage = qtrue;

	ent->e_DieFunc = dieF_ExplodeDeath_Wait;

	if(ent->targetname)
		ent->e_UseFunc = useF_GoExplodeDeath;

	gi.linkentity (ent);
}
