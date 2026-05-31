/// @file test_datetime.cpp
/// @brief Tests for nift::core datetime utilities.

#include <catch2/catch_test_macros.hpp>

#include "nift/core/datetime.hpp"

using namespace nift::core;

TEST_CASE("now_epoch_seconds is reasonable", "[datetime]") {
  auto now = now_epoch_seconds();
  // Should be after 2020-01-01 (1577836800).
  REQUIRE(now > 1577836800);
}

TEST_CASE("format_iso8601 known epoch", "[datetime]") {
  // 2024-01-01 00:00:00 UTC = 1704067200
  REQUIRE(format_iso8601(1704067200) == "2024-01-01T00:00:00Z");
}

TEST_CASE("format_date known epoch", "[datetime]") {
  REQUIRE(format_date(1704067200) == "2024-01-01");
}

TEST_CASE("format_datetime known epoch", "[datetime]") {
  REQUIRE(format_datetime(1704067200) == "2024-01-01 00:00:00");
}

TEST_CASE("format_iso8601 unix epoch", "[datetime]") {
  REQUIRE(format_iso8601(0) == "1970-01-01T00:00:00Z");
}
