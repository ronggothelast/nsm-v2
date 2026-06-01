/// @file test_path.cpp
/// @brief Tests for nift::core::Path.

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "nift/core/path.hpp"

using nift::core::Path;
namespace fs = std::filesystem;

TEST_CASE("Path default constructor", "[path]") {
  Path p;
  REQUIRE(p.empty());
  REQUIRE(p.str().empty());
}

TEST_CASE("Path from string_view", "[path]") {
  Path p{"hello/world.txt"};
  // Use generic_str for cross-platform comparison (forward slashes).
  REQUIRE(p.generic_str() == "hello/world.txt");
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
  REQUIRE(p.parent().generic_str() == "/foo/bar");
}

TEST_CASE("Path with_extension", "[path]") {
  Path p{"file.txt"};
  auto q = p.with_extension(".md");
  REQUIRE(q.generic_str() == "file.md");
}

TEST_CASE("Path join", "[path]") {
  Path p{"/foo"};
  auto q = p.join("bar/baz.txt");
  REQUIRE(q.generic_str() == "/foo/bar/baz.txt");
}

TEST_CASE("Path is_absolute / is_relative", "[path]") {
  // Use platform-appropriate absolute paths.
#ifdef _WIN32
  Path abs{"C:\\Windows\\System32"};
#else
  Path abs{"/usr/local"};
#endif
  REQUIRE(abs.is_absolute());
  REQUIRE_FALSE(abs.is_relative());

  Path rel{"src/main.cpp"};
  REQUIRE(rel.is_relative());
  REQUIRE_FALSE(rel.is_absolute());
}

TEST_CASE("Path normalized", "[path]") {
  Path p{"foo/../bar/./baz.txt"};
  REQUIRE(p.normalized().generic_str() == "bar/baz.txt");
}

TEST_CASE("Path comparison", "[path]") {
  Path a{"foo/bar"};
  Path b{"foo/bar"};
  Path c{"foo/baz"};
  REQUIRE(a == b);
  REQUIRE(a != c);
}
