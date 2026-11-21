// Single-player ambient soundscape adapter. GPL-2.0-only.
#include "../server/exe_headers.h"
#include "../server/server.h"
#include "snd_public.h"
#include "jar_soundscape.h"

static void TraceSoundscape(trace_t* trace,const vec3_t start,const vec3_t end,int player,int mask){
 SV_TraceBrushLine(trace,start,end,player,mask);
}

#include "../../shared/client/jar_soundscape.inc"
