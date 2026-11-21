// Galak uses model-specific surface bits without changing other NPC/vehicle bits.
// Two spare bits carry beam phase for native client effects. GPL-2.0-only.
#pragma once
#define JAR_GALAK_CHARGING (1u<<30)
#define JAR_GALAK_FIRING (1u<<31)
#define JAR_GALAK_BEAM_MASK (JAR_GALAK_CHARGING|JAR_GALAK_FIRING)
static const char *JAR_GalakSurfaceName(int index) {
 static const char *names[] = {
 "helmet",
 "l_arm_augment",
 "l_arm_middle",
 "l_arm_wrist",
 "l_hand",
 "r_arm_middle",
 "r_hand",
 "torso_antenna",
 "torso_antenna_cap_off",
 "torso_collar_off",
 "torso_eyes_mouth_off",
 "torso_galakface_off",
 "torso_galakhead_off",
 "torso_galaktorso_off",
 "torso_shield_off",
 };
 return index>=0 && index<(int)(sizeof(names)/sizeof(names[0])) ? names[index] : NULL;
}
