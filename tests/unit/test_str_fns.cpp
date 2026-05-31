/// @file test_str_fns.cpp
/// @brief Tests for nift::core string utilities.

#include <catch2/catch_test_macros.hpp>

#include "nift/core/str_fns.hpp"

using namespace nift::core;

TEST_CASE("trim whitespace", "[str]") {
  REQUIRE(trim("  hello  ") == "hello");
  REQUIRE(trim("hello") == "hello");
  REQUIRE(trim("  ") == "");
  REQUIRE(trim("") == "");
  REQUIRE(trim("\t\n hello \r\n") == "hello");
}

TEST_CASE("trim_left", "[str]") {
  REQUIRE(trim_left("  hello  ") == "hello  ");
  REQUIRE(trim_left("hello") == "hello");
}

TEST_CASE("trim_right", "[str]") {
  REQUIRE(trim_right("  hello  ") == "  hello");
  REQUIRE(trim_right("hello") == "hello");
}

TEST_CASE("split by char", "[str]") {
  auto parts = split("a,b,c", ',');
  REQUIRE(parts.size() == 3);
  REQUIRE(parts[0] == "a");
  REQUIRE(parts[1] == "b");
  REQUIRE(parts[2] == "c");
}

TEST_CASE("split preserves empty parts", "[str]") {
  auto parts = split("a,,b", ',');
  REQUIRE(parts.size() == 3);
  REQUIRE(parts[0] == "a");
  REQUIRE(parts[1] == "");
  REQUIRE(parts[2] == "b");
}

TEST_CASE("split by string delimiter", "[str]") {
  auto parts = split("a::b::c", std::string_view{"::"});
  REQUIRE(parts.size() == 3);
  REQUIRE(parts[0] == "a");
  REQUIRE(parts[1] == "b");
  REQUIRE(parts[2] == "c");
}

TEST_CASE("split empty delimiter", "[str]") {
  auto parts = split("abc", std::string_view{""});
  REQUIRE(parts.size() == 1);
  REQUIRE(parts[0] == "abc");
}

TEST_CASE("join", "[str]") {
  std::vector<std::string_view> parts = {"a", "b", "c"};
  REQUIRE(join(parts, ", ") == "a, b, c");
  REQUIRE(join(parts, "") == "abc");
}

TEST_CASE("join empty", "[str]") {
  std::vector<std::string_view> parts;
  REQUIRE(join(parts, ", ") == "");
}

TEST_CASE("replace_all", "[str]") {
  REQUIRE(replace_all("hello world", "world", "nift") == "hello nift");
  REQUIRE(replace_all("aaa", "a", "bb") == "bbbbbb");
  REQUIRE(replace_all("abc", "x", "y") == "abc");
  REQUIRE(replace_all("abc", "", "x") == "abc");
}

TEST_CASE("starts_with", "[str]") {
  REQUIRE(starts_with("hello", "hel"));
  REQUIRE(starts_with("hello", "hello"));
  REQUIRE_FALSE(starts_with("hello", "world"));
  REQUIRE_FALSE(starts_with("hi", "hello"));
  REQUIRE(starts_with("hello", ""));
}

TEST_CASE("ends_with", "[str]") {
  REQUIRE(ends_with("hello", "llo"));
  REQUIRE(ends_with("hello", "hello"));
  REQUIRE_FALSE(ends_with("hello", "world"));
  REQUIRE(ends_with("hello", ""));
}

TEST_CASE("contains", "[str]") {
  REQUIRE(contains("hello world", "lo wo"));
  REQUIRE(contains("hello", "hello"));
  REQUIRE_FALSE(contains("hello", "xyz"));
  REQUIRE(contains("hello", ""));
}

TEST_CASE("to_lower", "[str]") {
  REQUIRE(to_lower("Hello World") == "hello world");
  REQUIRE(to_lower("ABC123") == "abc123");
  REQUIRE(to_lower("") == "");
}

TEST_CASE("to_upper", "[str]") {
  REQUIRE(to_upper("Hello World") == "HELLO WORLD");
  REQUIRE(to_upper("abc123") == "ABC123");
  REQUIRE(to_upper("") == "");
}
