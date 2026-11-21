// JA:R deterministic pickup randomization, version 1. GPL-2.0-only.
#ifndef JAR_RANDOMIZER_H
#define JAR_RANDOMIZER_H
#include <cstdint>
#include <cstring>
#include <string>

namespace jar {
static const int pickupSeedMaximum = 2147483647;
static const char *const pickupCvars[] = {
    "jar_randomSeed", "jar_randomWeapons", "jar_randomAmmo", "jar_randomSupplies",
    "jar_randomWide", "jar_randomEnemies", "jar_randomLoadout",
    "jar_randomObjects", "jar_randomProjectiles"
};

static const unsigned randomizerCvarCount = sizeof(pickupCvars) / sizeof(pickupCvars[0]);

inline bool ParsePickupSeed(const char *text, uint32_t& result) {
    if (!text || !*text) return false;
    uint32_t value = 0;
    for (const char *p = text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        const uint32_t digit = uint32_t(*p - '0');
        if (value > (uint32_t(pickupSeedMaximum) - digit) / 10u) return false;
        value = value * 10u + digit;
    }
    result = value;
    return true;
}

inline uint32_t PickupHashByte(uint32_t hash, uint32_t byte) {
    return (hash ^ (byte & 255u)) * 16777619u;
}
inline uint32_t PickupHashText(uint32_t hash, const char *text) {
    while (*text) hash = PickupHashByte(hash, static_cast<unsigned char>(*text++));
    return PickupHashByte(hash, 0); // terminate fields to avoid concatenation ambiguity
}
inline uint32_t PickupHashWord(uint32_t hash, uint32_t word) {
    for (int shift = 0; shift < 32; shift += 8) hash = PickupHashByte(hash, word >> shift);
    return hash;
}
inline uint32_t PickupDecision(uint32_t seed, const char *map, uint32_t ordinal,
                               const char *category, const char *original) {
    // Fixed unsigned arithmetic and byte order, independent of std::hash, frame
    // timing, entity allocation, other categories and the gameplay RNG.
    uint32_t hash = PickupHashText(2166136261u, "jar-pickups-v1");
    hash = PickupHashWord(hash, seed);
    hash = PickupHashText(hash, map);
    hash = PickupHashWord(hash, ordinal);
    hash = PickupHashText(hash, category);
    hash = PickupHashText(hash, original);
    hash ^= hash >> 16; hash *= 0x85ebca6bu;
    hash ^= hash >> 13; hash *= 0xc2b2ae35u;
    return hash ^ (hash >> 16);
}

// v1 fixed mappings use source identity, never event/map/entity order. Keep this
// namespace and byte arithmetic stable: saved games need no serialized RNG table.
inline bool ParseRandomMode(const char *text, bool& fixed) {
    if (!text || (std::strcmp(text,"0") && std::strcmp(text,"1"))) return false;
    fixed = *text == '0'; return true;
}
inline unsigned char CanonicalByte(unsigned char c) {
    return c == '\\' ? '/' : c >= 'A' && c <= 'Z' ? c + ('a'-'A') : c;
}
inline std::string CanonicalKey(const char *text) {
    std::string key; for (; *text; ++text) key += char(CanonicalByte(*text)); return key;
}
inline uint32_t CanonicalHash(uint32_t hash, const char *text) {
    for (; *text; ++text) hash = PickupHashByte(hash,CanonicalByte(*text));
    return PickupHashByte(hash,0);
}
inline uint32_t FixedMix(uint32_t hash) {
    hash ^= hash >> 16; hash *= 0x85ebca6bu;
    hash ^= hash >> 13; hash *= 0xc2b2ae35u;
    return hash ^ (hash >> 16);
}
inline uint32_t FixedDecision(uint32_t seed, const char *domain, const char *source) {
    uint32_t hash = PickupHashWord(PickupHashText(2166136261u,"jar-fixed-mapping-v1"),seed);
    return FixedMix(CanonicalHash(PickupHashText(hash,domain),source));
}
// Rank candidates by canonical name. Registry/vector order cannot affect the
// winner; adding/removing assets can change it. Ties have a stable lexical order.
// count must be nonzero; callers first enforce their category's eligibility.
template<class Name> unsigned FixedCatalogIndex(uint32_t seed, const char *domain,
        const char *source, unsigned count, Name name) {
    const uint32_t decision=FixedDecision(seed,domain,source);
    unsigned best=0; uint32_t score=0;
    for (unsigned i=0;i<count;++i) {
        const uint32_t candidate=FixedMix(CanonicalHash(decision,name(i)));
        if (!i || candidate>score || (candidate==score && CanonicalKey(name(i))<CanonicalKey(name(best)))) {
            best=i; score=candidate;
        }
    }
    return best;
}

}
#endif
