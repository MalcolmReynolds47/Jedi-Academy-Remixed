#include "qcommon/jar_cloud.h"
static void CG_Clouds( centity_t *cent )
{
 if(!cent->gent)return;
 const bool portable=cent->gent->model&&!strcmp(cent->gent->model,"@jar_cloud");
 if(portable)CG_General(cent);
 if((cent->gent->spawnflags&jar::cloud::offFlag)||(cent->gent->svFlags&SVF_INACTIVE)||(cent->currentState.eFlags&EF_NODRAW))return;
	refEntity_t		ent;

	// create the render entity
	memset( &ent, 0, sizeof( ent ));

	VectorCopy( cent->lerpOrigin, ent.origin );

	ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = ent.shaderRGBA[3] = 255;

	ent.radius = cent->gent->radius;
	ent.backlerp = cent->gent->wait;

	ent.reType = RT_CLOUDS;
	AnglesToAxis(cent->lerpAngles,ent.axis);

	if ( cent->gent->spawnflags & 1 ) // TUBE type, the one with a hole in the middle
	{
		ent.rotation = cent->gent->random;
		ent.renderfx = RF_GROW;// tube flag
	}

	if ( cent->gent->spawnflags & 2 ) // ALT type, uses a different shader
	{
		ent.customShader = cgi_R_RegisterShader( portable?"jar/world/cloud_alt":"gfx/world/haze2" );
	}
	else
	{
		ent.customShader = cgi_R_RegisterShader( portable?"jar/world/cloud":"gfx/world/haze" );
	}

	cgi_R_AddRefEntityToScene( &ent );
}

