/// @file build_cache.cpp
/// @brief BuildCache implementation — BLAKE3 hashing + JSON persistence.

#include "nift/project/build_cache.hpp"

#include <blake3.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstdio>

#include "nift/core/filesystem.hpp"

namespace nift::project {

using json = nlohmann::json;

namespace {

constexpr std::size_t kHashBytes = 32;  // BLAKE3 default 256-bit output

std::string bytes_to_hex(const std::array<std::uint8_t, kHashBytes>& bytes) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out;
  out.reserve(kHashBytes * 2);
  for (auto b : bytes) {
    out += kHex[(b >> 4) & 0x0F];
    out += kHex[b & 0x0F];
  }
  return out;
}

}  // namespace

std::string hash_content(std::string_view content) {
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  blake3_hasher_update(&hasher, content.data(), content.size());
  std::array<std::uint8_t, kHashBytes> out{};
  blake3_hasher_finalize(&hasher, out.data(), out.size());
  return bytes_to_hex(out);
}

::nift::Expected<std::string, ::nift::Error> hash_file(
    const ::nift::core::Path& path) {
  auto content = ::nift::core::read_file(path);
  if (!content) {
    return ::nift::unexpected<::nift::Error>(content.error());
  }
  return hash_content(*content);
}

std::string hash_combined(const std::vector<std::string>& hashes) {
  std::vector<std::string> sorted(hashes);
  std::sort(sorted.begin(), sorted.end());
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  for (const auto& h : sorted) {
    blake3_hasher_update(&hasher, h.data(), h.size());
    static constexpr char kSep = '\n';
    blake3_hasher_update(&hasher, &kSep, 1);
  }
  std::array<std::uint8_t, kHashBytes> out{};
  blake3_hasher_finalize(&hasher, out.data(), out.size());
  return bytes_to_hex(out);
}

// ─── BuildCache ──────────────────────────────────────────────────────

BuildCache::BuildCache(::nift::core::Path cache_dir)
    : cache_dir_(std::move(cache_dir)),
      index_path_(::nift::core::Path(cache_dir_.str() + "/index.json")) {
  // Create the cache directory if missing.
  ::nift::core::create_directories(cache_dir_);
}

std::optional<TrackedFile> BuildCache::get(
    const ::nift::core::Path& source) const {
  auto it = entries_.find(source.str());
  if (it == entries_.end()) return std::nullopt;
  return it->second;
}

void BuildCache::put(TrackedFile entry) {
  std::string key = entry.source.str();
  entries_[std::move(key)] = std::move(entry);
}

void BuildCache::remove(const ::nift::core::Path& source) {
  entries_.erase(source.str());
}

bool BuildCache::is_dirty(const ::nift::core::Path& source,
                          std::string_view current_hash) const {
  auto it = entries_.find(source.str());
  if (it == entries_.end()) return true;
  return it->second.content_hash != current_hash;
}

::nift::Expected<std::monostate, ::nift::Error> BuildCache::save() const {
  json j;
  j["version"] = 1;
  j["entries"] = json::array();
  for (const auto& [key, entry] : entries_) {
    json e;
    e["source"] = entry.source.str();
    e["output"] = entry.output.str();
    e["content_hash"] = entry.content_hash;
    e["deps_hash"] = entry.deps_hash;
    e["mtime"] = entry.mtime;
    e["built_at"] = entry.built_at;
    j["entries"].push_back(std::move(e));
  }

  // Atomic write: write to .tmp, then rename.
  ::nift::core::Path tmp_path(index_path_.str() + ".tmp");
  auto write_status = ::nift::core::write_file(tmp_path, j.dump(2));
  if (!write_status) {
    return ::nift::unexpected<::nift::Error>(write_status.error());
  }

  // Replace existing.
  std::error_code ec;
  std::filesystem::rename(std::filesystem::path(tmp_path.str()),
                          std::filesystem::path(index_path_.str()), ec);
  if (ec) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::io_error);
  }
  return std::monostate{};
}

::nift::Expected<std::monostate, ::nift::Error> BuildCache::load() {
  if (!::nift::core::file_exists(index_path_)) {
    entries_.clear();
    return std::monostate{};
  }
  auto content = ::nift::core::read_file(index_path_);
  if (!content) {
    return ::nift::unexpected<::nift::Error>(content.error());
  }
  try {
    auto parsed = json::parse(*content);
    entries_.clear();
    if (parsed.contains("entries") && parsed["entries"].is_array()) {
      for (const auto& e : parsed["entries"]) {
        TrackedFile tf;
        tf.source =
            ::nift::core::Path(e.value("source", std::string()));
        tf.output =
            ::nift::core::Path(e.value("output", std::string()));
        tf.content_hash = e.value("content_hash", std::string());
        tf.deps_hash = e.value("deps_hash", std::string());
        tf.mtime = e.value("mtime", std::int64_t{0});
        tf.built_at = e.value("built_at", std::int64_t{0});
        entries_[tf.source.str()] = std::move(tf);
      }
    }
  } catch (const std::exception&) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::parse_error);
  }
  return std::monostate{};
}

void BuildCache::clear() { entries_.clear(); }

}  // namespace nift::project
