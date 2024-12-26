
#include <windows.h>
#include <psapi.h>

#include <print>
#include <thread>
#include <future>
#include <optional>

#include "network_events_callback.hpp"
#include "utils/utils.hpp"
#include "debugger.hpp"
#include "pdump.hpp"

namespace pdm {

    std::string g_ExecutablePath;

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
            CREATE_SUSPENDED | CREATE_NEW_CONSOLE,
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
        //init global options
        g_DumpPktContent = str_to_bool(g_CmdOpts.get_flag_val("--dump"));
        g_DumpPktByteMax = std::stoull(g_CmdOpts.get_flag_val("--dump-max"));
        g_DumpDecoded    = str_to_bool(g_CmdOpts.get_flag_val("--dump-decode"));

        g_Verbose = str_to_bool(g_CmdOpts.get_flag_val("--verbose"));

        std::print("Version - WIP\n");
        std::print("created by ConsoleBreak\n\n");

        std::print("[INFO] Launching bds instance...\n");
        std::future<void> proc_future = std::async(std::launch::async, create_proc);

        while (hproc == NULL) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        std::print("[INFO] Debugger started.\n");
        uintptr_t mod_base = 0x0;
        while (mod_base == 0x0) {
            mod_base = GetBaseAddress(hproc);
            std::print("[INFO] Base address: {:#x}", mod_base);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        init_statistic();

        // signature for the func: 41 C1 ?? 0C 41 80 ?? 03
        // in the function, search for whole func

        g_Debugger.insert_breakpoint(mod_base + 0xD41CF8, receive_callback);
        g_Debugger.monitor(hproc);
    }

	CmdOptions_t g_CmdOpts;
}