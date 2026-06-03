/// @file watcher_types.hpp
/// @brief Shared types for file watchers (used by both FileWatcher and NativeFileWatcher).

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "nift/core/path.hpp"
#include "nift/core/types.hpp"

namespace nift::server {

enum class ChangeKind : std::uint8_t {
  Added,
  Modified,
  Removed,
};

struct ChangeEvent {
  ChangeKind kind;
  ::nift::core::Path path;
};

using WatchCallback = std::function<void(const std::vector<ChangeEvent>& events)>;

struct WatcherConfig {
  ::nift::core::Path root;
  std::chrono::milliseconds poll_interval{500};
  std::vector<std::string> ignored_substrings{".git", "node_modules", "build", ".nift"};
};

}  // namespace nift::server
