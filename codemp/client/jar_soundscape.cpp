// Multiplayer ambient soundscape adapter. GPL-2.0-only.
#include "qcommon/qcommon.h"
#include "qcommon/cm_public.h"
#include "snd_public.h"
#include "../../code/client/jar_soundscape.h"

static void TraceSoundscape(trace_t* trace,const vec3_t start,const vec3_t end,int,int mask){
    // A remote client has the map collision tree. The converted maps' walls
    // and water are part of model zero.
    CM_BoxTrace(trace,start,end,vec3_origin,vec3_origin,0,mask,qfalse);
}

#include "../../shared/client/jar_soundscape.inc"
