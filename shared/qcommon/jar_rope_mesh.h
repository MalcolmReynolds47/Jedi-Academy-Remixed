// Persistent, bounded rope geometry shared by SP and MP. GPL-2.0-only.
#ifndef JAR_ROPE_MESH_H
#define JAR_ROPE_MESH_H
#include <math.h>
#define JAR_ROPE_SEGMENTS 24
#define JAR_ROPE_VERTICES (JAR_ROPE_SEGMENTS * 16)
typedef struct { float xyz[3]; float st[2]; unsigned char color[4]; } jarRopeVertex_t;

typedef void (*jarRopeTrace_t)(const float* chord,float* point,void* data);
static inline float JAR_RopeArcLength(const float* delta,float sag){
 float length=0;for(int s=0;s<JAR_ROPE_SEGMENTS;++s){
  float t=(float)s/JAR_ROPE_SEGMENTS,u=(float)(s+1)/JAR_ROPE_SEGMENTS;
  float dz=delta[2]*(u-t)-4*sag*(u*(1-u)-t*(1-t));
  length+=sqrtf((delta[0]*delta[0]+delta[1]*delta[1])*(u-t)*(u-t)+dz*dz);
 }return length;
}
static inline int JAR_RopeMeshTrace(const float *start, const float *end, float length,
                              jarRopeVertex_t *vertices,jarRopeTrace_t trace,void* data) {
    float delta[3], distanceSquared=0, points[JAR_ROPE_SEGMENTS+1][3];
    float side[3], up[3], norm, sag;
    int a, s, face, v;
    if (!isfinite(length) || length<0 || length>8192) return 0;
    for (a=0;a<3;++a) {
        if (!isfinite(start[a]) || !isfinite(end[a]) || fabsf(start[a])>131072 || fabsf(end[a])>131072) return 0;
        delta[a]=end[a]-start[a]; distanceSquared+=delta[a]*delta[a];
    }
    // Coincident anchors retain visible slack; a taut zero-length rope has no surface.
    if (distanceSquared<.0001f && length<.01f) return 0;
    sag=0;
    if(length>sqrtf(distanceSquared)){
        float low=0,high=length;
        for(int step=0;step<12;++step){float mid=(low+high)*.5f;if(JAR_RopeArcLength(delta,mid)>length)high=mid;else low=mid;}
        sag=(low+high)*.5f;
    }
    for (s=0;s<=JAR_ROPE_SEGMENTS;++s) {
        float t=(float)s/JAR_ROPE_SEGMENTS;
        for(a=0;a<3;++a) points[s][a]=start[a]+delta[a]*t;
        float chord[3]={points[s][0],points[s][1],points[s][2]};
        points[s][2]-=4*t*(1-t)*sag;
        if(trace&&s>0&&s<JAR_ROPE_SEGMENTS&&sag>0)trace(chord,points[s],data);
    }
    // Fixed cross section is stable at vertical ropes and across a changing camera.
    side[0]=-delta[1]; side[1]=delta[0]; side[2]=0;
    norm=sqrtf(side[0]*side[0]+side[1]*side[1]);
    if(norm<.0001f) {side[0]=1;side[1]=0;} else {side[0]/=norm;side[1]/=norm;}
    for(s=0;s<JAR_ROPE_SEGMENTS;++s) {
        float tangent[3];
        for(a=0;a<3;++a) tangent[a]=points[s+1][a]-points[s][a];
        up[0]=tangent[1]*side[2]-tangent[2]*side[1];
        up[1]=tangent[2]*side[0]-tangent[0]*side[2];
        up[2]=tangent[0]*side[1]-tangent[1]*side[0];
        norm=sqrtf(up[0]*up[0]+up[1]*up[1]+up[2]*up[2]);
        if(norm<.0001f) {up[0]=0;up[1]=1;up[2]=0;} else for(a=0;a<3;++a)up[a]/=norm;
        for(face=0;face<4;++face)for(v=0;v<4;++v) {
            static const float ring[4][2]={{1,1},{-1,1},{-1,-1},{1,-1}};
            int corner=(face+(v==1||v==2))%4;
            jarRopeVertex_t *out=&vertices[(s*4+face)*4+v];
            for(a=0;a<3;++a)out->xyz[a]=points[s+(v>=2)][a]+1.2f*(ring[corner][0]*side[a]+ring[corner][1]*up[a]);
            out->st[0]=(float)(v==1||v==2);out->st[1]=(float)(s+(v>=2));
            out->color[0]=(unsigned char)(185-face*12);out->color[1]=(unsigned char)(150-face*10);out->color[2]=(unsigned char)(95-face*6);out->color[3]=255;
        }
    }
    return JAR_ROPE_VERTICES;
}
static inline int JAR_RopeMesh(const float*start,const float*end,float length,jarRopeVertex_t*vertices){return JAR_RopeMeshTrace(start,end,length,vertices,0,0);}
#endif
