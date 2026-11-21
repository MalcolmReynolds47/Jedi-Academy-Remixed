// Native holocron count is its granted Force level and already saved. GPL-2.0-only.
#ifndef G_HOLOCRON_H
#define G_HOLOCRON_H
#include "wp_saber.h"
inline bool G_HolocronEditable(const gentity_t&e){
 return e.inuse&&e.s.eType==ET_ITEM&&e.item&&e.item->giType==IT_HOLOCRON&&
  e.item->giTag>=0&&e.item->giTag<NUM_FORCE_POWERS&&e.count>=0&&e.count<NUM_FORCE_POWER_LEVELS;
}
#endif
