#include <debugger.hpp>
#include <expr_context.hpp>
#include <linenoise.h>
#include <registers.hpp>

#include <fcntl.h>		// open
#include <sys/ptrace.h> // ptrace
#include <sys/wait.h>	// waitpid

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

namespace mini_debugger {

// split the input `line` by `pattern`
static std::vector<std::string>
split(const std::string_view line, const char pattern)
{
	std::vector<std::string> result;
	std::string::size_type	 begin{ 0 };
	std::string::size_type	 end = line.find(pattern);

	while (end != std::string::npos) {
		if (end - begin != 0) {
			result.emplace_back(line.substr(begin, end - begin));
		}
		begin = end + 1;
		end	  = line.find(pattern, begin);
	}

	if (begin != line.length()) {
		result.emplace_back(line.substr(begin));
	}
	return result;
}

// check if `s` is a prefix of `str`
static bool
is_prefix(const std::string_view s, const std::string_view str)
{
	if (s.size() > str.size())
		return false;
	return std::equal(s.begin(), s.end(), str.begin());
}

// check if `s` is a suffix of `str`
static bool
is_suffix(const std::string_view s, const std::string_view str)
{
	if (s.size() > str.size())
		return false;
	auto diff = str.size() - s.size();
	return std::equal(s.begin(), s.end(), str.begin() + diff);
}

std::string
to_string(symbol_type symbol)
{
	switch (symbol) {
		case symbol_type::notype:
			return "notype";
		case symbol_type::object:
			return "object";
		case symbol_type::func:
			return "func";
		case symbol_type::section:
			return "section";
		case symbol_type::file:
			return "file";
	}
	return {};
}

symbol_type
to_symbol_type(elf::stt sym)
{
	switch (sym) {
		case ::elf::stt::notype:
			return symbol_type::notype;
		case ::elf::stt::object:
			return symbol_type::object;
		case ::elf::stt::func:
			return symbol_type::func;
		case ::elf::stt::section:
			return symbol_type::section;
		case ::elf::stt::file:
			return symbol_type::file;
		default:
			return symbol_type::notype;
	}
}

Debugger::Debugger(std::string prog_name, const pid_t pid)
	: m_prog_name{ std::move(prog_name) }
	, m_pid{ pid }
	, m_load_address{ 0 }
{
	auto fd = open(m_prog_name.c_str(), O_RDONLY);

	m_elf	= elf::elf{ elf::create_mmap_loader(fd) };
	m_dwarf = dwarf::dwarf{ dwarf::elf::create_loader(m_elf) };
}

void
Debugger::run()
{
	// wait until the child process has finished launching
	wait_for_signal();
	// find the load address of the program
	initialise_load_address();

	// listen and handle user input with linenoise
	char* line{ nullptr };
	while ((line = linenoise("mini_dbg> ")) != NULL) {
		handle_command(line);
		linenoiseHistoryAdd(line);
		linenoiseFree(line);
	}
}

void
Debugger::handle_command(const std::string_view line)
{
	// parse input command
	auto args	 = split(line, ' ');
	auto command = args.at(0);

	if (is_prefix(command, "continue")) {
		continue_execution();
	} else if (is_prefix(command, "break")) {
		if (args.at(1)[0] == '0' && args.at(1)[1] == 'x') {
			// naively assume that the user has written 0xADDRESS
			std::string addr{ args.at(1), 2 }; // exlude 0x from the string
			set_breakpoint_at_address(std::stoull(addr, 0, WORD_SIZE));
		} else if (args.at(1).find(':') != std::string::npos) {
			auto file_and_line = split(args.at(1), ':');
			std::cout << file_and_line[0] << ' ' << file_and_line[1] << '\n';
			set_breakpoint_at_source_line(file_and_line.at(0),
										  std::stoi(file_and_line.at(1)));
		} else {
			set_breakpoint_at_function(args.at(1));
		}
	} else if (is_prefix(command, "register")) {
		if (is_prefix(args.at(1), "dump")) {
			dump_registers();
		} else if (is_prefix(args.at(1), "read")) {
			std::cout << get_register_value(m_pid,
											get_register_from_name(args.at(2)))
					  << std::endl;
		} else if (is_prefix(args.at(1), "write")) {
			std::string val{ args.at(3), 2 }; // assume 0xValue
			set_register_value(m_pid,
							   get_register_from_name(args.at(2)),
							   std::stoull(val, 0, WORD_SIZE));
		}
	} else if (is_prefix(command, "memory")) {
		std::string addr{ args.at(2), 2 }; // assume 0xADDRESS

		if (is_prefix(args.at(1), "read")) {
			std::cout << std::hex
					  << read_memory(std::stoull(addr, 0, WORD_SIZE))
					  << std::endl;
		}
		if (is_prefix(args.at(1), "write")) {
			std::string value{ args.at(3), 2 }; // assume 0xValue
			write_memory(std::stoull(addr, 0, WORD_SIZE),
						 std::stoull(value, 0, WORD_SIZE));
		}
	} else if (is_prefix(command, "step")) {
		step_in();
	} else if (is_prefix(command, "next")) {
		step_over();
	} else if (is_prefix(command, "finish")) {
		step_out();
	} else if (is_prefix(command, "symbol")) {
		auto syms = lookup_symbol(args.at(1));
		for (auto&& sym : syms) {
			std::cout << sym.name << ' ' << mini_debugger::to_string(sym.type)
					  << " 0x" << std::hex << sym.addr << std::endl;
		}
	} else if (is_prefix(command, "backtrace")) {
		print_backtrace();
	} else if (is_prefix(command, "variables")) {
		read_variables();
	} else if (is_prefix(command, "quit")) {
		std::cout << "Exited from mini debugger\n";
		exit(0);
	} else {
		std::cerr << "Unknown command\n";
	}
}

void
Debugger::continue_execution()
{
	step_over_breakpoint();
	ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
	wait_for_signal();
}

void
Debugger::set_breakpoint_at_address(const std::intptr_t addr)
{
	std::cout << "Set breakpoint at address 0x" << std::hex << addr
			  << std::endl;

	Breakpoint bp{ m_pid, addr };
	bp.enable();
	m_breakpoints[addr] = bp;
}

void
Debugger::dump_registers()
{
	for (const auto& register_descriptor : g_register_descriptors) {
		std::cout << std::setfill(' ') << std::setw(9)
				  << register_descriptor.name << " 0x" << std::setfill('0')
				  << std::setw(WORD_SIZE) << std::hex
				  << get_register_value(m_pid, register_descriptor.reg) << '\n';
	}
}

std::intptr_t
Debugger::read_memory(const std::intptr_t address)
{
	return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void
Debugger::write_memory(const std::intptr_t address, const uint64_t value)
{
	ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

std::intptr_t
Debugger::get_pc()
{
	return get_register_value(m_pid, Reg::rip);
}

std::intptr_t
Debugger::get_offset_pc()
{
	return offset_load_address(get_pc());
}

void
Debugger::set_pc(const std::intptr_t pc)
{
	set_register_value(m_pid, Reg::rip, pc);
}

void
Debugger::step_over_breakpoint()
{
	auto pc = get_pc();
	if (m_breakpoints.count(pc)) {
		auto& bp = m_breakpoints[pc];
		// check if breakpoint is set for the current pc
		if (bp.is_enabled()) {
			// disable the breakpoint
			bp.disable();
			// step over the original instruction
			ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
			wait_for_signal();
			// re-enable the breakpoint
			bp.enable();
		}
	}
}

void
Debugger::wait_for_signal()
{
	int wait_status{};
	int options{};
	waitpid(m_pid, &wait_status, options);

	auto signal_info = get_signal_info();
	switch (signal_info.si_signo) {
		case SIGTRAP:
			handle_sigtrap(signal_info);
			break;
		case SIGSEGV:
			std::cerr << "segfault. " << signal_info.si_code << std::endl;
			break;
		default:
			std::cout << "Got signal " << strsignal(signal_info.si_signo)
					  << std::endl;
	}
}

dwarf::die
Debugger::get_function_from_pc(const std::intptr_t pc)
{
	// iterate through compilation_units until pc is found
	for (auto& compilation_unit : m_dwarf.compilation_units()) {
		if (!die_pc_range(compilation_unit.root()).contains(pc))
			continue;

		// iterate through children until we find relevant function
		// (DW_TAG_subprogram)
		for (const auto& die : compilation_unit.root()) {
			if (die.tag == dwarf::DW_TAG::subprogram &&
				die_pc_range(die).contains(pc)) {
				return die;
			}
		}
	}

	throw std::out_of_range{ "Cannot find function" };
}

dwarf::line_table::iterator
Debugger::get_line_entry_from_pc(const std::intptr_t pc)
{
	for (auto& compilation_unit : m_dwarf.compilation_units()) {
		if (!die_pc_range(compilation_unit.root()).contains(pc))
			continue;

		auto& line_table = compilation_unit.get_line_table();
		auto  it		 = line_table.find_address(pc);
		if (it == line_table.end()) {
			throw std::out_of_range{ "Cannot find line entry" };
		} else {
			return it;
		}
	}

	throw std::out_of_range{ "Cannot find line entry" };
}

void
Debugger::initialise_load_address()
{
	if (m_elf.get_hdr().type != elf::et::dyn)
		return;
	// if this is a dynamic library
	// the load address is found in /proc/<pid>/maps
	std::ifstream map("/proc/" + std::to_string(m_pid) + "/maps");

	// read the first address from the file
	std::string addr{};
	std::getline(map, addr, '-');
	m_load_address = std::stoul(addr, 0, WORD_SIZE);
}

std::intptr_t
Debugger::offset_load_address(const std::intptr_t addr)
{
	return addr - m_load_address;
}

void
Debugger::print_source(const std::string_view file_name,
					   const unsigned		  line_num,
					   const unsigned		  n_lines_context)
{
	std::ifstream file{ file_name.data() };

	// window around the desired line
	auto start_line =
		line_num <= n_lines_context ? 1 : line_num - n_lines_context;
	auto end_line =
		line_num + n_lines_context +
		(line_num < n_lines_context ? n_lines_context - line_num : 0) + 1;

	char ch{};
	auto current_line{ 1u };
	// skip lines until start_line
	while (current_line != start_line && file.get(ch)) {
		if (ch == '\n')
			++current_line;
	}

	// start of window
	std::cout << std::string(DEBUG_WINDOW_LEN, '=');
	std::cout << std::endl;

	// output cursor if we are at the current_line
	std::cout << (current_line == line_num ? "> " : " ");
	// print lines until end_line
	while (current_line <= end_line && file.get(ch)) {
		std::cout << ch;
		if (ch == '\n') {
			++current_line;
			std::cout << (current_line == line_num ? "> " : " ");
		}
	}

	// end of window
	std::cout << std::string(DEBUG_WINDOW_LEN, '=');
	// flush output stream
	std::cout << std::endl;
}

siginfo_t
Debugger::get_signal_info()
{
	siginfo_t info;
	ptrace(PTRACE_GETSIGINFO, m_pid, nullptr, &info);
	return info;
}

void
Debugger::handle_sigtrap(const siginfo_t info)
{
	switch (info.si_code) {
		// one of these will be set if a breakpoint was hit
		case SI_KERNEL:
		case TRAP_BRKPT: {
			// resume execution from the point before the breakpoint was hit
			auto pc = get_pc();
			// minus 1 since execution will go past the breakpoint
			set_pc(pc - 1);
			std::cout << "Hit breakpoint at address 0x" << std::hex << pc
					  << std::endl;

			// offset pc for querying DWARF
			auto offset_pc	= offset_load_address(pc);
			auto line_entry = get_line_entry_from_pc(offset_pc);
			print_source(line_entry->file->path, line_entry->line);
			return;
		}
		// signal 0 checks if the process is running
		case 0:
		// TRAP_TRACE will be set if the signal was sent by single stepping
		case TRAP_TRACE:
			return;
		default:
			std::cout << "Unknown SIGTRAP code " << info.si_code << std::endl;
			return;
	}
}

void
Debugger::single_step_instruction()
{
	ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
	wait_for_signal();
}

void
Debugger::single_step_instruction_with_breakpoint_check()
{
	// check and see if we need to disable and enable a breakpoint
	if (m_breakpoints.count(get_pc())) {
		step_over_breakpoint();
	} else {
		single_step_instruction();
	}
}

void
Debugger::step_out()
{
	// set a breakpoint at the return address of the function and continue
	auto frame_pointer = get_register_value(m_pid, Reg::rbp);
	// return address is stored 8 bytes after the start of a stack frame
	auto return_address = read_memory(frame_pointer + 8);

	bool should_remove_breakpoint{ false };
	if (!m_breakpoints.count(return_address)) {
		set_breakpoint_at_address(return_address);
		should_remove_breakpoint = true;
	}

	continue_execution();

	if (should_remove_breakpoint) {
		remove_breakpoint(return_address);
	}
}

void
Debugger::remove_breakpoint(const std::intptr_t addr)
{
	if (m_breakpoints.at(addr).is_enabled()) {
		m_breakpoints.at(addr).disable();
	}
	m_breakpoints.erase(addr);
}

void
Debugger::step_in()
{
	auto line = get_line_entry_from_pc(get_offset_pc())->line;
	// keep on stepping over instructions until we get to a new line
	while (get_line_entry_from_pc(get_offset_pc())->line == line) {
		single_step_instruction_with_breakpoint_check();
	}

	auto line_entry = get_line_entry_from_pc(get_offset_pc());
	print_source(line_entry->file->path, line_entry->line);
}

std::intptr_t
Debugger::offset_dwarf_address(const std::intptr_t addr)
{
	return addr + m_load_address;
}

void
Debugger::step_over()
{
	// implement step_over by setting a temporary breakpoint at every line in
	// the current function
	auto func = get_function_from_pc(get_offset_pc());
	// at_low_pc and at_high_pc are functions from libelfin, which will get
	// the value of low and high PC values for the given function DIE
	auto func_entry = dwarf::at_low_pc(func);
	auto func_end	= dwarf::at_high_pc(func);

	auto line		= get_line_entry_from_pc(func_entry);
	auto start_line = get_line_entry_from_pc(get_offset_pc());

	// we will need to remove any breakpoints set so they don't leak out of step
	// function keep track of these breakpoints in a std::vector
	std::vector<std::intptr_t> to_delete;
	// to set all the breakpoints, loop over the line table entries until one
	// outside the range of function is hit
	while (line->address < func_end) {
		auto load_address = offset_dwarf_address(line->address);
		// make sure no breakpoint is already set
		if (line->address != start_line->address &&
			!m_breakpoints.count(load_address)) {
			set_breakpoint_at_address(load_address);
			to_delete.emplace_back(load_address);
		}
		++line;
	}

	// set a breakpoint at return address
	auto frame_pointer	= get_register_value(m_pid, Reg::rbp);
	auto return_address = read_memory(frame_pointer + 8);
	if (!m_breakpoints.count(return_address)) {
		set_breakpoint_at_address(return_address);
		to_delete.emplace_back(return_address);
	}

	// continue_execution until one of the breakpoint is hit
	continue_execution();
	// remove all the temporary breakpoints
	for (auto addr : to_delete) {
		remove_breakpoint(addr);
	}
}

void
Debugger::set_breakpoint_at_function(const std::string_view func_name)
{
	// iterate through all compilation units and search for functions with name
	// which matches
	for (const auto& compilation_unit : m_dwarf.compilation_units()) {
		for (const auto& die : compilation_unit.root()) {
			if (die.has(dwarf::DW_AT::name) &&
				dwarf::at_name(die) == func_name.data()) {
				auto low_pc = dwarf::at_low_pc(die);
				auto entry	= get_line_entry_from_pc(low_pc);
				// increment the line entry by one to get the first line of the
				// user code instead of the prologue
				++entry;
				set_breakpoint_at_address(offset_dwarf_address(entry->address));
			}
		}
	}
}

void
Debugger::set_breakpoint_at_source_line(const std::string_view file,
										unsigned			   line)
{
	for (const auto& compilation_unit : m_dwarf.compilation_units()) {
		if (!is_suffix(file, dwarf::at_name(compilation_unit.root())))
			continue;
		const auto& line_entry = compilation_unit.get_line_table();

		for (const auto& entry : line_entry) {
			// check the line table entry is marked as the beginning of a
			// statement
			if (entry.is_stmt && entry.line == line) {
				set_breakpoint_at_address(offset_dwarf_address(entry.address));
				return;
			}
		}
	}
}

std::vector<symbol>
Debugger::lookup_symbol(const std::string_view symbol_name)
{
	// loop through sections of ELF and collect every symbols found into a
	// vector
	std::vector<symbol> syms;
	for (auto& section : m_elf.sections()) {
		if (section.get_hdr().type != elf::sht::symtab &&
			section.get_hdr().type != elf::sht::dynsym)
			continue;

		for (auto sym : section.as_symtab()) {
			if (sym.get_name() != symbol_name)
				continue;
			auto& data = sym.get_data();
			syms.emplace_back(symbol{
				to_symbol_type(data.type()), sym.get_name(), data.value });
		}
	}
	return syms;
}

void
Debugger::print_backtrace()
{

	// output format of each frame
	auto output_frame = [frame_number = 0](auto&& func) mutable {
		std::cout << "frame #" << frame_number++ << ": 0x"
				  << dwarf::at_low_pc(func) << ' ' << dwarf::at_name(func)
				  << std::endl;
	};

	// get current function
	auto current_func = get_function_from_pc(offset_load_address(get_pc()));
	output_frame(current_func);

	// frame pointer is stored in the rbp register
	std::intptr_t frame_pointer = get_register_value(m_pid, Reg::rbp);
	// return address is 8 bytes up the stack from the frame pointer
	std::intptr_t return_address = read_memory(frame_pointer + 8);

	// keep unwinding until debugger hits main
	while (dwarf::at_name(current_func) != "main") {
		current_func =
			get_function_from_pc(offset_load_address(return_address));
		output_frame(current_func);
		frame_pointer  = read_memory(frame_pointer);
		return_address = read_memory(frame_pointer + 8);
	}
}

void
Debugger::read_variables()
{
	// find current function
	auto func = get_function_from_pc(get_offset_pc());

	// iterate through entries and look for variables
	for (const auto& die : func) {
		if (die.tag != dwarf::DW_TAG::variable)
			continue;

		auto location_val = die[dwarf::DW_AT::location];
		if (location_val.get_type() == dwarf::value::type::exprloc) {
			Ptrace_Expr_Context context{ m_pid, m_load_address };
			auto result = location_val.as_exprloc().evaluate(&context);

			switch (result.location_type) {
				case dwarf::expr_result::type::address: {
					auto offset_addr = result.value;
					auto value		 = read_memory(offset_addr);
					std::cout << at_name(die) << " (0x" << std::hex
							  << offset_addr;
					std::cout << std::dec << ") = " << value << std::endl;
					break;
				}
				case dwarf::expr_result::type::reg: {
					auto value = get_register_value_from_dwarf_register(
						m_pid, result.value);
					std::cout << at_name(die) << " (reg " << result.value
							  << ") = " << value << std::endl;
					break;
				}
				default:
					throw std::runtime_error{ "Unhandled variable location" };
			}
		} else {
			throw std::runtime_error{ "Unhandled variable location" };
		}
	}
}
};
