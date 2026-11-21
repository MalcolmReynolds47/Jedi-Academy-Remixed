// On-foot firing of the mounted weapon families exposed in Sandbox. GPL-2.0-only.
static bool G_UseCarriedMountedWeapon(const gentity_t& shooter)
{
	return jar::carriedWeapon::OnFoot(shooter) && jar::carriedWeapon::Find(shooter.s.weapon);
}

static bool G_PrepareCarriedMountedWeaponShot(const gentity_t& shooter)
{
	const auto& ps = shooter.client->ps;
	AngleVectors(ps.viewangles, forwardVec, vrightVec, up);
	vec3_t start;
	VectorCopy(shooter.currentOrigin, start);
	start[2] += ps.viewheight - 6;
	VectorMA(start, 12, forwardVec, muzzle);
	VectorMA(muzzle, 6 + ps.leanofs, vrightVec, muzzle);

	// Reach from the player's body to the muzzle without crossing a thin wall.
	const vec3_t mins = {-5, -5, -5};
	const vec3_t maxs = {5, 5, 5};
	trace_t trace{};
	gi.trace(&trace, start, mins, maxs, muzzle, shooter.s.number,
		MASK_SOLID | CONTENTS_SHOTCLIP, G2_NOCOLLIDE, 0);
	if (trace.startsolid || trace.allsolid)
		return false;
	VectorCopy(trace.endpos, muzzle);
	return true;
}

static void G_FireCarriedTurret(gentity_t* shooter)
{
	// Match the stock personal assault sentry's projectile and impact effects.
	auto* bolt = CreateMissile(muzzle, forwardVec, 900, 10000, shooter);
	if (!bolt)
		return;

	bolt->classname = "turret_proj";
	bolt->s.weapon = WP_TURRET;
	bolt->damage = weaponData[WP_TURRET].damage > 0 ? weaponData[WP_TURRET].damage : 2;
	bolt->dflags = DAMAGE_NO_KNOCKBACK;
	bolt->methodOfDeath = MOD_ENERGY;
	bolt->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	VectorSet(bolt->mins, -1, -1, -1);
	VectorSet(bolt->maxs, 1, 1, 1);
}
