// Read stock recordings of the graphical client using its real packet-entity parser.
#include <fstream>

static void DemoGameState(msg_t *msg) {
    // A gamestate starts a new set of baselines and delta history.
    memset(cl.entityBaselines, 0, sizeof(cl.entityBaselines));
    memset(cl.snapshots, 0, sizeof(cl.snapshots));
    cl.parseEntitiesNum = 0;
    MSG_ReadLong(msg);
    for (int op = MSG_ReadByte(msg); op != svc_EOF; op = MSG_ReadByte(msg)) {
        if (op == svc_configstring) {
            MSG_ReadShort(msg);
            MSG_ReadBigString(msg);
        } else if (op == svc_baseline) {
            int number = MSG_ReadBits(msg, GENTITYNUM_BITS);
            if (number < 0 || number >= ENTITYNUM_WORLD)
                throw std::runtime_error("Invalid demo baseline");
            entityState_t empty{};
            MSG_ReadDeltaEntity(msg, &empty, &cl.entityBaselines[number], number);
        } else {
            throw std::runtime_error("Unexpected demo gamestate command");
        }
    }
    MSG_ReadLong(msg); // Client slot.
    MSG_ReadLong(msg); // Pure checksum feed.
    if (MSG_ReadShort(msg) != 0)
        throw std::runtime_error("Unsupported demo terrain data");
}

enum class DemoView { Crowd, Rider, Vehicle, Timing };

static void DemoCrowdFrame(msg_t *msg, int sequence, DemoView view) {
    clSnapshot_t frame{};
    frame.messageNum = sequence;
    frame.serverTime = MSG_ReadLong(msg);
    const int delta = MSG_ReadByte(msg);
    frame.snapFlags = MSG_ReadByte(msg);
    const int areaBytes = MSG_ReadByte(msg);
    if (areaBytes < 0 || static_cast<size_t>(areaBytes) > sizeof(frame.areamask))
        throw std::runtime_error("Invalid demo area mask");
    MSG_ReadData(msg, frame.areamask, areaBytes);
    clSnapshot_t *prior = delta ? &cl.snapshots[(sequence - delta) & PACKET_MASK] : nullptr;
    if (prior && (!prior->valid || prior->messageNum != sequence - delta ||
                  cl.parseEntitiesNum - prior->parseEntitiesNum > MAX_PARSE_ENTITIES - 128))
        throw std::runtime_error("Missing demo delta base");
    MSG_ReadDeltaPlayerstate(msg, prior ? &prior->ps : nullptr, &frame.ps);
    if (frame.ps.m_iVehicleNum)
        MSG_ReadDeltaPlayerstate(msg, prior ? &prior->vps : nullptr, &frame.vps, qtrue);
    CL_ParsePacketEntities(msg, prior, &frame);
    frame.valid = qtrue;
    cl.snapshots[sequence & PACKET_MASK] = frame;
    if (frame.snapFlags & SNAPFLAG_NOT_ACTIVE)
        return;
    if (view == DemoView::Timing) {
        printf("%d %d\n", frame.serverTime, (frame.snapFlags & SNAPFLAG_RATE_DELAYED) != 0);
        return;
    }
    printf("%d %d %d", frame.serverTime, frame.ps.clientNum, frame.ps.stats[STAT_HEALTH]);
    const bool vehicle = view == DemoView::Vehicle;
    if (view != DemoView::Crowd)
        printf(" s,%d,%d,%.9g,%.9g,%.9g", frame.ps.clientNum,
               vehicle ? frame.ps.m_iVehicleNum : frame.ps.groundEntityNum,
               frame.ps.origin[0], frame.ps.origin[1], frame.ps.origin[2]);
    if (vehicle && frame.ps.m_iVehicleNum)
        printf(" v,%d,%d,%.9g,%.9g,%.9g", frame.ps.m_iVehicleNum, frame.vps.m_iVehicleNum,
               frame.vps.origin[0], frame.vps.origin[1], frame.vps.origin[2]);
    for (int i = 0; i < frame.numEntities; ++i) {
        const auto &entity = cl.parseEntities[(frame.parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
        if (vehicle && entity.number == frame.ps.m_iVehicleNum && frame.ps.m_iVehicleNum)
            continue; // The driver receives the full vehicle playerstate above.
        if (view != DemoView::Crowd && entity.eType == ET_PLAYER && entity.number != frame.ps.clientNum)
            printf(" r,%d,%d,%.9g,%.9g,%.9g", entity.number,
                   vehicle ? entity.m_iVehicleNum : entity.groundEntityNum,
                   entity.pos.trBase[0], entity.pos.trBase[1], entity.pos.trBase[2]);
        else if (vehicle && entity.eType == ET_NPC && entity.NPC_class == CLASS_VEHICLE)
            printf(" v,%d,%d,%.9g,%.9g,%.9g", entity.number, entity.m_iVehicleNum,
                   entity.pos.trBase[0], entity.pos.trBase[1], entity.pos.trBase[2]);
        else if (entity.eType == ET_NPC || (entity.eType == ET_GENERAL && entity.generic1 == 250))
            printf(" %c,%d,%d,%.9g,%.9g,%.9g", entity.eType == ET_NPC ? 'n' : 'p', entity.number,
                   entity.eFlags, entity.pos.trBase[0], entity.pos.trBase[1], entity.pos.trBase[2]);
    }
    puts("");
}

static int ReadCrowdDemo(const char *path, DemoView view = DemoView::Crowd) {
    std::ifstream stream(path, std::ios::binary);
    std::vector<byte> bytes(MAX_MSGLEN);
    try {
        for (;;) {
            int32_t header[2];
            if (!stream.read(reinterpret_cast<char *>(header), sizeof(header)))
                throw std::runtime_error("Missing demo end marker");
            const int sequence = LittleLong(header[0]), size = LittleLong(header[1]);
            if (size == -1 && sequence == -1)
                return 0;
            if (sequence < 0 || size < 0 || size > MAX_MSGLEN ||
                !stream.read(reinterpret_cast<char *>(bytes.data()), size))
                throw std::runtime_error("Incomplete demo packet");
            msg_t msg;
            MSG_Init(&msg, bytes.data(), MAX_MSGLEN);
            msg.cursize = size;
            MSG_BeginReading(&msg);
            MSG_ReadLong(&msg);
            for (int op = MSG_ReadByte(&msg); op != svc_EOF; op = MSG_ReadByte(&msg)) {
                switch (op) {
                case svc_nop: case svc_mapchange: break;
                case svc_serverCommand: MSG_ReadLong(&msg); MSG_ReadString(&msg); break;
                case svc_setgame: MSG_ReadString(&msg); break;
                case svc_gamestate: DemoGameState(&msg); break;
                case svc_snapshot: DemoCrowdFrame(&msg, sequence, view); break;
                default: throw std::runtime_error("Unexpected demo command");
                }
            }
            if (msg.readcount > msg.cursize)
                throw std::runtime_error("Truncated demo payload");
        }
    } catch (const std::exception &error) {
        fprintf(stderr, "%s: %s\n", path, error.what());
        return 1;
    }
}
