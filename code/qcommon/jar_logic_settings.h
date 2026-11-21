// Bounded settings for portable Sandbox logic markers. GPL-2.0-only.
#ifndef JAR_LOGIC_SETTINGS_H
#define JAR_LOGIC_SETTINGS_H

#include <cstring>
#include <cstdint>

namespace jar { namespace logic {
enum Kind { None, Delay, Counter, Timer, Random, Relay, Score, Kill, Give, Enable, Disable };
constexpr uint32_t delayId = 88353352u;
constexpr uint32_t counterId = 93130259u;
constexpr uint32_t timerId = 87329097u;
constexpr uint32_t randomId = 95510898u;
constexpr uint32_t relayId = 93643346u;
constexpr uint32_t scoreId = 84334693u;
constexpr uint32_t killId = 93514441u;
constexpr uint32_t giveId = 85925740u;
constexpr uint32_t enableId = 86463936u;
constexpr uint32_t disableId = 93278435u;
constexpr int maxPoints = 10000;
constexpr int maxDelay = 60000;
enum KillMode { Normal, Falling, Electrical, KillModes };
constexpr int minInterval = 100;
constexpr int outputCount = 4;
constexpr int maxHits = 1000;

inline Kind Class(const char* name) {
 if(!name)return None;
 if(!std::strcmp(name,"target_delay"))return Delay;
 if(!std::strcmp(name,"target_counter"))return Counter;
 if(!std::strcmp(name,"func_timer"))return Timer;
 if(!std::strcmp(name,"target_random"))return Random;
 if(!std::strcmp(name,"target_relay"))return Relay;
 if(!std::strcmp(name,"target_score"))return Score;
 if(!std::strcmp(name,"target_kill"))return Kill;
 if(!std::strcmp(name,"target_give"))return Give;
 if(!std::strcmp(name,"target_activate"))return Enable;
 if(!std::strcmp(name,"target_deactivate"))return Disable;
 return None;
}

inline uint32_t Catalog(Kind kind) {
 switch(kind){
 case Delay:return delayId;
 case Counter:return counterId;
 case Timer:return timerId;
 case Random:return randomId;
 case Relay:return relayId;
 case Score:return scoreId;
 case Kill:return killId;
 case Give:return giveId;
 case Enable:return enableId;
 case Disable:return disableId;
 default:return 0;
 }
}

inline bool UseGate(Kind kind) {return kind==Enable||kind==Disable;}

struct State {
 Kind kind = None;
 int delayMs = 0, jitterMs = 0;
 int hits = 0, remaining = 0;
 bool repeat = false, enabled = true;
 int cooldownMs = 0, points = 0, killMode = Normal;
};

inline bool Valid(const State& value) {
 if(value.kind==Relay)
  return value.delayMs>=0 && value.delayMs<=maxDelay && value.cooldownMs>=0 &&
   value.cooldownMs<=maxDelay && !value.jitterMs && !value.hits &&
   (value.remaining==0||value.remaining==1) && (!value.repeat||value.remaining==1) &&
   !value.points && value.killMode==Normal;
 if(value.cooldownMs)return false;
 if(value.kind==Score||value.kind==Kill){
  if(value.delayMs||value.jitterMs||value.hits||value.remaining||value.repeat)return false;
  return value.kind==Score?value.points>=-maxPoints&&value.points<=maxPoints&&value.killMode==Normal:
   !value.points&&value.killMode>=Normal&&value.killMode<KillModes;
 }
 if(value.points||value.killMode!=Normal)return false;
 if(value.kind==Give||UseGate(value.kind))return !value.delayMs&&!value.jitterMs&&!value.hits&&!value.remaining&&!value.repeat;
 if(value.kind==Delay||value.kind==Timer){
  const int minimum=value.kind==Timer?minInterval:0;
  return value.delayMs>=minimum && value.delayMs<=maxDelay && value.jitterMs>=0 &&
   value.jitterMs<=value.delayMs-minimum && value.jitterMs<=maxDelay-value.delayMs &&
   !value.hits && !value.remaining && !value.repeat;
 }
 if(value.kind==Random)
  return !value.delayMs && !value.jitterMs && !value.hits &&
   (value.remaining==0||value.remaining==1) && (!value.repeat||value.remaining==1);
 if(value.kind==Counter)
  return value.hits>=1 && value.hits<=maxHits && value.remaining>=0 &&
   value.remaining<=value.hits && !value.delayMs && !value.jitterMs;
 return false;
}

inline bool Slotted(Kind kind) {return kind==Random||kind==Give||UseGate(kind);}
inline const char* SlotCvar(Kind kind) {
 return kind==Give?"jar_giveLinkSlot":UseGate(kind)?"jar_useGateLinkSlot":"jar_randomLinkSlot";
}

inline bool OneShotMode(Kind kind) {return kind==Random||kind==Relay;}

inline bool SameSettings(const State& a,const State& b) {
 return a.kind==b.kind && a.delayMs==b.delayMs && a.jitterMs==b.jitterMs &&
  a.hits==b.hits && a.repeat==b.repeat && a.enabled==b.enabled &&
  a.cooldownMs==b.cooldownMs && a.points==b.points && a.killMode==b.killMode;
}
} }

#endif
