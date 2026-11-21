// Collision path for authoritative SP/MP ropes. GPL-2.0-only.
#ifndef JAR_ROPE_PATH_H
#define JAR_ROPE_PATH_H
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define JAR_ROPE_PATH_BENDS 8
#define JAR_ROPE_PATH_TRACE_LIMIT 512
#define JAR_ROPE_PATH_RADIUS 1.8f
// Native brush sweeps have a .125 surface epsilon. Route with a larger hull
// so interpolated points cannot become startsolid in later mesh sweeps.
#define JAR_ROPE_PATH_CLEARANCE (JAR_ROPE_PATH_RADIUS+.25f)
#define JAR_ROPE_PATH_SKIN .0625f
#define JAR_ROPE_PATH_CONTENTS (MASK_SOLID|CONTENTS_BODY)
#define JAR_ROPE_PATH_NEARBY 128
#define JAR_ROPE_CONTACT_PAD .375f
#define JAR_ROPE_CONTACT_REACH 1.5f

// Only these world-space pivots need persistence. Callers own the rope's
// identity, endpoint transforms, save validation and map lifetime.
typedef struct { int count; float bend[JAR_ROPE_PATH_BENDS][3]; } jarRopePath_t;
typedef struct {
    float fraction, point[3], normal[3];
    int solid, prop; // Entity number plus one for a prop, zero for other surfaces.
    unsigned generation;
} jarRopePathHit_t;
// Sweep the rope hull through scenery, excluding both attached bodies. A
// blocked start must set solid. Fraction/point follow native trace semantics.
typedef void (*jarRopePathTrace_t)(const float*, const float*, float radius, jarRopePathHit_t*, void*);
typedef enum {
    JAR_ROPE_PATH_OK, JAR_ROPE_PATH_BLOCKED, JAR_ROPE_PATH_BUDGET, JAR_ROPE_PATH_INVALID
} jarRopePathStatus_t;
typedef struct { int prop,bend; unsigned generation; float point[3], pull[3]; } jarRopeContact_t;
typedef struct {
    float length, pullA[3], pullB[3];
    int traces, contactCount;
    jarRopePathStatus_t status;
    jarRopeContact_t contacts[JAR_ROPE_PATH_BENDS];
} jarRopePathResult_t;
typedef struct {
    jarRopePathTrace_t trace;
    void* data;
    int used, limit;
    jarRopePathStatus_t failure;
    float radius;
} jarRopePathWork_t;

