// Scoped collision exclusion for motion of a portable section. GPL-2.0-only.
#ifndef G_BSP_SECTION_MOTION_H
#define G_BSP_SECTION_MOTION_H
#include <vector>

class BSPSectionMotionScope {
    std::vector<gentity_t*> unlinked;
    bool allowed = true;
public:
    explicit BSPSectionMotionScope(const gentity_t& root);
    ~BSPSectionMotionScope();
    BSPSectionMotionScope(const BSPSectionMotionScope&) = delete;
    BSPSectionMotionScope& operator=(const BSPSectionMotionScope&) = delete;
    explicit operator bool() const { return allowed; }

    // Unlinked children move inside the section hull. Include bodies in that
    // conservative sweep so a previously clear child cannot enter an actor.
    int Mask(int mask) const { return unlinked.empty() ? mask : mask | CONTENTS_BODY; }
};
#endif
