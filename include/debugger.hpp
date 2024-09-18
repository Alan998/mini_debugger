#pragma once
#include <breakpoint.hpp>
#include <cstdint> // intptr_t
#include <dwarf/dwarf++.hh>
#include <elf/elf++.hh>
#include <signal.h>
#include <string>
#include <unordered_map>

template class std::initializer_list<dwarf::taddr>;
namespace mini_debugger {

enum class symbol_type
{
	notype,	 // no type (e.g., absolute symbol)
	object,	 // data object
	func,	 // function entry point
	section, // symbol is associated with a section
	file,	 // source file associated with the object file
};

struct symbol
{
	symbol_type	   type;
	std::string	   name;
	std::uintptr_t addr;
};

static constexpr int WORD_SIZE{ 16 };

static constexpr short DEBUG_WINDOW_LEN{ 78 };

class Debugger
{
public:
	explicit Debugger(std::string prog_name, const pid_t pid);

	// start executing the debugger
	void run();
	// set a breakpoint at given address 0xADDRESS
	void set_breakpoint_at_address(const std::intptr_t addr);
	// set a breakpoint at given function name (does not support function
	// overloading)
	void set_breakpoint_at_function(const std::string_view func_name);
	void set_breakpoint_at_source_line(const std::string_view file,
									   unsigned				  line);

	// print all registers's values
	void dump_registers();
	// print out source code when breakpoint hits
	void print_source(const std::string_view file_name,
					  const unsigned		 line_num,
					  const unsigned		 n_lines_context = 3);
	// unwind and print the stack
	void print_backtrace();
	void single_step_instruction();
	void single_step_instruction_with_breakpoint_check();
	// step in function
	void step_in();
	// step over function
	void step_over();
	// step out function
	void step_out();
	void remove_breakpoint(const std::intptr_t addr);
	void read_variables();

private:
	// handle user input
	void handle_command(const std::string_view str_view);
	void handle_sigtrap(const siginfo_t info);

	// continue command for debugger
	void continue_execution();

	std::intptr_t read_memory(const std::intptr_t address);
	void write_memory(const std::intptr_t address, const uint64_t value);

	// get program counter
	std::intptr_t get_pc();
	std::intptr_t get_offset_pc();
	// set program counter
	void set_pc(const std::intptr_t pc);
	void step_over_breakpoint();

	// wait until process m_pid is finished
	void wait_for_signal();

	// get signal type when signal is received
	siginfo_t get_signal_info();

	// retrieve line entries and function DIEs from PC values
	dwarf::die					get_function_from_pc(const std::intptr_t pc);
	dwarf::line_table::iterator get_line_entry_from_pc(const std::intptr_t pc);

	void		  initialise_load_address();
	std::intptr_t offset_load_address(const std::intptr_t addr);
	std::intptr_t offset_dwarf_address(const std::intptr_t addr);

	std::vector<symbol> lookup_symbol(const std::string_view symbol_name);

	std::string									  m_prog_name;
	pid_t										  m_pid;
	std::intptr_t								  m_load_address;
	std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
	dwarf::dwarf								  m_dwarf;
	elf::elf									  m_elf;
};

};
