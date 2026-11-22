// Multiplayer sandbox behavior for unscripted ground NPCs.
extern void G_UcmdMoveForDir(gentity_t *self, usercmd_t *cmd, vec3_t direction);

static qboolean NPC_TacticalFirearm(int weapon)
{
	switch (weapon)
	{
	case WP_BRYAR_PISTOL:
	case WP_BRYAR_OLD:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_DEMP2:
	case WP_FLECHETTE:
		return qtrue;
	default:
		return qfalse;
	}
}

static qboolean NPC_SafeTacticalStep(const vec3_t direction, trace_t *obstacle)
{
	const gentity_t *npc = NPCS.NPC;
	vec3_t mins, end, ground, sample;
	trace_t trace;
	VectorCopy(npc->r.mins, mins);
	mins[2] += 18.0f; // Native step height.
	VectorMA(npc->r.currentOrigin, 64.0f, direction, end);
	trap->Trace(obstacle, npc->r.currentOrigin, mins, npc->r.maxs, end,
	            npc->s.number, npc->clipmask, qfalse, 0, 0);
	if (obstacle->startsolid || obstacle->allsolid || obstacle->fraction < 1.0f)
		return qfalse;

	for (int distance = 32; distance <= 64; distance += 32)
	{
		VectorMA(npc->r.currentOrigin, distance, direction, sample);
		VectorCopy(sample, ground);
		ground[2] += npc->r.mins[2] - 36.0f;
		trap->Trace(&trace, sample, NULL, NULL, ground, npc->s.number,
		            MASK_SOLID, qfalse, 0, 0);
		if (trace.startsolid || trace.fraction == 1.0f || trace.plane.normal[2] < 0.7f)
			return qfalse;
	}
	return qtrue;
}

static void NPC_TacticalWalk(vec3_t direction)
{
	NPCS.NPCInfo->combatMove = qtrue;
	NPCS.client->ps.speed = NPCS.NPCInfo->stats.runSpeed;
	G_UcmdMoveForDir(NPCS.NPC, &NPCS.ucmd, direction);
}

static qboolean NPC_FollowAlongWall(const vec3_t heading, const trace_t *wall)
{
	if (wall->fraction == 1.0f || wall->startsolid || wall->allsolid ||
	    fabsf(wall->plane.normal[2]) >= 0.7f || wall->entityNum == ENTITYNUM_NONE ||
	    (wall->entityNum != ENTITYNUM_WORLD && g_entities[wall->entityNum].client))
		return qfalse;

	vec3_t tangent = {-wall->plane.normal[1],wall->plane.normal[0],0};
	VectorNormalize(tangent);
	gNPC_t *info = NPCS.NPCInfo;
	int side = info->sandboxFollowSide;
	if (!side || level.time >= info->sandboxFollowSideUntil)
	{
		const float bias = DotProduct(tangent, heading);
		side = fabsf(bias) > .1f ? (bias > 0 ? 1 : -1) : (NPCS.NPC->s.number & 1 ? 1 : -1);
	}
	VectorScale(tangent, side, tangent);
	trace_t step;
	if (!NPC_SafeTacticalStep(tangent, &step))
	{
		// An uneven face can need clearance before the follower can pass.
		tangent[0] += .4f * wall->plane.normal[0];
		tangent[1] += .4f * wall->plane.normal[1];
		VectorNormalize(tangent);
		if (!NPC_SafeTacticalStep(tangent, &step))
			return qfalse;
	}
	// Adjacent props can form one wall. Keep the same side through the
	// seams and short clear steps, even when the owner is behind the NPC.
	info->sandboxFollowSide = side;
	info->sandboxFollowSideUntil = level.time + 1000;
	NPC_TacticalWalk(tangent);
	return qtrue;
}

static qboolean NPC_TacticalMove(const vec3_t heading, qboolean following)
{
	// Keep the same preferred side when repeatedly negotiating an obstruction.
	const float side = NPCS.NPC->s.number & 1 ? 1.0f : -1.0f;
	const float turns[] = {0, 35 * side, -35 * side, 75 * side, -75 * side, 90 * side, -90 * side};
	for (unsigned i = 0; i < ARRAY_LEN(turns); ++i)
	{
		vec3_t direction;
		RotatePointAroundVector(direction, (vec3_t){0, 0, 1}, heading, turns[i]);
		trace_t step;
		if (NPC_SafeTacticalStep(direction, &step))
		{
			NPC_TacticalWalk(direction);
			return qtrue;
		}
		if (!i && following && NPC_FollowAlongWall(heading, &step))
			return qtrue;
	}
	return qfalse;
}

static void NPC_FollowSandboxLeader(gentity_t *leader)
{
	vec3_t heading;
	VectorSubtract(leader->r.currentOrigin, NPCS.NPC->r.currentOrigin, heading);
	heading[2] = 0;
	if (VectorNormalize(heading) <= 112.0f)
		return;
	if (NPC_TacticalMove(heading, qtrue) || !trap->Nav_GetNumNodes())
	{
		NPC_FaceEntity(leader, qfalse);
		return;
	}

	gentity_t *savedGoal = NPCS.NPCInfo->goalEntity;
	NPCS.NPCInfo->goalEntity = leader;
	NPC_MoveToGoal(qtrue);
	NPCS.NPCInfo->goalEntity = savedGoal;
	NPC_UpdateAngles(qtrue, qtrue);
}

