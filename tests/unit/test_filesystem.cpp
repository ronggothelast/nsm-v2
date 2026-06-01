/// @file test_filesystem.cpp
/// @brief Tests for nift::core filesystem utilities.

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <filesystem>

#if defined(_WIN32)
#include <io.h>
#include <stdlib.h>
#else
#include <stdlib.h>
#include <unistd.h>
#endif

#include "nift/core/filesystem.hpp"

using namespace nift::core;

namespace {

#if defined(_WIN32)
// Windows MinGW lacks mkdtemp; emulate with _mktemp_s + create_directory.
inline char* portable_mkdtemp(char* tmpl) {
  if (_mktemp_s(tmpl, std::strlen(tmpl) + 1) != 0)
    return nullptr;
  std::error_code ec;
  if (!std::filesystem::create_directory(tmpl, ec) || ec)
    return nullptr;
  return tmpl;
}
#else
inline char* portable_mkdtemp(char* tmpl) {
  return ::mkdtemp(tmpl);
}
#endif

/// RAII temp directory for test isolation.
struct TempDir {
  Path path;
  TempDir() : path{std::filesystem::temp_directory_path() / "nift_test_XXXXXX"} {
    auto tmpl = path.str();
    char* result = portable_mkdtemp(tmpl.data());
    REQUIRE(result != nullptr);
    path = Path{std::string(result)};
  }
  ~TempDir() { std::filesystem::remove_all(path.native()); }
};
}  // namespace

TEST_CASE("write_file + read_file round-trip", "[filesystem]") {
  TempDir tmp;
  auto file = tmp.path.join("hello.txt");

  auto ws = write_file(file, "Hello, Nift!");
  REQUIRE(ws.has_value());

  auto content = read_file(file);
  REQUIRE(content.has_value());
  REQUIRE(*content == "Hello, Nift!");
}

TEST_CASE("read_file on missing file returns not_found", "[filesystem]") {
  auto r = read_file(Path{"/tmp/nift_nonexistent_file_xyz123.txt"});
  REQUIRE_FALSE(r.has_value());
  REQUIRE(r.error() == nift::Error::not_found);
}

TEST_CASE("write_file creates parent dirs", "[filesystem]") {
  TempDir tmp;
  auto file = tmp.path.join("a/b/c/deep.txt");

  auto ws = write_file(file, "deep");
  REQUIRE(ws.has_value());

  auto content = read_file(file);
  REQUIRE(content.has_value());
  REQUIRE(*content == "deep");
}

TEST_CASE("file_exists", "[filesystem]") {
  TempDir tmp;
  auto file = tmp.path.join("exists.txt");

  REQUIRE_FALSE(file_exists(file));
  REQUIRE(write_file(file, "x").has_value());
  REQUIRE(file_exists(file));
}

TEST_CASE("is_directory", "[filesystem]") {
  TempDir tmp;
  REQUIRE(is_directory(tmp.path));
  REQUIRE_FALSE(is_directory(tmp.path.join("nonexistent")));
}

TEST_CASE("list_directory", "[filesystem]") {
  TempDir tmp;
  REQUIRE(write_file(tmp.path.join("a.txt"), "a").has_value());
  REQUIRE(write_file(tmp.path.join("b.txt"), "b").has_value());

  auto entries = list_directory(tmp.path);
  REQUIRE(entries.has_value());
  REQUIRE(entries->size() == 2);
}

TEST_CASE("list_directory on non-directory returns not_found", "[filesystem]") {
  auto r = list_directory(Path{"/tmp/nift_nonexistent_dir_xyz"});
  REQUIRE_FALSE(r.has_value());
}

TEST_CASE("create_directories", "[filesystem]") {
  TempDir tmp;
  auto deep = tmp.path.join("x/y/z");
  REQUIRE(create_directories(deep).has_value());
  REQUIRE(is_directory(deep));
}

TEST_CASE("remove_path", "[filesystem]") {
  TempDir tmp;
  auto file = tmp.path.join("del.txt");
  REQUIRE(write_file(file, "gone").has_value());
  REQUIRE(file_exists(file));
  REQUIRE(remove_path(file).has_value());
  REQUIRE_FALSE(file_exists(file));
}

TEST_CASE("copy_file", "[filesystem]") {
  TempDir tmp;
  auto src = tmp.path.join("src.txt");
  auto dst = tmp.path.join("sub/dst.txt");
  REQUIRE(write_file(src, "copy me").has_value());
  REQUIRE(copy_file(src, dst).has_value());

  auto content = read_file(dst);
  REQUIRE(content.has_value());
  REQUIRE(*content == "copy me");
}

TEST_CASE("file_size", "[filesystem]") {
  TempDir tmp;
  auto file = tmp.path.join("sized.txt");
  REQUIRE(write_file(file, "12345").has_value());

  auto sz = file_size(file);
  REQUIRE(sz.has_value());
  REQUIRE(*sz == 5);
}

TEST_CASE("last_modified returns reasonable epoch", "[filesystem]") {
  TempDir tmp;
  auto file = tmp.path.join("timed.txt");
  REQUIRE(write_file(file, "time").has_value());

  auto mtime = last_modified(file);
  REQUIRE(mtime.has_value());
  // Should be a recent epoch (after 2020).
  REQUIRE(*mtime > 1577836800);
}
