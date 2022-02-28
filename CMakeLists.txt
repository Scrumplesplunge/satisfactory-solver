cmake_minimum_required(VERSION 3.1)

project(solver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Werror -Wall -Wextra -pedantic -g3)

add_library(rational_lib rational.cpp rational.hpp)

add_library(data_lib data.cpp data.hpp)
target_link_libraries(data_lib rational_lib)

add_library(parser_lib parser.cpp parser.hpp)
target_link_libraries(parser_lib data_lib)

add_library(table_lib table.cpp table.hpp)

add_library(solver_lib solver.cpp solver.hpp)
target_link_libraries(solver_lib data_lib table_lib)

add_executable(solver main.cpp)
target_link_libraries(solver parser_lib solver_lib)
