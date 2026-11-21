// JA:R MP capacity contract, shared by native server, client and modules.
// GPL-2.0-only. Counts are capacity limits, not performance guarantees.
#pragma once
#define JAR_MP_ENTITY_BITS 14
#define JAR_MP_ENTITY_SLOTS (1 << JAR_MP_ENTITY_BITS)
#define JAR_MP_ENTITY_RESERVE 128
#define JAR_MP_SNAPSHOT_ENTITIES 4096
#define JAR_MP_SNAPSHOT_HISTORY_LIMIT (512 * 1024)
#define JAR_MP_MESSAGE_BYTES (4 * 1024 * 1024)
#define JAR_MP_MODEL_SLOTS 2048
#define JAR_MP_SOUND_SLOTS 2048
#define JAR_MP_CONFIGSTRINGS 8192
#define JAR_MP_GAMESTATE_CHARS (512 * 1024)
#define JAR_MP_REFENTITY_BITS 15
#define JAR_MP_GHOUL_BITS 15
#define JAR_MP_PROP_LIMIT 4096
#define JAR_MP_ATTACHMENT_LIMIT 128
