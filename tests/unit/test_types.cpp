/// @file test_types.cpp
/// @brief Tests for nift::core types (Expected, Error, Result).

#include <catch2/catch_test_macros.hpp>

#include "nift/core/types.hpp"

TEST_CASE("Expected success path", "[types]") {
  nift::Result<int> r = 42;
  REQUIRE(r.has_value());
  REQUIRE(*r == 42);
  REQUIRE(r.value() == 42);
}

TEST_CASE("Expected error path", "[types]") {
  nift::Result<int> r = nift::unexpected(nift::Error::not_found);
  REQUIRE_FALSE(r.has_value());
  REQUIRE(r.error() == nift::Error::not_found);
}

TEST_CASE("Expected bool conversion", "[types]") {
  nift::Result<std::string> ok_r = std::string("hello");
  REQUIRE(static_cast<bool>(ok_r));

  nift::Result<std::string> err_r =
      nift::unexpected(nift::Error::io_error);
  REQUIRE_FALSE(static_cast<bool>(err_r));
}

TEST_CASE("Status ok()", "[types]") {
  nift::Status s = nift::ok();
  REQUIRE(s.has_value());
}

TEST_CASE("Error to_string covers all variants", "[types]") {
  REQUIRE(nift::to_string(nift::Error::not_found) == "not found");
  REQUIRE(nift::to_string(nift::Error::permission_denied) ==
          "permission denied");
  REQUIRE(nift::to_string(nift::Error::already_exists) == "already exists");
  REQUIRE(nift::to_string(nift::Error::invalid_argument) ==
          "invalid argument");
  REQUIRE(nift::to_string(nift::Error::io_error) == "I/O error");
  REQUIRE(nift::to_string(nift::Error::parse_error) == "parse error");
  REQUIRE(nift::to_string(nift::Error::unknown) == "unknown error");
}
