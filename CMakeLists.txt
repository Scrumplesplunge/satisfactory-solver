cmake_minimum_required(VERSION 3.9)

project(solver)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION $<IF:$<CONFIG:Release>,ON,OFF>)

add_compile_options(
  -Werror
  -Wall
  -Wextra
  -Wno-unused
  -pedantic
  $<$<CONFIG:Debug>:-g3>
  $<$<CONFIG:Debug>:-Og>
)

add_library(integer_lib integer.cpp integer.hpp)

add_executable(integer_test integer_test.cpp)
target_link_libraries(integer_test integer_lib)
add_test(NAME integer_test COMMAND integer_test)

add_library(rational_lib rational.cpp rational.hpp)
target_link_libraries(rational_lib integer_lib)

add_library(data_lib data.cpp data.hpp)
target_link_libraries(data_lib rational_lib)

add_library(parser_lib parser.cpp parser.hpp)
target_link_libraries(parser_lib data_lib)

add_library(table_lib table.cpp table.hpp)

add_library(solver_lib solver.cpp solver.hpp)
target_link_libraries(solver_lib data_lib table_lib)

add_executable(solver main.cpp)
target_link_libraries(solver parser_lib solver_lib)
