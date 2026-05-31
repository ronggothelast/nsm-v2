/// @file test_system_info.cpp
/// @brief Tests for nift::core system info.

#include <catch2/catch_test_macros.hpp>

#include "nift/core/system_info.hpp"

using namespace nift::core;

TEST_CASE("detect_os returns known value", "[sysinfo]") {
  auto os = detect_os();
  // On this Linux CI, it should be linux.
  REQUIRE(os == OS::linux_os);
}

TEST_CASE("os_name returns non-empty", "[sysinfo]") {
  auto name = os_name();
  REQUIRE(std::string_view(name) == "linux");
}

TEST_CASE("hostname returns non-empty", "[sysinfo]") {
  auto h = hostname();
  REQUIRE_FALSE(h.empty());
}

TEST_CASE("hardware_concurrency >= 1", "[sysinfo]") {
  REQUIRE(hardware_concurrency() >= 1);
}
