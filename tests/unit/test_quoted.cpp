/// @file test_quoted.cpp
/// @brief Tests for nift::core quoted string parsing.

#include <catch2/catch_test_macros.hpp>

#include "nift/core/quoted.hpp"

using namespace nift::core;

TEST_CASE("parse_quoted double-quoted", "[quoted]") {
  auto r = parse_quoted("\"hello world\"");
  REQUIRE(r.has_value());
  REQUIRE(r->content == "hello world");
  REQUIRE(r->consumed == 13);
}

TEST_CASE("parse_quoted single-quoted", "[quoted]") {
  auto r = parse_quoted("'hello'");
  REQUIRE(r.has_value());
  REQUIRE(r->content == "hello");
  REQUIRE(r->consumed == 7);
}

TEST_CASE("parse_quoted with escapes", "[quoted]") {
  auto r = parse_quoted("\"hello\\nworld\"");
  REQUIRE(r.has_value());
  REQUIRE(r->content == "hello\nworld");
}

TEST_CASE("parse_quoted with escaped quote", "[quoted]") {
  auto r = parse_quoted("\"say \\\"hi\\\"\"");
  REQUIRE(r.has_value());
  REQUIRE(r->content == "say \"hi\"");
}

TEST_CASE("parse_quoted with backslash escape", "[quoted]") {
  auto r = parse_quoted("\"path\\\\to\\\\file\"");
  REQUIRE(r.has_value());
  REQUIRE(r->content == "path\\to\\file");
}

TEST_CASE("parse_quoted empty string", "[quoted]") {
  auto r = parse_quoted("\"\"");
  REQUIRE(r.has_value());
  REQUIRE(r->content.empty());
  REQUIRE(r->consumed == 2);
}

TEST_CASE("parse_quoted no quote returns nullopt", "[quoted]") {
  REQUIRE_FALSE(parse_quoted("hello").has_value());
  REQUIRE_FALSE(parse_quoted("").has_value());
}

TEST_CASE("parse_quoted unterminated returns nullopt", "[quoted]") {
  REQUIRE_FALSE(parse_quoted("\"hello").has_value());
}

TEST_CASE("escape_string", "[quoted]") {
  REQUIRE(escape_string("hello\nworld") == "hello\\nworld");
  REQUIRE(escape_string("say \"hi\"") == "say \\\"hi\\\"");
  REQUIRE(escape_string("a\\b") == "a\\\\b");
  REQUIRE(escape_string("") == "");
}

TEST_CASE("unquote", "[quoted]") {
  REQUIRE(unquote("\"hello\"") == "hello");
  REQUIRE(unquote("'hello'") == "hello");
  REQUIRE(unquote("hello") == "hello");
  REQUIRE(unquote("\"\"") == "");
  REQUIRE(unquote("''") == "");
  REQUIRE(unquote("") == "");
  REQUIRE(unquote("\"a'") == "\"a'");  // mismatched
}
