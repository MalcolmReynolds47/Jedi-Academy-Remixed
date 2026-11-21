// Portable workshop launchers use native missiles without a fake client.
// GPL-2.0-only.
typedef struct {
	int weapon;
	const char *muzzle[2], *fire[2], *projectile;
} shooterAssets_t;

static const shooterAssets_t shooterAssets[] = {
	{ WP_BRYAR_PISTOL, { "bryar/muzzle_flash", "bryar/altmuzzle_flash" },
		{ "sound/weapons/bryar/fire.mp3", "sound/weapons/bryar/alt_fire.mp3" }, "effects/bryar/shot.efx" },
	{ WP_BLASTER, { "jar/sandbox/e11_muzzle", "jar/sandbox/e11_muzzle" },
		{ "sound/weapons/blaster/fire.mp3", "sound/weapons/blaster/alt_fire.mp3" }, "effects/blaster/shot.efx" },
	{ WP_THERMAL, { "thermal/muzzle_flash", "thermal/muzzle_flash" },
		{ "sound/weapons/thermal/fire.mp3", "sound/weapons/thermal/fire.mp3" }, "models/weapons2/thermal/thermal_proj.md3" },
	{ WP_ROCKET_LAUNCHER, { "rocket/muzzle_flash", "rocket/altmuzzle_flash" },
		{ "sound/weapons/rocket/fire.mp3", "sound/weapons/rocket/alt_fire.mp3" }, "models/weapons2/merr_sonn/projectile.md3" }
};

enum { SHOOTER_THERMAL_LOOP, SHOOTER_THERMAL_WARN, SHOOTER_THERMAL_BOUNCE1, SHOOTER_THERMAL_BOUNCE2 };
static const char *const shooterThermalSounds[] = {
	"sound/weapons/thermal/thermloop.wav", "sound/weapons/thermal/warning.mp3",
	"sound/weapons/thermal/bounce1.mp3", "sound/weapons/thermal/bounce2.mp3"
};
static const char shooterDeflectEffect[] = "blaster/deflect";

static const shooterAssets_t *ShooterAssets(int weapon)
{
	for (unsigned i = 0; i < ARRAY_LEN(shooterAssets); ++i)
		if (shooterAssets[i].weapon == weapon)
			return &shooterAssets[i];
	return NULL;
}

static qboolean ShooterAssetExists(const char *path)
{
	fileHandle_t file = 0;
	const int bytes = trap->FS_Open(path, &file, FS_READ);
	if (file)
		trap->FS_Close(file);
	return file && bytes > 0;
}

static int ShooterConfigIndex(const char *name, int base, int limit)
{
	for (int i = 1; i < limit; ++i)
	{
		char value[MAX_QPATH];
		trap->GetConfigstring(base + i, value, sizeof(value));
		if (!*value)
			return -i;
		if (!strcmp(value, name))
			return i;
	}
	return -limit;
}

static qboolean ShooterResourcesFit(const char *const *names, unsigned count, int base, int limit)
{
	int missing = 0, firstFree = limit;
	for (unsigned i = 0; i < count; ++i)
	{
		const int index = ShooterConfigIndex(names[i], base, limit);
		if (index >= 0)
			continue;
		++missing;
		firstFree = -index;
	}
	const int reserve = base == CS_EFFECTS ? 16 : 64;
	return !missing || limit - firstFree >= missing + reserve;
}

qboolean JAR_MP_ShooterAssets(int weapon)
{
	const shooterAssets_t *assets = ShooterAssets(weapon);
	if (!assets)
		return qfalse;
	gitem_t *item = BG_FindItemForWeapon(weapon);
	const char *effects[3] = { assets->muzzle[0] };
	unsigned effectCount = 1;
	if (strcmp(assets->muzzle[0], assets->muzzle[1]))
		effects[effectCount++] = assets->muzzle[1];
	if (weapon == WP_BRYAR_PISTOL || weapon == WP_BLASTER)
		effects[effectCount++] = shooterDeflectEffect;
	if (!item || !item->world_model[0] || !ShooterAssetExists(item->world_model[0]) ||
		!ShooterAssetExists(assets->projectile) || !ShooterAssetExists(assets->fire[0]) ||
		!ShooterAssetExists(assets->fire[1]) || !ShooterResourcesFit(effects, effectCount, CS_EFFECTS, MAX_FX))
		return qfalse;
	for (unsigned i = 0; i < effectCount; ++i)
		if (!ShooterAssetExists(va("effects/%s.efx", effects[i])))
			return qfalse;
	if (weapon == WP_BLASTER)
	{
		if (!ShooterResourcesFit(assets->fire, ARRAY_LEN(assets->fire), CS_SOUNDS, MAX_SOUNDS))
			return qfalse;
		for (unsigned i = 0; i < ARRAY_LEN(assets->fire); ++i)
			G_SoundIndex(assets->fire[i]);
	}
	if (weapon == WP_THERMAL)
	{
		if (!ShooterResourcesFit(shooterThermalSounds, ARRAY_LEN(shooterThermalSounds), CS_SOUNDS, MAX_SOUNDS))
			return qfalse;
		for (unsigned i = 0; i < ARRAY_LEN(shooterThermalSounds); ++i)
			if (!ShooterAssetExists(shooterThermalSounds[i]))
				return qfalse;
		for (unsigned i = 0; i < ARRAY_LEN(shooterThermalSounds); ++i)
			G_SoundIndex(shooterThermalSounds[i]);
	}
	for (unsigned i = 0; i < effectCount; ++i)
		G_EffectIndex(effects[i]);
	RegisterItem(item);
	SaveRegisteredItems();
	return qtrue;
}

