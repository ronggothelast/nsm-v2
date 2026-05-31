/// @file test_simd_scanner.cpp

#include <catch2/catch_test_macros.hpp>

#include <string>

#include "nift/build/simd_scanner.hpp"

using namespace nift::build;

TEST_CASE("scan_byte: empty input", "[build][simd]") {
  auto pos = scan_byte("", '@');
  CHECK(pos.empty());
}

TEST_CASE("scan_byte: no matches", "[build][simd]") {
  auto pos = scan_byte("hello world no specials", '@');
  CHECK(pos.empty());
}

TEST_CASE("scan_byte: single match", "[build][simd]") {
  auto pos = scan_byte("hello @ world", '@');
  REQUIRE(pos.size() == 1);
  CHECK(pos[0] == 6);
}

TEST_CASE("scan_byte: multiple matches", "[build][simd]") {
  auto pos = scan_byte("@a @b @c @d", '@');
  REQUIRE(pos.size() == 4);
  CHECK(pos[0] == 0);
  CHECK(pos[1] == 3);
  CHECK(pos[2] == 6);
  CHECK(pos[3] == 9);
}

TEST_CASE("scan_byte: matches across SIMD batch boundary", "[build][simd]") {
  // 100 bytes, with `@` at positions 0, 31, 32, 63, 64, 99 — these straddle
  // typical 16/32-byte SIMD batches.
  std::string buf(100, '.');
  std::vector<std::size_t> expected = {0, 31, 32, 63, 64, 99};
  for (auto p : expected) buf[p] = '@';
  auto got = scan_byte(buf, '@');
  CHECK(got == expected);
}

TEST_CASE("scan_byte: dense matches", "[build][simd]") {
  std::string buf(64, '@');
  auto pos = scan_byte(buf, '@');
  CHECK(pos.size() == 64);
  for (std::size_t i = 0; i < 64; ++i) CHECK(pos[i] == i);
}

TEST_CASE("scan_positions: @ + $ + \\", "[build][simd]") {
  auto pos = scan_positions("@x $y \\z plain", {true, true, true});
  REQUIRE(pos.size() == 3);
  CHECK(pos[0] == 0);
  CHECK(pos[1] == 3);
  CHECK(pos[2] == 6);
}

TEST_CASE("scan_positions: only @", "[build][simd]") {
  ScanTargets t{true, false, false};
  auto pos = scan_positions("@x $y \\z", t);
  REQUIRE(pos.size() == 1);
  CHECK(pos[0] == 0);
}

TEST_CASE("contains_any: yes", "[build][simd]") {
  CHECK(contains_any("hello @world"));
}

TEST_CASE("contains_any: no", "[build][simd]") {
  CHECK_FALSE(contains_any("plain text only"));
}

TEST_CASE("find_first: returns first @", "[build][simd]") {
  CHECK(find_first("plain @ here") == 6);
}

TEST_CASE("find_first: npos when none", "[build][simd]") {
  CHECK(find_first("nothing special") == std::string_view::npos);
}

TEST_CASE("scan: no targets returns empty", "[build][simd]") {
  ScanTargets none{false, false, false};
  CHECK(scan_positions("anything", none).empty());
}

TEST_CASE("scan: large buffer correctness", "[build][simd]") {
  // 10 KB of dots with `@` every 137 bytes — odd stride to avoid alignment luck.
  std::string buf(10 * 1024, '.');
  std::vector<std::size_t> expected;
  for (std::size_t i = 137; i < buf.size(); i += 137) {
    buf[i] = '@';
    expected.push_back(i);
  }
  auto got = scan_byte(buf, '@');
  CHECK(got == expected);
}
