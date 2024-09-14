#include <debugger.hpp>

#include <iostream>
#include <sys/personality.h>
#include <sys/ptrace.h> // ptrace
#include <sys/wait.h>
#include <unistd.h> // execl, fork

using mini_debugger::Debugger;

void
execute_debugee(const std::string& program)
{
	if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
		std::cerr << "Error in ptrace\n";
		return;
	}
	execl(program.c_str(), program.c_str(), nullptr);
}

int
main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "Program name not specified\n";
		return -1;
	}

	auto program = argv[1];
	auto pid	 = fork();
	if (pid == 0) {
		// disable address space randomization so address breakpoints can be set
		personality(ADDR_NO_RANDOMIZE);
		// Entered the child process, exectue debugee
		execute_debugee(program);
	} else if (pid >= 1) {
		// Entered the parent process, exectue debugger
		std::cout << "Started debugging process " << pid << '\n';
		std::cout << "Press <Ctrl+d> to quit\n";

		Debugger dbg{ program, pid };
		dbg.run();

		// wait for child process to end
		wait(NULL);
	}

	return 0;
}
