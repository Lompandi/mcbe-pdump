
#include <thread>
#include <future>

#include "pdump.hpp"
#include "utils/utils.hpp"
#include "debug/debugger.hpp"
#include "network_events_callback.hpp"
#include "mc_packets/mc_packetid.hpp"

namespace pdm {
    std::string g_ExecutablePath;
    std::string g_OutputDumpPath = "D:\\bedrock-server-1.21.51.02\\outputs";

    static uintptr_t GetBaseAddress(HANDLE hProcess) {
        HMODULE hModules[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
            return (uintptr_t)hModules[0];
        }
        return 0x0;
    }

    pdm::Debugger_t g_Debugger;
    HANDLE hproc;

    void create_proc() {
        STARTUPINFOA si = { 0 };
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = { 0 };

        if (!CreateProcessA(
            g_ExecutablePath.data(),
            NULL,
            NULL,
            NULL,
            FALSE,
            CREATE_SUSPENDED | CREATE_NO_WINDOW,
            NULL,
            NULL,
            &si,
            &pi)) {
            std::cerr << "CreateProcess failed: " << std::hex << GetLastError() << std::endl;
            return;
        }

        hproc = pi.hProcess;
        if (!DebugActiveProcess(pi.dwProcessId)) {
            std::cerr << "Failed to attach debugger to process: " << std::hex << GetLastError() << std::endl;
            CloseHandle(hproc);
            return;
        }

        ResumeThread(pi.hThread);

        DWORD dwResult = WaitForInputIdle(hproc, INFINITE);
        if (dwResult == 0) {
            std::cerr << "Process didn't reach idle state, possible timeout!" << std::endl;
            CloseHandle(hproc);
            return;
        }

        g_Debugger.monitor(hproc);
        return;
    }

    void start() {
        g_DumpPktContent = str_to_bool(g_CmdOpts.get_flag_val("--dump"));
        g_DumpPktByteMax = std::stoull(g_CmdOpts.get_flag_val("--dump-max"));
        g_DumpDecoded    = str_to_bool(g_CmdOpts.get_flag_val("--dump-decode"));

        g_Verbose = str_to_bool(g_CmdOpts.get_flag_val("--verbose"));
        g_OutputDumpPath = g_CmdOpts.get_flag_val("--output");


        std::print("Version - WIP\n");
        std::print("created by ConsoleBreak\n\n");

        log(log_level::Info, "Launching bds instance...");

        std::future<void> proc_future = std::async(std::launch::async, create_proc);

        while (hproc == NULL) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        log(log_level::Info, "Debugger started.");

        uintptr_t mod_base = GetBaseAddress(hproc);
        log(log_level::Debug, "Base address: {:#x}", mod_base);

        init_statistic();

        g_Debugger.insert_breakpoint_tmp(mod_base + 0xD4B2D0, [](CONTEXT* ctx, HANDLE hproc) {
            log(log_level::Info, "Server started.");
        });

        g_Debugger.insert_breakpoint(mod_base + 0xD72D20, start_encryption_callback);
        g_Debugger.insert_breakpoint(mod_base + 0xD41CF8, receive_callback);

        g_Debugger.monitor(hproc);
    }

	CmdOptions_t g_CmdOpts;
}