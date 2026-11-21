// Shared Sandbox vertical oscillator. Phase is measured in millionths of a cycle.
#pragma once
#include <math.h>

enum {
 JAR_BOB_ID=98040309,
 JAR_BOB_AMPLITUDE=12, JAR_BOB_MIN_AMPLITUDE=1, JAR_BOB_MAX_AMPLITUDE=128,
 JAR_BOB_PERIOD=4000, JAR_BOB_MIN_PERIOD=500, JAR_BOB_MAX_PERIOD=30000,
 JAR_BOB_CYCLE=1000000, JAR_BOB_TOP=JAR_BOB_CYCLE/4, JAR_BOB_BOTTOM=3*JAR_BOB_CYCLE/4
};
enum { JAR_BOB_STOPPED, JAR_BOB_MOVING, JAR_BOB_BLOCKED };

typedef struct {
 int amplitude, periodMs, phase, running, inactive;
} jarBobState_t;

static inline int JAR_BobValid(jarBobState_t state){
 return state.amplitude>=JAR_BOB_MIN_AMPLITUDE&&state.amplitude<=JAR_BOB_MAX_AMPLITUDE&&
  state.periodMs>=JAR_BOB_MIN_PERIOD&&state.periodMs<=JAR_BOB_MAX_PERIOD&&
  state.phase>=0&&state.phase<JAR_BOB_CYCLE&&state.running>=0&&state.running<=1&&state.inactive>=0&&state.inactive<=1;
}

static inline double JAR_BobWave(int phase){
 return sin(6.28318530717958647692*phase/JAR_BOB_CYCLE);
}

static inline int JAR_BobNextTurn(int phase){
 if(phase<JAR_BOB_TOP)return JAR_BOB_TOP;
 if(phase<JAR_BOB_BOTTOM)return JAR_BOB_BOTTOM;
 return JAR_BOB_CYCLE+JAR_BOB_TOP;
}

static inline float JAR_BobHeight(int amplitude,int phase,int anchorPhase,float anchorZ){
 return (float)(anchorZ+amplitude*(JAR_BobWave(phase)-JAR_BobWave(anchorPhase)));
}
