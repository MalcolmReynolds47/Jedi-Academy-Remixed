// Native NPC temporary/default-state precedence. GPL-2.0-only.
#pragma once
inline bState_t G_CurrentBState( gNPC_t *gNPC )
{
	if ( gNPC->tempBehavior != BS_DEFAULT )
	{//Overrides normal behavior until cleared
		return (gNPC->tempBehavior);
	}

	if( gNPC->behaviorState == BS_DEFAULT )
	{
		gNPC->behaviorState = gNPC->defaultBehavior;
	}

	return (gNPC->behaviorState);
}
