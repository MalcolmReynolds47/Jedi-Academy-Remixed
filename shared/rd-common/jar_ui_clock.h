// Scope reduced 2D shader clocks to one draw; world time always resumes.
// GPL-2.0-only.
#ifndef JAR_UI_CLOCK_H
#define JAR_UI_CLOCK_H
class JAR_UIClock {
 float &seconds;int &milliseconds;float *shader;
 const float savedSeconds;const int savedMilliseconds;const float savedShader;
public:
 JAR_UIClock(bool freeze,float &floatTime,int &time,float *shaderTime=nullptr)
  :seconds(floatTime),milliseconds(time),shader(shaderTime),savedSeconds(floatTime),savedMilliseconds(time),savedShader(shaderTime?*shaderTime:0) {
  if(freeze){seconds=0;milliseconds=0;if(shader)*shader=0;}
 }
 ~JAR_UIClock(){seconds=savedSeconds;milliseconds=savedMilliseconds;if(shader)*shader=savedShader;}
 JAR_UIClock(const JAR_UIClock&)=delete;
 JAR_UIClock&operator=(const JAR_UIClock&)=delete;
};
#endif
