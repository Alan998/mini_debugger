// software breakpoint implementation
#pragma once
#include <cstdint>	   // intptr_t
#include <sys/types.h> // pid_t

namespace mini_debugger {

class Breakpoint
{
public:
	Breakpoint() = default;
	Breakpoint(const pid_t pid, const std::intptr_t addr);

	// setup the breakpoint
	void enable();
	// destroy the breakpoint
	void disable();

	bool		  is_enabled() const;
	std::intptr_t get_address() const;

private:
	pid_t		  m_pid;
	std::intptr_t m_addr;
	bool		  m_enabled;

	// when setting up a breakpoint, we overwrite the original instruction with
	// the `INT3` instruction, which causes a SIGTRAP to stop the process
	// since the original instruction will be overwritten, data which used to be
	// at the breakpoint address needs to be saved in order to restore it later
	uint8_t m_saved_data;
};

};