static qboolean NPC_TacticalAim(const vec3_t target)
{
	vec3_t muzzle, direction, angles;
	CalcEntitySpot(NPCS.NPC, SPOT_WEAPON, muzzle);
	VectorSubtract(target, muzzle, direction);
	vectoangles(direction, angles);
	NPCS.NPCInfo->desiredPitch = angles[PITCH];
	NPCS.NPCInfo->desiredYaw = angles[YAW];
	NPC_UpdateAngles(qtrue, qtrue);

	// Check the aim that this command will actually use for the shot.
	const playerState_t *ps = &NPCS.client->ps;
	const float pitch = SHORT2ANGLE(NPCS.ucmd.angles[PITCH] + ps->delta_angles[PITCH]);
	const float yaw = SHORT2ANGLE(NPCS.ucmd.angles[YAW] + ps->delta_angles[YAW]);
	const float tolerance = atan2f(8.0f, VectorLength(direction)) * (180.0f / (float)M_PI);
	return fabsf(AngleDelta(angles[PITCH], pitch)) <= tolerance &&
	       fabsf(AngleDelta(angles[YAW], yaw)) <= tolerance;
}

static qboolean NPC_SandboxTactics(void)
{
	gentity_t *npc = NPCS.NPC;
	gNPC_t *info = NPCS.NPCInfo;
	const playerState_t *ps = &NPCS.client->ps;
	if (!JAR_MP_AutonomousNPC(npc) || npc->localAnimIndex > 1 ||
	    NPCS.client->playerTeam == NPCTEAM_NEUTRAL ||
	    NPCS.client->NPC_class == CLASS_BOBAFETT || npc->next_roff_time > level.time ||
	    (info->scriptFlags & (SCF_FORCED_MARCH | SCF_FIRE_WEAPON | SCF_ALT_FIRE)) ||
	    info->charmedTime > level.time || info->surrenderTime > level.time ||
	    trap->ICARUS_TaskIDPending((sharedEntity_t *)npc, TID_MOVE_NAV) ||
	    trap->ICARUS_TaskIDPending((sharedEntity_t *)npc, TID_ANGLE_FACE) ||
	    ps->pm_type != PM_NORMAL || ps->m_iVehicleNum || npc->waterlevel > 1)
		return qfalse;

	gentity_t *leader = JAR_MP_NPCLeader(npc);
	const qboolean firearm = NPC_TacticalFirearm(ps->weapon);
	if (!firearm && !leader)
		return qfalse;

	const float pursuitRange = info->stats.visrange * 1.5f;
	if (!NPC_ValidEnemy(npc->enemy) ||
	    DistanceSquared(npc->r.currentOrigin, npc->enemy->r.currentOrigin) > pursuitRange * pursuitRange)
	{
		G_ClearEnemy(npc);
		if ((info->scriptFlags & SCF_LOOK_FOR_ENEMIES) && !(info->scriptFlags & SCF_DONT_FIRE))
			NPC_FindEnemy(qfalse);
	}

	if (!npc->enemy)
	{
		if (!leader)
			return qfalse;
		if (ps->groundEntityNum != ENTITYNUM_NONE)
			NPC_FollowSandboxLeader(leader);
		return qtrue;
	}
	if (!firearm)
		return qfalse;
	if (npc->painDebounceTime > level.time || PM_InKnockDown(ps))
		return qtrue;

	const qboolean visible = NPC_ClearLOS4(npc->enemy);
	if (visible)
	{
		VectorCopy(npc->enemy->r.currentOrigin, info->enemyLastSeenLocation);
		info->enemyLastSeenTime = level.time;
	}
	else if (!info->enemyLastSeenTime || level.time - info->enemyLastSeenTime > 5000)
	{
		G_ClearEnemy(npc);
		return qtrue;
	}

	const qboolean clearShot = visible && NPC_ShotEntity(npc->enemy, NULL) == npc->enemy->s.number;
	vec3_t aim, heading;
	if (visible)
		CalcEntitySpot(npc->enemy, SPOT_CHEST, aim);
	else
		VectorCopy(info->enemyLastSeenLocation, aim);
	const qboolean facing = NPC_TacticalAim(aim);
	if (clearShot && facing && !(info->scriptFlags & SCF_DONT_FIRE))
		WeaponThink(qtrue);
	else
		info->attackHoldTime = 0;

	if (!(info->scriptFlags & SCF_CHASE_ENEMIES) || ps->groundEntityNum == ENTITYNUM_NONE)
		return qtrue;

	VectorSubtract(aim, npc->r.currentOrigin, heading);
	heading[2] = 0;
	const float distance = VectorNormalize(heading);
	if (visible && distance < 160.0f)
		VectorScale(heading, -1.0f, heading);
	else if (visible && !clearShot && distance <= 448.0f)
	{
		const float side = npc->s.number & 1 ? 1.0f : -1.0f;
		VectorSet(heading, -heading[1] * side, heading[0] * side, 0);
	}
	else if (clearShot && distance <= 448.0f)
		return qtrue;
	if (distance > 1.0f)
		NPC_TacticalMove(heading, qfalse);
	return qtrue;
}
