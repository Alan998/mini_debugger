#include <breakpoint.hpp>
#include <sys/ptrace.h>

namespace mini_debugger {

Breakpoint::Breakpoint(const pid_t pid, const std::intptr_t addr)
	: m_pid{ pid }
	, m_addr{ addr }
	, m_enabled{ false }
	, m_saved_data{}
{
}

void
Breakpoint::enable()
{
	// read memory of the traced process
	auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
	// save the original data
	m_saved_data = static_cast<uint8_t>(data & 0xff);

	// overwrite with INT3 instruction to set a breakpoint
	// `INT3` instruction is encoded as 0xcc
	const uint64_t INT3{ 0xcc };
	// set the low byte to 0xcc
	uint64_t data_with_int3 = ((data & ~0xff) | INT3);
	ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);

	m_enabled = true;
}

void
Breakpoint::disable()
{
	// read memory of the traced process
	auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
	// restore the low byte with the original data
	auto restored_data = ((data & ~0xff) | m_saved_data);
	// write original instruction back to memory
	ptrace(PTRACE_POKEDATA, m_pid, m_addr, restored_data);

	m_enabled = false;
}

bool
Breakpoint::is_enabled() const
{
	return m_enabled;
}

std::intptr_t
Breakpoint::get_address() const
{
	return m_addr;
}

};
