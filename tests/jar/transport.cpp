#include "../../code/qcommon/net_chan.cpp"
#include "server/server.h"
#include "test_support.h"
#include <vector>
serverStatic_t svs;

int main()
{
    return RunTests("large local messages, fragmentation, ring wrap and queue pressure", [] {
        loopbacks = new loopback_t[2]();
        cvar_t quiet = {};
        showpackets = showdrop = qport = &quiet;
        netadr_t local = {}; local.type = NA_LOOPBACK;
        std::vector<byte> payload(MAX_MSGLEN), output(MAX_MSGLEN);
        for (size_t i=0; i<payload.size(); ++i) payload[i] = byte(i * 37);
        msg_t packet;
        netadr_t from;
        for (int direction=0; direction<2; ++direction) {
            netchan_t sender = {}, receiver = {};
            sender.sock = netsrc_t(direction); receiver.sock = netsrc_t(direction ^ 1);
            sender.remoteAddress = local; sender.outgoingSequence = 1;
            for (int length : {0, 1, 16000, FRAGMENT_SIZE, 32768, 65536, 200000, MAX_MSGLEN-4}) {
                Netchan_Transmit(&sender, length, payload.data());
                int complete = 0;
                while (true) {
                    MSG_Init(&packet, output.data(), int(output.size()));
                    if (!NET_GetLoopPacket(receiver.sock, &from, &packet)) break;
                    if (Netchan_Process(&receiver, &packet)) {
                        ++complete;
                        CHECK(packet.cursize - packet.readcount == length);
                        CHECK(!std::memcmp(packet.data + packet.readcount, payload.data(), length));
                    }
                }
                CHECK(complete == 1);
            }
        }
        // Exercise many wraps and queue exhaustion without overwriting unread packets.
        for (int i=0; i<40; ++i) {
            int length = 130001 + i;
            NET_SendLoopPacket(NS_SERVER, length, payload.data(), local);
            MSG_Init(&packet, output.data(), int(output.size()));
            CHECK(NET_GetLoopPacket(NS_CLIENT, &from, &packet));
            CHECK(packet.cursize == length);
            CHECK(!std::memcmp(output.data(), payload.data(), length));
        }
        for (int i=0; i<20; ++i) NET_SendLoopPacket(NS_SERVER, MAX_MSGLEN, payload.data(), local);
        int accepted = 0;
        while (true) {
            MSG_Init(&packet, output.data(), int(output.size()));
            if (!NET_GetLoopPacket(NS_CLIENT, &from, &packet)) break;
            ++accepted; CHECK(packet.cursize == MAX_MSGLEN);
            CHECK(!std::memcmp(output.data(), payload.data(), payload.size()));
        }
        CHECK(accepted > 0 && accepted < 20);
        NET_SendLoopPacket(NS_SERVER, MAX_MSGLEN+1, payload.data(), local);
        CHECK(!NET_GetLoopPacket(NS_CLIENT, &from, &packet));
        delete[] loopbacks; loopbacks = NULL;
    });
}
