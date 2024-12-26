#pragma once

#include <cstdint>
#include <functional>
#include <windows.h>
#include <psapi.h>

#include "utils/utils.hpp"
#include <span>

namespace pdm {
	struct Breakpoint_t;

	class Debugger_t {
		using callback_t = std::function<void(CONTEXT*, HANDLE)>;

		HANDLE m_debug_process;

		std::unordered_map<uintptr_t, Breakpoint_t>	m_breakpoints;
		uintptr_t									m_last_breakpoint_hit;

		std::string process_memory;			//bds' main memory region
	public:
		void monitor(HANDLE hprocess);

		bool insert_breakpoint(uintptr_t address, callback_t callback);

		/* disassemble functionallity */

		template <SearchDirection searchDir>
		uintptr_t find_func_start(uintptr_t cur_addr, size_t max_search = 0x1000) {
			uint8_t instr_byte = 0x00;
			int offset = 0x0;

			uintptr_t searchTarget = cur_addr;

			while (instr_byte != 0xCC && instr_byte != 0xC3 && std::abs(offset) < max_search) {
				ReadProcessMemory(m_debug_process, (LPVOID)searchTarget, &instr_byte, 1, NULL);
				if constexpr (searchDir == SearchDirection::Up)
					offset--;
				else
					offset++;
				searchTarget += offset;
			}

			return searchTarget;
		}

		std::optional<uintptr_t> find_signature(std::span<uint8_t>& signature, std::span<uint8_t>& mask,
			uintptr_t module_base_addr, size_t size);
	};

	struct Breakpoint_t {
		using callback_t = std::function<void(CONTEXT*, HANDLE)>;
	public:
		uintptr_t	m_baddr;
		uint8_t		m_patchinst;

		callback_t  m_callback;
		HANDLE		m_hproc;

		Breakpoint_t() noexcept = default;

		Breakpoint_t(HANDLE hproc, uintptr_t addr, callback_t callback) :
			m_baddr(addr), m_hproc(hproc), m_callback(callback) {}

		void reset() {
			this->set(m_hproc, this->m_baddr, &this->m_patchinst);
		}

		void restore(HANDLE hproc, uintptr_t address, uint8_t* restore_byte) {
			set_byte(hproc, address, restore_byte);
		}

		void process_breakpoint_hit(DEBUG_EVENT& debugEvent);

		bool set(HANDLE hproc, uintptr_t address, uint8_t* patched);
		void set_byte(HANDLE hproc, uintptr_t address, uint8_t* target_byte, uint8_t* patched = nullptr);

		[[nodiscard]] uintptr_t get_break_addr() const { return m_baddr; }

		void restore() {
			set_byte(m_hproc, m_baddr, &m_patchinst);
		}

		bool set(HANDLE hproc, uintptr_t address, callback_t callback);
	};
}