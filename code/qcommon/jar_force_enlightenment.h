// Single-player temporary Force ranks. The learned ranks always remain canonical.
// GPL-2.0-only.
#pragma once
#include "game/bg_public.h"
#include "jar_force_boon.h"

namespace jar { namespace enlightenment {
enum Side { None, Light, Dark };
enum PickupTag { LightPickup = 256, DarkPickup };

struct Pickup {
 const char* classname;
 const char* title;
 const char* model;
 const char* icon;
 int tag;
};
constexpr Pickup pickups[] = {
 {"item_force_enlighten_light", "Light Force Enlightenment", "models/map_objects/mp/jedi_enlightenment.md3", "gfx/hud/mpi_jlight", LightPickup},
 {"item_force_enlighten_dark", "Dark Force Enlightenment", "models/map_objects/mp/dk_enlightenment.md3", "gfx/hud/mpi_dklight", DarkPickup},
};
constexpr char sound[] = "sound/player/enlightenment.mp3";
constexpr int seconds = 25;
constexpr int rank = 3;

constexpr int lightMask = (1<<FP_HEAL)|(1<<FP_TELEPATHY)|(1<<FP_PROTECT)|(1<<FP_ABSORB);
constexpr int darkMask = (1<<FP_GRIP)|(1<<FP_LIGHTNING)|(1<<FP_DRAIN)|(1<<FP_RAGE);
constexpr int commonMask = ((1<<NUM_FORCE_POWERS)-1)&~(lightMask|darkMask);

inline Side PickupSide(int tag) {
 return tag==LightPickup ? Light : tag==DarkPickup ? Dark : None;
}

inline Side Active(const playerState_t& ps) {
 const int encoded = ps.powerups[PW_FORCE_ENLIGHTENMENT];
 if (ps.clientNum!=0 || encoded<=3)
  return None;
 const int side = encoded&3;
 return side==Light ? Light : side==Dark ? Dark : None;
}

inline int Deadline(const playerState_t& ps) {
 return ps.powerups[PW_FORCE_ENLIGHTENMENT]&~3;
}

inline int Mask(Side side) {
 return side==None ? 0 : commonMask|(side==Light ? lightMask : darkMask);
}

inline int Rank(const playerState_t& ps,int power) {
 const int learned = ps.forcePowerLevel[power];
 return (Mask(Active(ps))&(1<<power)) ? std::max(learned,rank) : learned;
}

inline int Known(const playerState_t& ps) {
 return ps.forcePowersKnown|Mask(Active(ps));
}

inline Side Alignment(const playerState_t& ps) {
 int64_t light = 0, dark = 0;
 for (int power=0;power<NUM_FORCE_POWERS;++power) {
  if (lightMask&(1<<power))
   light += ps.forcePowerLevel[power];
  if (darkMask&(1<<power))
   dark += ps.forcePowerLevel[power];
 }
 return light>dark ? Light : light<dark ? Dark : None;
}

inline const Pickup* Definition(const char* classname) {
 for (const auto& pickup : pickups)
  if (!strcmp(classname,pickup.classname))
   return &pickup;
 return nullptr;
}

inline bool CanCollect(const playerState_t& ps,Side side) {
 if (ps.clientNum!=0 || side==None)
  return false;
 const auto aligned = Alignment(ps);
 const auto active = Active(ps);
 return (aligned==None || aligned==side) && (active==None || active==side);
}

inline void Extend(playerState_t& ps,Side side,int time,int duration) {
 const int previous = Active(ps)==side ? Deadline(ps) : 0;
 const int deadline = forceBoon::Deadline(time,previous,duration);
 // The unused Haste slot preserves both the deadline and the side in old saves.
 ps.powerups[PW_FORCE_ENLIGHTENMENT]=(deadline&~3)|side;
}

inline int AffectedActivePowers(const playerState_t& ps) {
 const int candidates = ps.forcePowersActive&Mask(Active(ps));
 int stop = 0;
 for (int power=0;power<NUM_FORCE_POWERS;++power)
  if ((candidates&(1<<power)) &&
      (!(ps.forcePowersKnown&(1<<power)) || ps.forcePowerLevel[power]<rank))
   stop |= 1<<power;
 return stop;
}
} }
