
#include <print>

#include "breakpoint.hpp"

namespace pdm {
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

    Breakpoint_t::process_action Breakpoint_t::process_breakpoint_hit(
        DEBUG_EVENT& debugEvent
    ) {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_FULL;

        HANDLE hEventThread = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
        if (!hEventThread) {
            std::print("Failed to open thread.\n");
            return No_spec;
        }

        GetThreadContext(hEventThread, &ctx);
        if (m_callback) {
            m_callback(&ctx, m_hproc);
        }

        if (m_bp_type == BreakpointType::Reuseable) {
            ctx.Rip -= 1;
            ctx.EFlags |= 0x100; //set trap flag, which raise "single-step" exception
        }

        SetThreadContext(hEventThread, &ctx);

        this->restore();
        CloseHandle(hEventThread);

        if (get_bp_type() == BreakpointType::Reuseable)
            return process_action::Restore;
        else if (get_bp_type() == BreakpointType::Temporary)
            return process_action::Remove;
        return process_action::No_spec;
    }
}