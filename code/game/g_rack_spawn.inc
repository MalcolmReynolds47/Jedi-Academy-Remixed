// Native Raven/OpenJK callbacks shared with the headless regression fixture.
// GPL-2.0-only; copyright and license retained in the including source file.
#define RACK_BLASTER	1
#define RACK_REPEATER	2
#define RACK_ROCKET		4

/*QUAKED misc_model_gun_rack (1 0 0.25) (-14 -14 -4) (14 14 30) BLASTER REPEATER ROCKET
model="models/map_objects/kejim/weaponsrack.md3"

NOTE: can mix and match these spawnflags to get multi-weapon racks.  If only one type is checked the rack will be full of those weapons
BLASTER - Puts one or more blaster guns on the rack.
REPEATER - Puts one or more repeater guns on the rack.
ROCKET - Puts one or more rocket launchers on the rack.
*/

void GunRackAddItem( gentity_t *parent, gitem_t *gun, vec3_t org, vec3_t angs, float ffwd, float fright, float fup )
{
	vec3_t		fwd, right;
	gentity_t	*it_ent = gun ? G_JAR_SpawnSetupChild(parent) : nullptr;
	qboolean	rotate = qtrue;

	AngleVectors( angs, fwd, right, NULL );

	if ( it_ent && gun )
	{
		// FIXME: scaling the ammo will probably need to be tweaked to a reasonable amount...adjust as needed
		// Set base ammo per type
		if ( gun->giType == IT_WEAPON )
		{
			it_ent->spawnflags |= 16;// VERTICAL

			switch( gun->giTag )
			{
			case WP_BLASTER:
				it_ent->count = 15;
				break;
			case WP_REPEATER:
				it_ent->count = 100;
				break;
			case WP_ROCKET_LAUNCHER:
				it_ent->count = 4;
				break;
			}
		}
		else
		{
			rotate = qfalse;

			// must deliberately make it small, or else the objects will spawn inside of each other.
			VectorSet( it_ent->maxs, 6.75f, 6.75f, 6.75f );
			VectorScale( it_ent->maxs, -1, it_ent->mins );
		}

		it_ent->spawnflags |= 1;// ITMSF_SUSPEND
		it_ent->classname = G_NewString(gun->classname);	//copy it so it can be freed safely
		G_SpawnItem( it_ent, gun );

		// FinishSpawningItem handles everything, so clear the thinkFunc that was set in G_SpawnItem
		FinishSpawningItem( it_ent );

		if ( gun->giType == IT_AMMO )
		{
			if ( gun->giTag == AMMO_BLASTER ) // I guess this just has to use different logic??
			{
				if ( g_spskill->integer >= 2 )
				{
					it_ent->count += 10; // give more on higher difficulty because there will be more/harder enemies?
				}
			}
			else
			{
				// scale ammo based on skill
				switch ( g_spskill->integer )
				{
				case 0: // do default
					break;
				case 1:
					it_ent->count *= 0.75f;
					break;
				case 2:
					it_ent->count *= 0.5f;
					break;
				}
			}
		}

		it_ent->nextthink = 0;

		VectorCopy( org, it_ent->s.origin );
		VectorMA( it_ent->s.origin, fright, right, it_ent->s.origin );
		VectorMA( it_ent->s.origin, ffwd, fwd, it_ent->s.origin );
		it_ent->s.origin[2] += fup;

		VectorCopy( angs, it_ent->s.angles );

		// by doing this, we can force the amount of ammo we desire onto the weapon for when it gets picked-up
		it_ent->flags |= ( FL_DROPPED_ITEM | FL_FORCE_PULLABLE_ONLY );
		it_ent->physicsBounce = 0.1f;

		for ( int t = 0; t < 3; t++ )
		{
			if ( rotate )
			{
				if ( t == YAW )
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + 180 + Q_flrand(-1.0f, 1.0f) * 14 );
				}
				else
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + Q_flrand(-1.0f, 1.0f) * 4 );
				}
			}
			else
			{
				if ( t == YAW )
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + 90 + Q_flrand(-1.0f, 1.0f) * 4 );
				}
			}
		}

		G_SetAngles( it_ent, it_ent->s.angles );
		G_SetOrigin( it_ent, it_ent->s.origin );
		gi.linkentity( it_ent );
	}
}

