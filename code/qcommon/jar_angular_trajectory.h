// SP angular paths in a world-space axis frame. GPL-2.0-only.
#ifndef JAR_ANGULAR_TRAJECTORY_H
#define JAR_ANGULAR_TRAJECTORY_H
#include "q_shared.h"
#include <cmath>
namespace jar { namespace angular {
inline bool Path(trType_t type) { return type>=TR_AXIS_STATIONARY && type<=TR_AXIS_INTERPOLATE; }
inline bool Interpolates(trType_t type) { return type==TR_INTERPOLATE || type==TR_AXIS_INTERPOLATE; }
inline bool Stationary(trType_t type) { return type==TR_STATIONARY || type==TR_AXIS_STATIONARY; }
inline void Angles(const vec3_t axes[3],vec3_t out) {
 const double horizontal=std::hypot(double(axes[0][0]),double(axes[0][1])),degrees=180.0/M_PI;
 out[0]=float(-atan2(axes[0][2],horizontal)*degrees);
 if(horizontal>1e-6){out[1]=float(atan2(axes[0][1],axes[0][0])*degrees);out[2]=float(atan2(axes[1][2],axes[2][2])*degrees);}
 else{out[1]=float(atan2(-axes[1][0],axes[1][1])*degrees);out[2]=0;}
}
inline void Compose(const vec3_t local,const vec3_t frame,vec3_t out) {
 vec3_t a[3],b[3],world[3]{};AnglesToAxis(local,a);AnglesToAxis(frame,b);
 for(int i=0;i<3;++i)for(int j=0;j<3;++j)VectorMA(world[i],a[i][j],b[j],world[i]);Angles(world,out);
}
inline void Relative(const vec3_t world,const vec3_t frame,vec3_t out) {
 vec3_t a[3],b[3],local[3];AnglesToAxis(world,a);AnglesToAxis(frame,b);
 for(int i=0;i<3;++i)for(int j=0;j<3;++j)local[i][j]=DotProduct(a[i],b[j]);Angles(local,out);
}
inline void RebaseVector(const vec3_t value,const vec3_t before,const vec3_t after,vec3_t out) {
 vec3_t a[3],b[3],result{};AnglesToAxis(before,a);AnglesToAxis(after,b);
 for(int i=0;i<3;++i)VectorMA(result,DotProduct(value,a[i]),b[i],result);VectorCopy(result,out);
}
inline void RebaseAngles(const vec3_t value,const vec3_t before,const vec3_t after,vec3_t out) {
 vec3_t axes[3];AnglesToAxis(value,axes);for(auto &axis:axes)RebaseVector(axis,before,after,axis);Angles(axes,out);
}
inline void DeltaMatrix(const vec3_t before,const vec3_t after,float out[3][3]) {
 vec3_t a[3],b[3];AnglesToAxis(before,a);AnglesToAxis(after,b);
 for(int i=0;i<3;++i)for(int j=0;j<3;++j){out[i][j]=0;for(int k=0;k<3;++k)out[i][j]+=b[k][i]*a[k][j];}
}
inline bool Rebase(trajectory_t &path,const vec3_t before,const vec3_t after) {
 if(!Path(path.trType))return false;
 RebaseAngles(path.trBase,before,after,path.trBase);RebaseVector(path.trDelta,before,after,path.trDelta);return true;
}
inline bool Evaluate(const trajectory_t &path,int time,vec3_t out) {
 if(!Path(path.trType))return false;
 if(Stationary(path.trType)||Interpolates(path.trType)){VectorCopy(path.trBase,out);return true;}
 double phase=(double(time)-path.trTime)*.001;
 if(path.trType==TR_AXIS_SINE)phase=path.trDuration>0?sin((double(time)-path.trTime)/path.trDuration*M_PI*2):0;
 const double magnitude=std::hypot(std::hypot(double(path.trDelta[0]),double(path.trDelta[1])),double(path.trDelta[2]));
 if(!std::isfinite(magnitude)||magnitude<1e-12){VectorCopy(path.trBase,out);return true;}
 vec3_t axis;for(int i=0;i<3;++i)axis[i]=float(path.trDelta[i]/magnitude);
 const double angle=std::remainder(magnitude*phase,360.0)*M_PI/180.0,c=cos(angle),s=sin(angle);
 vec3_t axes[3];AnglesToAxis(path.trBase,axes);
 for(auto &v:axes){vec3_t cross,result;CrossProduct(axis,v,cross);const float dot=DotProduct(axis,v);
  for(int i=0;i<3;++i)result[i]=float(v[i]*c+cross[i]*s+axis[i]*dot*(1-c));VectorCopy(result,v);}
 Angles(axes,out);return true;
}
inline bool Velocity(const trajectory_t &path,int time,vec3_t out) {
 if(!Path(path.trType))return false;
 double scale=1;
 if(Stationary(path.trType)||Interpolates(path.trType))scale=0;
 else if(path.trType==TR_AXIS_SINE)scale=path.trDuration>0?cos((double(time)-path.trTime)/path.trDuration*M_PI*2)*M_PI*2000/path.trDuration:0;
 // Angular paths return a world-axis velocity in degrees/second.
 VectorScale(path.trDelta,float(scale),out);return true;
}
inline void Convert(trajectory_t &path,const vec3_t frame,const vec3_t local,trType_t type) {
 vec3_t axis{},basis[3],world{};
 if(path.trDelta[ROLL]){AnglesToAxis(local,basis);VectorScale(basis[0],path.trDelta[ROLL],axis);}
 else if(path.trDelta[PITCH]){vec3_t yaw={0,local[YAW],0};AnglesToAxis(yaw,basis);VectorScale(basis[1],path.trDelta[PITCH],axis);}
 else axis[2]=path.trDelta[YAW];
 AnglesToAxis(frame,basis);for(int i=0;i<3;++i)VectorMA(world,axis[i],basis[i],world);
 VectorCopy(world,path.trDelta);path.trType=type;
}
inline void ModelAxis(const vec3_t body,const vec3_t offset,vec3_t out[3]) {
 vec3_t world;Compose(offset,body,world);AnglesToAxis(world,out);
}
inline void Interpolate(const vec3_t from,const vec3_t to,float fraction,vec3_t out) {
 // Quaternion interpolation keeps a held brush steady across Euler singularities.
 const auto quat=[](const vec3_t a,double q[4]) {
  const double p=a[PITCH]*M_PI/360,y=a[YAW]*M_PI/360,r=a[ROLL]*M_PI/360;
  const double cp=cos(p),sp=sin(p),cy=cos(y),sy=sin(y),cr=cos(r),sr=sin(r);
  q[0]=cy*cp*sr-sy*sp*cr;q[1]=cy*sp*cr+sy*cp*sr;q[2]=sy*cp*cr-cy*sp*sr;q[3]=cy*cp*cr+sy*sp*sr;
 };
 double a[4],b[4],q[4],dot=0;quat(from,a);quat(to,b);
 for(int i=0;i<4;++i)dot+=a[i]*b[i];
 if(dot<0){dot=-dot;for(auto &v:b)v=-v;}
 double wa=1-fraction,wb=fraction;
 if(dot<.9995){const double angle=acos(dot),denom=sin(angle);wa=sin((1-fraction)*angle)/denom;wb=sin(fraction*angle)/denom;}
 double norm=0;for(int i=0;i<4;++i){q[i]=a[i]*wa+b[i]*wb;norm+=q[i]*q[i];}norm=sqrt(norm);for(auto&v:q)v/=norm;
 const double x=q[0],y=q[1],z=q[2],w=q[3];
 vec3_t axes[3]={{float(1-2*(y*y+z*z)),float(2*(x*y+z*w)),float(2*(x*z-y*w))},
  {float(2*(x*y-z*w)),float(1-2*(x*x+z*z)),float(2*(y*z+x*w))},
  {float(2*(x*z+y*w)),float(2*(y*z-x*w)),float(1-2*(x*x+y*y))}};
 Angles(axes,out);
}
} }
#endif
