// Persistent Sandbox E-Web. Reuses the stock E-Web rig and reciprocal seat.
// GPL-2.0-only.
#include "g_jar_sandbox.h"
#include "g_jar_native_eweb.h"
#include "jar_mp_emplaced.h"

static const char portableEWebModel[] = "models/map_objects/hoth/eweb_model.glm";
enum { EWEB_MUZZLE, EWEB_TRAIL, EWEB_EXPLOSION, EWEB_SMOKE, EWEB_WALL, EWEB_FLESH };
static const char *const portableEWebEffects[] = {
	"eweb/muzzle_flash", "eweb/shot", "emplaced/explode", "emplaced/dead_smoke",
	"eweb/wall_impact", "eweb/flesh_impact"
};
static const char *const portableEWebSounds[] = {
	"sound/weapons/eweb/eweb_mount.mp3", "sound/weapons/eweb/eweb_dismount.mp3",
	"sound/weapons/eweb/eweb_aim.wav"
};

// Reserve all required configstrings before the stock registration calls.
static qboolean EWebResourcesFit(const char *const *names, int count, int start, int limit)
{
	int present = 0, index;
	for (index = 1; index < limit; ++index)
	{
		char value[MAX_QPATH];
		trap->GetConfigstring(start + index, value, sizeof(value));
		if (!value[0])
			break;
		for (int i = 0; i < count; ++i)
			if (!strcmp(value, names[i]))
				++present;
	}
	return limit - index >= count - present;
}

static qboolean EWebAsset(const char *name)
{
	fileHandle_t handle = 0;
	const int size = trap->FS_Open(name, &handle, FS_READ);
	if (handle)
		trap->FS_Close(handle);
	return handle && size > 0;
}

static qboolean EWebPreflight(void)
{
	const char *const bones[] = { ewebYawBone, ewebPitchBone };
	if (!EWebResourcesFit(bones, ARRAY_LEN(bones), CS_G2BONES, MAX_G2BONES) ||
		!EWebResourcesFit(portableEWebEffects, ARRAY_LEN(portableEWebEffects), CS_EFFECTS, MAX_FX) ||
		!EWebResourcesFit(portableEWebSounds, ARRAY_LEN(portableEWebSounds), CS_SOUNDS, MAX_SOUNDS) ||
		!EWebAsset(portableEWebModel) || !EWebAsset("models/map_objects/hoth/model.gla") ||
		!EWebAsset("sound/weapons/eweb/eweb_fire.mp3")) // played by the muzzle effect
		return qfalse;
	for (int i = 0; i < ARRAY_LEN(portableEWebEffects); ++i)
		if (!EWebAsset(va("effects/%s.efx", portableEWebEffects[i])))
			return qfalse;
	for (int i = 0; i < ARRAY_LEN(portableEWebSounds); ++i)
		if (!EWebAsset(portableEWebSounds[i]))
			return qfalse;
	return qtrue;
}

static gentity_t *EWebEvent(int type, int resource, const vec3_t origin)
{
	gentity_t *event = G_TrySpawn();
	if (!event)
		return NULL;
	event->classname = "jar_eweb_event";
	event->s.eType = ET_EVENTS + type;
	event->s.eventParm = resource;
	event->eventTime = level.time;
	event->freeAfterEvent = qtrue;
	vec3_t snapped;
	VectorCopy(origin, snapped);
	SnapVector(snapped);
	G_SetOrigin(event, snapped);
	VectorCopy(origin, event->s.origin);
	return event;
}

static void EWebEffect(int effect, const vec3_t origin, const vec3_t direction)
{
	gentity_t *event = EWebEvent(EV_PLAY_EFFECT_ID, effect, origin);
	if (!event)
		return;
	vectoangles(direction, event->s.angles);
	trap->LinkEntity((sharedEntity_t *)event);
}

static void EWebSound(gentity_t *gun, int sound)
{
	gentity_t *event = EWebEvent(EV_GENERAL_SOUND, sound, gun->r.currentOrigin);
	if (!event)
		return;
	event->s.saberEntityNum = CHAN_AUTO;
	trap->LinkEntity((sharedEntity_t *)event);
}

static void EWebPositionPersistentUser(gentity_t *user, vec3_t destination)
{
	EWebApplySeat(user, destination);
	VectorClear(user->client->ps.velocity);
	trap->LinkEntity((sharedEntity_t *)user);
}

