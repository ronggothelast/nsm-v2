/// @file file_watcher.hpp
/// @brief Polling-based file watcher.
///
/// Watches a directory recursively. Polls mtimes every interval and fires
/// a callback when changes are detected. Polling is portable and dead
/// simple — Phase 6+ may swap in inotify (Linux) / FSEvents (macOS) /
/// ReadDirectoryChangesW (Windows) for sub-millisecond latency.

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
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

using WatchCallback =
    std::function<void(const std::vector<ChangeEvent>& events)>;

struct WatcherConfig {
  ::nift::core::Path root;
  std::chrono::milliseconds poll_interval{500};
  std::vector<std::string> ignored_substrings{".git", "node_modules",
                                              "build", ".nift"};
};

class FileWatcher {
 public:
  explicit FileWatcher(WatcherConfig config);
  ~FileWatcher();

  FileWatcher(const FileWatcher&) = delete;
  FileWatcher& operator=(const FileWatcher&) = delete;

  /// Start watching. Spawns a background thread.
  ::nift::Expected<std::monostate, ::nift::Error> start(WatchCallback cb);

  /// Stop watching. Idempotent.
  void stop();

  bool is_running() const noexcept;

  /// Force a one-shot scan synchronously (for tests).
  void poll_once();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace nift::server
