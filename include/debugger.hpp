#pragma once
#include <breakpoint.hpp>
#include <cstdint> // intptr_t
#include <dwarf/dwarf++.hh>
#include <elf/elf++.hh>
#include <signal.h>
#include <string>
#include <unordered_map>

namespace mini_debugger {

static constexpr int DWORD_SIZE{ 16 };

static constexpr short DEBUG_WINDOW_LEN{ 78 };

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

	// print out source code when breakpoint hits
	void print_source(const std::string_view file_name,
					  const unsigned		 line_num,
					  const unsigned		 n_lines_context = 3);

	void single_step_instruction();
	void single_step_instruction_with_breakpoint_check();
	void step_in();
	void step_out();
	void remove_breakpoint(const std::intptr_t addr);
	void step_over();

private:
	// handle user input
	void handle_command(const std::string_view str_view);

	// continue command for debugger
	void continue_execution();

	uint64_t read_memory(const uint64_t address);
	void	 write_memory(const uint64_t address, const uint64_t value);

	// get program counter
	uint64_t get_pc();
	uint64_t get_offset_pc();
	// set program counter
	void set_pc(const uint64_t pc);

	// wait until process m_pid is finished
	void wait_for_signal();

	// retrieve line entries and function DIEs from PC values
	dwarf::die get_function_from_pc(const uint64_t pc);

	dwarf::line_table::iterator get_line_entry_from_pc(const uint64_t pc);

	void	 initialise_load_address();
	uint64_t offset_load_address(const uint64_t addr);

	// get signal type when signal is received
	siginfo_t get_signal_info();

	void handle_sigtrap(const siginfo_t info);

	uint64_t offset_dwarf_address(const uint64_t addr);

	std::string									  m_prog_name;
	pid_t										  m_pid;
	uint64_t									  m_load_address;
	std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
	dwarf::dwarf								  m_dwarf;
	elf::elf									  m_elf;
};

};
