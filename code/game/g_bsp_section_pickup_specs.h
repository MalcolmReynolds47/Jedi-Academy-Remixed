// Stock pickup timing and counts shared by both game modules. GPL-2.0-only.
#ifndef G_BSP_SECTION_PICKUP_SPECS_H
#define G_BSP_SECTION_PICKUP_SPECS_H
#include "../../shared/qcommon/jar_bsp_section_policy.h"
#include "g_bsp_section_korriban.h"
#include "g_bsp_section_sentinel.h"
#include "g_bsp_section_tatooine.h"
#include "g_bsp_section_taspir.h"
#include "g_bsp_section_factory.h"
#include "g_bsp_section_coruscant.h"
#include "g_bsp_section_hilltops.h"
#include "g_bsp_section_dropzone.h"
#include "g_bsp_section_wasteland.h"

namespace jar { namespace bspSection {
inline const rift::Pickup* PickupSpecs(Mode mode, size_t& count) {
    if (mode == Mode::Rift) { count = ARRAY_LEN(rift::pickups); return rift::pickups; }
    if (mode == Mode::Korriban) { count = ARRAY_LEN(korriban::pickups); return korriban::pickups; }
    if (mode == Mode::Sentinel) { count = ARRAY_LEN(sentinel::pickups); return sentinel::pickups; }
    if (mode == Mode::Tatooine) { count = ARRAY_LEN(tatooine::pickups); return tatooine::pickups; }
    if (mode == Mode::Taspir) { count = ARRAY_LEN(taspir::pickups); return taspir::pickups; }
    if (mode == Mode::Factory) { count = ARRAY_LEN(factory::pickups); return factory::pickups; }
    if (mode == Mode::Coruscant) { count = ARRAY_LEN(coruscant::pickups); return coruscant::pickups; }
    if (mode == Mode::Hilltops) { count = ARRAY_LEN(hilltops::pickups); return hilltops::pickups; }
    if (mode == Mode::DropZone) { count = ARRAY_LEN(dropzone::pickups); return dropzone::pickups; }
    if (mode == Mode::Wasteland) { count = ARRAY_LEN(wasteland::pickups); return wasteland::pickups; }
    count = 0;
    return nullptr;
}
} }
#endif
