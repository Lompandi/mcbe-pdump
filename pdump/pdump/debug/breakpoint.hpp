#pragma once

#include <windows.h>
#include <functional>

namespace pdm {
	struct Breakpoint_t {
		using callback_t = std::function<void(CONTEXT*, HANDLE)>;
	public:
		enum class BreakpointType : char {
			Temporary = 0,
			Reuseable,
		};

		enum process_action {
			No_spec = 0,
			Restore,
			Remove,
		};

		Breakpoint_t() noexcept = default;
		Breakpoint_t(HANDLE hproc, uintptr_t addr) noexcept : 
			m_baddr(addr), m_patchinst(0x0), m_hproc(hproc), m_callback(nullptr), m_bp_type(BreakpointType::Reuseable) {}
		Breakpoint_t(HANDLE hproc, uintptr_t addr, callback_t callback) noexcept :
			m_baddr(addr), m_patchinst(0x0), m_hproc(hproc), m_callback(callback), m_bp_type(BreakpointType::Reuseable) {}
		Breakpoint_t(HANDLE hproc, uintptr_t addr, callback_t callback, BreakpointType bp_type) noexcept :
			m_baddr(addr), m_patchinst(0x0), m_hproc(hproc), m_callback(callback), m_bp_type(bp_type) {}

		[[nodiscard]] BreakpointType get_bp_type() const noexcept { return m_bp_type; }
		[[nodiscard]] uintptr_t	get_break_addr() const { return m_baddr; }

		bool set(HANDLE hproc, uintptr_t address, callback_t callback);
		
		void reset() { this->set(m_hproc, this->m_baddr, &this->m_patchinst); }
		void restore() { set_byte(m_hproc, m_baddr, &m_patchinst); }

		process_action process_breakpoint_hit(DEBUG_EVENT& debugEvent);
	private:
		bool set(HANDLE hproc, uintptr_t address, uint8_t* patched);
		void set_byte(HANDLE hproc, uintptr_t address, uint8_t* target_byte, uint8_t* patched = nullptr);
		void restore(HANDLE hproc, uintptr_t address, uint8_t* restore_byte) { set_byte(hproc, address, restore_byte); }

	private:
		uintptr_t	m_baddr;
		uint8_t		m_patchinst;

		callback_t  m_callback;
		HANDLE		m_hproc;

		BreakpointType m_bp_type;
	};
}