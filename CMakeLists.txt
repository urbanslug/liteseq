# === CMake Setup ===
cmake_minimum_required(VERSION 3.14)

# Project name, version and languages
project(liteseq LANGUAGES C CXX)

# === BUILD OPTIONS ===
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
# Check if CMAKE_BUILD_TYPE is set; if not, default to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug, Release)" FORCE)
endif()

option(LITESEQ_BUILD_EXAMPLE "Build the liteseq CLI binary" OFF)

# === Platform & Toolchain Configuration ===

# Directories
set(EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRC_INTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/internal)
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# --- liteseq Library ---

# Add the library
add_library(liteseq
  ${SRC_INTERNAL_DIR}/lq_types.c
  ${SRC_INTERNAL_DIR}/lq_utils.c
  ${SRC_INTERNAL_DIR}/lq_io.c
  ${SRC_DIR}/gfa.c
  ${SRC_DIR}/refs.c
)

# C standard & basic props
set_target_properties(liteseq PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED YES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
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


# Linker options for sanitizers
# Linker options for sanitizers
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Default to ON in Debug mode, but allow manual override
  option(USE_SANITIZER "Enable sanitizers (ON by default in Debug mode)" ON)
else()
  # Default to OFF in non-Debug modes, and allow manual override
  option(USE_SANITIZER "Enable sanitizers (ON by default in Debug mode)" OFF)
endif()

# Include sanitizers only if USE_SANITIZER is ON
if (USE_SANITIZER)
  include(cmake/sanitizers.cmake)
endif()

# --- Threading Configuration ---
if(APPLE)  # true for macOS/iOS simulators, etc.
  find_library(DISPATCH_LIB NAMES dispatch)
  if(DISPATCH_LIB)
    message(STATUS "Using Grand Central Dispatch (GCD) for threading on Apple platforms.")
    target_link_libraries(liteseq PRIVATE "${DISPATCH_LIB}")
  else()
    message(FATAL_ERROR "Grand Central Dispatch (libdispatch) not found.")
  endif()
else()
  find_package(Threads REQUIRED)
  message(STATUS "Using pthreads for threading on non-Apple platforms.")
  target_link_libraries(liteseq PRIVATE Threads::Threads)
endif()

# Include directories for the library
# expose the given directories to anything linking against liteseqlib
target_include_directories(liteseq
  PUBLIC ${INCLUDE_DIR}
  PRIVATE ${SRC_INTERNAL_DIR}
)

include(GNUInstallDirs)
install(TARGETS liteseq
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/liteseq/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/liteseq)


# === Tests ===

if (LITESEQ_ENABLE_TESTING)
  # Add the global TESTING definition
  add_compile_definitions(TESTING)

  # --- Dependencies ---
  # Use FetchContent to fetch and build dependencies from GitHub
  include(FetchContent)

  # GoogleTest
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE  # Add this line
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_subdirectory(tests)
endif()

# === Example Binary ===
if(LITESEQ_BUILD_EXAMPLE)

  # Source files for binary
  set(SOURCES
    ${EXAMPLES_DIR}/example.c
  )

  # Add the executable
  add_executable(liteseq-example ${SOURCES})
  # Link the library to the executable
  target_link_libraries(liteseq-example PRIVATE liteseq)
endif()

