#include "qcommon/q_shared.h"
#include "qcommon/jar_bolt.h"
#include "qcommon/jar_savegame.h"
#include "qcommon/ojk_i_saved_game.h"
#include "qcommon/ojk_saved_game_helper.h"
#include "test_support.h"
#include <vector>

#include "memory_save.h"

int main()
{
    return RunTests("high entity IDs in attachments and save serializers", [] {
        for (int entity : {0, 127, 128, 1023, 4095, 8191, 8192, 9999, 16381}) {
            for (int model : {0, 1, MODEL_AND}) {
                for (int bolt : {1, 127, BOLT_AND}) {
                    int packed = 0;
                    CHECK(jar::PackEntityBolt(packed, entity, model, bolt));
                    CHECK(int((uint32_t(packed) >> ENTITY_SHIFT) & ENTITY_AND) == entity);
                    CHECK(int((uint32_t(packed) >> MODEL_SHIFT) & MODEL_AND) == model);
                    CHECK(int(uint32_t(packed) & BOLT_AND) == bolt);
                    CHECK(packed != -1 && packed != 0);
                    if (entity >= 8192) CHECK(packed < 0);
                }
            }
        }
        int packed = 123;
        CHECK(!jar::PackEntityBolt(packed, ENTITYNUM_WORLD, 0, 0));
        CHECK(!jar::PackEntityBolt(packed, -1, 0, 0));
        CHECK(!jar::PackEntityBolt(packed, 1, MODEL_AND + 1, 0));
        CHECK(!jar::PackEntityBolt(packed, 1, 0, BOLT_AND + 1));
        CHECK(packed == 123);

        CHECK(jar::WriteSaveVersion(5,false)==5);
        CHECK(jar::WriteSaveVersion(5,true)==jar::actorRecoverySaveV1);
        CHECK(jar::ReadableSaveVersion(5,5));
        CHECK(jar::ReadableSaveVersion(jar::actorRecoverySaveV1,5));
        CHECK(jar::ReadableSaveVersion(jar::workshopSaveV1,5));
        CHECK(jar::ReadableSaveVersion(jar::workshopSaveV2,5));
    CHECK(jar::ReadableSaveVersion(jar::workshopSaveV3,5));
        CHECK(jar::ReadableSaveVersion(jar::workshopSaveV4,5));
    CHECK(jar::ReadableSaveVersion(jar::workshopSaveV5,5));
    CHECK(jar::ReadableSaveVersion(jar::workshopSaveV6,5));
    CHECK(jar::ReadableSaveVersion(jar::workshopSaveV9,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV10,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV11,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV12,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV13,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV14,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV15,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV16,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV17,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV18,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV19,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV20,1));
 CHECK(jar::ReadableSaveVersion(jar::workshopSaveV30,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV31,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV32,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV33,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV34,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV35,5));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV36,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV37,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV38,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV39,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV40,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV41,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV42,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV43,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV44,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV45,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV46,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV47,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV48,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV49,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV50,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV51,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV52,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV53,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV54,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV55,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV56,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV57,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV58,1));CHECK(!jar::ReadableSaveVersion(jar::workshopSaveV58+1,5));
        CHECK(!jar::ReadableSaveVersion(4,5));
        MemorySave buffer;
        ojk::SavedGameHelper helper(&buffer);
        for (int entity : {0, 127, 128, 1023, 8192, 16381, ENTITYNUM_WORLD, ENTITYNUM_NONE}) {
            buffer.reset_buffer();
            trace_t source = {}, result = {};
            source.entityNum = entity;
            source.fraction = 0.75f;
            source.endpos[2] = 123.5f;
            source.G2CollisionMap[0].mEntityNum = entity;
            source.sg_export(helper);
            buffer.reset_buffer_offset();
            result.sg_import(helper);
            CHECK(result.entityNum == entity);
            CHECK(result.G2CollisionMap[0].mEntityNum == entity);
            CHECK(result.fraction == source.fraction && result.endpos[2] == source.endpos[2]);
            buffer.ensure_all_data_read();

            buffer.reset_buffer();
            entityState_t state = {}, restored = {};
            state.number = entity; state.otherEntityNum = entity; state.groundEntityNum = entity;
            state.sg_export(helper); buffer.reset_buffer_offset(); restored.sg_import(helper);
            CHECK(restored.number == entity && restored.otherEntityNum == entity && restored.groundEntityNum == entity);
            buffer.ensure_all_data_read();
        }
    });
}
