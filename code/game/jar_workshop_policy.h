// Shared native-state predicates for the inventory and owned-object tools.
// GPL-2.0-only.
#ifndef JAR_WORKSHOP_POLICY_H
#define JAR_WORKSHOP_POLICY_H
#include "qcommon/q_shared.h"
#include "bg_public.h"
namespace jar { namespace workshop {
inline bool NativeControlAllowed(const playerState_t& ps,int health,int time,
 bool scriptedCamera,bool riding,bool weaponsLocked,bool humanoid,bool protectedAnimation) {
 return health>0 && !scriptedCamera && !ps.viewEntity && !riding && !weaponsLocked && humanoid &&
  !protectedAnimation && ps.saberLockTime<=time &&
  !(ps.eFlags&(EF_IN_ATST|EF_LOCKED_TO_WEAPON|EF_HELD_BY_RANCOR|EF_HELD_BY_WAMPA|
    EF_HELD_BY_SAND_CREATURE|EF_FORCE_GRIPPED|EF_FORCE_DRAINED));
}
inline bool NativeEquipAllowed(const playerState_t& ps,bool controlled,bool knockdown,bool roll) {
 // Native saber and gun raising both settle in IDLE, not always READY.
 // Empty hands can remain in zero-time RAISING after a vehicle/script exit.
 // The control/weapon lock checks still protect authored sequences.
 return controlled && !ps.saberInFlight && ps.weaponTime<=0 &&
  (ps.weaponstate==WEAPON_READY || ps.weaponstate==WEAPON_IDLE || (ps.weapon==WP_NONE && ps.weaponstate==WEAPON_RAISING)) && !knockdown && !roll;
}
} }
#endif
