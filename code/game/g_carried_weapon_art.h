// Stock art for on-foot copies of native mounted weapons. GPL-2.0-only.
#pragma once

#include "g_local.h"

namespace jar { namespace carriedWeapon {

constexpr const char* placeholderView = "models/weapons2/noweap/noweap.md3";
constexpr const char* placeholderPickup = "models/weapons2/noweap/noweap.glm";

struct Art {
	const char* view;
	const char* hands;
	const char* pickup;
	const char* barrel;
	const char* icon;
};

struct Entry {
	int weapon;
	const char* item;
	const Art& art;
};

inline const auto& Entries()
{
	static constexpr Art concussion = {
		"models/weapons2/concussion/c_rifle.md3",
		"models/weapons2/concussion/c_rifle_hand.md3",
		"models/weapons2/concussion/c_rifle_w.glm",
		"models/weapons2/concussion/c_rifle_barrel.md3", "gfx/hud/w_icon_c_rifle"
	};
	static constexpr Art rocket = {
		"models/weapons2/merr_sonn/merr_sonn.md3",
		"models/weapons2/merr_sonn/merr_sonn_hand.md3",
		"models/weapons2/merr_sonn/merr_sonn_w.glm",
		"models/weapons2/merr_sonn/merr_sonn_barrel.md3", "gfx/hud/w_icon_merrsonn"
	};
	static constexpr Art repeater = {
		"models/weapons2/heavy_repeater/heavy_repeater.md3",
		"models/weapons2/heavy_repeater/heavy_repeater_hand.md3",
		"models/weapons2/heavy_repeater/heavy_repeater_w.glm",
		"models/weapons2/heavy_repeater/heavy_repeater_barrel.md3", "gfx/hud/w_icon_repeater"
	};
	static constexpr Art blaster = {
		"models/weapons2/blaster_r/blaster.md3",
		"models/weapons2/blaster_r/blaster_hand.md3",
		"models/weapons2/blaster_r/blaster_w.glm", nullptr, "gfx/hud/w_icon_blaster"
	};
	static constexpr Art bryar = {
		"models/weapons2/briar_pistol/briar_pistol.md3",
		"models/weapons2/briar_pistol/briar_pistol_hand.md3",
		"models/weapons2/briar_pistol/briar_pistol_w.glm", nullptr, "gfx/hud/w_icon_briar"
	};

	static constexpr Entry entries[] = {
		{WP_ATST_MAIN, "weapon_atst_main", concussion},
		{WP_ATST_SIDE, "weapon_atst_side", rocket},
		{WP_EMPLACED_GUN, "weapon_emplaced_gun", repeater},
		{WP_TIE_FIGHTER, "weapon_tie_fighter", blaster},
		{WP_RAPID_FIRE_CONC, "weapon_rapid_concussion", repeater},
		{WP_TURRET, "weapon_turret", blaster},
		{WP_BOT_LASER, "weapon_botwelder", bryar},
	};
	return entries;
}

inline const Entry* Find(int weapon)
{
	for (const auto& entry : Entries())
		if (entry.weapon == weapon)
			return &entry;
	return nullptr;
}

inline bool OnFoot(const gentity_t& player)
{
	return player.client && player.s.number == 0 && !player.s.m_iVehicleNum && !player.m_pVehicle &&
		player.client->NPC_class != CLASS_VEHICLE &&
		!(player.client->ps.eFlags & (EF_IN_ATST | EF_LOCKED_TO_WEAPON));
}

inline const Art* StockWeaponArt(int weapon)
{
	const auto* entry = Find(weapon);
	return entry && !Q_stricmp(weaponData[weapon].weaponMdl, placeholderView) ? &entry->art : nullptr;
}

inline const Art* StockPickupArt(const gitem_t& item)
{
	if (item.giType != IT_WEAPON || !item.classname || !item.world_model ||
		(Q_stricmp(item.world_model, placeholderView) && Q_stricmp(item.world_model, placeholderPickup)))
		return nullptr;
	const auto* entry = Find(item.giTag);
	return entry && !strcmp(entry->item, item.classname) ? &entry->art : nullptr;
}

inline const char* HandModel(const gentity_t& player, const char* model)
{
	if (!OnFoot(player) || Q_stricmp(model, placeholderView))
		return model;
	// Native callers pass the weapon definition's buffer. The placeholder text
	// is identical for these weapons, and a scripted equip can precede ps.weapon.
	for (const auto& entry : Entries())
		if (model == weaponData[entry.weapon].weaponMdl)
			return gi.FS_ReadFile(entry.art.pickup, nullptr) > 0 ? entry.art.pickup : model;
	return model;
}

}}
