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

if(CMAKE_BUILD_TYPE STREQUAL "ASAN")
  add_compile_options("-fsanitize=address" "-g" "-O0")
  add_link_options("-fsanitize=address")
endif()

add_library(common_objects OBJECT
  src/gameboy.cpp
  src/cpu.cpp
  src/bus.cpp
  src/controller.cpp
  src/communication.cpp
  src/div_timer.cpp
  src/sound.cpp
  src/ppu.cpp
  src/interrupt_state.cpp
  src/cartridge.cpp
  src/logging.cpp
  )

include(FetchContent)
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG        v2.1.2
)

FetchContent_MakeAvailable(cli11)

find_package(SDL2 REQUIRED)

add_executable(gbemu src/gbemu.cpp)
target_link_libraries(gbemu common_objects fmt SDL2 CLI11::CLI11)

add_executable(get_opcodes src/get_opcodes.cpp)
target_link_libraries(get_opcodes common_objects fmt)
