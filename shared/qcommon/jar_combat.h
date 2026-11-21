// Shared SP/MP ignition and punch admission policy. GPL-2.0-only.
#ifndef JAR_COMBAT_H
#define JAR_COMBAT_H
#include <float.h>
#define JAR_SABER_EXTENSION_CHOICES {{"Quick (default)","1"},{"Legacy","0"}}
#define JAR_FIST_INPUT_CHOICES {{"Continuous (default)","1"},{"Legacy","0"}}
static inline float JAR_ExtendSaber(float length,float maximum,int frameMs,int selected,int protectedBlade) {
    if (!(length>=-FLT_MAX && length<=FLT_MAX) || !(maximum>0 && maximum<=FLT_MAX)) return length;
    if (frameMs<=0 || length>=maximum) return length;
    const float delta=selected==1 && !protectedBlade ? maximum*((float)(frameMs)/250.0f) : maximum/10*frameMs/100;
    const float next=length+delta;
    return next>maximum ? maximum : next;
}
static inline int JAR_PunchReady(int weaponTime,int torsoTime,int kick,int legsTime) {
    return weaponTime==0 && torsoTime==0 && (!kick || legsTime==0);
}
static inline int JAR_FistsSelected(const char*value) {
    return value && value[0]=='1' && value[1]==0;
}
#endif
