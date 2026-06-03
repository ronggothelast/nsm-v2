cmake_minimum_required(VERSION 3.22)

# ─── vcpkg integration (before project()) ──────────────────────────────
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")
endif()

# ─── project ───────────────────────────────────────────────────────────
project(nift
  VERSION 1.0.0
  DESCRIPTION "C++20 static site generator"
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

# ─── options ───────────────────────────────────────────────────────
# cpp-httplib requires Win8+ APIs (GetAddrInfoExCancel) that MinGW only
# exposes when _WIN32_WINNT >= 0x0602. Set globally so every target that
# includes httplib.h — including test binaries — picks it up.
if(WIN32)
  add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()

option(NIFT_BUILD_TESTS "Build test suites" ON)
option(NIFT_BUILD_BENCH "Build benchmarks" OFF)
option(NIFT_BUILD_FUZZ "Build fuzz targets" OFF)
option(NIFT_NO_COLOR "Disable ANSI color output" OFF)
option(NIFT_NO_PROGRESS "Disable progress bars" OFF)
option(NIFT_NO_CLEAR_LINES "Disable terminal cursor manipulation" OFF)
option(NIFT_STATIC_LINK "Link statically (Linux only)" OFF)
option(NIFT_COVERAGE "Enable code coverage (gcc/clang)" OFF)

# Static linking
if(NIFT_STATIC_LINK)
  if(MSVC)
    message(WARNING "Static linking not fully supported on MSVC")
  elseif(APPLE)
    # Apple Clang does not support -static-libc++.
    # Ship libc++.dylib alongside or use install_name_tool.
    message(FATAL_ERROR "Static linking is not supported on macOS. "
                        "Build without -DNIFT_STATIC_LINK.")
  else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  endif()
endif()

# Coverage instrumentation
if(NIFT_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
  else()
    message(WARNING "Coverage only supported with GCC/Clang")
  endif()
endif()

# ─── find vcpkg-managed deps ─────────────────────────────────────────
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(blake3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(mio CONFIG REQUIRED)
find_package(xsimd CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(cmark-gfm CONFIG REQUIRED)
find_package(cmark-gfm-extensions CONFIG REQUIRED)
find_package(Threads REQUIRED)

# MinGW's ws2tcpip.h does not declare GetAddrInfoExCancel /
# GetAddrInfoExW, which cpp-httplib's non-blocking DNS needs.
# Strip the INTERFACE define so it is never passed to MinGW builds.
if(MINGW AND TARGET httplib::httplib)
  get_target_property(_httplib_defs httplib::httplib INTERFACE_COMPILE_DEFINITIONS)
  if(_httplib_defs)
    list(REMOVE_ITEM _httplib_defs "CPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO")
    set_target_properties(httplib::httplib PROPERTIES
      INTERFACE_COMPILE_DEFINITIONS "${_httplib_defs}")
  endif()
endif()

if(NIFT_BUILD_TESTS)
  find_package(Catch2 3 CONFIG REQUIRED)
endif()

# ─── subdirectories ──────────────────────────────────────────────────
add_subdirectory(crates/core)
add_subdirectory(crates/parser)
add_subdirectory(crates/markdown)
add_subdirectory(crates/tailwind)
add_subdirectory(crates/assets)
add_subdirectory(crates/images)
add_subdirectory(crates/runtime)
add_subdirectory(crates/project)
add_subdirectory(crates/build)
add_subdirectory(crates/server)
add_subdirectory(crates/compat)
add_subdirectory(crates/plugin)
add_subdirectory(crates/cli)
add_subdirectory(apps/nift)

if(NIFT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

option(NIFT_QA_SECURITY "Build adversarial Lua sandbox suite" OFF)
option(NIFT_QA_FUZZING  "Build libFuzzer harnesses (Clang only)" OFF)
if(NIFT_QA_SECURITY)
  enable_testing()
  add_subdirectory(tests/qa/security)
endif()
if(NIFT_QA_FUZZING)
  add_subdirectory(tests/qa/fuzzing)
endif()