qboolean JAR_MP_NativeEWebSeat(gentity_t *gun, gentity_t *user)
{
	const playerState_t *ps = &user->client->ps;
	const float facing = AngleSubtract(ps->viewangles[YAW], gun->s.angles[YAW] + gun->angle);
	vec3_t destination;
	if (ps->emplacedTime > level.time || ps->forceHandExtend != HANDEXTEND_NONE ||
		ps->isJediMaster || ps->m_iVehicleNum || (ps->pm_flags & PMF_DUCKED) ||
		(ps->eFlags & EF_JETPACK_ACTIVE) || fabsf(facing) > 41.4f ||
		DistanceSquared(gun->r.currentOrigin, ps->origin) > 80.0f * 80.0f ||
		EWebTraceSeat(user, gun, qtrue, destination) != EWEB_SEAT_CLEAR)
		return qfalse;
	EWebPositionPersistentUser(user, destination);
	gun->genericValue8 = level.time + 500;
	EWebSound(gun, G_SoundIndex(portableEWebSounds[0]));
	return qtrue;
}

void JAR_MP_NativeEWebDismounted(gentity_t *gun)
{
	gun->s.loopSound = 0;
	gun->genericValue8 = 0;
	if (gun->genericValue3)
	{
		EWebRigAnim(gun, 0, 1);
		gun->genericValue3 = 0;
	}
	EWebSound(gun, G_SoundIndex(portableEWebSounds[1]));
}

static qboolean EWebPersistentAim(gentity_t *gun, gentity_t *user)
{
	float yaw;
	playerState_t *ps = &user->client->ps;
	if (BG_EmplacedView(ps->viewangles, gun->s.angles, &yaw, JAR_MP_EmplacedConstraint(&gun->s)))
		ps->viewangles[YAW] = yaw;
	const float nextYaw = EWebTurnYaw(user, gun, 4.0f);
	const float nextPitch = EWebTurnPitch(user, gun);
	vec3_t yawAngles = { nextYaw, 0, 0 }, pitchAngles = { 0, 0, nextPitch };
	vec3_t destination;
	const qboolean moving = nextYaw != gun->angle || nextPitch != gun->s.boneAngles2[2];
	if (moving)
	{
		if (!EWebRigAngle(gun, ewebYawBone, yawAngles) || !EWebRigAngle(gun, ewebPitchBone, pitchAngles))
			return qfalse;
		gun->angle = nextYaw;
		VectorCopy(yawAngles, gun->s.boneAngles1);
		VectorCopy(pitchAngles, gun->s.boneAngles2);
	}
	gun->s.loopSound = moving ? gun->noise_index : 0;
	if (EWebTraceSeat(user, gun, qtrue, destination) != EWEB_SEAT_CLEAR)
		return qfalse;
	EWebPositionPersistentUser(user, destination);
	return qtrue;
}

static qboolean EWebPersistentFire(gentity_t *gun, gentity_t *user)
{
	vec3_t muzzle, direction, start;
	if (!EWebBolt(gun, gun->genericValue10, NEGATIVE_Y, muzzle, direction) ||
		VectorNormalize(direction) == 0.0f || !JAR_MP_EmplacedCanFire(gun))
		return qfalse;
	VectorMA(muzzle, -16.0f, direction, start);
	// CanFire reserves space for the native missile before allocating any FX.
	gentity_t *missile = CreateMissile(start, direction, 6000.0f, 10000, user, qfalse);
	missile->classname = JAR_MP_EWEB_SHOT_CLASS;
	missile->s.weapon = WP_TURRET;
	missile->s.otherEntityNum2 = gun->soundPos1; // native missile trail effect override
	missile->s.emplacedOwner = gun->soundPos2; // native missile impact effect override
	missile->genericValue10 = gun->genericValue15; // stock flesh impact
	missile->damage = 150;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_TURBLAST;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->passThroughNum = gun->s.number + 1;
	missile->bounceCount = 8;
	EWebEffect(gun->genericValue12, muzzle, direction);
	return qtrue;
}

static void EWebPersistentPain(gentity_t *gun, gentity_t *attacker, int damage)
{
	G_ScaleNetHealth(gun);
	JAR_MP_EmplacedChanged(gun);
}

static void EWebPersistentDie(gentity_t *gun, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	if (gun->genericValue4)
		return;
	gun->genericValue4 = 1;
	gun->s.time = level.time + 50;
	gun->health = 0;
	gun->takedamage = qfalse;
	gun->r.svFlags &= ~SVF_PLAYER_USABLE;
	G_ScaleNetHealth(gun);
	const qboolean hadSeat = gun->activator || gun->s.activeForcePass == NUM_FORCE_POWERS+1;
	JAR_MP_NativeEmplacedDetach(gun);
	if (!hadSeat)
		JAR_MP_EmplacedChanged(gun);
}