static void ShooterSound(const gentity_t *missile, int channel, int sound)
{
	gentity_t *event = sound > 0 ? G_TryMissileEvent(missile->r.currentOrigin, EV_GENERAL_SOUND) : NULL;
	if (event)
	{
		event->s.eventParm = sound;
		event->s.saberEntityNum = channel;
	}
}

void G_ShooterBounceSound(const gentity_t *missile)
{
	const int i = Q_irand(SHOOTER_THERMAL_BOUNCE1, SHOOTER_THERMAL_BOUNCE2);
	ShooterSound(missile, CHAN_BODY, ShooterConfigIndex(shooterThermalSounds[i], CS_SOUNDS, MAX_SOUNDS));
}

static void ShooterThermalDetonate(gentity_t *missile)
{
	// The original SP timed blast uses full splash damage, even when a
	// shooter-launched grenade uses scaled damage on collision.
	missile->splashDamage = 90;
	G_ExplodeMissile(missile);
}

static void ShooterThermalWarn(gentity_t *missile)
{
	missile->takedamage = qfalse;
	missile->r.contents = 0;
	missile->think = ShooterThermalDetonate;
	missile->nextthink = level.time + 800;
	missile->r.svFlags |= SVF_BROADCAST;
	ShooterSound(missile, CHAN_WEAPON,
		ShooterConfigIndex(shooterThermalSounds[SHOOTER_THERMAL_WARN], CS_SOUNDS, MAX_SOUNDS));
}

static void ShooterThermalDie(gentity_t *missile, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	ShooterThermalWarn(missile);
}

static qboolean ShooterVector(const vec3_t point)
{
	return point && isfinite(point[0]) && isfinite(point[1]) && isfinite(point[2]);
}

static qboolean ShooterMuzzle(gentity_t *source, const vec3_t origin, float radius,
	qboolean snap, vec3_t start, vec3_t mins, vec3_t maxs)
{
	const int mask = MASK_SHOT | CONTENTS_SHOTCLIP;
	trace_t trace;
	vec3_t center;
	VectorAdd(source->r.mins, source->r.maxs, center);
	VectorMA(source->r.currentOrigin, 0.5f, center, center);
	VectorSet(mins, -radius, -radius, -radius);
	VectorSet(maxs, radius, radius, radius);
	VectorCopy(origin, start);
	if (snap)
		SnapVector(start);
	trap->Trace(&trace, center, mins, maxs, start, source->s.number, mask, qfalse, 0, 0);
	if (trace.startsolid || trace.allsolid)
		return qfalse;
	if (trace.fraction == 1.0f)
		return qtrue;
	if (trace.fraction <= 0.0f)
		return qfalse;
	// Back off toward the marker's clear center before snapping a muzzle next
	// to the floor. Reject obstructions that would displace the shot far away.
	vec3_t inward;
	VectorSubtract(center, trace.endpos, inward);
	const float clearance = VectorNormalize(inward);
	VectorMA(trace.endpos, fminf(clearance, 1.0f), inward, start);
	if (snap)
		SnapVector(start);
	const float maxOffset = radius + 4.0f;
	if (DistanceSquared(origin, start) > maxOffset * maxOffset)
		return qfalse;
	trap->Trace(&trace, center, mins, maxs, start, source->s.number, mask, qfalse, 0, 0);
	return !trace.startsolid && !trace.allsolid && trace.fraction == 1.0f;
}

