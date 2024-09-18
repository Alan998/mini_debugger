// define register's structure and functions to interact with registers
#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace mini_debugger {

enum class Reg
{
	rax,
	rbx,
	rcx,
	rdx,
	rdi,
	rsi,
	rbp,
	rsp,
	r8,
	r9,
	r10,
	r11,
	r12,
	r13,
	r14,
	r15,
	rip,
	rflags,
	cs,
	orig_rax,
	fs_base,
	gs_base,
	fs,
	gs,
	ss,
	ds,
	es
};

static constexpr std::size_t TOTAL_REGISTERS{ 27 };

struct Reg_Descriptor
{
	Reg			reg;
	int			dwarf_r;
	std::string name;
};

static const std::array<Reg_Descriptor, TOTAL_REGISTERS> g_register_descriptors{
	{
		{ Reg::r15, 15, "r15" },
		{ Reg::r14, 14, "r14" },
		{ Reg::r13, 13, "r13" },
		{ Reg::r12, 12, "r12" },
		{ Reg::rbp, 6, "rbp" },
		{ Reg::rbx, 3, "rbx" },
		{ Reg::r11, 11, "r11" },
		{ Reg::r10, 10, "r10" },
		{ Reg::r9, 9, "r9" },
		{ Reg::r8, 8, "r8" },
		{ Reg::rax, 0, "rax" },
		{ Reg::rcx, 2, "rcx" },
		{ Reg::rdx, 1, "rdx" },
		{ Reg::rsi, 4, "rsi" },
		{ Reg::rdi, 5, "rdi" },
		{ Reg::orig_rax, -1, "orig_rax" },
		{ Reg::rip, -1, "rip" },
		{ Reg::cs, 51, "cs" },
		{ Reg::rflags, 49, "eflags" },
		{ Reg::rsp, 7, "rsp" },
		{ Reg::ss, 52, "ss" },
		{ Reg::fs_base, 58, "fs_base" },
		{ Reg::gs_base, 59, "gs_base" },
		{ Reg::ds, 53, "ds" },
		{ Reg::es, 50, "es" },
		{ Reg::fs, 54, "fs" },
		{ Reg::gs, 55, "gs" },
	}
};

// get requested register depending on which register is requested
std::intptr_t
get_register_value(const pid_t pid, const Reg request_reg);

// write data to register
void
set_register_value(const pid_t	  pid,
				   const Reg	  request_reg,
				   const uint64_t value);

std::intptr_t
get_register_value_from_dwarf_register(const pid_t pid, const int reg_num);

std::string
get_register_name(const Reg request_reg);

Reg
get_register_from_name(const std::string_view name);
};
