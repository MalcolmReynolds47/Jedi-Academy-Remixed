// Native vehicle hull policy, shared with Sandbox transform validation. GPL-2.0-only.
void BG_VehicleAdjustBBoxForOrientation( Vehicle_t *veh, vec3_t origin, vec3_t mins, vec3_t maxs,
										int clientNum, int tracemask,
										void (*localTrace)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask))
{
	if ( !veh
		|| !veh->m_pVehicleInfo->length
		|| !veh->m_pVehicleInfo->width
		|| !veh->m_pVehicleInfo->height )
		//|| veh->m_LandTrace.fraction < 1.0f )
	{
		return;
	}
	else if ( veh->m_pVehicleInfo->type != VH_FIGHTER
		//&& veh->m_pVehicleInfo->type != VH_SPEEDER
		&& veh->m_pVehicleInfo->type != VH_FLIER )
	{//only those types of vehicles have dynamic bboxes, the rest just use a static bbox
		VectorSet( maxs, veh->m_pVehicleInfo->width/2.0f, veh->m_pVehicleInfo->width/2.0f, veh->m_pVehicleInfo->height+DEFAULT_MINS_2 );
		VectorSet( mins, veh->m_pVehicleInfo->width/-2.0f, veh->m_pVehicleInfo->width/-2.0f, DEFAULT_MINS_2 );
		return;
	}
	else
	{
		matrix3_t	axis;
		vec3_t		point[8], newMins, newMaxs;
		int			curAxis = 0, i;
		trace_t		trace;

		AnglesToAxis( veh->m_vOrientation, axis );
		VectorMA( origin, veh->m_pVehicleInfo->length/2.0f, axis[0], point[0] );
		VectorMA( origin, -veh->m_pVehicleInfo->length/2.0f, axis[0], point[1] );
		//extrapolate each side up and down
		VectorMA( point[0], veh->m_pVehicleInfo->height/2.0f, axis[2], point[0] );
		VectorMA( point[0], -veh->m_pVehicleInfo->height, axis[2], point[2] );
		VectorMA( point[1], veh->m_pVehicleInfo->height/2.0f, axis[2], point[1] );
		VectorMA( point[1], -veh->m_pVehicleInfo->height, axis[2], point[3] );

		VectorMA( origin, veh->m_pVehicleInfo->width/2.0f, axis[1], point[4] );
		VectorMA( origin, -veh->m_pVehicleInfo->width/2.0f, axis[1], point[5] );
		//extrapolate each side up and down
		VectorMA( point[4], veh->m_pVehicleInfo->height/2.0f, axis[2], point[4] );
		VectorMA( point[4], -veh->m_pVehicleInfo->height, axis[2], point[6] );
		VectorMA( point[5], veh->m_pVehicleInfo->height/2.0f, axis[2], point[5] );
		VectorMA( point[5], -veh->m_pVehicleInfo->height, axis[2], point[7] );
		/*
		VectorMA( origin, veh->m_pVehicleInfo->height/2.0f, axis[2], point[4] );
		VectorMA( origin, -veh->m_pVehicleInfo->height/2.0f, axis[2], point[5] );
		*/
		//Now inflate a bbox around these points
		VectorCopy( origin, newMins );
		VectorCopy( origin, newMaxs );
		for ( curAxis = 0; curAxis < 3; curAxis++ )
		{
			for ( i = 0; i < 8; i++ )
			{
				if ( point[i][curAxis] > newMaxs[curAxis] )
				{
					newMaxs[curAxis] = point[i][curAxis];
				}
				else if ( point[i][curAxis] < newMins[curAxis] )
				{
					newMins[curAxis] = point[i][curAxis];
				}
			}
		}
		VectorSubtract( newMins, origin, newMins );
		VectorSubtract( newMaxs, origin, newMaxs );
		//now see if that's a valid way to be
		if (localTrace)
		{
			localTrace( &trace, origin, newMins, newMaxs, origin, clientNum, tracemask );
		}
		else
		{ //don't care about solid stuff then
			trace.startsolid = trace.allsolid = 0;
		}
		if ( !trace.startsolid && !trace.allsolid )
		{//let's use it!
			VectorCopy( newMins, mins );
			VectorCopy( newMaxs, maxs );
		}
		//else: just use the last one, I guess...?
		//FIXME: make it as close as possible?  Or actually prevent the change in m_vOrientation?  Or push away from anything we hit?
	}
}
