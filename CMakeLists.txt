# === CMake Setup ===
cmake_minimum_required(VERSION 3.14)

# Project name, version and languages
project(liteseq LANGUAGES C)

# === BUILD OPTIONS ===
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
# Check if CMAKE_BUILD_TYPE is set; if not, default to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug, Release)" FORCE)
endif()

option(LITESEQ_BUILD_EXAMPLE "Build the liteseq CLI binary" OFF)

# === Platform & Toolchain Configuration ===

# Linker options for sanitizers
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${SANITIZER_FLAGS}")

# Directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)


# --- liteseq Library ---


# Add the library
add_library(liteseq
  ${INCLUDE_DIR}/types.c
  ${INCLUDE_DIR}/utils.c
  ${INCLUDE_DIR}/gfa.c
  ${INCLUDE_DIR}/io.c
)

# C standard & basic props
set_target_properties(liteseq PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED YES
  C_EXTENSIONS OFF
)

# Compiler options
# Debug-specific flags

set(COMMON_WARNINGS
  -Wall
)

set(DEBUG_WARNINGS
  -Wextra
  -Wpedantic
  -Wshadow
  -Wunused
  -Wunknown-pragmas
  -Wunused-parameter
  -Wunused-variable
  -Wunused-function
  -Wunused-but-set-variable
  -Wmissing-field-initializers
  -Wunused-local-typedefs
  -Wuninitialized
  -Wmaybe-uninitialized
  # -Wpadded # Uncomment or gate by compiler if needed
)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-g)
  add_compile_options(${COMMON_WARNINGS})
  add_compile_options(${DEBUG_WARNINGS})
  add_compile_definitions(DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(${COMMON_WARNINGS})
  add_compile_options(-O3)
  add_compile_definitions(RELEASE)
endif()



# Sanitizers are only enabled in Debug mode to avoid performance overhead in Release builds.
include(sanitizers.cmake)

# Find the threading package. On Linux, for instance, this will
# link with pthread, and add -pthread flags automatically.
find_package(Threads REQUIRED)



# Source files for binary
set(SOURCES
    ${SRC_DIR}/main.c
)

if(BUILD_LITESEQ_EXAMPLES)
# Add the executable
add_executable(liteseq-example ${SOURCES})
# Link the library to the executable
target_link_libraries(liteseq-example PRIVATE liteseq Threads::Threads)
endif()

# Include directories for the library
# expose the given directories to anything linking against liteseqlib
target_include_directories(liteseq
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