static gentity_t *ShooterThermal(gentity_t *source, qboolean altFire, vec3_t start,
	const vec3_t direction, const vec3_t target, vec3_t mins, vec3_t maxs)
{
	const int loop = ShooterConfigIndex(shooterThermalSounds[SHOOTER_THERMAL_LOOP], CS_SOUNDS, MAX_SOUNDS);
	if (loop <= 0)
		return NULL;
	gentity_t *missile = G_TrySpawn();
	if (!missile)
		return NULL;
	vec3_t velocity;
	VectorScale(direction, TD_VELOCITY, velocity);
	velocity[2] += 120;
	if (ShooterVector(target) && DistanceSquared(start, target) >= 1.0f &&
		DistanceSquared(start, target) <= 4096.0f * 4096.0f)
	{
		vec3_t aim;
		VectorCopy(target, aim);
		LobFire(source, start, aim, mins, maxs, MASK_SHOT, velocity, qtrue, source->s.number,
			ENTITYNUM_NONE, 100, TD_VELOCITY, 300, qfalse, 48);
	}
	InitThermalMissile(missile, source, altFire, start, velocity, loop);
	VectorCopy(mins, missile->r.mins);
	VectorCopy(maxs, missile->r.maxs);
	missile->alt_fire = altFire;
	if (altFire)
		missile->s.eFlags |= EF_ALT_FIRING;
	missile->damage = 60;
	missile->splashDamage = 54;
	missile->splashRadius = TD_SPLASH_RAD;
	// MP shots include CONTENTS_CORPSE in their mask; player movement doesn't.
	missile->r.contents = CONTENTS_CORPSE;
	missile->health = 15;
	missile->takedamage = qtrue;
	missile->die = ShooterThermalDie;
	missile->think = ShooterThermalWarn;
	missile->nextthink = level.time + 4000;
	return missile;
}

static gentity_t *ShooterBlaster(gentity_t *source, qboolean altFire, vec3_t start, const vec3_t direction)
{
	// Match the single-player E-11 launcher on normal difficulty.
	const float spread = altFire ? 1.5f : 0.5f;
	vec3_t angles, varied;
	vectoangles(direction, angles);
	angles[PITCH] += Q_flrand(-1.0f, 1.0f) * spread;
	angles[YAW] += Q_flrand(-1.0f, 1.0f) * spread;
	AngleVectors(angles, varied, NULL, NULL);
	gentity_t *missile = G_TryCreateMissile(start, varied, BLASTER_VELOCITY, 10000, source, altFire);
	if (missile)
		InitBlasterMissile(missile, 12);
	return missile;
}

qboolean JAR_MP_FireShooter(gentity_t *source, int weapon, qboolean alt, const vec3_t origin,
	const vec3_t direction, const vec3_t target)
{
	unsigned epoch = 0;
	vec3_t start, forward, mins, maxs;
	const shooterAssets_t *assets = ShooterAssets(weapon);
	if (!assets || !JAR_MP_ShooterSource(source, &epoch) || !ShooterVector(origin) || !ShooterVector(direction))
		return qfalse;
	VectorCopy(direction, forward);
	if (VectorNormalize(forward) < 0.001f)
		return qfalse;
	const float radius = weapon == WP_THERMAL ? 4.0f : weapon == WP_ROCKET_LAUNCHER ? ROCKET_SIZE :
		weapon == WP_BRYAR_PISTOL && alt ? 2.5f : 0.0f;
	if (!ShooterMuzzle(source, origin, radius, weapon != WP_THERMAL, start, mins, maxs))
		return qfalse;
	const int effect = ShooterConfigIndex(assets->muzzle[!!alt], CS_EFFECTS, MAX_FX);
	if (effect <= 0)
		return qfalse;
	gentity_t *missile;
	if (weapon == WP_THERMAL)
		missile = ShooterThermal(source, alt, start, forward, target, mins, maxs);
	else if (weapon == WP_BLASTER)
		missile = ShooterBlaster(source, alt, start, forward);
	else if (weapon == WP_BRYAR_PISTOL)
	{
		missile = G_TryCreateMissile(start, forward, 1800, 10000, source, alt);
		if (missile)
			InitBryarMissile(missile, alt ? 70 : 14, alt ? 5 : 0);
	}
	else
	{
		missile = G_TryCreateMissile(start, forward, alt ? ROCKET_VELOCITY * 0.5f : ROCKET_VELOCITY, 10000, source, alt);
		if (missile)
			InitRocketMissile(missile, 40, alt);
	}
	if (!missile)
		return qfalse;
	missile->jarShooterSource = source->s.number + 1;
	missile->jarShooterEpoch = epoch;
	gentity_t *event = G_TryMissileEvent(start, EV_PLAY_EFFECT_ID);
	if (event)
	{
		event->s.eventParm = effect;
		vec3_t direction;
		VectorCopy(missile->s.pos.trDelta, direction);
		vectoangles(direction, event->s.angles);
	}
	if (weapon == WP_BLASTER)
		ShooterSound(missile, CHAN_WEAPON, ShooterConfigIndex(assets->fire[!!alt], CS_SOUNDS, MAX_SOUNDS));
	return qtrue;
}
