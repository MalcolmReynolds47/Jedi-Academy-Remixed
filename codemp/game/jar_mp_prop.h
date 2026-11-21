// Snapshot contract shared by native MP collision and rider prediction.
#ifndef JAR_MP_PROP_H
#define JAR_MP_PROP_H
#include <math.h>

// A point query can use this prop's local model bounds. Movement and native
// controllers still use their own collision volumes.
static inline qboolean JAR_MP_PhysgunBox(const entityState_t *state,const char *classname){
 return state->eType==ET_GENERAL&&state->generic1==250&&classname&&
        (!Q_stricmp(classname,"jar_prop")||!Q_stricmp(classname,"jar_construct_furniture")) ? qtrue : qfalse;
}

enum { JAR_MP_AMMO_RACK_CLIP_TAG = 5 };
enum { JAR_MP_ANGULAR_TAG = 19 };
enum { JAR_MP_MODEL_HULL_TAG = 20 };

// A dedicated snapshot tag distinguishes model collision from controller hulls.
static inline qboolean JAR_MP_ModelHullState(const entityState_t* state){
 return state->eType==ET_GENERAL&&state->generic1==250&&state->bolt2==JAR_MP_MODEL_HULL_TAG&&
        state->modelindex>0&&state->modelindex<MAX_MODELS?qtrue:qfalse;
}
#define JAR_MP_AMMO_RACK_CONTENTS (CONTENTS_SHOTCLIP|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP)

static inline qboolean JAR_MP_AmmoRackClip(const entityState_t *state){
 return state->eType==ET_GENERAL&&state->generic1==250&&state->bolt2==JAR_MP_AMMO_RACK_CLIP_TAG ? qtrue : qfalse;
}

// MP's two native weapon masks omit SHOTCLIP. Extend just those queries for
// the rack. The ordinary use mask also includes BODY and CORPSE.
static inline qboolean JAR_MP_AmmoRackClipTrace(int mask){
 return (mask&JAR_MP_AMMO_RACK_CONTENTS)||mask==MASK_SHOT||mask==(MASK_SHOT|CONTENTS_LIGHTSABER) ? qtrue : qfalse;
}
static qboolean JAR_MP_PropBounds(const entityState_t*s,vec3_t mins,vec3_t maxs){
 if(s->eType!=ET_GENERAL||s->generic1!=250)return qfalse;
 for(int a=0;a<3;++a)if(!isfinite(s->origin2[a])||!isfinite(s->angles2[a])||s->origin2[a]>=s->angles2[a]||fabsf(s->origin2[a])>4096||fabsf(s->angles2[a])>4096)return qfalse;
 VectorCopy(s->origin2,mins);VectorCopy(s->angles2,maxs);return qtrue;
}

static inline qboolean JAR_MP_AngularMover(const entityState_t* state){
 return state->eType==ET_GENERAL&&state->generic1==250&&state->bolt2==JAR_MP_ANGULAR_TAG?qtrue:qfalse;
}
// Props interpolate in snapshots but carry measured velocities for a short
// prediction interval. Bound extrapolation across stale/disconnected snapshots.
static qboolean JAR_MP_PropRider(const entityState_t*s,const vec3_t in,int from,int to,vec3_t out){
 vec3_t low,high,oldOrigin,newOrigin,oldAngles,newAngles,before[3],after[3],offset;
 if(!JAR_MP_PropBounds(s,low,high))return qfalse;
 float a=(float)((double)from-s->pos.trTime)*.001f,b=(float)((double)to-s->pos.trTime)*.001f;
 if(!isfinite(a)||!isfinite(b)||fabsf(a)>.1f||fabsf(b)>.1f)return qfalse;
 for(int k=0;k<3;++k){
  if(!isfinite(s->pos.trBase[k])||!isfinite(s->apos.trBase[k])||!isfinite(s->pos.trDelta[k])||!isfinite(s->apos.trDelta[k])||fabsf(s->pos.trDelta[k])>5000||fabsf(s->apos.trDelta[k])>20000)return qfalse;
  oldOrigin[k]=s->pos.trBase[k]+a*s->pos.trDelta[k];newOrigin[k]=s->pos.trBase[k]+b*s->pos.trDelta[k];
  oldAngles[k]=s->apos.trBase[k]+a*s->apos.trDelta[k];newAngles[k]=s->apos.trBase[k]+b*s->apos.trDelta[k];
 }
 AnglesToAxis(oldAngles,before);AnglesToAxis(newAngles,after);VectorSubtract(in,oldOrigin,offset);VectorCopy(newOrigin,out);
 for(int k=0;k<3;++k)VectorMA(out,DotProduct(offset,before[k]),after[k],out);return qtrue;
}
#endif
