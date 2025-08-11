cmake_minimum_required(VERSION 3.15)

# Project name and languages
project(liteseq LANGUAGES C CXX)

# Set the C standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Check if CMAKE_BUILD_TYPE is set; if not, default to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug, Release)" FORCE)
endif()

# Compiler options
# Debug-specific flags
set(DEBUG_FLAGS "-Wall -Wextra -Wpedantic -Wpadded -Wunused -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-private-field -Wno-unused-local-typedefs")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG ${DEBUG_FLAGS}") # let cmake.sanitizers set debug level to -O1
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Linker options for sanitizers
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${SANITIZER_FLAGS}")

# Directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Sanitizers are only enabled in Debug mode to avoid performance overhead in Release builds.
include(sanitizers.cmake)

# Find the threading package. On Linux, for instance, this will
# link with pthread, and add -pthread flags automatically.
find_package(Threads REQUIRED)

# ----------
# liteseq
# ----------

# Add the library
add_library(liteseqlib
    ${INCLUDE_DIR}/types.c
    ${INCLUDE_DIR}/utils.c
    ${INCLUDE_DIR}/gfa.c
    ${INCLUDE_DIR}/io.c
)

# Source files for binary
set(SOURCES
    ${SRC_DIR}/main.c
)

# Add the executable
add_executable(liteseq ${SOURCES})

# Link the library to the executable
target_link_libraries(liteseq PRIVATE liteseqlib Threads::Threads)

# Include directories for the library
# expose the given directories to anything linking against liteseqlib
target_include_directories(liteseqlib
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
