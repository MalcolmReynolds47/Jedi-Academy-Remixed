/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/
void G_SubBSPSpawnEntitiesFromString(const char *entityString, vec3_t posOffset, vec3_t angOffset)
{
	const char		*entities;

	G_SpawnContext context;
	entities = entityString;

	// allow calls to G_Spawn*()
	spawning = qtrue;
	NPCsPrecached = qfalse;
	numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars( &entities ) ) {
		G_Error( "SpawnEntities: no entities" );
	}

	// Skip this guy if its worldspawn fails
	if ( !SP_bsp_worldspawn() )
	{
		return;
	}

	// parse ents
	while( G_ParseSpawnVars(&entities) )
	{
		G_SpawnSubBSPGEntityFromSpawnVars(posOffset, angOffset);
	}
}

