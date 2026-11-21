// Native SP seat with Hoth Wasteland's destructible, respawning gun. GPL-2.0-only.
#pragma once
#include "g_bsp_section_wasteland.h"

namespace jar { namespace wasteland {
inline bool GunEntity(const gentity_t& e){
    return e.inuse&&e.s.time2==gunTag&&e.classname&&!strcmp(e.classname,"emplaced_gun");
}
}}

void G_WastelandGunForget(gentity_t*);