static inline int JAR_RopePathPoint(const float* p) {
    for (int a=0;a<3;++a) if (!isfinite(p[a]) || fabsf(p[a])>131072) return 0;
    return 1;
}
static inline float JAR_RopePathDistance(const float* a,const float* b) {
    float d=0; for (int k=0;k<3;++k) { float v=b[k]-a[k]; d+=v*v; } return sqrtf(d);
}
static inline int JAR_RopePathValid(const jarRopePath_t* path) {
    if (!path || path->count<0 || path->count>JAR_ROPE_PATH_BENDS) return 0;
    for (int i=0;i<path->count;++i) if (!JAR_RopePathPoint(path->bend[i])) return 0;
    return 1;
}
static inline const float* JAR_RopePathNode(const jarRopePath_t* path,const float* a,const float* b,int i) {
    return i==0?a:(i==path->count+1?b:path->bend[i-1]);
}
// Include the accepted spans, shortcuts and final corner probes in the cache
// envelope. A prop entering a probe must invalidate a cached contact too.
static inline void JAR_RopePathBounds(const jarRopePath_t* path,const float* a,const float* b,float* low,float* high) {
    for (int k=0;k<3;++k) { low[k]=fminf(a[k],b[k]); high[k]=fmaxf(a[k],b[k]); }
    for (int i=0;i<path->count;++i) for (int k=0;k<3;++k) {
        low[k]=fminf(low[k],path->bend[i][k]); high[k]=fmaxf(high[k],path->bend[i][k]);
    }
    const float margin=JAR_ROPE_PATH_CLEARANCE+(path->count?JAR_ROPE_CONTACT_PAD+JAR_ROPE_CONTACT_REACH:0);
    for (int k=0;k<3;++k) { low[k]-=margin; high[k]+=margin; }
}
static inline uint64_t JAR_RopeSceneEntity(int number,unsigned generation,int contents,int model,int frame,
        const float* origin,const float* angles,const float* mins,const float* maxs) {
    uint64_t hash=UINT64_C(14695981039346656037);
    const unsigned fields[]={(unsigned)number,generation,(unsigned)contents,(unsigned)model,(unsigned)frame};
    for (int i=0;i<5;++i) hash=(hash^fields[i])*UINT64_C(1099511628211);
    const float* vectors[]={origin,angles,mins,maxs};
    for (int i=0;i<4;++i) for (int k=0;k<3;++k) {
        uint32_t bits;
        memcpy(&bits,vectors[i]+k,sizeof(bits));
        hash=(hash^bits)*UINT64_C(1099511628211);
    }
    return hash;
}
// No endpoint force can be computed from a failed solve. A successful result
// gives inward directions at each endpoint and the length of the entire path.
static inline void JAR_RopePathMeasure(const jarRopePath_t* path,const float* a,const float* b,jarRopePathResult_t* out) {
    out->length=0; memset(out->pullA,0,sizeof(out->pullA)); memset(out->pullB,0,sizeof(out->pullB));
    for (int i=0;i<=path->count;++i) {
        const float* x=JAR_RopePathNode(path,a,b,i); const float* y=JAR_RopePathNode(path,a,b,i+1);
        float d=JAR_RopePathDistance(x,y); out->length+=d;
        if (d<=.0001f) continue;
        if (i==0) for (int k=0;k<3;++k) out->pullA[k]=(y[k]-x[k])/d;
        if (i==path->count) for (int k=0;k<3;++k) out->pullB[k]=(x[k]-y[k])/d;
    }
}
static inline int JAR_RopePathTrace(jarRopePathWork_t* w,const float* a,const float* b,jarRopePathHit_t* hit) {
    if (w->failure!=JAR_ROPE_PATH_OK) return 0;
    if (w->used>=w->limit) { w->failure=JAR_ROPE_PATH_BUDGET; return 0; }
    if (!JAR_RopePathPoint(a)||!JAR_RopePathPoint(b)) { w->failure=JAR_ROPE_PATH_INVALID; return 0; }
    memset(hit,0,sizeof(*hit)); ++w->used; w->trace(a,b,w->radius,hit,w->data);
    if (!isfinite(hit->fraction)||hit->fraction<0||hit->fraction>1||!JAR_RopePathPoint(hit->point)) {
        w->failure=JAR_ROPE_PATH_INVALID; return 0;
    }
    if (hit->solid) return 0;
    for (int k=0;k<3;++k) if (fabsf(hit->point[k]-(a[k]+(b[k]-a[k])*hit->fraction))>.125f) {
        w->failure=JAR_ROPE_PATH_INVALID; return 0;
    }
    if (hit->fraction==1) return 1;
    float norm=0; for (int k=0;k<3;++k) norm+=hit->normal[k]*hit->normal[k];
    if (!isfinite(norm)||norm<.99f||norm>1.01f) w->failure=JAR_ROPE_PATH_INVALID;
    return 0;
}
static inline int JAR_RopePathClear(jarRopePathWork_t* w,const float* a,const float* b) {
    jarRopePathHit_t hit; return JAR_RopePathTrace(w,a,b,&hit);
}
static inline void JAR_RopePathRemove(jarRopePath_t* path,int i) {
    --path->count;
    for (;i<path->count;++i) memcpy(path->bend[i],path->bend[i+1],sizeof(path->bend[i]));
    memset(path->bend[path->count],0,sizeof(path->bend[path->count]));
}
static inline int JAR_RopePathBridge(jarRopePathWork_t* w,const float* a,const float* b,
        const float* entry,const float* exit,const float* direction,float height,float p[3],float q[3]) {
    for (int k=0;k<3;++k) { p[k]=entry[k]+direction[k]*height; q[k]=exit[k]+direction[k]*height; }
    if (!JAR_RopePathPoint(p)||!JAR_RopePathPoint(q)) return 0;
    return JAR_RopePathClear(w,p,q)&&JAR_RopePathClear(w,a,p)&&JAR_RopePathClear(w,q,b);
}

enum { JAR_ROPE_BEARINGS=8, JAR_ROPE_OFFSETS=3, JAR_ROPE_CANDIDATES=JAR_ROPE_BEARINGS*JAR_ROPE_OFFSETS,
    JAR_ROPE_STAGGERED_BENDS=4, JAR_ROPE_THREE_BENDS=6, JAR_ROPE_DETOUR_BENDS=8,
    JAR_ROPE_MIDDLE_PAIRS=3, JAR_ROPE_MIDDLE_SWEEPS=64, JAR_ROPE_FINAL_SWEEPS=16 };
typedef struct { float point[3], beyond[3], approach; } jarRopeOffset_t;
typedef struct { float length; int first, second; } jarRopeBridge_t;

