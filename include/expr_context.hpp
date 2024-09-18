#pragma once
#include <cstdint>
#include <dwarf/dwarf++.hh>

// tell libelfin how to read registers from our process
class Ptrace_Expr_Context : public dwarf::expr_context
{
public:
	Ptrace_Expr_Context(const pid_t pid, const std::intptr_t load_address);

	dwarf::taddr reg(unsigned register_num) override;
	dwarf::taddr pc() override;
	dwarf::taddr deref_size(dwarf::taddr			  address,
							[[maybe_unused]] unsigned size) override;

private:
	pid_t		  m_pid;
	std::intptr_t m_load_address;
};
