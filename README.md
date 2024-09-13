A learning project to create a mini debugger following [this tutorial](https://blog.tartanllama.xyz/writing-a-linux-debugger-setup/).

# Build Instructions

## Prerequisites
- [linenoise](https://github.com/antirez/linenoise) for handling command line inputs
- [libelfin](https://github.com/TartanLlama/libelfin/tree/fbreg) for parsing debug information (using the fbreg branch)
- cmake
- g++ (with c++20 standards supported or above)

## Build
After cloning into your own directory, build linenoise and libelfin with make.

Use cmake to build the project
``` bash
# create an empty directory build
mkdir -p build
cmake -S . -B ./build
# or
#cd build; cmake ../
```

# Usage
```bash
./mini_debugger <program_executable>
```
## Available Commands
- continue

Press <Ctrl + d> to exit

## Goals
- [x] Launch, halt, and continue execution
- [ ] Set breakpoints on
	- [ ] Memory addresses
	- [ ] Source code lines
	- [ ] Function entry
- [ ] Read and write registers and memory
- [ ] Single stepping
- [ ] Print current source location
- [ ] Print backtrace
- [ ] Print values of simple variables
