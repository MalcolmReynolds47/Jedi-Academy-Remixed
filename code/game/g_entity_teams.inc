// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
#include "g_entity_team_link.h"
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
//	for ( i=1, e=g_entities,i ; i < globals.num_entities ; i++,e++ )
	for ( i=MAX_CLIENTS ; i < globals.num_entities ; i++ )
	{
//		if (!e->inuse)
//			continue;
		if(!PInUse(i))
			continue;
		e=&g_entities[i];

		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
//		for (j=i+1, e2=e+1 ; j < globals.num_entities ; j++,e2++)
		for (j=i+1; j < globals.num_entities ; j++)
		{
//			if (!e2->inuse)
//				continue;
			if(!PInUse(j))
				continue;

			e2=&g_entities[j];
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (G_SubBSPNamespace(e)==G_SubBSPNamespace(e2)&&!strcmp(e->team, e2->team))
			{
				c2++;
				jar::entityTeam::AddMember(*e, *e2);
			}
		}
	}

	//gi.Printf ("%i teams with %i entities\n", c, c2);
}


