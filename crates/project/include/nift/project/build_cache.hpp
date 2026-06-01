/// @file build_cache.hpp
/// @brief Incremental build cache — BLAKE3 content hash + JSON persist.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "nift/core/path.hpp"
#include "nift/core/types.hpp"
#include "nift/project/project.hpp"

namespace nift::project {

/// @brief BLAKE3-based content hash. 64 hex chars.
std::string hash_content(std::string_view content);

/// @brief Hash a file by reading its content. File-not-found returns Error.
::nift::Expected<std::string, ::nift::Error> hash_file(const ::nift::core::Path& path);

/// @brief Aggregate hash from multiple inputs (deterministic order).
std::string hash_combined(const std::vector<std::string>& hashes);

/// @brief Persistent build cache — survives across invocations.
class BuildCache {
 public:
  /// Construct a cache rooted at `cache_dir`. Will create dir if missing.
  explicit BuildCache(::nift::core::Path cache_dir);

  /// Look up tracked file metadata by source path. nullopt if missing.
  std::optional<TrackedFile> get(const ::nift::core::Path& source) const;

  /// Insert or update a tracked file entry.
  void put(TrackedFile entry);

  /// Remove a tracked entry.
  void remove(const ::nift::core::Path& source);

  /// Has the source content hash changed since last build?
  /// Returns true on cache miss too (so caller will rebuild).
  bool is_dirty(const ::nift::core::Path& source, std::string_view current_hash) const;

  /// Persist the cache to disk (atomic write via temp + rename).
  ::nift::Expected<std::monostate, ::nift::Error> save() const;

  /// Reload the cache from disk. Empty cache if file missing.
  ::nift::Expected<std::monostate, ::nift::Error> load();

  /// Clear all entries (does not persist until save() is called).
  void clear();

  /// Number of tracked entries.
  std::size_t size() const noexcept { return entries_.size(); }

  auto begin() const noexcept { return entries_.cbegin(); }
  auto end() const noexcept { return entries_.cend(); }

 private:
  ::nift::core::Path cache_dir_;
  ::nift::core::Path index_path_;
  std::unordered_map<std::string, TrackedFile> entries_;
};

}  // namespace nift::project
