// Bounded visible mesh following the authoritative rope path. GPL-2.0-only.
#ifndef JAR_ROPE_PATH_MESH_H
#define JAR_ROPE_PATH_MESH_H
#include "jar_rope_path.h"
#include "jar_rope_mesh.h"

static inline float JAR_RopePathArc(const float* delta,float sag,int segments) {
    float length=0;
    for (int i=0;i<segments;++i) {
        float t=(float)i/segments,u=(float)(i+1)/segments;
        float z=delta[2]*(u-t)-4*sag*(u*(1-u)-t*(1-t));
        length+=sqrtf((delta[0]*delta[0]+delta[1]*delta[1])*(u-t)*(u-t)+z*z);
    }
    return length;
}
// Each physical pivot is also a curve vertex; the renderer never smooths a
// corner into scenery. All legs together retain the existing 24-segment cap.
// Sag is cosmetic slack. If clipped samples would cut across scenery, that
// leg falls back to its already swept straight span. This does not simulate
// distributed rope mass or make a blocked physical path drawable.
static inline int JAR_RopePathCurveMode(const jarRopePath_t* path,const float* a,const float* b,float ropeLength,
        float points[JAR_ROPE_SEGMENTS+1][3],jarRopePathTrace_t trace,void* data,int snapshot) {
    if (!JAR_RopePathValid(path)||!a||!b||!points||!JAR_RopePathPoint(a)||!JAR_RopePathPoint(b)||
        !isfinite(ropeLength)||ropeLength<0||ropeLength>131072) return 0;
    float nodes[JAR_ROPE_PATH_BENDS+2][3],lengths[JAR_ROPE_PATH_BENDS+1],total=0;
    int counts[JAR_ROPE_PATH_BENDS+1],legs=0;
    memcpy(nodes[0],a,sizeof(nodes[0]));
    for (int i=1;i<=path->count+1;++i) {
        const float* next=JAR_RopePathNode(path,a,b,i);
        float d=JAR_RopePathDistance(nodes[legs],next);
        if (d<.0001f) continue;
        lengths[legs]=d; counts[legs]=1; total+=d;
        memcpy(nodes[++legs],next,sizeof(nodes[0]));
    }
    // A collapsed, nonzero slack rope retains its old visible loop geometry.
    if (!legs) {
        if (ropeLength<.01f) return 0;
        legs=1; lengths[0]=0; counts[0]=JAR_ROPE_SEGMENTS;
        memcpy(nodes[1],b,sizeof(nodes[1]));
    } else for (int left=JAR_ROPE_SEGMENTS-legs;left>0;--left) {
        int longest=0;
        for (int i=1;i<legs;++i) if (lengths[i]/counts[i]>lengths[longest]/counts[longest]) longest=i;
        ++counts[longest];
    }
    jarRopePathWork_t w={trace,data,0,JAR_ROPE_SEGMENTS*2+JAR_ROPE_PATH_BENDS+2,JAR_ROPE_PATH_OK,JAR_ROPE_PATH_RADIUS};
    int at=0;
    for (int leg=0;leg<legs;++leg) {
        const float* start=nodes[leg]; const float* end=nodes[leg+1];
        // A rendered client's interpolated body can overlap a chord from an
        // older server snapshot. Keep that authoritative chord without sag.
        int straight=trace&&!JAR_RopePathClear(&w,start,end);
        if (straight&&(!snapshot||w.failure)) return 0;
        float delta[3],chord[JAR_ROPE_SEGMENTS+1][3];
        for (int k=0;k<3;++k) delta[k]=end[k]-start[k];
        float desired=total>.0001f?fmaxf(total,ropeLength)*lengths[leg]/total:ropeLength;
        float sag=0;
        if (!straight && desired>lengths[leg] && counts[leg]>1) {
            float low=0,high=desired;
            for (int step=0;step<(snapshot?24:12);++step) {
                float mid=(low+high)*.5f;
                if (JAR_RopePathArc(delta,mid,counts[leg])>desired) high=mid; else low=mid;
            }
            sag=snapshot?low:(low+high)*.5f;
        }
        for (int i=0;i<=counts[leg];++i) {
            float t=(float)i/counts[leg];
            for (int k=0;k<3;++k) points[at+i][k]=chord[i][k]=start[k]+delta[k]*t;
            points[at+i][2]-=4*t*(1-t)*sag;
            if (trace&&!straight&&sag>0&&i>0&&i<counts[leg]) {
                jarRopePathHit_t hit;
                if (!JAR_RopePathTrace(&w,chord[i],points[at+i],&hit)) {
                    if (w.failure||(hit.solid&&!snapshot)) return 0;
                    if (hit.solid||snapshot) straight=1;
                    else memcpy(points[at+i],hit.point,sizeof(hit.point));
                }
            }
        }
        if (trace&&!straight&&sag>0) for (int i=0;i<counts[leg];++i) {
            if (!JAR_RopePathClear(&w,points[at+i],points[at+i+1])) {
                if (w.failure) return 0;
                straight=1;
                break;
            }
        }
        if (straight) for (int j=0;j<=counts[leg];++j) memcpy(points[at+j],chord[j],sizeof(chord[j]));
        at+=counts[leg];
    }
    return at;
}
static inline int JAR_RopePathCurve(const jarRopePath_t* path,const float* a,const float* b,float ropeLength,
        float points[JAR_ROPE_SEGMENTS+1][3],jarRopePathTrace_t trace,void* data) {
    return JAR_RopePathCurveMode(path,a,b,ropeLength,points,trace,data,0);
}
static inline int JAR_RopePathMesh(const jarRopePath_t* path,const float* a,const float* b,float ropeLength,
        jarRopeVertex_t* vertices,jarRopePathTrace_t trace,void* data,int snapshot) {
    float points[JAR_ROPE_SEGMENTS+1][3],side[JAR_ROPE_SEGMENTS+1][3],up[JAR_ROPE_SEGMENTS+1][3];
    if (!vertices) return 0;
    int segments=JAR_RopePathCurveMode(path,a,b,ropeLength,points,trace,data,snapshot);
    if (!segments) return 0;
    for (int i=0;i<=segments;++i) {
        float tangent[3],norm=0;
        for (int k=0;k<3;++k) { tangent[k]=points[i<segments?i+1:i][k]-points[i>0?i-1:i][k]; norm+=tangent[k]*tangent[k]; }
        norm=sqrtf(norm);
        if (norm<.0001f) { tangent[0]=0; tangent[1]=0; tangent[2]=1; }
        else for (int k=0;k<3;++k) tangent[k]/=norm;
        side[i][0]=-tangent[1]; side[i][1]=tangent[0]; side[i][2]=0;
        norm=sqrtf(side[i][0]*side[i][0]+side[i][1]*side[i][1]);
        if (norm<.0001f) { side[i][0]=1; side[i][1]=0; } else { side[i][0]/=norm; side[i][1]/=norm; }
        // Keep ring winding coherent through vertical and reversing sections.
        if (i&&side[i][0]*side[i-1][0]+side[i][1]*side[i-1][1]<0) { side[i][0]*=-1; side[i][1]*=-1; }
        up[i][0]=tangent[1]*side[i][2]-tangent[2]*side[i][1];
        up[i][1]=tangent[2]*side[i][0]-tangent[0]*side[i][2];
        up[i][2]=tangent[0]*side[i][1]-tangent[1]*side[i][0];
    }
    for (int i=0;i<segments;++i) for (int face=0;face<4;++face) for (int v=0;v<4;++v) {
        const float ring[4][2]={{1,1},{-1,1},{-1,-1},{1,-1}};
        int point=i+(v>=2),corner=(face+(v==1||v==2))%4;
        jarRopeVertex_t* out=&vertices[(i*4+face)*4+v];
        for (int k=0;k<3;++k) out->xyz[k]=points[point][k]+1.2f*(ring[corner][0]*side[point][k]+ring[corner][1]*up[point][k]);
        out->st[0]=(float)(v==1||v==2); out->st[1]=(float)point;
        out->color[0]=(unsigned char)(185-face*12); out->color[1]=(unsigned char)(150-face*10);
        out->color[2]=(unsigned char)(95-face*6); out->color[3]=255;
    }
    return segments*16;
}
#endif
