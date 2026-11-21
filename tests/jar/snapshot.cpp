// Include the production implementations to exercise their internal emitter.
#include "../../code/qcommon/snapshot_codec.cpp"
#include "test_support.h"
#include <vector>

serverStatic_t svs;
clientActive_t cl;

int main()
{
    return RunTests("full snapshots, high IDs, removals, ring wrap and invalid indices", [] {
        // Appended tool weapon IDs must survive both snapshot and command
        // transports, with the high possession bits intact under prediction.
        for(int tool:{29,30}){
            byte wire[32768];msg_t msg;MSG_Init(&msg,wire,sizeof(wire));
            playerState_t before{},sent{},received{};
            sent.weapon=tool;sent.stats[STAT_WEAPONS]=(1u<<1)|(1u<<29)|(1u<<30);
            MSG_WriteDeltaPlayerstate(&msg,&before,&sent);CHECK(!msg.overflowed);
            MSG_BeginReading(&msg);MSG_ReadDeltaPlayerstate(&msg,&before,&received);
            CHECK(received.weapon==tool && received.stats[STAT_WEAPONS]==sent.stats[STAT_WEAPONS]);
            MSG_Init(&msg,wire,sizeof(wire));usercmd_t oldCommand{},command{},decoded{};command.weapon=tool;
            MSG_WriteDeltaUsercmd(&msg,&oldCommand,&command);MSG_BeginReading(&msg);
            MSG_ReadDeltaUsercmd(&msg,&oldCommand,&decoded);CHECK(decoded.weapon==tool);
        }
        std::vector<entityState_t> states(JAR_SNAPSHOT_HISTORY);
        svs.snapshotEntities = states.data();
        svs.numSnapshotEntities = int(states.size());
        // New angular path kinds and their world-axis vectors survive the
        // exact entity transport used by the local client and server.
        for(auto type:{TR_AXIS_STATIONARY,TR_AXIS_LINEAR,TR_AXIS_SINE,TR_AXIS_INTERPOLATE}){
            byte wire[4096];msg_t msg;MSG_Init(&msg,wire,sizeof(wire));
            auto &sent=states[0];sent={};entityState_t received{};sent.number=7000;sent.apos.trType=type;
            sent.apos.trTime=500;sent.apos.trDuration=2400;
            const vec3_t base={89.5f,-32.25f,11.125f},axis={-24.5f,30.125f,80.75f};
            memcpy(sent.apos.trBase,base,sizeof(base));memcpy(sent.apos.trDelta,axis,sizeof(axis));
            MSG_WriteEntity(&msg,&sent,0);MSG_BeginReading(&msg);CHECK(MSG_ReadBits(&msg,GENTITYNUM_BITS)==7000);MSG_ReadEntity(&msg,&received);
            CHECK(received.number==sent.number&&received.apos.trType==type&&received.apos.trTime==500&&received.apos.trDuration==2400);
            CHECK(!memcmp(received.apos.trBase,sent.apos.trBase,sizeof(vec3_t))&&!memcmp(received.apos.trDelta,sent.apos.trDelta,sizeof(vec3_t)));
        }
        std::vector<byte> data(MAX_MSGLEN);
        clientSnapshot_t previous = {};
        clSnapshot_t oldFrame = {};
        int sequence = JAR_SNAPSHOT_HISTORY - 8000;
        bool havePrevious = false;
        auto frame = [&](const std::vector<int>& ids) {
            clientSnapshot_t current = {};
            current.first_entity = sequence;
            current.num_entities = int(ids.size());
            for (int id : ids) {
                entityState_t &state = states[sequence++ % states.size()];
                state = {};
                state.number = id;
                state.otherEntityNum = ENTITYNUM_WORLD - 1;
                state.pos.trBase[0] = float(id);
            }
            msg_t msg;
            MSG_Init(&msg, data.data(), int(data.size()));
            SV_EmitPacketEntities(havePrevious ? &previous : NULL, &current, &msg);
            CHECK(!msg.overflowed);
            MSG_BeginReading(&msg);
            clSnapshot_t parsed = {};
            CL_ParsePacketEntities(&msg, havePrevious ? &oldFrame : NULL, &parsed);
            CHECK(parsed.numEntities == int(ids.size()));
            for (size_t i = 0; i < ids.size(); ++i) {
                const entityState_t &state = cl.parseEntities[(parsed.parseEntitiesNum + i) & (MAX_PARSE_ENTITIES-1)];
                CHECK(state.number == ids[i]);
                CHECK(state.pos.trBase[0] == float(ids[i]));
                CHECK(state.otherEntityNum == ENTITYNUM_WORLD - 1);
            }
            oldFrame = parsed; previous = current; havePrevious = true;
        };
        std::vector<int> all;
        for (int i = 0; i < ENTITYNUM_WORLD; ++i) all.push_back(i);
        for (int i = 0; i < 10; ++i) frame(all);
        frame({1023, 4095, 8191, 9999, 10000, ENTITYNUM_WORLD-1});
        frame({ENTITYNUM_WORLD-1});
        frame({}); frame(all); frame({});
        msg_t msg;
        MSG_Init(&msg, data.data(), int(data.size()));
        MSG_WriteBits(&msg, 0, 1); MSG_WriteLong(&msg, svs.numSnapshotEntities);
        MSG_BeginReading(&msg);
        entityState_t out = {};
        bool rejected = false;
        try { MSG_ReadEntity(&msg, &out); } catch (const std::runtime_error&) { rejected = true; }
        CHECK(rejected);
        std::printf("entityState_t=%zu bytes; full snapshot=%zu entities; buffers=%zu MiB each\n",
            sizeof(entityState_t), all.size(), states.size()*sizeof(entityState_t)/(1024*1024));
    });
}
