#pragma once
#include <breakpoint.hpp>
#include <cstdint> // intptr_t
#include <string>
#include <unordered_map>

namespace mini_debugger {

static constexpr int WORD_SIZE{ 16 };

class Debugger
{
public:
	explicit Debugger(std::string prog_name, const pid_t pid);

	// start executing the debugger
	void run();

	// set a breakpoint at given address 0xADDRESS
	void set_breakpoint_at_address(const std::intptr_t addr);

	// print all registers's values
	void dump_registers();

	void step_over_breakpoint();

private:
	// handle user input
	void handle_command(const std::string_view str_view);

	// continue command for debugger
	void continue_execution();

	uint64_t read_memory(const uint64_t address);
	void	 write_memory(const uint64_t address, const uint64_t value);

	// get program counter
	uint64_t get_pc();
	// set program counter
	void set_pc(const uint64_t pc);

	// wait until process m_pid is finished
	void wait_for_signal();

	std::string									  m_prog_name;
	pid_t										  m_pid;
	std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
};

};
