// Draw the retail material at the same local bounds used by collision.
#include "qcommon/jar_volume.h"
static void CG_PortableShield(centity_t*cent){
 const auto*e=cent->gent;
 if(!e||!e->inuse||!e->classname||strcmp(e->classname,"jar_portable_shield")||!e->count||
    (cent->currentState.eFlags&EF_NODRAW)||!jar::volume::Valid(e->pos1,e->pos2))return;
 vec3_t axis[3];AnglesToAxis(cent->lerpAngles,axis);polyVert_t quad[4]{};
 for(int i=0;i<4;++i){const float y=(i==1||i==2)?e->pos2[1]:e->pos1[1],z=i>=2?e->pos2[2]:e->pos1[2];
  VectorMA(cent->lerpOrigin,y,axis[1],quad[i].xyz);VectorMA(quad[i].xyz,z,axis[2],quad[i].xyz);
  quad[i].st[0]=(i==1||i==2)?1:0;quad[i].st[1]=i>=2?0:1;memset(quad[i].modulate,255,4);
 }
 const auto shader=cgi_R_RegisterShader(e->painDebounceTime>cg.time?"gfx/misc/blue_dmgshield":"gfx/misc/blue_portashield");
 cgi_R_AddPolyToScene(shader,4,quad);
}
