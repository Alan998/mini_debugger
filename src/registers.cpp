// functions to interact with registers
#include <registers.hpp>

#include <algorithm>
#include <stdexcept>
#include <sys/ptrace.h>
#include <sys/user.h> // user_regs_struct

namespace mini_debugger {

uint64_t
get_register_value(const pid_t pid, const Reg request_reg)
{
	user_regs_struct regs;
	ptrace(PTRACE_GETREGS, pid, nullptr, &regs);

	// search for the index of the register descriptor
	auto it = std::find_if(g_register_descriptors.begin(),
						   g_register_descriptors.end(),
						   [request_reg](auto&& register_descriptor) {
							   return register_descriptor.reg == request_reg;
						   });

	// cast to uint64_t is safe because user_regs_struct is a standard layout
	// type
	return *(reinterpret_cast<uint64_t*>(&regs) +
			 (it - g_register_descriptors.begin()));
}

void
set_register_value(const pid_t pid, const Reg request_reg, const uint64_t value)
{
	user_regs_struct regs;
	ptrace(PTRACE_GETREGS, pid, nullptr, &regs);

	// search for the index of the register descriptor
	auto it = std::find_if(g_register_descriptors.begin(),
						   g_register_descriptors.end(),
						   [request_reg](auto&& register_descriptor) {
							   return register_descriptor.reg == request_reg;
						   });

	// write value into the requested register
	*(reinterpret_cast<uint64_t*>(&regs) +
	  (it - g_register_descriptors.begin())) = value;
	ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
}

uint64_t
get_register_value_from_dwarf_register(const pid_t pid, const int reg_num)
{
	auto it = std::find_if(g_register_descriptors.begin(),
						   g_register_descriptors.end(),
						   [reg_num](auto&& register_descriptor) {
							   return register_descriptor.dwarf_r == reg_num;
						   });

	// check for an error in case of wrong DWARF information
	if (it == g_register_descriptors.end()) {
		throw std::out_of_range("Unknown dwarf register");
	}

	return get_register_value(pid, it->reg);
}

std::string
get_register_name(const Reg request_reg)
{
	auto it = std::find_if(g_register_descriptors.begin(),
						   g_register_descriptors.end(),
						   [request_reg](auto&& register_descriptor) {
							   return register_descriptor.reg == request_reg;
						   });
	return it->name;
}

Reg
get_register_from_name(const std::string_view name)
{
	auto it = std::find_if(g_register_descriptors.begin(),
						   g_register_descriptors.end(),
						   [name](auto&& register_descriptor) {
							   return register_descriptor.name == name.data();
						   });
	return it->reg;
}

};
