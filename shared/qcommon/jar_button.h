// Stock five-frame Imperial switch and its portable momentary action.
#pragma once

#define JAR_BUTTON_ID 96653372u
#define JAR_BUTTON_MODEL "models/map_objects/imperial/switch.md3"
#define JAR_BUTTON_PRESS_SOUND "sound/movers/switches/button_08.mp3"
#define JAR_BUTTON_RETURN_SOUND "sound/movers/switches/button_09.mp3"

enum { JAR_BUTTON_IDLE, JAR_BUTTON_PRESSING, JAR_BUTTON_HELD, JAR_BUTTON_RETURNING };
enum { JAR_BUTTON_TICK=50, JAR_BUTTON_LAST=4, JAR_BUTTON_WAIT=1000, JAR_BUTTON_MAX_WAIT=30000 };
enum { JAR_BUTTON_PRESSED=1, JAR_BUTTON_RELEASED=2, JAR_BUTTON_RETURN=4 };

typedef struct { int waitMs, phase, frame, remaining; } jarButtonState_t;

static inline int JAR_ButtonFrameValid(int phase,int frame)
{
 switch(phase){
 case JAR_BUTTON_IDLE:return frame==0;
 case JAR_BUTTON_PRESSING:return frame>=0&&frame<JAR_BUTTON_LAST;
 case JAR_BUTTON_HELD:return frame==JAR_BUTTON_LAST;
 case JAR_BUTTON_RETURNING:return frame>0&&frame<=JAR_BUTTON_LAST;
 }
 return 0;
}

static inline int JAR_ButtonValid(jarButtonState_t s)
{
 if(s.waitMs<0||s.waitMs>JAR_BUTTON_MAX_WAIT||!JAR_ButtonFrameValid(s.phase,s.frame))return 0;
 if(s.phase==JAR_BUTTON_IDLE)return !s.remaining;
 return s.remaining>0&&s.remaining<=(s.phase==JAR_BUTTON_HELD?s.waitMs:JAR_BUTTON_TICK);
}

static inline int JAR_ButtonFrameStep(int phase,int frame,int nextPhase,int nextFrame)
{
 if(!JAR_ButtonFrameValid(phase,frame)||!JAR_ButtonFrameValid(nextPhase,nextFrame))return 0;
 if(phase==JAR_BUTTON_PRESSING)
  return nextFrame==frame+1&&(nextPhase==JAR_BUTTON_PRESSING||
    (nextFrame==JAR_BUTTON_LAST&&(nextPhase==JAR_BUTTON_HELD||nextPhase==JAR_BUTTON_RETURNING)));
 if(phase==JAR_BUTTON_RETURNING)
  return nextFrame==frame-1&&(nextPhase==JAR_BUTTON_RETURNING||nextPhase==JAR_BUTTON_IDLE);
 return 0;
}

static inline int JAR_ButtonAdvance(jarButtonState_t *s,int elapsed)
{
 int events=0;
 while(s->phase!=JAR_BUTTON_IDLE&&elapsed>=s->remaining){
  elapsed-=s->remaining;
  s->remaining=JAR_BUTTON_TICK;
  if(s->phase==JAR_BUTTON_PRESSING){
   if(++s->frame==JAR_BUTTON_LAST){
    s->phase=JAR_BUTTON_HELD;s->remaining=s->waitMs;
    events|=JAR_BUTTON_PRESSED;
   }
  }else if(s->phase==JAR_BUTTON_HELD){
   s->phase=JAR_BUTTON_RETURNING;events|=JAR_BUTTON_RETURN;
  }else if(!--s->frame){
   s->phase=JAR_BUTTON_IDLE;s->remaining=0;events|=JAR_BUTTON_RELEASED;
  }
 }
 if(s->phase!=JAR_BUTTON_IDLE)s->remaining-=elapsed;
 return events;
}

static inline const char* JAR_ButtonPhaseName(int phase)
{
 switch(phase){
 case JAR_BUTTON_PRESSING:return "Pressing";
 case JAR_BUTTON_HELD:return "Pressed";
 case JAR_BUTTON_RETURNING:return "Returning";
 default:return "Ready";
 }
}
