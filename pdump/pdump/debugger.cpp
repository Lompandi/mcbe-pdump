
#include <print>
#include "debugger.hpp"

namespace pdm {
    void Debugger_t::monitor(HANDLE hprocess) {
        this->m_debug_process = hprocess;

        DEBUG_EVENT debugEvent;
        DWORD continueStatus = DBG_CONTINUE;

        DebugSetProcessKillOnExit(FALSE);

        while (true) {
            WaitForDebugEvent(&debugEvent, INFINITE);

            switch (debugEvent.dwDebugEventCode) {
            case EXCEPTION_DEBUG_EVENT:
                if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {
                    /* Hit breakpoint */
                    auto bp_addr = (uintptr_t)debugEvent.u.Exception.ExceptionRecord.ExceptionAddress;

                    if (m_breakpoints.find(bp_addr) == m_breakpoints.end()) break;
                    m_breakpoints[bp_addr].process_breakpoint_hit(debugEvent);
                    m_last_breakpoint_hit = bp_addr;
                    continueStatus = DBG_CONTINUE;
                }
                else if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) {
                    m_breakpoints[m_last_breakpoint_hit].reset();
                    m_last_breakpoint_hit = 0x0;
                    continueStatus = DBG_CONTINUE;
                }
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                CloseHandle(hprocess);
                return;
            case CREATE_PROCESS_DEBUG_EVENT:
                break;
            default:
                break;
            }

            ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus);
        }

        CloseHandle(hprocess);
    }

    bool Debugger_t::insert_breakpoint(uintptr_t address, callback_t callback) {
        if (!m_debug_process)
            return false;
        Breakpoint_t breakpoint(m_debug_process, address, callback);
        m_breakpoints[address] = breakpoint;
        m_breakpoints[address].set(m_debug_process, address, callback);
        return true;
    }

	bool Breakpoint_t::set(HANDLE hproc, uintptr_t addr, uint8_t* patched) {
        set_byte(hproc, addr, (uint8_t*)"\xCC", patched);
		return true;
	}

    void Breakpoint_t::set_byte(HANDLE hproc, uintptr_t address, uint8_t* target_byte, uint8_t* patched) {
        DWORD old_prot;
        VirtualProtectEx(hproc, (void*)address, 1, PAGE_EXECUTE_READWRITE, &old_prot);
        if (patched != nullptr) {
            ReadProcessMemory(hproc, (void*)address, patched, 1, NULL);
        }
        WriteProcessMemory(hproc, (void*)address, (void*)target_byte, 1, NULL);
        VirtualProtectEx(hproc, (void*)address, 1, old_prot, &old_prot);
    }

    bool Breakpoint_t::set(HANDLE hproc, uintptr_t address, callback_t callback) {
        this->m_hproc = hproc;
        this->m_baddr = address;
        this->m_callback = callback;


        uint8_t patched_byte = 0x0;
        while ((patched_byte & 0xFF) != (uint8_t)0xCC) {
            this->set(m_hproc, m_baddr, &m_patchinst);
            ReadProcessMemory(hproc, (PVOID)address, &patched_byte, 1, NULL);
        }

        return true;
    }

    void Breakpoint_t::process_breakpoint_hit(
        DEBUG_EVENT& debugEvent
    ) {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_FULL;

        HANDLE hEventThread = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
        if (!hEventThread) {
            std::print("Failed to open thread.\n");
            return;
        }

        GetThreadContext(hEventThread, &ctx);
        if (m_callback) {
            m_callback(&ctx, m_hproc);
        }

        ctx.Rip -= 1;
        ctx.EFlags |= 0x100; //set trap flag, which raise "single-step" exception

        SetThreadContext(hEventThread, &ctx);

        this->restore();
        CloseHandle(hEventThread);
    }

    std::optional<uintptr_t>
    Debugger_t::find_signature(std::span<uint8_t>& signature, std::span<uint8_t>& mask, uintptr_t module_base_addr, size_t size) {
        if (process_memory.empty()) {
            process_memory.resize(size);
            std::cout << "Loading process memory...\n"; 
            ReadProcessMemory(m_debug_process, (LPVOID)module_base_addr,
                process_memory.data(), size, NULL);
        }

        for (size_t i = 0; i <= size - signature.size(); i++) {
            bool match = true;

            for (size_t j = 0; j < signature.size(); j++) {
                if (mask[j] == 0xFF)
                    continue;
                if (process_memory[i + j] != signature[j]) {
                    match = false;
                    break;
                }
            }

            if (match)
                return module_base_addr + i;
        }

        return std::nullopt;  
    }
}