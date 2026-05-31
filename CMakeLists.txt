cmake_minimum_required(VERSION 3.22)

# ─── vcpkg integration (before project()) ──────────────────────────────
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")
endif()

# ─── project ───────────────────────────────────────────────────────────
project(nift
  VERSION 2.0.0
  DESCRIPTION "World's fastest C++ static site generator"
  HOMEPAGE_URL "https://github.com/ronggothelast/nsm-v2"
  LANGUAGES CXX C
)

# ─── global settings ──────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Feature-detect C++23 — use when available
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++23" NIFT_HAS_CXX23)
if(NIFT_HAS_CXX23)
  set(CMAKE_CXX_STANDARD 23)
  message(STATUS "Nift: C++23 available, using it")
else()
  message(STATUS "Nift: C++23 not available, using C++20 baseline")
endif()

# ─── compiler warnings (always) ──────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
  add_compile_options(/W4)
endif()

# -Werror only in CI (enabled via preset or env)
option(NIFT_WERROR "Treat warnings as errors" OFF)
if(NIFT_WERROR)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Werror)
  elseif(MSVC)
    add_compile_options(/WX)
  endif()
endif()

# ─── options ──────────────────────────────────────────────────────────
option(NIFT_BUILD_TESTS "Build test suites" ON)
option(NIFT_BUILD_BENCH "Build benchmarks" OFF)
option(NIFT_BUILD_FUZZ "Build fuzz targets" OFF)
option(NIFT_NO_COLOR "Disable ANSI color output" OFF)
option(NIFT_NO_PROGRESS "Disable progress bars" OFF)
option(NIFT_NO_CLEAR_LINES "Disable terminal cursor manipulation" OFF)

# ─── find vcpkg-managed deps ─────────────────────────────────────────
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

if(NIFT_BUILD_TESTS)
  find_package(Catch2 3 CONFIG REQUIRED)
endif()

# ─── subdirectories ──────────────────────────────────────────────────
add_subdirectory(crates/core)
add_subdirectory(crates/parser)
# add_subdirectory(crates/runtime)  # Phase 3
# add_subdirectory(crates/project)  # Phase 3
# add_subdirectory(crates/build)    # Phase 4
# add_subdirectory(crates/server)   # Phase 5
# add_subdirectory(crates/cli)      # Phase 6
# add_subdirectory(crates/plugin)   # Phase 6
# add_subdirectory(crates/compat)   # Phase 6
# add_subdirectory(apps/nift)       # Phase 6

if(NIFT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
