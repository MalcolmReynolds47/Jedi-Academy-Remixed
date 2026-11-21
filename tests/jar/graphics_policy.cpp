#include "qcommon/jar_graphics.h"
#include "qcommon/jar_camera.h"
#include "game/g_npc_lod.h"
#include "test_support.h"
#include <limits>

int main() {
    return RunTests("graphics preset choices and optional NPC decision scheduling", [] {
        for (int selected : {-1,0,1,2,3,999}) {
            CHECK(jar::CameraPreset(selected,true)==0);
            const int preset=jar::CameraPreset(selected,false);
            CHECK(preset==(selected==1 || selected==2 ? selected : 0));
            for (float legacy:{-1.0f,0.0f,0.3f,0.5f,1.0f}) {
                CHECK(jar::CameraDamping(0,legacy)==legacy);
                CHECK(jar::CameraDamping(1,legacy)==0.7f);
                CHECK(jar::CameraDamping(2,legacy)==1.0f);
            }
        }
        CHECK(jar::CameraMotion(0)==1.0f && jar::CameraMotion(1)==0.25f && jar::CameraMotion(2)==0.0f);
        int perPage[11] = {};
        for (const auto& setting : jar::graphicsSettings) {
            CHECK(setting.page >= 0 && setting.page < 11);
            ++perPage[setting.page];
            CHECK(jar::GraphicsChoiceValid(setting, setting.initial));
            for (const char *value : setting.preset) if (value) CHECK(jar::GraphicsChoiceValid(setting, value));
            for (const auto& other : jar::graphicsSettings)
                CHECK(&setting == &other || std::strcmp(setting.name, other.name));
            if (!std::strcmp(setting.name, "jar_npcThinkLOD")) {
                CHECK(!std::strcmp(setting.initial, "0"));
                CHECK(!std::strcmp(setting.preset[0], "0") && !std::strcmp(setting.preset[1], "0"));
                CHECK(!std::strcmp(setting.preset[2], "1"));
            }
        }
        for (int i=0;i<11;++i) CHECK(perPage[i] <= (i==8 ? 13 : 8));
        for (int normal : {50, 100}) {
            CHECK(jar::NpcDecisionInterval(false, true, 9000000, normal) == normal);
            CHECK(jar::NpcDecisionInterval(true, false, 9000000, normal) == normal);
            CHECK(jar::NpcDecisionInterval(true, true, 1024*1024, normal) == normal);
            CHECK(jar::NpcDecisionInterval(true, true, std::numeric_limits<float>::quiet_NaN(), normal) == normal);
            CHECK(jar::NpcDecisionInterval(true, true, 1025*1025, normal) == 200);
            CHECK(jar::NpcDecisionInterval(true, true, 2049*2049, normal) == 400);
        }
        int normalCount = 0, reducedCount = 0;
        for (int actor = 1; actor <= 100; ++actor) {
            int next = 0;
            for (int t = 0; t < 5000; t += 50) {
                if (t%100 == 0) ++normalCount;
                if (t < next) continue;
                ++reducedCount;
                next = jar::NpcNextDecision(t, 400, actor, 50);
                CHECK(next > t && next <= t+400 && next%50 == 0);
            }
        }
        CHECK(reducedCount < normalCount / 3);
        std::printf("Synthetic scheduling only: %d normal vs %d reduced decisions in 5 seconds / 100 idle NPCs.\n", normalCount, reducedCount);
    });
}