//---------------------------------------------
void SP_misc_model_gun_rack( gentity_t *ent )
{
	gitem_t		*blaster = NULL, *repeater = NULL, *rocket = NULL;
	int			ct = 0;
	float		ofz[3];
	gitem_t		*itemList[3];

	// If BLASTER is checked...or nothing is checked then we'll do blasters
	if (( ent->spawnflags & RACK_BLASTER ) || !(ent->spawnflags & ( RACK_BLASTER | RACK_REPEATER | RACK_ROCKET )))
	{
		blaster	= FindItemForWeapon( WP_BLASTER );
	}

	if (( ent->spawnflags & RACK_REPEATER ))
	{
		repeater = FindItemForWeapon( WP_REPEATER );
	}

	if (( ent->spawnflags & RACK_ROCKET ))
	{
		rocket = FindItemForWeapon( WP_ROCKET_LAUNCHER );
	}

	//---------weapon types
	if ( blaster )
	{
		ofz[ct] = 23.0f;
		itemList[ct++] = blaster;
	}

	if ( repeater )
	{
		ofz[ct] = 24.5f;
		itemList[ct++] = repeater;
	}

	if ( rocket )
	{
		ofz[ct] = 25.5f;
		itemList[ct++] = rocket;
	}

	if ( ct ) //..should always have at least one item on their, but just being safe
	{
		for ( ; ct < 3 ; ct++ )
		{
			ofz[ct] = ofz[0];
			itemList[ct] = itemList[0]; // first weapon ALWAYS propagates to fill up the shelf
		}
	}

	// now actually add the items to the shelf...validate that we have a list to add
	if ( ct )
	{
		for ( int i = 0; i < ct; i++ )
		{
			GunRackAddItem( ent, itemList[i], ent->s.origin, ent->s.angles, Q_flrand(-1.0f, 1.0f) * 2, ( i - 1 ) * 9 + Q_flrand(-1.0f, 1.0f) * 2, ofz[i] );
		}
	}

	ent->s.modelindex = G_ModelIndex( "models/map_objects/kejim/weaponsrack.md3" );

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	ent->contents = CONTENTS_SOLID;

	gi.linkentity( ent );
}

#define RACK_METAL_BOLTS	2
#define RACK_ROCKETS		4
#define RACK_WEAPONS		8
#define RACK_HEALTH			16
#define RACK_PWR_CELL		32
#define RACK_NO_FILL		64

/*QUAKED misc_model_ammo_rack (1 0 0.25) (-14 -14 -4) (14 14 30) BLASTER METAL_BOLTS ROCKETS WEAPON HEALTH PWR_CELL NO_FILL
model="models/map_objects/kejim/weaponsrung.md3"

NOTE: can mix and match these spawnflags to get multi-ammo racks.  If only one type is checked the rack will be full of that ammo.  Only three ammo packs max can be displayed.


BLASTER - Adds one or more ammo packs that are compatible with Blasters and the Bryar pistol.
METAL_BOLTS - Adds one or more metal bolt ammo packs that are compatible with the heavy repeater and the flechette gun
ROCKETS - Puts one or more rocket packs on a rack.
WEAPON - adds a weapon matching a selected ammo type to the rack.
HEALTH - adds a health pack to the top shelf of the ammo rack
PWR_CELL - Adds one or more power cell packs that are compatible with the Disuptor, bowcaster, and demp2
NO_FILL - Only puts selected ammo on the rack, it never fills up all three slots if only one or two items were checked
*/

extern gitem_t	*FindItemForAmmo( ammo_t ammo );

//---------------------------------------------
void SP_misc_model_ammo_rack( gentity_t *ent )
{
// If BLASTER is checked...or nothing is checked then we'll do blasters
	if (( ent->spawnflags & RACK_BLASTER ) || !(ent->spawnflags & ( RACK_BLASTER | RACK_METAL_BOLTS | RACK_ROCKETS | RACK_PWR_CELL )))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			RegisterItem( FindItemForWeapon( WP_BLASTER ));
		}
		RegisterItem( FindItemForAmmo( AMMO_BLASTER ));
	}

	if (( ent->spawnflags & RACK_METAL_BOLTS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			RegisterItem( FindItemForWeapon( WP_REPEATER ));
		}
		RegisterItem( FindItemForAmmo( AMMO_METAL_BOLTS ));
	}

	if (( ent->spawnflags & RACK_ROCKETS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			RegisterItem( FindItemForWeapon( WP_ROCKET_LAUNCHER ));
		}
		RegisterItem( FindItemForAmmo( AMMO_ROCKETS ));
	}

	if (( ent->spawnflags & RACK_PWR_CELL ))
	{
		RegisterItem( FindItemForAmmo( AMMO_POWERCELL ));
	}

	if (( ent->spawnflags & RACK_HEALTH ))
	{
		RegisterItem( FindItem( "item_medpak_instant" ));
	}

	ent->e_ThinkFunc = thinkF_spawn_rack_goods;
	ent->nextthink = level.time + 100;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	ent->contents = CONTENTS_SHOTCLIP|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//CONTENTS_SOLID;//so use traces can go through them

	gi.linkentity( ent );
}

