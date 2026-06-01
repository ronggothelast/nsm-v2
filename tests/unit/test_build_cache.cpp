/// @file test_build_cache.cpp

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "nift/project/build_cache.hpp"

using namespace nift::project;
namespace fs = std::filesystem;

namespace {
::nift::core::Path tmp_cache_dir(const std::string& tag) {
  auto p = fs::temp_directory_path() /
           ("nift_test_cache_" + tag + "_" + std::to_string(std::rand()));
  fs::remove_all(p);
  return ::nift::core::Path(p.string());
}
}  // namespace

TEST_CASE("hash_content: deterministic", "[project][cache][hash]") {
  auto a = hash_content("hello world");
  auto b = hash_content("hello world");
  CHECK(a == b);
  CHECK(a.size() == 64);  // 32 bytes hex-encoded
}

TEST_CASE("hash_content: different input -> different hash", "[project][cache][hash]") {
  auto a = hash_content("hello");
  auto b = hash_content("world");
  CHECK(a != b);
}

TEST_CASE("hash_content: empty string", "[project][cache][hash]") {
  auto h = hash_content("");
  CHECK(h.size() == 64);
}

TEST_CASE("hash_combined: order-independent", "[project][cache][hash]") {
  auto a = hash_combined({"alpha", "beta", "gamma"});
  auto b = hash_combined({"gamma", "alpha", "beta"});
  CHECK(a == b);
}

TEST_CASE("BuildCache: empty cache size 0", "[project][cache]") {
  auto dir = tmp_cache_dir("empty");
  BuildCache cache(dir);
  CHECK(cache.size() == 0);
}

TEST_CASE("BuildCache: put/get round-trip", "[project][cache]") {
  auto dir = tmp_cache_dir("putget");
  BuildCache cache(dir);

  TrackedFile tf;
  tf.source = ::nift::core::Path("content/index.md");
  tf.output = ::nift::core::Path("public/index.html");
  tf.content_hash = "deadbeef";
  tf.deps_hash = "cafebabe";
  tf.mtime = 1000;
  tf.built_at = 2000;
  cache.put(tf);

  auto got = cache.get(::nift::core::Path("content/index.md"));
  REQUIRE(got);
  CHECK(got->content_hash == "deadbeef");
  CHECK(got->mtime == 1000);
}

TEST_CASE("BuildCache: get missing returns nullopt", "[project][cache]") {
  auto dir = tmp_cache_dir("missing");
  BuildCache cache(dir);
  auto got = cache.get(::nift::core::Path("not/there"));
  CHECK_FALSE(got.has_value());
}

TEST_CASE("BuildCache: is_dirty true on missing", "[project][cache]") {
  auto dir = tmp_cache_dir("dirty1");
  BuildCache cache(dir);
  CHECK(cache.is_dirty(::nift::core::Path("x"), "abc"));
}

TEST_CASE("BuildCache: is_dirty false on match", "[project][cache]") {
  auto dir = tmp_cache_dir("dirty2");
  BuildCache cache(dir);
  TrackedFile tf;
  tf.source = ::nift::core::Path("a.md");
  tf.content_hash = "samehash";
  cache.put(tf);
  CHECK_FALSE(cache.is_dirty(::nift::core::Path("a.md"), "samehash"));
}

TEST_CASE("BuildCache: is_dirty true on hash mismatch", "[project][cache]") {
  auto dir = tmp_cache_dir("dirty3");
  BuildCache cache(dir);
  TrackedFile tf;
  tf.source = ::nift::core::Path("a.md");
  tf.content_hash = "old";
  cache.put(tf);
  CHECK(cache.is_dirty(::nift::core::Path("a.md"), "new"));
}

TEST_CASE("BuildCache: remove deletes entry", "[project][cache]") {
  auto dir = tmp_cache_dir("remove");
  BuildCache cache(dir);
  TrackedFile tf;
  tf.source = ::nift::core::Path("a.md");
  cache.put(tf);
  CHECK(cache.size() == 1);
  cache.remove(::nift::core::Path("a.md"));
  CHECK(cache.size() == 0);
}

TEST_CASE("BuildCache: clear empties everything", "[project][cache]") {
  auto dir = tmp_cache_dir("clear");
  BuildCache cache(dir);
  for (int i = 0; i < 5; ++i) {
    TrackedFile tf;
    tf.source = ::nift::core::Path("file" + std::to_string(i));
    cache.put(tf);
  }
  CHECK(cache.size() == 5);
  cache.clear();
  CHECK(cache.size() == 0);
}

TEST_CASE("BuildCache: save and load round-trip", "[project][cache]") {
  auto dir = tmp_cache_dir("persist");
  {
    BuildCache cache(dir);
    TrackedFile tf;
    tf.source = ::nift::core::Path("post1.md");
    tf.output = ::nift::core::Path("post1.html");
    tf.content_hash = "hash1";
    tf.deps_hash = "deps1";
    tf.mtime = 12345;
    cache.put(tf);
    auto status = cache.save();
    REQUIRE(status);
  }
  // New instance loads from disk.
  BuildCache cache2(dir);
  auto loaded = cache2.load();
  REQUIRE(loaded);
  CHECK(cache2.size() == 1);
  auto got = cache2.get(::nift::core::Path("post1.md"));
  REQUIRE(got);
  CHECK(got->content_hash == "hash1");
  CHECK(got->mtime == 12345);
}

TEST_CASE("BuildCache: load missing index -> empty cache", "[project][cache]") {
  auto dir = tmp_cache_dir("noindex");
  BuildCache cache(dir);
  auto status = cache.load();
  REQUIRE(status);
  CHECK(cache.size() == 0);
}

TEST_CASE("BuildCache: iteration", "[project][cache]") {
  auto dir = tmp_cache_dir("iter");
  BuildCache cache(dir);
  for (int i = 0; i < 3; ++i) {
    TrackedFile tf;
    tf.source = ::nift::core::Path("file" + std::to_string(i));
    tf.content_hash = "h" + std::to_string(i);
    cache.put(tf);
  }
  int count = 0;
  for (const auto& [key, entry] : cache) {
    (void)key;
    (void)entry;
    ++count;
  }
  CHECK(count == 3);
}
