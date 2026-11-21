#pragma once
#include "jar_lift.h"

typedef struct { int entity, linked; vec3_t origin, mins, maxs; } jarLiftBody_t;
enum { JAR_LIFT_MAX_SUPPORT = 64 };

static inline int JAR_LiftOverlaps(const jarLiftBody_t* deck,const jarLiftBody_t* rider)
{
 for(int a=0;a<2;++a)
  if(rider->origin[a]+rider->maxs[a]<=deck->origin[a]+deck->mins[a]+.125f||
     rider->origin[a]+rider->mins[a]>=deck->origin[a]+deck->maxs[a]-.125f)return 0;
 return 1;
}

static inline float JAR_LiftGap(const jarLiftBody_t* deck,const jarLiftBody_t* rider)
{
 return rider->origin[2]+rider->mins[2]-deck->origin[2]-deck->maxs[2];
}

static inline int JAR_LiftSupported(const jarLiftBody_t* deck,const jarLiftBody_t* rider,int grounded)
{
 const float gap=JAR_LiftGap(deck,rider);
 if(!JAR_LiftOverlaps(deck,rider))return 0;
 return gap>=-.5f&&gap<=(grounded?3.f:1.f);
}

static inline float JAR_LiftVehicleReach(float hoverHeight,float landingHeight,int landed,
                                         const vec3_t velocity,int takingOff)
{
 if(takingOff||velocity[0]<-80.f||velocity[0]>80.f||velocity[1]<-80.f||velocity[1]>80.f||
    velocity[2]<-80.f||velocity[2]>(hoverHeight>0.f?64.f:24.f))return 0;
 const float reach=(landed?landingHeight*.15f:hoverHeight)+3.f;
 return reach>JAR_LIFT_MAX_SUPPORT?JAR_LIFT_MAX_SUPPORT:reach>3.f?reach:3.f;
}

static inline void JAR_LiftQuery(const jarLiftBody_t* deck,vec3_t low,vec3_t high)
{
 for(int a=0;a<2;++a){
  low[a]=deck->origin[a]+deck->mins[a];
  high[a]=deck->origin[a]+deck->maxs[a];
 }
 low[2]=deck->origin[2]+deck->maxs[2]-2;
 high[2]=deck->origin[2]+deck->maxs[2]+JAR_LIFT_MAX_SUPPORT+2;
}

static inline int JAR_LiftAddBody(jarLiftBody_t* bodies,int* count,const jarLiftBody_t* body)
{
 if(*count>JAR_LIFT_MAX_RIDERS)return 0;
 for(int i=0;i<*count;++i)if(bodies[i].entity==body->entity)return 0;
 bodies[(*count)++]=*body;
 return 1;
}

static inline int JAR_LiftSweep(const jarLiftBody_t* bodies,int count,float dz,
                               int(*clear)(const jarLiftBody_t*,const vec3_t))
{
 for(int i=0;i<count;++i){
  vec3_t destination;
  VectorCopy(bodies[i].origin,destination);destination[2]+=dz;
  if(!(destination[2]>=-65535.f&&destination[2]<=65535.f&&clear(&bodies[i],destination)))return 0;
 }
 return 1;
}

static inline int JAR_LiftMove(const jarLiftBody_t* bodies,int count,float dz,
                              int(*clear)(const jarLiftBody_t*,const vec3_t),
                              void(*linked)(int,int),void(*place)(int,const vec3_t,int))
{
 // All participants move by the same amount. Unlink them together so they
 // cannot block each other, then commit only if every hull can make the trip.
 for(int i=0;i<count;++i)if(bodies[i].linked)linked(bodies[i].entity,0);
 const int valid=JAR_LiftSweep(bodies,count,dz,clear);
 if(valid)for(int i=0;i<count;++i){
  vec3_t destination;
  VectorCopy(bodies[i].origin,destination);destination[2]+=dz;
  place(bodies[i].entity,destination,bodies[0].entity);
 }
 for(int i=0;i<count;++i)if(bodies[i].linked)linked(bodies[i].entity,1);
 return valid;
}
