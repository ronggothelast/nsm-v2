/// @file project.hpp
/// @brief ProjectInfo — immutable site configuration + tracked file state.

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "nift/core/path.hpp"
#include "nift/core/types.hpp"

namespace nift::project {

/// @brief Static project configuration. Immutable after load.
struct ProjectConfig {
  std::string name = "nift-site";
  std::string version = "0.0.1";
  ::nift::core::Path content_dir = ::nift::core::Path("content");
  ::nift::core::Path output_dir = ::nift::core::Path("public");
  ::nift::core::Path template_dir = ::nift::core::Path("templates");
  ::nift::core::Path cache_dir = ::nift::core::Path(".nift/cache");
  ::nift::core::Path tracked_path = ::nift::core::Path(".nift/tracked.json");
  std::string default_template = "default.template";
  std::vector<std::string> ignored_paths{".git", "node_modules", ".nift"};

  /// Load from a JSON file. Falls back to defaults on missing file.
  static ::nift::Expected<ProjectConfig, ::nift::Error> load(
      const ::nift::core::Path& config_path);

  /// Serialize to JSON (canonical form).
  ::nift::Expected<std::string, ::nift::Error> to_json() const;

  /// Parse from JSON string.
  static ::nift::Expected<ProjectConfig, ::nift::Error> from_json(
      std::string_view json);
};

/// @brief Tracked file metadata — used for incremental builds.
struct TrackedFile {
  ::nift::core::Path source;  // input path (relative to content_dir)
  ::nift::core::Path output;  // output path (relative to output_dir)
  std::string content_hash;   // BLAKE3 of source content
  std::string deps_hash;      // BLAKE3 of all transitive deps
  std::int64_t mtime = 0;     // last modification (epoch seconds)
  std::int64_t built_at = 0;  // last successful build (epoch seconds)
};

}  // namespace nift::project
