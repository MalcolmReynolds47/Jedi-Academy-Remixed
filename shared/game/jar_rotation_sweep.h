// Conservative AABB envelopes for short turns about an arbitrary pivot.
// Included after the engine's vector types. GPL-2.0-only.
#pragma once

static inline int JAR_RotationSlices(float radians){
 const int slices=(int)ceilf(radians/(M_PI/24.f));
 return slices>0?slices:1;
}

static inline void JAR_RotationPadding(const vec3_t axis,float radians,float radius,vec3_t pad){
 const float sagitta=radius*(1-cosf(radians*.5f));
 for(int a=0;a<3;++a)pad[a]=sagitta*sqrtf(fmaxf(0,1-axis[a]*axis[a]));
}

static inline void JAR_RotationEnvelope(const vec3_t move,const vec3_t low,const vec3_t high,
 const vec3_t nextLow,const vec3_t nextHigh,const vec3_t pad,vec3_t sweepLow,vec3_t sweepHigh){
 // Endpoint boxes enclose each corner's chord. Padding also covers the arc.
 for(int a=0;a<3;++a){
  sweepLow[a]=fminf(low[a],move[a]+nextLow[a])-pad[a];
  sweepHigh[a]=fmaxf(high[a],move[a]+nextHigh[a])+pad[a];
 }
}
