// Transactional hand model replacement for live Sandbox NPCs. GPL-2.0-only.
#include "g_carried_weapon_art.h"

bool G_ReplaceG2HandWeapon(gentity_t& actor, const char* model)
{
	const int old = actor.weaponModel[0];
	const int body = actor.playerModel;
	const int count = actor.ghoul2.size();
	if (body < 0 || body >= count || actor.ghoul2[body].mModelindex < 0 || actor.weaponModel[1] > 0 ||
		(old > 0 && (old == body || old >= count || actor.ghoul2[old].mModelindex < 0)))
		return false;

	int next = -1;
	if (model)
	{
		// The native path can insert "_w" when converting an MD3 to a GLM.
		if (actor.handRBolt < 0 || !model[0] || strlen(model) >= 62)
			return false;
		if (count >= 8)
		{
			bool freeSlot = false;
			for (int i = 0; i < count; ++i)
				if (actor.ghoul2[i].mModelindex == -1)
					freeSlot = true;
			if (!freeSlot)
				return false;
		}
		actor.weaponModel[0] = -1;
		G_CreateG2AttachedWeaponModel(&actor, model, actor.handRBolt, 0);
		next = actor.weaponModel[0];
		actor.weaponModel[0] = old;
		if (next < 0 || next >= actor.ghoul2.size() || next == old)
			return false;
		if (next == body || actor.ghoul2[next].mModelBoltLink < 0)
		{
			if (next != body)
				gi.G2API_RemoveGhoul2Model(actor.ghoul2, next);
			return false;
		}
	}
	if (old > 0)
		gi.G2API_RemoveGhoul2Model(actor.ghoul2, old);
	actor.weaponModel[0] = next;
	return true;
}

void G_RestoreCarriedWeaponHand(gentity_t& actor)
{
	if (!jar::carriedWeapon::OnFoot(actor))
		return;
	const auto* art = jar::carriedWeapon::StockWeaponArt(actor.client->ps.weapon);
	if (!art || gi.FS_ReadFile(art->pickup, nullptr) <= 0)
		return;
	const int slot = actor.weaponModel[0];
	if (slot > 0 && (slot >= actor.ghoul2.size() ||
		Q_stricmp(actor.ghoul2[slot].mFileName, jar::carriedWeapon::placeholderPickup)))
		return;
	G_ReplaceG2HandWeapon(actor, art->pickup);
}

