// Server-only integration between the stock E-Web rig and the portable seat.
// GPL-2.0-only.
#pragma once

qboolean JAR_MP_NativeEWebSeat(gentity_t *gun, gentity_t *user);
void JAR_MP_NativeEWebDismounted(gentity_t *gun);
gentity_t *JAR_MP_NativeEmplacedUpdateSeat(gentity_t *gun);
