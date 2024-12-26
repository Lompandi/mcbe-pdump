
#include <format>
#include <vector>

#include "utils/utils.hpp"
#include "network_events_callback.hpp"
#include "mc_packets/mc_packetid.hpp"

std::unordered_map<uint32_t, size_t> g_ReceiveStatistic;

size_t  g_RecvCount = 0x0;

bool    g_DumpDecoded;
bool    g_DumpPktContent;
size_t  g_DumpPktByteMax;

void receive_callback(CONTEXT* ctx, HANDLE hproc) {
    uintptr_t buffer_ptr = 0x0;
    size_t size = 0x0;

    ReadProcessMemory(hproc, (void*)(ctx->Rdx + 8), &size, sizeof(size), NULL);
    ReadProcessMemory(hproc, (void*)ctx->Rdx, &buffer_ptr, sizeof(buffer_ptr), NULL);

    std::vector<uint8_t> packet;
    packet.resize(size);
    ReadProcessMemory(hproc, (void*)buffer_ptr, packet.data(), size, NULL);

    auto packet_id = pkd::get_unsigned_varint((const char*)packet.data(), size);

    if (!packet_id.has_value()) {
        std::print("WARNING", "Error decoding packet.\n");
        return;
    }

    if (g_DumpPktContent) {
        std::print("[INFO] <- Receive {} bytes \n", size);
        std::print("Packet id: {:#x}, {}\n", packet_id.value_or(0xffff), pkd::pktid_to_name(packet_id.value_or(0xffff)));
        pdm::hexdump((const char*)packet.data(), size, 16, g_DumpPktByteMax, g_DumpDecoded);
    }


    //g_ReceiveStatistic[packet_id.value()] += 1;

    /*if (size > 0xA0 && (g_ReceiveStatistic[packet_id.value()] < 0x50
        || PacketId_t(packet_id.value()) == PacketId_t::SubChunkRequest)) {
        std::print("[INFO] <- Receive {} bytes \n", size);
        std::print("Packet id: {:#x}, {}\n", packet_id.value(), pkd::pktid_to_name(packet_id.value()));
        pkd::hexdump((const char*)packet.data(), size, 16, 1024, true);

        std::string hash = pkd::format_m128i(pkd::hash(packet.data(), size, 0));
        pkd::write_file(std::format("D:/bedrock-server-1.21.50.10/outputs/{:02x}-{}.dmp", packet_id.value(), hash), (const char*)packet.data(), size);
        std::print("[INFO] packet written to {:02x}-{}.dmp\n", packet_id.value(), hash);
        g_RecvCount += 1;
    }*/

    /*std::string statistic;
    for (int i = 0; i < 0x136; i++) {
        if (g_ReceiveStatistic[i] > 0) {
            statistic += std::format("{}\t\t\t- {} times\n",
                pkd::pktid_to_name(i),
                g_ReceiveStatistic[i]);
        }
    }*/

   /*if (PacketId_t(packet_id.value()) == PacketId_t::ClientCacheBlobStatus
       && size > 0x40
       && g_RecvCount < 0x70) {
        std::print("[INFO] <- Receive {} bytes \n", size);
        std::print("Packet id: {:#x}, {}\n", packet_id.value(), pkd::pktid_to_name(packet_id.value()));
        pkd::hexdump((const char*)packet.data(), size, 16, 1024, true);

        std::string hash = pkd::format_m128i(pkd::hash(packet.data(), size, 0));
        pkd::write_file(std::format("D:/bedrock-server-1.21.50.10/outputs/{:02x}-{}.dmp", packet_id.value(), hash), (const char*)packet.data(), size);
        std::print("[INFO] packet written to {:02x}-{}.dmp\n", packet_id.value(), hash);
        g_RecvCount += 1;
   }*/
}