// Find openings just across each collision face. Keep the nearest and two
// farthest tested offsets of each bearing, so a narrow opening and a wider
// route around the same face both get tested. Both ends get the same budget.
static inline int JAR_RopePathOffsets(jarRopePathWork_t* w,const float* endpoint,const float* face,
        const float directions[JAR_ROPE_BEARINGS][3],const float* advance,float depth,float reach,int allowance,
        jarRopeOffset_t out[JAR_ROPE_CANDIDATES]) {
    jarRopeOffset_t samples[JAR_ROPE_BEARINGS][JAR_ROPE_OFFSETS];
    int count[JAR_ROPE_BEARINGS]={0};
    const int limit=w->used+allowance;
    for (float height=2; !w->failure&&w->used<limit; height=fminf(reach,height*1.5f)) {
        // Sample just past the two lateral offsets as well. A geometric
        // height step alone can miss a narrow opening in a rotated brush.
        for (int sample=0; sample<JAR_ROPE_BEARINGS+2&&!w->failure&&w->used<limit; ++sample) {
            const int extra=sample>=JAR_ROPE_BEARINGS;
            const int dir=extra?sample-JAR_ROPE_BEARINGS+1:sample;
            const float displacement=height+(extra?JAR_ROPE_PATH_CLEARANCE:0);
            if (displacement>reach) continue;
            float point[3],inside[3];
            for (int k=0;k<3;++k) {
                point[k]=face[k]+directions[dir][k]*displacement;
                inside[k]=point[k]+advance[k];
            }
            if (!JAR_RopePathPoint(point)||!JAR_RopePathPoint(inside)) continue;
            jarRopePathHit_t hit;
            const int clear=JAR_RopePathTrace(w,point,inside,&hit);
            if (w->failure||(!clear&&(hit.solid||hit.fraction*depth<2*JAR_ROPE_PATH_CLEARANCE))) continue;
            // Stop short of the hit plane if another obstacle limits this leg.
            const float fraction=clear?1:hit.fraction-JAR_ROPE_PATH_SKIN/depth;
            if (count[dir]==JAR_ROPE_OFFSETS) {
                samples[dir][1]=samples[dir][2];
                --count[dir];
            }
            jarRopeOffset_t* offset=&samples[dir][count[dir]++];
            memcpy(offset->point,point,sizeof(point));
            for (int k=0;k<3;++k)
                offset->beyond[k]=point[k]+advance[k]*fraction;
            offset->approach=JAR_RopePathDistance(endpoint,point);
        }
        if (height>=reach) break;
    }
    int total=0;
    for (int dir=0;dir<JAR_ROPE_BEARINGS;++dir)
        for (int sample=0;sample<count[dir];++sample) out[total++]=samples[dir][sample];
    return total;
}

static inline int JAR_RopeBridgeOrder(const void* lhs,const void* rhs) {
    const jarRopeBridge_t* a=(const jarRopeBridge_t*)lhs;
    const jarRopeBridge_t* b=(const jarRopeBridge_t*)rhs;
    if (a->length<b->length) return -1;
    if (a->length>b->length) return 1;
    return a->first!=b->first?a->first-b->first:a->second-b->second;
}

// Test a third passage between the two outer passages. The caller sets a
// local trace allowance and retains work for validating the finished route.
static inline int JAR_RopePathMiddle(jarRopePathWork_t* w,const float* a,const float* b,
        const jarRopePathHit_t* front,const jarRopePathHit_t* back,
        const float directions[JAR_ROPE_BEARINGS][3],float reach,float p[3],float q[3]) {
    float entry[3],exit[3];
    for (int k=0;k<3;++k) {
        entry[k]=front->point[k]+JAR_ROPE_PATH_SKIN*front->normal[k];
        exit[k]=back->point[k]+JAR_ROPE_PATH_SKIN*back->normal[k];
    }
    for (float height=4; !w->failure; height=fminf(reach,height*2)) {
        for (int dir=0; dir<JAR_ROPE_BEARINGS&&!w->failure; ++dir)
            if (JAR_RopePathBridge(w,a,b,entry,exit,directions[dir],height,p,q)) return 1;
        if (height>=reach) break;
    }
    return 0;
}

