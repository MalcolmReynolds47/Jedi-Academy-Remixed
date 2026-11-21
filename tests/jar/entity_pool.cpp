#include "game/g_entity_pool.h"
#include "qcommon/jar_capacity.h"
#include "test_support.h"
#include <vector>

int main()
{
    return RunTests("entity fill, reserve, exhaustion, cooldown, reuse and restored high-water mark", [] {
        const int world = JAR_ENTITY_SLOTS - 2;
        const int optionalEnd = world - JAR_ENTITY_RESERVE;
        std::vector<bool> used(JAR_ENTITY_SLOTS, false);
        std::vector<int> freed(JAR_ENTITY_SLOTS, 0);
        used[0] = used[world] = used[world + 1] = true;
        int high = 1;
        auto allocate = [&](bool essential, int now) {
            int slot = jar::FindEntitySlot(1, high, essential ? world : optionalEnd, now,
                [&](int i) { CHECK(i > 0 && i < world); return used[i]; },
                [&](int i) { return freed[i]; });
            if (slot >= 0) { CHECK(!used[slot]); used[slot] = true; if (slot >= high) high = slot + 1; }
            return slot;
        };
        for (int i = 1; i < optionalEnd; ++i) CHECK(allocate(false, 10000) == i);
        CHECK(allocate(false, 10000) == -1);
        for (int i = optionalEnd; i < world; ++i) CHECK(allocate(true, 10000) == i);
        for (int i = 0; i < 1000; ++i) {
            CHECK(allocate(false, 10000) == -1);
            CHECK(allocate(true, 10000) == -1);
        }
        // Optional requests cannot steal a freed critical reserve slot.
        used[world - 1] = false;
        CHECK(allocate(false, 10000) == -1);
        CHECK(allocate(true, 10000) == world - 1);
        // A recently freed slot is available on the forced second pass.
        used[12000] = false; freed[12000] = 9900;
        CHECK(allocate(false, 10000) == 12000);
        used[12000] = false; used[13000] = false; freed[13000] = 5000;
        CHECK(allocate(false, 10000) == 13000);
        CHECK(allocate(false, 10000) == 12000);
        // Prefer a fresh slot over one still in its interpolation cooldown.
        high = 100; std::fill(used.begin()+100, used.begin()+world, false);
        used[50] = false; freed[50] = 9950;
        CHECK(allocate(false, 10000) == 100);
        CHECK(allocate(false, 11000) == 50);
        CHECK(used[0] && used[world] && used[world+1]);
    });
}
