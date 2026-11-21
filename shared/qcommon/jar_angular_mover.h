// Portable spinner and pendulum settings and motion shared by SP and MP.
#pragma once
#include <math.h>

enum { JAR_ANGULAR_UP, JAR_ANGULAR_FORWARD, JAR_ANGULAR_SIDE, JAR_ANGULAR_AXES };
enum { JAR_SPINNER_RATE = 100, JAR_PENDULUM_SWING = 15, JAR_ANGULAR_PERIOD = 2000,
       JAR_SPINNER_MAX = 180, JAR_PENDULUM_MAX = 75, JAR_ANGULAR_MIN_PERIOD = 750,
       JAR_ANGULAR_MAX_PERIOD = 30000 };

static inline int JAR_AngularSettingsValid(int pendulum, int axis, int rate, int period, int phase)
{
 if(axis < 0 || axis >= JAR_ANGULAR_AXES)return 0;
 if(!pendulum)return rate >= -JAR_SPINNER_MAX && rate <= JAR_SPINNER_MAX && rate && period == JAR_ANGULAR_PERIOD && !phase;
 return rate >= 1 && rate <= JAR_PENDULUM_MAX && period >= JAR_ANGULAR_MIN_PERIOD &&
  period <= JAR_ANGULAR_MAX_PERIOD && phase >= 0 && phase < period;
}

static inline const char *JAR_AngularAxisLabel(int axis)
{
 static const char *const labels[] = {"Up (yaw)", "Forward (roll)", "Side (pitch)"};
 return labels[axis >= 0 && axis < JAR_ANGULAR_AXES ? axis : JAR_ANGULAR_UP];
}

static inline int JAR_AngularMaxStep(int pendulum, int rate, int period)
{
 const double maximum = pendulum ? rate * 6283.185307179586 / period : fabs((double)rate);
 return (int)(8000 / maximum); // Valid settings keep each step at eight degrees or less.
}

static inline float JAR_AngularStep(int pendulum, int rate, int period, int elapsed, int *phase)
{
 if(!pendulum)return rate * elapsed * .001f;
 const int previous = *phase;
 *phase = (int)(((long long)previous + elapsed) % period);
 const double radians = 6.283185307179586 / period;
 return (float)(rate * (sin(*phase * radians) - sin(previous * radians)));
}