// When the outer offsets lead to opposite sides, try two inner passages.
// Project the reversal along each hit face so it does not enter the brush
// on an oblique approach. Each leg still needs a native sweep.
static inline int JAR_RopePathMiddleFour(jarRopePathWork_t* w,const jarRopeOffset_t* start,
        const jarRopeOffset_t* end,const float* outerEntry,const float* outerExit,const float* forward,
        const jarRopePathHit_t* front,const jarRopePathHit_t* back,float bends[JAR_ROPE_DETOUR_BENDS][3]) {
    float near[3],far[3],axis[3],squareLength=0;
    for (int k=0;k<3;++k) {
        near[k]=outerEntry[k]-start->point[k];
        far[k]=outerExit[k]-end->point[k];
        axis[k]=near[k]-far[k];
        squareLength+=axis[k]*axis[k];
    }
    if (squareLength<.001f) return 0;
    float nearScale=0,farScale=0;
    for (int k=0;k<3;++k) {
        nearScale+=near[k]*axis[k];
        farScale+=far[k]*axis[k];
    }
    if (nearScale<=0||farScale>=0) return 0;
    float nearNormal=0,farNormal=0;
    for (int k=0;k<3;++k) {
        near[k]=axis[k]*nearScale/squareLength;
        far[k]=axis[k]*farScale/squareLength;
        nearNormal+=near[k]*front->normal[k];
        farNormal+=far[k]*back->normal[k];
    }
    for (int k=0;k<3;++k) {
        near[k]-=nearNormal*front->normal[k];
        far[k]-=farNormal*back->normal[k];
    }
    const float advance=JAR_RopePathDistance(front->point,back->point)/3;
    const float scales[]={2,1.9f,2.1f};
    for (int trial=0;trial<3&&!w->failure;++trial) {
        for (int k=0;k<3;++k) {
            bends[2][k]=front->point[k]+JAR_ROPE_PATH_SKIN*front->normal[k]+near[k]*scales[trial];
            bends[3][k]=bends[2][k]+forward[k]*advance;
            bends[5][k]=back->point[k]+JAR_ROPE_PATH_SKIN*back->normal[k]+far[k]*scales[trial];
            bends[4][k]=bends[5][k]-forward[k]*advance;
        }
        if (JAR_RopePathClear(w,start->beyond,bends[2])&&JAR_RopePathClear(w,bends[2],bends[3])&&
            JAR_RopePathClear(w,bends[3],bends[4])&&JAR_RopePathClear(w,bends[4],bends[5])&&
            JAR_RopePathClear(w,bends[5],end->beyond)) return 1;
    }
    return 0;
}

static inline int JAR_RopePathOuterPassages(jarRopePathWork_t* w,const jarRopeOffset_t* start,
        jarRopeOffset_t end,const float* b,const float* entry,const float* exit,const float* forward,int maxBends,
        const float directions[JAR_ROPE_BEARINGS][3],float reach,float bends[JAR_ROPE_DETOUR_BENDS][3]) {
    if (!JAR_RopePathClear(w,start->point,start->beyond)) return 0;
    // The samples cross inward from each outside face. Native brush
    // tolerance can require extra clearance in the outbound direction.
    if (!JAR_RopePathClear(w,end.beyond,end.point)) {
        const float distance=JAR_RopePathDistance(end.point,exit);
        if (distance<.001f) return 0;
        for (int k=0;k<3;++k) {
            const float shift=(end.point[k]-exit[k])*(JAR_ROPE_PATH_CLEARANCE*.5f/distance);
            end.point[k]+=shift;
            end.beyond[k]+=shift;
        }
        if (!JAR_RopePathClear(w,end.beyond,end.point)||!JAR_RopePathClear(w,end.point,b)) return 0;
    }
    jarRopePathHit_t front,back;
    if (JAR_RopePathTrace(w,start->beyond,end.beyond,&front)||front.solid||
        JAR_RopePathTrace(w,end.beyond,start->beyond,&back)||back.solid||w->failure) return 0;
    int count=JAR_ROPE_THREE_BENDS;
    // Choose the likely middle geometry first so a blocked narrow span
    // cannot consume the candidate pair's budget before trying its detour.
    if (maxBends>=JAR_ROPE_DETOUR_BENDS&&
        JAR_RopePathMiddleFour(w,start,&end,entry,exit,forward,&front,&back,bends))
        count=JAR_ROPE_DETOUR_BENDS;
    else if (!JAR_RopePathMiddle(w,start->beyond,end.beyond,&front,&back,directions,reach,bends[2],bends[3]))
        return 0;
    memcpy(bends[0],start->point,sizeof(bends[0]));
    memcpy(bends[1],start->beyond,sizeof(bends[1]));
    memcpy(bends[count-2],end.beyond,sizeof(bends[0]));
    memcpy(bends[count-1],end.point,sizeof(bends[0]));
    return count;
}

