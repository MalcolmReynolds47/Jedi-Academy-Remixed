// Server trigger contact. GPL-2.0-only.
#include "../game/g_bsp_section_hurt.h"
#include "../../shared/qcommon/jar_sweep_hull.h"

qboolean	SV_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *gEnt ) {
	const float	*origin, *angles;
	clipHandle_t	ch;
	trace_t			trace;

    // The game supplies the actor hull sampled at each step, which may be
    // earlier than its current pose. Use it directly for this exact OBB test.
    if ( G_BSPSectionBoxTagged(*gEnt) )
        return G_BSPSectionBoxContact(*gEnt, mins, maxs) ? qtrue : qfalse;

	// check for exact collision
	origin = gEnt->currentOrigin;
	angles = gEnt->currentAngles;

	ch = SV_ClipHandleForEntity( gEnt );
	if(G_BSPSectionBrushPad(*gEnt)||G_BSPSectionBrushHurt(*gEnt)){
		// Keep the stock angled planes. Transform all eight actor corners
		// into the brush frame instead of rotating only the box center.
		JarSweepHull hull{};
		hull.pointCount=8;
		vec3_t center;
		for(int a=0;a<3;++a)center[a]=(mins[a]+maxs[a])*.5f;
		for(int i=0;i<8;++i)for(int a=0;a<3;++a)hull.points[i][a]=(i&(1<<a)?maxs[a]:mins[a])-center[a];
		hull.faceCount=hull.edgeCount=3;
		for(int a=0;a<3;++a)hull.faces[a][a]=hull.edges[a][a]=1;
		const int mask=gEnt->s.time2==BSPSectionLavaTag?CONTENTS_NODROP:CONTENTS_TRIGGER;
		CM_HullTrace(&trace,center,center,ch,mask,origin,angles,hull);
		return trace.startsolid;
	}
	CM_TransformedBoxTrace ( &trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles );

	return trace.startsolid;
}
