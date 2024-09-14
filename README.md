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
|Commands|Options|description|
|--------|-------|-----------|
|continue | - |continue program execution|
|register|dump|print all registers' value|
|register|read \[register name\]|read the register's value|
|register|write \[register name\] \[value\]|write value to register (value needs to start with 0x)|
|memory|read \[address\]|read memory at given address (address needs to start with 0x)|
|memory|write \[address\] \[value\]|write value into memory at given address (address and value needs to start with 0x)|

Press <Ctrl + d> to exit

## Goals
- [x] Launch, halt, and continue execution
- [ ] Set breakpoints on
	- [x] Memory addresses
	- [ ] Source code lines
	- [ ] Function entry
- [x] Read and write registers and memory
- [ ] Single stepping
- [ ] Print current source location
- [ ] Print backtrace
- [ ] Print values of simple variables
