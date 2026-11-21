// Brush-controller specifications for portable retail CTF arenas. GPL-2.0-only.
#pragma once

namespace jar { namespace sectionCTF {
struct Lift {int model,lip,speed;const char* target;bool linear;};
struct Pad {int model,lift,delay,yaw,wait;}; // Negative yaw denotes an unrestricted trigger.
} }
