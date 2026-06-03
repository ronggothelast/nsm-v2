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
#include "nift/server/watcher_types.hpp"

namespace nift::server {

/// Platform detection.
enum class WatchBackend : std::uint8_t {
  Inotify,  // Linux
  Kqueue,   // macOS / BSD
  Win32,    // Windows
  Polling,  // Fallback
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
