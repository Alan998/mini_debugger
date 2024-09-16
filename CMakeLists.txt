cmake_minimum_required(VERSION 3.20)
project(mini_debugger)

# set cpp standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# collect source files
file(GLOB SOURCES "src/*.cpp")
message(STATUS "Source file: ${SOURCES}")

add_executable(mini_debugger ${SOURCES} ext/linenoise/linenoise.c)
target_include_directories(mini_debugger PRIVATE ext/linenoise ext/libelfin include)
target_compile_options(mini_debugger PRIVATE -Wall -Wextra)
target_link_libraries(mini_debugger PRIVATE
	${PROJECT_SOURCE_DIR}/ext/libelfin/dwarf/libdwarf++.so
	${PROJECT_SOURCE_DIR}/ext/libelfin/elf/libelf++.so)

# example files for testing
# NOTE: since libelfin only supports DWARFv4, and g++ uses DWARFv5
# we need to compile the debugee program with -gdwarf-2 flag
add_executable(hello examples/hello.cpp)
target_compile_options(hello PRIVATE -g -gdwarf-2 -O0)

add_executable(variable examples/variable.cpp)
target_compile_options(variable PRIVATE -g -gdwarf-2 -O0)

add_executable(unwinding examples/stack_unwinding.cpp)
target_compile_options(unwinding PRIVATE -g -gdwarf-2 -O0)

# format files
# search for .cpp and .hpp files and pass them to clang-format
# -o argument in find is used to specify logical OR
add_custom_target(format
	COMMAND find ./src ./include -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Formatting code...")
