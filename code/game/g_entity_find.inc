// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match)
{
	char	*s;
	const bool scopedNames=G_SubBSPNameField(fieldofs);

	if(!match || !match[0])
	{
		return NULL;
	}

	if (!from)
		from = g_entities;
	else
		from++;

//	for ( ; from < &g_entities[globals.num_entities] ; from++)
	int i=from-g_entities;
	for ( ; i < globals.num_entities ; i++)
	{
//		if (!from->inuse)
		if(!PInUse(i))
			continue;

		from=&g_entities[i];
		if(scopedNames&&!G_SubBSPNameVisible(from))continue;
		s = *(char **) ((byte *)from + fieldofs);
		if (!s)
			continue;
		if (!Q_stricmp (s, match))
			return from;
	}

	return NULL;
}


