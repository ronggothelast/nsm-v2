/// @file test_path.cpp
/// @brief Tests for nift::core::Path.

#include <catch2/catch_test_macros.hpp>

#include "nift/core/path.hpp"

using nift::core::Path;

TEST_CASE("Path default constructor", "[path]") {
  Path p;
  REQUIRE(p.empty());
  REQUIRE(p.str().empty());
}

TEST_CASE("Path from string_view", "[path]") {
  Path p{"hello/world.txt"};
  REQUIRE(p.str() == "hello/world.txt");
  REQUIRE_FALSE(p.empty());
}

TEST_CASE("Path filename", "[path]") {
  Path p{"/foo/bar/baz.html"};
  REQUIRE(p.filename() == "baz.html");
}

TEST_CASE("Path stem", "[path]") {
  Path p{"/foo/bar/baz.html"};
  REQUIRE(p.stem() == "baz");
}

TEST_CASE("Path extension", "[path]") {
  Path p{"/foo/bar/baz.html"};
  REQUIRE(p.extension() == ".html");
}

TEST_CASE("Path parent", "[path]") {
  Path p{"/foo/bar/baz.html"};
  REQUIRE(p.parent().str() == "/foo/bar");
}

TEST_CASE("Path with_extension", "[path]") {
  Path p{"file.txt"};
  auto q = p.with_extension(".md");
  REQUIRE(q.str() == "file.md");
}

TEST_CASE("Path join", "[path]") {
  Path p{"/foo"};
  auto q = p.join("bar/baz.txt");
  REQUIRE(q.str() == "/foo/bar/baz.txt");
}

TEST_CASE("Path is_absolute / is_relative", "[path]") {
  Path abs{"/usr/local"};
  REQUIRE(abs.is_absolute());
  REQUIRE_FALSE(abs.is_relative());

  Path rel{"src/main.cpp"};
  REQUIRE(rel.is_relative());
  REQUIRE_FALSE(rel.is_absolute());
}

TEST_CASE("Path normalized", "[path]") {
  Path p{"foo/../bar/./baz.txt"};
  REQUIRE(p.normalized().str() == "bar/baz.txt");
}

TEST_CASE("Path comparison", "[path]") {
  Path a{"foo/bar"};
  Path b{"foo/bar"};
  Path c{"foo/baz"};
  REQUIRE(a == b);
  REQUIRE(a != c);
}
