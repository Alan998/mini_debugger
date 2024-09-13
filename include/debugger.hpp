#pragma once
#include <string>

class Debugger
{
public:
	explicit Debugger(std::string prog_name, const pid_t pid);
	void run();
	void handle_command(const std::string_view str_view);
	void continue_execution();

private:
	std::string m_prog_name;
	pid_t		m_pid;
};
