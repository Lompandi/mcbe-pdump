#pragma once

#include <windows.h>

#include <unordered_map>

extern std::unordered_map<uint32_t, size_t> g_ReceiveStatistic;

extern bool g_DumpPktContent;
extern bool g_DumpDecoded;
extern size_t g_DumpPktByteMax;

inline void init_statistic() {
	for (int i = 0; i < 0x150; i++) {
		g_ReceiveStatistic[i] = 0;
	}
}

void start_encryption_callback(CONTEXT* ctx, HANDLE hproc);
void receive_callback(CONTEXT* ctx, HANDLE hproc);