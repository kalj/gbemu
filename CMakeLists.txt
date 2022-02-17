cmake_minimum_required(VERSION 3.18)

project(gbemu CXX)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall -Wpedantic -Werror)

# add compiler output coloring also for ninja
if (CMAKE_GENERATOR STREQUAL "Ninja")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color)
  endif()
endif()

add_library(common_objects OBJECT
  src/cpu.cpp
  src/bus.cpp
  src/ppu.cpp
  src/log.cpp
  )

add_executable(gbemu src/gbemu.cpp)
target_link_libraries(gbemu common_objects fmt)

add_executable(get_opcodes src/get_opcodes.cpp)
target_link_libraries(get_opcodes common_objects fmt)
