#include <debugger.hpp>

#include <iostream>
#include <sys/ptrace.h> // ptrace
#include <sys/wait.h>
#include <unistd.h> // execl, fork

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
		// Entered the child process, exectue debugee
		ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
		execl(program, program, nullptr);
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
