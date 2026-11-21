// These fixtures do not construct a portable BSP root. The native pusher and
// portable section fixtures exercise the production guard separately.
#pragma once
#include "game/g_bsp_section.h"
#include "game/g_bsp_section_motion.h"
#include <stdexcept>
BSPSectionMotionScope::BSPSectionMotionScope(const gentity_t& e) {
    if (jar::bspSection::Portable(e)) throw std::runtime_error("Portable section is outside this fixture");
}
BSPSectionMotionScope::~BSPSectionMotionScope() = default;
