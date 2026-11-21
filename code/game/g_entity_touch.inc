// Native saved-callback dispatch, extracted from g_functions.cpp.
// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void GEntity_TouchFunc(gentity_t *self, gentity_t *other, trace_t *trace)
{
#define TOUCHCASE(blah) case touchF_ ## blah: blah(self,other,trace); break;

	switch (self->e_TouchFunc)
	{
	case touchF_portable_shield_touch: portable_shield_touch(self,other,trace); break;
	case touchF_factory_flag_touch: factory_flag_touch(self,other,trace); break;
	case touchF_siege_item_touch: siege_item_touch(self,other,trace); break;
	case touchF_NULL:
		break;

	TOUCHCASE( Touch_Item )
	TOUCHCASE( teleporter_touch )
	TOUCHCASE( charge_stick )
	TOUCHCASE( Touch_DoorTrigger )
	TOUCHCASE( Touch_PlatCenterTrigger )
	TOUCHCASE( Touch_Plat )
	TOUCHCASE( Touch_Button )
	TOUCHCASE( Touch_Multi )
	TOUCHCASE( trigger_push_touch )
	TOUCHCASE( trigger_teleporter_touch )
	TOUCHCASE( hurt_touch )
	TOUCHCASE( NPC_Touch )
	TOUCHCASE( touch_ammo_crystal_tigger )
	TOUCHCASE( funcBBrushTouch )
	TOUCHCASE( touchLaserTrap )
	TOUCHCASE( prox_mine_stick )
	TOUCHCASE( func_rotating_touch )
	TOUCHCASE( TouchTieBomb )
	TOUCHCASE( space_touch )
	TOUCHCASE( bidirectional_touch )
	TOUCHCASE( shipboundary_touch )

	default:
		Com_Error(ERR_DROP, "GEntity_TouchFunc: case %d not handled!\n",self->e_TouchFunc);
	}
}

