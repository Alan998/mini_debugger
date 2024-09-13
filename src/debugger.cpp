#include <debugger.hpp>
#include <linenoise.h>

#include <sys/ptrace.h> // ptrace
#include <sys/wait.h>	// waitpid

#include <iostream>
#include <vector>

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
	int wait_status{};
	int options{};

	// wait until the child process has finished launching
	waitpid(m_pid, &wait_status, options);

	// listen and handle user input with linenoise
	char* line{ nullptr };
	while ((line = linenoise("mini_dbg> ")) != nullptr) {
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
	auto command = args[0];

	if (is_prefix(command, "continue")) {
		continue_execution();
	} else {
		std::cerr << "Unknown command\n";
	}
}

void
Debugger::continue_execution()
{
	ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);

	int wait_status{};
	int options{};
	waitpid(m_pid, &wait_status, options);
}
