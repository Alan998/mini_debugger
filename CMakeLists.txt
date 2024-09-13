cmake_minimum_required(VERSION 3.20)
project(mini_debugger)

# set cpp standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "./src/*.cpp")
add_executable(mini_debugger ${SOURCES} ext/linenoise/linenoise.c)
target_include_directories(mini_debugger PRIVATE ext/linenoise include)
target_compile_options(mini_debugger PRIVATE -Wall -Wextra -Werror)

# example files for testing
add_executable(hello examples/hello.cpp)
target_compile_options(hello PRIVATE -g -O0)

# format files
# search for .cpp and .hpp files and pass them to clang-format
# -o argument in find is used to specify logical OR
add_custom_target(format
	COMMAND find ./src ./include -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Formatting code...")
