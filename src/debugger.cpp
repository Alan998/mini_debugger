#include <debugger.hpp>
#include <linenoise.h>
#include <registers.hpp>

#include <sys/ptrace.h> // ptrace
#include <sys/wait.h>	// waitpid

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

Debugger::Debugger(std::string prog_name, const pid_t pid)
	: m_prog_name{ std::move(prog_name) }
	, m_pid{ pid }
{
}

void
Debugger::run()
{
	// wait until the child process has finished launching
	wait_for_signal();

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
			set_breakpoint_at_address(std::stol(addr, 0, WORD_SIZE));
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
							   std::stol(val, 0, WORD_SIZE));
		}
	} else if (is_prefix(command, "memory")) {
		std::string addr{ args.at(2), 2 }; // assume 0xADDRESS

		if (is_prefix(args.at(1), "read")) {
			std::cout << std::hex << read_memory(std::stol(addr, 0, WORD_SIZE))
					  << std::endl;
		}
		if (is_prefix(args.at(1), "write")) {
			std::string value{ args.at(3), 2 };
			write_memory(std::stol(addr, 0, WORD_SIZE),
						 std::stol(value, 0, WORD_SIZE));
		}
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
		std::cout << register_descriptor.name << " 0x" << std::setfill('0')
				  << std::setw(WORD_SIZE) << std::hex
				  << get_register_value(m_pid, register_descriptor.reg) << '\n';
	}
}

uint64_t
Debugger::read_memory(const uint64_t address)
{
	return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void
Debugger::write_memory(const uint64_t address, const uint64_t value)
{
	ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

uint64_t
Debugger::get_pc()
{
	return get_register_value(m_pid, Reg::rip);
}

void
Debugger::set_pc(const uint64_t pc)
{
	set_register_value(m_pid, Reg::rip, pc);
}

void
Debugger::step_over_breakpoint()
{
	// minus 1 since execution will go past the breakpoint
	auto possible_breakpoint_location = get_pc() - 1;
	if (m_breakpoints.count(possible_breakpoint_location)) {
		auto& bp = m_breakpoints[possible_breakpoint_location];

		// check if breakpoint is set for the current pc
		if (bp.is_enabled()) {
			// resume execution from the point before the breakpoint was hit
			auto previous_instruction_address = possible_breakpoint_location;
			set_pc(previous_instruction_address);

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
}

};