// AMMO RACK!!
void spawn_rack_goods( gentity_t *ent )
{
	float		v_off = 0;
	gitem_t		*blaster = NULL, *metal_bolts = NULL, *rockets = NULL, *it = NULL;
	gitem_t		*am_blaster = NULL, *am_metal_bolts = NULL, *am_rockets = NULL, *am_pwr_cell = NULL;
	gitem_t		*health = NULL;
	int			pos = 0, ct = 0;
	gitem_t		*itemList[4]; // allocating 4, but we only use 3.  done so I don't have to validate that the array isn't full before I add another

	gi.unlinkentity( ent );

	// If BLASTER is checked...or nothing is checked then we'll do blasters
	if (( ent->spawnflags & RACK_BLASTER ) || !(ent->spawnflags & ( RACK_BLASTER | RACK_METAL_BOLTS | RACK_ROCKETS | RACK_PWR_CELL )))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			blaster	= FindItemForWeapon( WP_BLASTER );
		}
		am_blaster	= FindItemForAmmo( AMMO_BLASTER );
	}

	if (( ent->spawnflags & RACK_METAL_BOLTS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			metal_bolts = FindItemForWeapon( WP_REPEATER );
		}
		am_metal_bolts = FindItemForAmmo( AMMO_METAL_BOLTS );
	}

	if (( ent->spawnflags & RACK_ROCKETS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			rockets = FindItemForWeapon( WP_ROCKET_LAUNCHER );
		}
		am_rockets = FindItemForAmmo( AMMO_ROCKETS );
	}

	if (( ent->spawnflags & RACK_PWR_CELL ))
	{
		am_pwr_cell = FindItemForAmmo( AMMO_POWERCELL );
	}

	if (( ent->spawnflags & RACK_HEALTH ))
	{
		health = FindItem( "item_medpak_instant" );
		RegisterItem( health );
	}

	//---------Ammo types
	if ( am_blaster )
	{
		itemList[ct++] = am_blaster;
	}

	if ( am_metal_bolts )
	{
		itemList[ct++] = am_metal_bolts;
	}

	if ( am_pwr_cell )
	{
		itemList[ct++] = am_pwr_cell;
	}

	if ( am_rockets )
	{
		itemList[ct++] = am_rockets;
	}

	if ( !(ent->spawnflags & RACK_NO_FILL) && ct ) //double negative..should always have at least one item on there, but just being safe
	{
		for ( ; ct < 3 ; ct++ )
		{
			itemList[ct] = itemList[0]; // first item ALWAYS propagates to fill up the shelf
		}
	}

	// now actually add the items to the shelf...validate that we have a list to add
	if ( ct )
	{
		for ( int i = 0; i < ct; i++ )
		{
			GunRackAddItem( ent, itemList[i], ent->s.origin, ent->s.angles, Q_flrand(-1.0f, 1.0f) * 0.5f, (i-1)* 8, 7.0f );
		}
	}

	// -----Weapon option
	if ( ent->spawnflags & RACK_WEAPONS )
	{
		if ( !(ent->spawnflags & ( RACK_BLASTER | RACK_METAL_BOLTS | RACK_ROCKETS | RACK_PWR_CELL )))
		{
			// nothing was selected, so we assume blaster pack
			it = blaster;
		}
		else
		{
			// if weapon is checked...and so are one or more ammo types, then pick a random weapon to display..always give weaker weapons first
			if ( blaster )
			{
				it = blaster;
				v_off = 25.5f;
			}
			else if ( metal_bolts )
			{
				it = metal_bolts;
				v_off = 27.0f;
			}
			else if ( rockets )
			{
				it = rockets;
				v_off = 28.0f;
			}
		}

		if ( it )
		{
			// since we may have to put up a health pack on the shelf, we should know where we randomly put
			//	the gun so we don't put the pack on the same spot..so pick either the left or right side
			pos = ( Q_flrand(0.0f, 1.0f) > .5 ) ? -1 : 1;

			GunRackAddItem( ent, it, ent->s.origin, ent->s.angles, Q_flrand(-1.0f, 1.0f) * 2, ( Q_flrand(0.0f, 1.0f) * 6 + 4 ) * pos, v_off );
		}
	}

	// ------Medpack
	if (( ent->spawnflags & RACK_HEALTH ) && health )
	{
		if ( !pos )
		{
			// we haven't picked a side already...
			pos = ( Q_flrand(0.0f, 1.0f) > .5 ) ? -1 : 1;
		}
		else
		{
			// switch to the opposite side
			pos *= -1;
		}

		GunRackAddItem( ent, health, ent->s.origin, ent->s.angles, Q_flrand(-1.0f, 1.0f) * 0.5f, ( Q_flrand(0.0f, 1.0f) * 4 + 4 ) * pos, 24 );
	}

	ent->s.modelindex = G_ModelIndex( "models/map_objects/kejim/weaponsrung.md3" );

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	gi.linkentity( ent );
}
