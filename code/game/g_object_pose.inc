// Publish native object motion and its attached rack pickups together. GPL-2.0-only.
void G_StopObjectMoving( gentity_t *object )
{
	object->s.pos.trType = TR_STATIONARY;
	VectorCopy( object->currentOrigin, object->s.origin );
	VectorCopy( object->currentOrigin, object->s.pos.trBase );
	VectorClear( object->s.pos.trDelta );
}

static bool G_AdvanceObjectPose(gentity_t& object,const vec3_t origin,const vec3_t angles)
{
	if (!G_JAR_WorkshopMoveRackStock(object, origin, angles))
	{
		// The rack body cleared the trace, but wider shelf stock did not.
		// Stop at the last clear pose without reporting a body impact.
		G_StopObjectMoving(&object);
		object.s.apos.trType = TR_STATIONARY;
		VectorCopy(object.currentAngles, object.s.apos.trBase);
		if (object.e_ThinkFunc == thinkF_G_RunObject)
		{
			object.e_ThinkFunc = thinkF_NULL;
			object.nextthink = 0;
		}
		return false;
	}
	VectorCopy(origin, object.currentOrigin);
	VectorCopy(angles, object.currentAngles);
	return true;
}
