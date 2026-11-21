// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
#define MAXCHOICES	32

gentity_t *G_PickTarget (char *targetname)
{
	gentity_t	*ent = NULL;
	int		num_choices = 0;
	gentity_t	*choice[MAXCHOICES];

	if (!targetname)
	{
		gi.Printf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while(1)
	{
		ent = G_Find (ent, FOFS(targetname), targetname);
		if (!ent)
			break;
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices)
	{
		gi.Printf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}

void G_UseTargets2 (gentity_t *ent, gentity_t *activator, const char *string)
{
	G_SubBSPCapture subBspCapture(ent);
	gentity_t		*t;

//
// fire targets
//
	if (string)
	{
		if( !Q_stricmp( string, "self") )
		{
			t = ent;
			if (t->e_UseFunc != useF_NULL)	// check can be omitted
			{
				GEntity_UseFunc(t, ent, activator);
			}

			if (!ent->inuse)
			{
				gi.Printf("entity was removed while using targets\n");
				return;
			}
		}
		else
		{
			t = NULL;
			while ( (t = G_Find (t, FOFS(targetname), (char *) string)) != NULL )
			{
				if (t == ent)
				{
	//				gi.Printf ("WARNING: Entity used itself.\n");
				}
				if (t->e_UseFunc != useF_NULL)	// check can be omitted
				{
					GEntity_UseFunc(t, ent, activator);
				}

				if (!ent->inuse)
				{
					gi.Printf("entity was removed while using targets\n");
					return;
				}
			}
		}
	}
}

/*
==============================
G_UseTargets

"activator" should be set to the entity that initiated the firing.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets (gentity_t *ent, gentity_t *activator)
{
//
// fire targets
//
	G_UseTargets2 (ent, activator, ent->target);
}

