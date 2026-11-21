// JA:R on-foot Physgun input within the existing usercmd wire format. GPL-2.0-only.
#pragma once

static inline int JAR_PhysgunTurnActive(const playerState_t* ps,int buttons,int catcher)
{
	return !catcher && ps->weapon == WP_PHYSGUN && ps->stats[STAT_JAR_PHYSGUN_HOLD] &&
		!ps->m_iVehicleNum && (buttons & (BUTTON_ATTACK | BUTTON_USE | BUTTON_ALT_ATTACK)) == (BUTTON_ATTACK | BUTTON_USE);
}

static inline int JAR_PhysgunTurnAxis(float degrees,float* remainder)
{
	if (degrees!=degrees) degrees=0;
	const float total=Com_Clamp(-31.75f,31.75f,degrees+*remainder);
	const float scaled=total*4;
	const int steps=(int)(scaled+(scaled<0?-.5f:.5f));
	*remainder=total-steps*.25f;
	return steps & 255;
}

static inline int JAR_PackPhysgunTurn(float yaw,float pitch,float* yawRemainder,float* pitchRemainder)
{
	const int y=JAR_PhysgunTurnAxis(yaw,yawRemainder);
	const int p=JAR_PhysgunTurnAxis(pitch,pitchRemainder);
	return y | (p << 8);
}

static inline float JAR_UnpackPhysgunAxis(int packed)
{
	const int byteValue=packed & 255;
	return (byteValue>=128?byteValue-256:byteValue)*.25f;
}

static inline void JAR_RestorePhysgunCommandView(const playerState_t* ps,usercmd_t* cmd)
{
	cmd->angles[ROLL]=(ANGLE2SHORT(ps->viewangles[ROLL])-ps->delta_angles[ROLL])&65535;
	cmd->buttons&=~BUTTON_JAR_PHYSGUN_TURN;
}
