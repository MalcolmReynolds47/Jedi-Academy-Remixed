// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
#include "g_sandbox_angular.h"
void func_rotating_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if (G_SandboxAngular(*self)) {
		G_SandboxAngularUse(*self);
		return;
	}
	const bool axis = jar::angular::Path(self->s.apos.trType);
	if (axis) {VectorCopy(self->currentAngles,self->s.apos.trBase);self->s.apos.trTime=level.time;}
	if (self->s.apos.trType == TR_LINEAR || self->s.apos.trType == TR_AXIS_LINEAR)
	{
		self->s.apos.trType = axis ? TR_AXIS_STATIONARY : TR_STATIONARY;
		// stop the sound if it stops moving
		self->s.loopSound = 0;
		// play stop sound too?
		if ( VALIDSTRING( self->soundSet ) == true )
		{
			G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_END ));
		}
	}
	else
	{
		if ( VALIDSTRING( self->soundSet ) == true )
		{
			G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_START ));
			self->s.loopSound = CAS_GetBModelSound( self->soundSet, BMS_MID );
			if ( self->s.loopSound < 0 )
			{
				self->s.loopSound = 0;
			}
		}
		self->s.apos.trType = axis ? TR_AXIS_LINEAR : TR_LINEAR;
	}
}
