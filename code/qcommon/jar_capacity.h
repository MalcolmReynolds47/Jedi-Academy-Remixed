// Jedi Academy Remixed capacity profile. GPL-2.0-only; see LICENSE.txt.
#ifndef JAR_CAPACITY_H
#define JAR_CAPACITY_H

#define JAR_CAPACITY_MULTIPLIER 16
#define JAR_ENTITY_BITS 14
#define JAR_ENTITY_SLOTS (1 << JAR_ENTITY_BITS)
// Optional allocations cannot use the final 128 normal slots.
#define JAR_ENTITY_RESERVE 128
#define JAR_SNAPSHOT_HISTORY (4 * JAR_ENTITY_SLOTS)
#define JAR_MESSAGE_BYTES (256 * 1024)
#define JAR_REFENTITY_BITS 15
#define JAR_GHOUL2_BITS 15
#define JAR_GHOUL2_RENDER_SURFACES (2048 * JAR_CAPACITY_MULTIPLIER)
#define JAR_SAVE_VERSION 101
#define JAR_SCHEDULED_EFFECTS 65536

static_assert(JAR_ENTITY_SLOTS == 1024 * JAR_CAPACITY_MULTIPLIER,
              "JA:R must provide sixteen times the stock entity slots");

#endif
