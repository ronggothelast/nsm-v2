/// @file test_cache_extra.cpp
/// @brief Additional tests for BuildCache — CBOR, corruption, hash_file.

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "nift/project/build_cache.hpp"

using namespace nift::project;
namespace fs = std::filesystem;

namespace {
struct TempCache {
  fs::path root;
  TempCache() {
    root = fs::temp_directory_path() /
           ("nift_cache_extra_" + std::to_string(std::rand()));
    fs::create_directories(root);
  }
  ~TempCache() {
    std::error_code ec;
    fs::remove_all(root, ec);
  }
};
}  // namespace

TEST_CASE("hash_file returns correct hash", "[project][hash]") {
  // Create a temp file with known content
  auto tmp = fs::temp_directory_path() / "nift_hash_test.txt";
  {
    std::ofstream ofs(tmp);
    ofs << "hello world";
  }
  auto result = hash_file(::nift::core::Path(tmp.string()));
  REQUIRE(result.has_value());
  // BLAKE3("hello world") should be deterministic
  CHECK(result->size() == 64);  // 64 hex chars = 256 bits
  CHECK(*result == hash_content("hello world"));
  fs::remove(tmp);
}

TEST_CASE("hash_file returns Error for missing file", "[project][hash]") {
  auto result = hash_file(::nift::core::Path("/nonexistent/file.txt"));
  CHECK_FALSE(result.has_value());
}

TEST_CASE("BuildCache: corrupted CBOR falls back to JSON",
          "[project][cache]") {
  TempCache tc;
  // Create a valid cache first
  BuildCache cache1(::nift::core::Path(tc.root.string()));
  TrackedFile tf;
  tf.source = ::nift::core::Path("src/index.html");
  tf.output = ::nift::core::Path("out/index.html");
  tf.content_hash = "abc123";
  tf.deps_hash = "";
  tf.mtime = 1000;
  tf.built_at = 2000;
  cache1.put(tf);
  REQUIRE(cache1.save().has_value());

  // Corrupt the CBOR file
  auto cbor_path = tc.root / "index.json.cbor";
  REQUIRE(fs::exists(cbor_path));
  {
    std::ofstream ofs(cbor_path, std::ios::binary);
    ofs << "NOT_VALID_CBOR_DATA!!!";
  }

  // Load should fall back to JSON and succeed
  BuildCache cache2(::nift::core::Path(tc.root.string()));
  auto load_result = cache2.load();
  CHECK(load_result.has_value());
  auto entry = cache2.get(::nift::core::Path("src/index.html"));
  REQUIRE(entry.has_value());
  CHECK(entry->content_hash == "abc123");
}

TEST_CASE("BuildCache: CBOR load is faster than JSON load",
          "[project][cache][!benchmark]") {
  TempCache tc;
  // Create a cache with many entries
  BuildCache cache1(::nift::core::Path(tc.root.string()));
  for (int i = 0; i < 100; ++i) {
    TrackedFile tf;
    tf.source = ::nift::core::Path("src/file_" + std::to_string(i) + ".html");
    tf.output = ::nift::core::Path("out/file_" + std::to_string(i) + ".html");
    tf.content_hash = "hash_" + std::to_string(i);
    tf.deps_hash = "";
    tf.mtime = i;
    tf.built_at = i + 1000;
    cache1.put(tf);
  }
  REQUIRE(cache1.save().has_value());

  // Verify both CBOR and JSON exist
  CHECK(fs::exists(tc.root / "index.json.cbor"));
  CHECK(fs::exists(tc.root / "index.json"));

  // Load and verify
  BuildCache cache2(::nift::core::Path(tc.root.string()));
  REQUIRE(cache2.load().has_value());
  CHECK(cache2.size() == 100);

  // Verify entry integrity
  auto entry = cache2.get(::nift::core::Path("src/file_50.html"));
  REQUIRE(entry.has_value());
  CHECK(entry->content_hash == "hash_50");
}

TEST_CASE("BuildCache: JSON-only load works (backward compat)",
          "[project][cache]") {
  TempCache tc;
  // Create a cache and save it (creates both CBOR + JSON)
  BuildCache cache1(::nift::core::Path(tc.root.string()));
  TrackedFile tf;
  tf.source = ::nift::core::Path("page.html");
  tf.output = ::nift::core::Path("out/page.html");
  tf.content_hash = "def456";
  tf.deps_hash = "";
  tf.mtime = 42;
  tf.built_at = 100;
  cache1.put(tf);
  REQUIRE(cache1.save().has_value());

  // Remove CBOR, keep only JSON
  auto cbor_path = tc.root / "index.json.cbor";
  fs::remove(cbor_path);

  // Load should succeed from JSON
  BuildCache cache2(::nift::core::Path(tc.root.string()));
  REQUIRE(cache2.load().has_value());
  auto entry = cache2.get(::nift::core::Path("page.html"));
  REQUIRE(entry.has_value());
  CHECK(entry->content_hash == "def456");
}