static void EWebPersistentThink(gentity_t *gun)
{
	gun->nextthink = level.time + 50;
	if (gun->genericValue4 == 1 && gun->s.time <= level.time && !(gun->flags & FL_INACTIVE))
	{
		vec3_t origin, effectOrigin, up = { 0, 0, 1 };
		VectorCopy(gun->r.currentOrigin, origin);
		VectorMA(origin, 20, up, effectOrigin);
		EWebEffect(gun->genericValue13, effectOrigin, up);
		VectorMA(origin, 35, up, effectOrigin);
		EWebEffect(gun->genericValue14, effectOrigin, up);
		// Publish a terminal state before blast callbacks can affect another gun.
		gun->genericValue4 = 2;
		gun->s.time = -1;
		JAR_MP_EmplacedChanged(gun);
		G_RadiusDamage(origin, gun, gun->splashDamage, gun->splashRadius, gun, NULL, MOD_UNKNOWN);
		return;
	}
	gentity_t *user = JAR_MP_NativeEmplacedUpdateSeat(gun);
	if (!user)
		return;
	const usercmd_t *cmd = &user->client->pers.cmd;
	if ((level.time >= gun->genericValue8 && cmd->upmove > 0) || !EWebPersistentAim(gun, user))
	{
		JAR_MP_NativeEmplacedDetach(gun);
		return;
	}
	if (level.time < gun->genericValue5)
		return;
	if ((cmd->buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)) && gun->count > 0 && EWebPersistentFire(gun, user))
	{
		gun->s.otherEntityNum2 = --gun->count;
		gun->genericValue5 = level.time + 150;
		gun->genericValue3 = 1;
		EWebRigAnim(gun, 2, 4);
		JAR_MP_EmplacedChanged(gun);
	}
	else if (gun->genericValue3)
	{
		EWebRigAnim(gun, 0, 1);
		gun->genericValue3 = 0;
	}
}

qboolean JAR_MP_NativeEWebInit(gentity_t *gun)
{
	vec3_t point, axis;
	if (!gun->s.modelindex || !EWebPreflight() ||
		trap->G2API_InitGhoul2Model(&gun->ghoul2, portableEWebModel, 0, 0, 0, 0, 0) < 0 || !gun->ghoul2)
		return qfalse;
	gun->genericValue9 = trap->G2API_AddBolt(gun->ghoul2, 0, ewebYawBone);
	gun->genericValue10 = trap->G2API_AddBolt(gun->ghoul2, 0, "*cannonflash");
	if (trap->G2API_AddBolt(gun->ghoul2, 0, ewebPitchBone) < 0 ||
		!EWebRigAngle(gun, ewebYawBone, vec3_origin) || !EWebRigAngle(gun, ewebPitchBone, vec3_origin) ||
		!EWebRigAnim(gun, 0, 1) || !EWebBolt(gun, gun->genericValue9, NEGATIVE_X, point, axis) ||
		!EWebBolt(gun, gun->genericValue10, NEGATIVE_Y, point, axis))
		return qfalse;
	gun->s.boneIndex1 = G_BoneIndex(ewebYawBone);
	gun->s.boneIndex2 = G_BoneIndex(ewebPitchBone);
	gun->genericValue12 = G_EffectIndex(portableEWebEffects[EWEB_MUZZLE]);
	gun->soundPos1 = G_EffectIndex(portableEWebEffects[EWEB_TRAIL]);
	gun->genericValue13 = G_EffectIndex(portableEWebEffects[EWEB_EXPLOSION]);
	gun->genericValue14 = G_EffectIndex(portableEWebEffects[EWEB_SMOKE]);
	gun->soundPos2 = G_EffectIndex(portableEWebEffects[EWEB_WALL]);
	gun->genericValue15 = G_EffectIndex(portableEWebEffects[EWEB_FLESH]);
	G_SoundIndex(portableEWebSounds[0]);
	G_SoundIndex(portableEWebSounds[1]);
	gun->noise_index = G_SoundIndex(portableEWebSounds[2]);
	RegisterItem(BG_FindItemForWeapon(WP_EMPLACED_GUN));
	RegisterItem(BG_FindItemForWeapon(WP_TURRET));
	SaveRegisteredItems();
	gun->s.eType = ET_GENERAL;
	gun->s.generic1 = 250;
	gun->s.bolt2 = JAR_MP_EWEB_TAG;
	gun->s.speed = 180;
	gun->s.weapon = WP_NONE;
	gun->s.emplacedOwner = ENTITYNUM_NONE;
	gun->s.owner = MAX_CLIENTS + 1;
	gun->s.shouldtarget = qtrue;
	gun->s.modelGhoul2 = 1;
	gun->s.g2radius = 128;
	gun->s.pos.trType = TR_STATIONARY;
	gun->r.contents = CONTENTS_SOLID;
	gun->r.svFlags |= SVF_PLAYER_USABLE;
	gun->health = gun->maxHealth = 250;
	G_ScaleNetHealth(gun);
	gun->s.otherEntityNum2 = gun->count = JAR_MP_EWEB_DEFAULT_AMMO;
	gun->splashDamage = 40;
	gun->splashRadius = 100;
	gun->takedamage = qtrue;
	gun->pain = EWebPersistentPain;
	gun->die = EWebPersistentDie;
	gun->think = EWebPersistentThink;
	gun->nextthink = level.time + 50;
	return qtrue;
}
