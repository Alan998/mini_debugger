#include <expr_context.hpp>
#include <registers.hpp>
#include <sys/ptrace.h>
#include <sys/user.h> // user_regs_struct

Ptrace_Expr_Context::Ptrace_Expr_Context(const pid_t		 pid,
										 const std::intptr_t load_address)
	: m_pid(pid)
	, m_load_address(load_address)
{
}

dwarf::taddr
Ptrace_Expr_Context::reg(unsigned register_num)
{
	return mini_debugger::get_register_value_from_dwarf_register(m_pid,
																 register_num);
}

dwarf::taddr
Ptrace_Expr_Context::pc()
{
	user_regs_struct regs;
	ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);
	return regs.rip - m_load_address;
}

dwarf::taddr
Ptrace_Expr_Context::deref_size(dwarf::taddr			  address,
								[[maybe_unused]] unsigned size)
{
	return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}
