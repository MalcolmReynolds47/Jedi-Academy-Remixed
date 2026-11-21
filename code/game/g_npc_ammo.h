// Ammo policy for native single-player NPC firing. GPL-2.0-only.
#pragma once

#include "g_local.h"

inline void G_NPCRefillWeaponAmmo(gentity_t& actor)
{
	const int weapon = actor.client->ps.weapon;
	const auto& data = weaponData[weapon];
	const int ammo = actor.client->ps.ammo[data.ammoIndex];
	if ((actor.NPC->aiFlags & NPCAI_MANUAL_AMMO) || ammo == AMMO_UNLIMITED)
		return;

	// Preserve the stock AI's automatic refill and its preferred order.
	if (ammo < data.energyPerShot)
		Add_Ammo(&actor, weapon, data.energyPerShot * 10);
	else if (ammo < data.altEnergyPerShot)
		Add_Ammo(&actor, weapon, data.altEnergyPerShot * 5);
}

inline bool G_NPCConsumeScriptShot(gentity_t& actor, bool alternate)
{
	if (!(actor.NPC->aiFlags & NPCAI_MANUAL_AMMO))
		return true;

	const int weapon = actor.s.weapon;
	if (weapon <= WP_NONE || weapon >= WP_NATIVE_WEAPON_COUNT)
		return true;
	const auto& data = weaponData[weapon];
	if (data.ammoIndex <= AMMO_NONE || data.ammoIndex >= AMMO_MAX)
		return true;

	int& ammo = actor.client->ps.ammo[data.ammoIndex];
	const int cost = alternate ? data.altEnergyPerShot : data.energyPerShot;
	if (!BG_WeaponHasAmmo(ammo, cost))
		return false;
	if (ammo != AMMO_UNLIMITED)
		ammo -= cost;
	actor.NPC->currentAmmo = ammo;
	return true;
}
