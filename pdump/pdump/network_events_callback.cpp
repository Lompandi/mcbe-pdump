
#include <format>
#include <vector>

#include "utils/utils.hpp"
#include "network_events_callback.hpp"
#include "mc_packets/mc_packetid.hpp"

std::unordered_map<uint32_t, size_t> g_ReceiveStatistic;

size_t  g_RecvCount = 0x0;

bool    g_DumpDecoded = true;
bool    g_DumpPktContent = true;
size_t  g_DumpPktByteMax = 1024;

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
        pdm::log(pdm::log_level::Warning, "Error decoding packet.\n");
        return;
    }

    auto PacketId = PacketId_t(packet_id.value());

    if (g_DumpPktContent) {
        pdm::log(pdm::log_level::Info, "<- Receive {} bytes", size);
        std::print("Packet id: {:#x}, {}\n", packet_id.value(), pdm::enum_utils::enum_name(PacketId));
        pdm::hexdump((const char*)packet.data(), size, 16, g_DumpPktByteMax, g_DumpDecoded);
    }
}

void start_encryption_callback(CONTEXT* ctx, HANDLE hproc) {
    std::vector<uint8_t> sym_key;
    size_t key_size = 0x0;

    ReadProcessMemory(hproc, (LPVOID)(ctx->Rdx + 24), &key_size, 8, NULL);
    sym_key.resize(key_size);

    if (key_size > 15) [[likely]] {
        uintptr_t pData = 0x0;
        ReadProcessMemory(hproc, (LPVOID)(ctx->Rdx), &pData, 8, NULL);
        ReadProcessMemory(hproc, (LPVOID)(pData), sym_key.data(), key_size, NULL);
    }
    else {
        ReadProcessMemory(hproc, (LPVOID)(ctx->Rdx), sym_key.data(), key_size, NULL);
    }

    pdm::log(pdm::log_level::Info, "Encryption start, key:\n");
    pdm::hexdump((const char*)sym_key.data(), key_size, 33, std::nullopt, false);
}