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

#include "g_local.h"

char itemRegistered[MAX_ITEMS+1];

// Full saves restore CS_ITEMS after the initial BSP spawn. Its precache bitmap
// is authoritative for the restored entities, including randomized pickups.
// Keep subsequent RegisterItem calls from replacing it with the BSP's bitmap.
void RestoreRegisteredItems( void ) {
	char savedItems[MAX_ITEMS+1] = {};
	gi.GetConfigstring(CS_ITEMS, savedItems, sizeof(savedItems));
	for (int i = 0; i < bg_numItems; ++i) {
		itemRegistered[i] = savedItems[i] == '1' ? '1' : '0';
	}
	itemRegistered[bg_numItems] = 0;
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
	if ( !item ) {
		G_Error( "RegisterItem: NULL" );
	}
	itemRegistered[ item - bg_itemlist ] = '1';
	gi.SetConfigstring(CS_ITEMS, itemRegistered);	//Write the needed items to a config string
}


/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems( void ) {
/*	char	string[MAX_ITEMS+1];
	int		i;
	int		count;

	count = 0;
	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( itemRegistered[i] ) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[ bg_numItems ] = 0;

	gi.Printf( "%i items registered\n", count );
	gi.SetConfigstring(CS_ITEMS, string);
*/
	gi.SetConfigstring(CS_ITEMS, itemRegistered);
}

