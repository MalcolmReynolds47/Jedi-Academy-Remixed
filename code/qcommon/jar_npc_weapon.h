// Hand-held single-player NPC weapon choices. GPL-2.0-only.
#pragma once

#include "game/weapons.h"

namespace jar { namespace npcWeapon {
struct Choice { int weapon; const char* name; };
constexpr Choice choices[] = {
 {WP_NONE,"Unarmed"}, {WP_MELEE,"Fists"}, {WP_BLASTER_PISTOL,"Blaster pistol"},
 {WP_BRYAR_PISTOL,"Bryar pistol"}, {WP_BLASTER,"E-11 blaster rifle"},
 {WP_DISRUPTOR,"Disruptor rifle"}, {WP_BOWCASTER,"Bowcaster"},
 {WP_REPEATER,"Imperial repeater"}, {WP_DEMP2,"DEMP 2"},
 {WP_FLECHETTE,"Flechette launcher"}, {WP_ROCKET_LAUNCHER,"Rocket launcher"},
 {WP_THERMAL,"Thermal detonator"}, {WP_TRIP_MINE,"Trip mine"},
 {WP_DET_PACK,"Det pack"}, {WP_CONCUSSION,"Concussion rifle"},
 {WP_STUN_BATON,"Stun baton"}, {WP_JAWA,"Jawa ion blaster"},
 {WP_TUSKEN_RIFLE,"Tusken rifle"}, {WP_TUSKEN_STAFF,"Tusken staff"},
 {WP_NOGHRI_STICK,"Noghri staff"}
};
constexpr int count = sizeof(choices) / sizeof(choices[0]);

inline const char* Name(int weapon)
{
 for (const auto& choice : choices)
  if (choice.weapon == weapon)
   return choice.name;
 return nullptr;
}
}
}
