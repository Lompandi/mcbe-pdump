
#include <print>
#include "debugger.hpp"

namespace pdm {
    void Debugger_t::monitor(HANDLE hprocess) {
        this->m_debug_process = hprocess;

        DEBUG_EVENT debugEvent;
        DWORD continueStatus = DBG_CONTINUE;

        DebugSetProcessKillOnExit(TRUE);

        while (true) {
            WaitForDebugEvent(&debugEvent, INFINITE);

            switch (debugEvent.dwDebugEventCode) {
            case EXCEPTION_DEBUG_EVENT:
                if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {
                    auto bp_addr = (uintptr_t)debugEvent.u.Exception.ExceptionRecord.ExceptionAddress;

                    if (m_breakpoints.find(bp_addr) == m_breakpoints.end()) break;
                    auto action = m_breakpoints[bp_addr].process_breakpoint_hit(debugEvent);
                    if (action != Breakpoint_t::process_action::Remove)
                        m_last_breakpoint_hit = bp_addr;
                    else
                        m_breakpoints.erase(bp_addr);
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

    bool Debugger_t::insert_breakpoint_tmp(uintptr_t address, callback_t callback) {
        if (!m_debug_process)
            return false;
        Breakpoint_t breakpoint(m_debug_process, address, callback, Breakpoint_t::BreakpointType::Temporary);
        m_breakpoints[address] = breakpoint;
        m_breakpoints[address].set(m_debug_process, address, callback);
        return true;
    }

    void Debugger_t::remove_breakpoint(uintptr_t address) {
        if (m_breakpoints.find(address) != m_breakpoints.end())
            m_breakpoints.erase(address);
    }
}