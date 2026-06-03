/// @file native_watcher.hpp
/// @brief Platform-native filesystem watcher.
///
/// On Linux uses inotify, on macOS uses kqueue, on Windows uses
/// ReadDirectoryChangesW. Falls back to polling on unsupported platforms.

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

using WatchCallback = std::function<void(const std::vector<ChangeEvent>& events)>;

struct WatcherConfig {
  ::nift::core::Path root;
  std::chrono::milliseconds poll_interval{500};
  std::vector<std::string> ignored_substrings{".git", "node_modules", "build", ".nift"};
};

/// Platform detection.
enum class WatchBackend : std::uint8_t {
  Inotify,   // Linux
  Kqueue,    // macOS / BSD
  Win32,     // Windows
  Polling,   // Fallback
};

/// Returns the best available backend for this platform.
WatchBackend detect_best_backend();

/// Returns human-readable backend name.
const char* backend_name(WatchBackend b);

/// Native filesystem watcher. Dispatches to platform-specific implementation.
class NativeFileWatcher {
 public:
  explicit NativeFileWatcher(WatcherConfig config);
  ~NativeFileWatcher();

  NativeFileWatcher(const NativeFileWatcher&) = delete;
  NativeFileWatcher& operator=(const NativeFileWatcher&) = delete;

  /// Start watching. Spawns a background thread.
  ::nift::Expected<std::monostate, ::nift::Error> start(WatchCallback cb);

  /// Stop watching. Idempotent.
  void stop();

  bool is_running() const noexcept;

  /// Which backend is active.
  WatchBackend backend() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace nift::server
