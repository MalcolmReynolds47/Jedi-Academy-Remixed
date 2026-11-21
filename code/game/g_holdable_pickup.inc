// Native holdable inventory grant, shared with regression checks. GPL-2.0-only.
#include "qcommon/jar_context.h"
int Pickup_Holdable( gentity_t *ent, gentity_t *other )
{
	int		i,original;

	if (!ent || !ent->item || !other || !other->client || ent->item->giTag < 0 || ent->item->giTag >= INV_MAX) return 0;
	if (ent->item->giTag==INV_DATAPAD && other->s.number!=0) return 0;
	other->client->ps.stats[STAT_ITEMS] |= (1<<ent->item->giTag);

	if ( ent->item->giTag == INV_SECURITY_KEY )
	{//give the key
		//FIXME: temp message
		gi.SendServerCommand( 0, "cp @SP_INGAME_YOU_TOOK_SECURITY_KEY" );
		INV_SecurityKeyGive( other, ent->message );
	}
	else if ( ent->item->giTag == INV_GOODIE_KEY )
	{//give the key
		//FIXME: temp message
		gi.SendServerCommand( 0, "cp @SP_INGAME_YOU_TOOK_SUPPLY_KEY" );
		INV_GoodieKeyGive( other );
	}
	else if (ent->item->giTag==INV_DATAPAD)
	{
		// A permanent reader, not consumable charges. Further pads can deliver
		// another short note and their normal map targets without adding copies.
		other->client->ps.inventory[INV_DATAPAD]=1;
		if (jar::context::Label(ent->message)) gi.SendServerCommand(0,"cp \"%s\"",ent->message);
	}
	else
	{// Picking up a normal item?
		other->client->ps.inventory[ent->item->giTag]++;
	}
	// Got a security key

	// Set the inventory select, just in case it hasn't
	original = cg.inventorySelect;
	for ( i = 0 ; i < INV_MAX ; i++ )
	{
		if ((cg.inventorySelect < INV_ELECTROBINOCULARS) || (cg.inventorySelect >= INV_MAX))
		{
			cg.inventorySelect = (INV_MAX - 1);
		}

		if ( G_InventorySelectable( cg.inventorySelect,other ) )
		{
			return 60;
		}
		cg.inventorySelect++;
	}

	cg.inventorySelect = original;

	return 60;
}


