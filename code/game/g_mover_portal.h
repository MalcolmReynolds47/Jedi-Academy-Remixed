// Keep relocated native movers from changing the map's area portals.
// GPL-2.0-only.
#pragma once

constexpr int jarDetachedMoverAreaPortal = 0x00800000; // Saved in gentity flags.

inline void G_AdjustMoverAreaPortalState(gentity_t* mover,qboolean open){
 if(!(mover->flags&jarDetachedMoverAreaPortal))gi.AdjustAreaPortalState(mover,open);
}