// The first and last collisions can be on different obstacles. A clear
// bridge may need a different offset at each end. For offset openings,
// crossing each face before turning lets the center span run between them.
static inline int JAR_RopePathStaggered(jarRopePathWork_t* w,const float* a,const float* b,
        const float* entry,const float* exit,const float* forward,const float directions[JAR_ROPE_BEARINGS][3],
        float reach,int maxBends,float bends[JAR_ROPE_DETOUR_BENDS][3]) {
    const int available=w->limit-w->used;
    if (available<48) {
        w->failure=JAR_ROPE_PATH_BUDGET;
        return 0;
    }
    // Also leave room for paired sweeps, final path validation and tightening.
    const int allowance=available/4<96?available/4:96;
    const int allowFourBends=maxBends>=JAR_ROPE_STAGGERED_BENDS;
    const float depth=JAR_RopePathDistance(entry,exit);
    const float step=allowFourBends?depth/3:fminf(2*JAR_ROPE_PATH_CLEARANCE,depth*.5f);
    float inward[3],outward[3];
    for (int k=0;k<3;++k) { inward[k]=forward[k]*step; outward[k]=-inward[k]; }
    jarRopeOffset_t from[JAR_ROPE_CANDIDATES],to[JAR_ROPE_CANDIDATES];
    const int fromCount=JAR_RopePathOffsets(w,a,entry,directions,inward,step,reach,allowance,from);
    if (!fromCount||w->failure) return 0;
    const int toCount=JAR_RopePathOffsets(w,b,exit,directions,outward,step,reach,allowance,to);
    if (!toCount||w->failure) return 0;

    jarRopeBridge_t pairs[JAR_ROPE_CANDIDATES*JAR_ROPE_CANDIDATES];
    int count=0;
    for (int i=0;i<fromCount;++i) for (int j=0;j<toCount;++j) {
        jarRopeBridge_t pair={from[i].approach+JAR_RopePathDistance(from[i].point,to[j].point)+to[j].approach,i,j};
        pairs[count++]=pair;
    }
    qsort(pairs,count,sizeof(*pairs),JAR_RopeBridgeOrder);
    // Each prefix or suffix is swept at most once, even if multiple pairs use it.
    int start[JAR_ROPE_CANDIDATES]={0},finish[JAR_ROPE_CANDIDATES]={0};
    const int passes=allowFourBends?2:1;
    jarRopeBridge_t middle[JAR_ROPE_MIDDLE_PAIRS];
    int middleCount=0;
    const int directLimit=w->used+(w->limit-w->used)/3;
    for (int pass=0;pass<passes&&!w->failure;++pass) {
        for (int k=0;k<count&&!w->failure;++k) {
            if (!pass&&passes==2&&w->used>=directLimit) break;
            if (w->limit-w->used<JAR_ROPE_FINAL_SWEEPS) {
                w->failure=JAR_ROPE_PATH_BUDGET;
                return 0;
            }
            const int i=pairs[k].first,j=pairs[k].second;
            if (start[i]<0||finish[j]<0) continue;
            const float* p=pass?from[i].beyond:from[i].point;
            const float* q=pass?to[j].beyond:to[j].point;
            const int bridge=JAR_RopePathClear(w,p,q);
            if (!bridge&&(!pass||maxBends<JAR_ROPE_THREE_BENDS||middleCount==JAR_ROPE_MIDDLE_PAIRS)) continue;
            if (!start[i]) start[i]=JAR_RopePathClear(w,a,from[i].point)?1:-1;
            if (start[i]<0) continue;
            if (!finish[j]) finish[j]=JAR_RopePathClear(w,to[j].point,b)?1:-1;
            if (finish[j]<0) continue;
            if (!bridge) {
                middle[middleCount++]=pairs[k];
                continue;
            }
            memcpy(bends[0],from[i].point,sizeof(from[i].point));
            if (!pass) {
                memcpy(bends[1],to[j].point,sizeof(to[j].point));
                return 2;
            }
            memcpy(bends[1],p,sizeof(bends[1]));
            memcpy(bends[2],q,sizeof(bends[2]));
            memcpy(bends[3],to[j].point,sizeof(to[j].point));
            return JAR_ROPE_STAGGERED_BENDS;
        }
    }
    // Spend bounded work on the best outer pairs only after the shorter
    // routes fail. Each pair gets its own allowance under the same ceiling.
    const int searchLimit=w->limit-JAR_ROPE_FINAL_SWEEPS;
    for (int k=0; k<middleCount&&!w->failure&&w->used<searchLimit; ++k) {
        jarRopePathWork_t probe=*w;
        const int pairLimit=w->used+JAR_ROPE_MIDDLE_SWEEPS;
        probe.limit=pairLimit<searchLimit?pairLimit:searchLimit;
        const int inserted=JAR_RopePathOuterPassages(&probe,&from[middle[k].first],to[middle[k].second],
            b,entry,exit,forward,maxBends,directions,reach,bends);
        w->used=probe.used;
        if (probe.failure==JAR_ROPE_PATH_INVALID||
            (probe.failure==JAR_ROPE_PATH_BUDGET&&probe.limit==searchLimit)) {
            w->failure=probe.failure;
            return 0;
        }
        if (inserted) return inserted;
    }
    return 0;
}

