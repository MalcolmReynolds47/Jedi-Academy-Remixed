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
char *G_AddSpawnVarToken( const char *string ) {
	int		l;
	char	*dest;

	l = strlen( string );
	if ( numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_VARS" );
	}

	dest = spawnVarChars + numSpawnVarChars;
	memcpy( dest, string, l+1 );

	numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( const char **data ) {
	char		keyname[MAX_STRING_CHARS];
	const char	*com_token;

	numSpawnVars = 0;
	numSpawnVarChars = 0;

	// parse the opening brace
	COM_BeginParseSession();
	com_token = COM_Parse( data );
	if ( !*data ) {
		// end of spawn string
		COM_EndParseSession();
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		COM_EndParseSession();
		G_Error( "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		com_token = COM_Parse( data );
		if ( !*data ) {
			COM_EndParseSession();
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			break;
		}

		Q_strncpyz( keyname, com_token, sizeof(keyname) );

		// parse value
		com_token = COM_Parse( data );
		if ( !*data ) {
			COM_EndParseSession();
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}
		if ( com_token[0] == '}' ) {
			COM_EndParseSession();
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( numSpawnVars == MAX_SPAWN_VARS ) {
			COM_EndParseSession();
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		spawnVars[ numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		spawnVars[ numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		numSpawnVars++;
	}

	COM_EndParseSession();
	return qtrue;
}

