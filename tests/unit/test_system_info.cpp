/// @file test_system_info.cpp
/// @brief Tests for nift::core system info.

#include <catch2/catch_test_macros.hpp>

#include "nift/core/system_info.hpp"

using namespace nift::core;

TEST_CASE("detect_os returns known value", "[sysinfo]") {
  auto os = detect_os();
  // Must match the actual platform we compiled on.
#if defined(__linux__)
  REQUIRE(os == OS::linux_os);
#elif defined(__APPLE__)
  REQUIRE(os == OS::macos);
#elif defined(_WIN32)
  REQUIRE(os == OS::windows);
#elif defined(__FreeBSD__)
  REQUIRE(os == OS::freebsd);
#else
  REQUIRE(os == OS::unknown);
#endif
}

TEST_CASE("os_name returns non-empty", "[sysinfo]") {
  auto name = os_name();
#if defined(__linux__)
  REQUIRE(std::string_view(name) == "linux");
#elif defined(__APPLE__)
  REQUIRE(std::string_view(name) == "macos");
#elif defined(_WIN32)
  REQUIRE(std::string_view(name) == "windows");
#elif defined(__FreeBSD__)
  REQUIRE(std::string_view(name) == "freebsd");
#else
  REQUIRE(std::string_view(name) == "unknown");
#endif
  REQUIRE_FALSE(std::string_view(name).empty());
}

TEST_CASE("hostname returns non-empty", "[sysinfo]") {
  auto h = hostname();
  REQUIRE_FALSE(h.empty());
}

TEST_CASE("hardware_concurrency >= 1", "[sysinfo]") {
  REQUIRE(hardware_concurrency() >= 1);
}