// Find a clear route around the occluding volume, then tighten its pivots.
// Search is bounded, deterministic, and only needed when a span is obstructed.
// Retained pivots choose the existing side of an obstacle on subsequent frames.
static inline int JAR_RopePathDetour(jarRopePathWork_t* w,const float* a,const float* b,
        const jarRopePathHit_t* front,float reach,int maxBends,float bends[JAR_ROPE_DETOUR_BENDS][3]) {
    jarRopePathHit_t back;
    if (front->solid||JAR_RopePathTrace(w,b,a,&back)||back.solid||w->failure) return 0;
    float forward[3],right[3],up[3],entry[3],exit[3];
    float d=JAR_RopePathDistance(a,b); if (d<.0001f) return 0;
    for (int k=0;k<3;++k) {
        forward[k]=(b[k]-a[k])/d;
        entry[k]=front->point[k]+JAR_ROPE_PATH_SKIN*front->normal[k];
        exit[k]=back.point[k]+JAR_ROPE_PATH_SKIN*back.normal[k];
    }
    right[0]=-forward[1]; right[1]=forward[0]; right[2]=0;
    float n=sqrtf(right[0]*right[0]+right[1]*right[1]);
    if (n<.001f) { right[0]=1; right[1]=right[2]=0; }
    else { right[0]/=n; right[1]/=n; }
    up[0]=forward[1]*right[2]-forward[2]*right[1];
    up[1]=forward[2]*right[0]-forward[0]*right[2];
    up[2]=forward[0]*right[1]-forward[1]*right[0];
    const float dirs[JAR_ROPE_BEARINGS][2]={{1,0},{0,1},{0,-1},{-1,0},{.70710678f,.70710678f},
        {.70710678f,-.70710678f},{-.70710678f,.70710678f},{-.70710678f,-.70710678f}};
    float directions[JAR_ROPE_BEARINGS][3];
    for (int dir=0;dir<JAR_ROPE_BEARINGS;++dir)
        for (int k=0;k<3;++k) directions[dir][k]=dirs[dir][0]*up[k]+dirs[dir][1]*right[k];
    int found=0; float shortest=0;
    for (int dir=0;dir<JAR_ROPE_BEARINGS && !w->failure;++dir) {
        // Reserve work for final validation and tightening a found route.
        if (found && w->limit-w->used<64) break;
        const float* direction=directions[dir];
        float low=0;
        for (float height=2; !w->failure; height=fminf(reach,height*2)) {
            float p[3],q[3];
            if (found && 2*height>shortest+d) break;
            if (JAR_RopePathBridge(w,a,b,entry,exit,direction,height,p,q)) {
                float high=height;
                for (int step=0;step<7 && w->limit-w->used>=64;++step) {
                    float mid=(low+high)*.5f,trialP[3],trialQ[3];
                    if (JAR_RopePathBridge(w,a,b,entry,exit,direction,mid,trialP,trialQ)) {
                        high=mid; memcpy(p,trialP,sizeof(p)); memcpy(q,trialQ,sizeof(q));
                    } else low=mid;
                }
                float length=JAR_RopePathDistance(a,p)+JAR_RopePathDistance(p,q)+JAR_RopePathDistance(q,b);
                if (!found||length<shortest-.001f) { found=1; shortest=length; memcpy(bends[0],p,sizeof(p)); memcpy(bends[1],q,sizeof(q)); }
                break;
            }
            if (height>=reach || (found && w->limit-w->used<64)) break;
            low=height;
        }
    }
    if (w->failure||found) return found&&!w->failure?2:0;
    return JAR_RopePathStaggered(w,a,b,entry,exit,forward,directions,reach,maxBends,bends);
}
static inline void JAR_RopePathTighten(jarRopePathWork_t* w,jarRopePath_t* path,const float* a,const float* b,int passes) {
    for (int pass=0;pass<passes;++pass) for (int i=0;i<path->count;++i) {
        if (w->limit-w->used<40) return;
        const float* prev=JAR_RopePathNode(path,a,b,i); const float* next=JAR_RopePathNode(path,a,b,i+2);
        if (JAR_RopePathClear(w,prev,next)) { JAR_RopePathRemove(path,i--); continue; }
        float delta[3],norm=0,dot=0,original[3],target[3],accepted[3];
        memcpy(original,path->bend[i],sizeof(original)); memcpy(accepted,original,sizeof(accepted));
        for (int k=0;k<3;++k) { delta[k]=next[k]-prev[k]; norm+=delta[k]*delta[k]; dot+=(original[k]-prev[k])*delta[k]; }
        float fraction=norm>.0001f?fmaxf(0,fminf(1,dot/norm)):0;
        for (int k=0;k<3;++k) target[k]=prev[k]+fraction*delta[k];
        float low=0,high=1;
        for (int step=0;step<7 && w->limit-w->used>=16;++step) {
            float mid=(low+high)*.5f,p[3]; for (int k=0;k<3;++k) p[k]=original[k]+mid*(target[k]-original[k]);
            if (JAR_RopePathClear(w,original,p)&&JAR_RopePathClear(w,prev,p)&&JAR_RopePathClear(w,p,next)) {
                low=mid; memcpy(accepted,p,sizeof(p));
            } else high=mid;
        }
        memcpy(path->bend[i],accepted,sizeof(accepted));
    }
}
// A taut bend pulls toward both neighbors. Probe around the final corner with
// enough width to cover the native sweep epsilon even at a glancing contact.
static inline void JAR_RopePathContacts(jarRopePathWork_t* w,const jarRopePath_t* path,
        const float* a,const float* b,jarRopePathResult_t* result) {
    if (w->limit-w->used<path->count) return;
    for (int i=0;i<path->count&&w->used<w->limit&&!w->failure;++i) {
        const float* corner=path->bend[i];
        const float* prev=JAR_RopePathNode(path,a,b,i);
        const float* next=JAR_RopePathNode(path,a,b,i+2);
        const float left=JAR_RopePathDistance(corner,prev),right=JAR_RopePathDistance(corner,next);
        if (left<.001f||right<.001f) continue;
        float pull[3],start[3],end[3],squared=0;
        for (int k=0;k<3;++k) {
            pull[k]=(prev[k]-corner[k])/left+(next[k]-corner[k])/right;
            squared+=pull[k]*pull[k];
        }
        if (squared<.000001f) continue;
        const float step=JAR_ROPE_CONTACT_REACH/sqrtf(squared);
        for (int k=0;k<3;++k) { start[k]=corner[k]-step*pull[k]; end[k]=corner[k]+step*pull[k]; }
        jarRopePathHit_t hit;
        const float oldRadius=w->radius;
        const float probeRadius=oldRadius+JAR_ROPE_CONTACT_PAD;
        w->radius=probeRadius;
        const int clear=JAR_RopePathTrace(w,start,end,&hit);
        w->radius=oldRadius;
        if (clear||w->failure||hit.solid||hit.fraction<.25f||!hit.prop) continue;
        jarRopeContact_t* contact=&result->contacts[result->contactCount++];
        contact->prop=hit.prop; contact->bend=i; contact->generation=hit.generation;
        for (int k=0;k<3;++k) {
            contact->point[k]=hit.point[k]-(hit.normal[k]>.0001f?probeRadius:hit.normal[k]<-.0001f?-probeRadius:0);
            contact->pull[k]=pull[k];
        }
    }
    if (w->failure) result->contactCount=0;
}
typedef struct { int prop; float linear[3], torque[3], mass; } jarRopeLoad_t;
static inline void JAR_RopeContactPoint(const jarRopeContact_t* contact,const float* origin,
        const float* mins,const float* maxs,float* point) {
    for (int k=0;k<3;++k) point[k]=fmaxf(origin[k]+mins[k],fminf(origin[k]+maxs[k],contact->point[k]));
}
// A prop can support several corners. Accumulate the full impulse and torque
// for one unit of cable tension before evaluating that body's effective mass.
static inline void JAR_RopeAddLoad(jarRopeLoad_t* loads,int* count,const jarRopeContact_t* contact,
        const float* point,const float* origin,float mass) {
    int i=0;
    while (i<*count&&loads[i].prop!=contact->prop) ++i;
    jarRopeLoad_t* load=&loads[i];
    if (i==*count) { memset(load,0,sizeof(*load)); load->prop=contact->prop; load->mass=mass; ++*count; }
    float lever[3];
    for (int k=0;k<3;++k) { lever[k]=point[k]-origin[k]; load->linear[k]+=contact->pull[k]; }
    for (int k=0;k<3;++k) {
        int j=(k+1)%3,l=(k+2)%3;
        load->torque[k]+=lever[j]*contact->pull[l]-lever[l]*contact->pull[j];
    }
}
static inline float JAR_RopeTensionImpulse(float stretch,float outward,float inverseMass,float dt) {
    if (!(stretch>0)||!(inverseMass>0)||!(dt>0)||!isfinite(stretch)||!isfinite(outward)||!isfinite(inverseMass)) return 0;
    const float stiffness=1200,damping=160;
    const float impulse=dt*(stiffness*stretch+damping*outward)/(1+dt*(damping+dt*stiffness)*inverseMass);
    return fmaxf(0,fminf(impulse,120000*dt));
}
// Transactional: BLOCKED/BUDGET/INVALID leave the input path byte-for-byte
// unchanged. The caller must not use old pivots with new endpoints after a
// failure; retain/reject the attempted move instead of pulling through walls.
// Contacts are transient and derived from the accepted world-space pivots.
static inline jarRopePathResult_t JAR_RopePathSolve(jarRopePath_t* path,const float* a,const float* b,
        float ropeLength,jarRopePathTrace_t trace,void* data,int budget) {
    jarRopePathResult_t result; memset(&result,0,sizeof(result)); result.status=JAR_ROPE_PATH_INVALID;
    if (!JAR_RopePathValid(path)||!a||!b||!JAR_RopePathPoint(a)||!JAR_RopePathPoint(b)||!trace||
        !isfinite(ropeLength)||ropeLength<.01f||ropeLength>131072||budget<1||budget>JAR_ROPE_PATH_TRACE_LIMIT) return result;
    jarRopePath_t next=*path;
    int tightenPasses=1;
    jarRopePathWork_t w={trace,data,0,budget,JAR_ROPE_PATH_OK,JAR_ROPE_PATH_CLEARANCE};
    // A moving hull can swallow a pivot. Drop that pivot before retracing its
    // neighboring spans, and otherwise retain topology around blocked shortcuts.
    for (int i=0;i<next.count && !w.failure;++i) {
        if (!JAR_RopePathClear(&w,next.bend[i],next.bend[i]) ||
            JAR_RopePathClear(&w,JAR_RopePathNode(&next,a,b,i),JAR_RopePathNode(&next,a,b,i+2))) JAR_RopePathRemove(&next,i--);
    }
    for (int i=0;i<=next.count && !w.failure;++i) {
        const float* x=JAR_RopePathNode(&next,a,b,i); const float* y=JAR_RopePathNode(&next,a,b,i+1);
        jarRopePathHit_t hit; if (JAR_RopePathTrace(&w,x,y,&hit)) continue;
        if (w.failure) break;
        if (next.count>JAR_ROPE_PATH_BENDS-2) { w.failure=JAR_ROPE_PATH_BLOCKED; break; }
        float bends[JAR_ROPE_DETOUR_BENDS][3];
        const int inserted=JAR_RopePathDetour(&w,x,y,&hit,fmaxf(64,ropeLength),JAR_ROPE_PATH_BENDS-next.count,bends);
        if (!inserted) {
            if (!w.failure) w.failure=JAR_ROPE_PATH_BLOCKED; break;
        }
        for (int k=next.count-1;k>=i;--k) memcpy(next.bend[k+inserted],next.bend[k],sizeof(next.bend[k]));
        for (int k=0;k<inserted;++k) memcpy(next.bend[i+k],bends[k],sizeof(bends[k]));
        next.count+=inserted;
        // A new detour needs a second pass to settle its provisional bends.
        tightenPasses=2;
        // The bridge is already fully traced. Continue after its last span.
        i+=inserted;
    }
    if (!w.failure) JAR_RopePathTighten(&w,&next,a,b,tightenPasses);
    if (!w.failure) for (int i=0;i<=next.count;++i) {
        if (!JAR_RopePathClear(&w,JAR_RopePathNode(&next,a,b,i),JAR_RopePathNode(&next,a,b,i+1))) {
            if (!w.failure) w.failure=JAR_ROPE_PATH_BLOCKED; break;
        }
    }
    result.status=w.failure;
    if (!w.failure) {
        *path=next;
        JAR_RopePathMeasure(path,a,b,&result);
        JAR_RopePathContacts(&w,path,a,b,&result);
    }
    result.traces=w.used;
    return result;
}
#endif
