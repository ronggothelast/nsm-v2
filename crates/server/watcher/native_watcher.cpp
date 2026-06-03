/// @file native_watcher.cpp
/// @brief Platform-native filesystem watcher implementation.
///
/// Linux: inotify with recursive directory monitoring.
/// Others: delegates to polling FileWatcher.

#include "nift/server/native_watcher.hpp"

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

// ─── Platform detection ────────────────────────────────────────────────────

#if defined(__linux__)
#define NIFT_USE_INOTIFY 1
#elif defined(__APPLE__)
#define NIFT_USE_KQUEUE 1
#elif defined(_WIN32)
#define NIFT_USE_WIN32 1
#else
#define NIFT_USE_POLLING 1
#endif

// ─── Linux inotify ─────────────────────────────────────────────────────────

#if NIFT_USE_INOTIFY

#include <sys/inotify.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace nift::server {

WatchBackend detect_best_backend() {
  return WatchBackend::Inotify;
}

const char* backend_name(WatchBackend b) {
  switch (b) {
    case WatchBackend::Inotify:
      return "inotify";
    case WatchBackend::Kqueue:
      return "kqueue";
    case WatchBackend::Win32:
      return "ReadDirectoryChangesW";
    case WatchBackend::Polling:
      return "polling";
  }
  return "unknown";
}

namespace fs = std::filesystem;

struct NativeFileWatcher::Impl {
  WatcherConfig config;
  WatchCallback callback;
  std::thread thread;
  std::atomic<bool> running{false};
  WatchBackend active_backend{WatchBackend::Inotify};

  // inotify state
  int inotify_fd{-1};
  std::unordered_map<int, std::string> wd_to_path;
  std::unordered_map<std::string, int> path_to_wd;
  std::mutex mu;

  bool is_ignored(const std::string& path) const {
    for (const auto& s : config.ignored_substrings) {
      if (path.find(s) != std::string::npos)
        return true;
    }
    return false;
  }

  /// Add inotify watch for a directory and all subdirectories.
  void add_watch_recursive(const fs::path& dir) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec))
      return;

    std::string dir_str = dir.string();
    if (is_ignored(dir_str))
      return;

    add_watch(dir_str);

    for (auto& entry : fs::directory_iterator(dir, ec)) {
      if (ec) {
        ec.clear();
        continue;
      }
      if (entry.is_directory(ec)) {
        add_watch_recursive(entry.path());
      }
    }
  }

  /// Add a single inotify watch.
  void add_watch(const std::string& path) {
    if (path_to_wd.count(path))
      return;

    constexpr uint32_t mask = IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM |
                              IN_MOVED_TO | IN_DELETE_SELF | IN_ATTRIB;

    int wd = inotify_add_watch(inotify_fd, path.c_str(), mask);
    if (wd < 0)
      return;

    wd_to_path[wd] = path;
    path_to_wd[path] = wd;
  }

  /// Remove a watch by path.
  void remove_watch(const std::string& path) {
    auto it = path_to_wd.find(path);
    if (it == path_to_wd.end())
      return;
    inotify_rm_watch(inotify_fd, it->second);
    wd_to_path.erase(it->second);
    path_to_wd.erase(it);
  }

  /// Process inotify events into ChangeEvents.
  std::vector<ChangeEvent> process_events(const char* buf, ssize_t len) {
    std::vector<ChangeEvent> events;
    // Track directories we need to add/remove watches for.
    std::vector<std::string> new_dirs;
    std::vector<std::string> removed_dirs;

    const char* ptr = buf;
    while (ptr < buf + len) {
      auto* event = reinterpret_cast<const inotify_event*>(ptr);

      std::string dir;
      {
        std::lock_guard<std::mutex> lk(mu);
        auto it = wd_to_path.find(event->wd);
        if (it == wd_to_path.end()) {
          ptr += sizeof(inotify_event) + event->len;
          continue;
        }
        dir = it->second;
      }

      // Skip events with no filename (watch directory itself).
      if (event->len == 0 || event->name[0] == '\0') {
        // Directory itself was deleted.
        if (event->mask & IN_DELETE_SELF) {
          std::lock_guard<std::mutex> lk(mu);
          removed_dirs.push_back(dir);
        }
        ptr += sizeof(inotify_event) + event->len;
        continue;
      }

      std::string name(event->name);
      // Skip ignored names.
      if (is_ignored(name)) {
        ptr += sizeof(inotify_event) + event->len;
        continue;
      }

      std::string full_path = dir + "/" + name;
      bool is_dir = (event->mask & IN_ISDIR) != 0;

      if (event->mask & (IN_CREATE | IN_MOVED_TO)) {
        events.push_back({ChangeKind::Added, ::nift::core::Path(full_path)});
        if (is_dir) {
          new_dirs.push_back(full_path);
        }
      } else if (event->mask & (IN_MODIFY | IN_ATTRIB)) {
        if (!is_dir) {
          events.push_back({ChangeKind::Modified, ::nift::core::Path(full_path)});
        }
      } else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
        events.push_back({ChangeKind::Removed, ::nift::core::Path(full_path)});
        if (is_dir) {
          std::lock_guard<std::mutex> lk(mu);
          removed_dirs.push_back(full_path);
        }
      }

      ptr += sizeof(inotify_event) + event->len;
    }

    // Apply directory watch changes.
    for (auto& d : new_dirs) {
      add_watch_recursive(fs::path(d));
    }
    for (auto& d : removed_dirs) {
      remove_watch(d);
    }

    return events;
  }

  void run() {
    // 64KB event buffer.
    constexpr size_t buf_size = 64 * 1024;
    char buf[buf_size] __attribute__((aligned(__alignof__(struct inotify_event))));

    while (running.load()) {
      // Use select() with timeout so we can check running flag.
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(inotify_fd, &fds);

      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 200000;  // 200ms timeout

      int ret = select(inotify_fd + 1, &fds, nullptr, nullptr, &tv);
      if (ret < 0) {
        if (errno == EINTR)
          continue;
        break;
      }
      if (ret == 0)
        continue;  // timeout, loop to check running

      ssize_t len = read(inotify_fd, buf, buf_size);
      if (len <= 0) {
        if (errno == EINTR)
          continue;
        break;
      }

      auto events = process_events(buf, len);
      if (!events.empty() && callback) {
        try {
          callback(events);
        } catch (...) {
          // Swallow callback exceptions.
        }
      }
    }
  }
};

NativeFileWatcher::NativeFileWatcher(WatcherConfig config)
    : impl_(std::make_unique<Impl>()) {
  impl_->config = std::move(config);
}

NativeFileWatcher::~NativeFileWatcher() {
  stop();
}

::nift::Expected<std::monostate, ::nift::Error> NativeFileWatcher::start(
    WatchCallback cb) {
  if (impl_->running.load())
    return std::monostate{};
  if (!cb)
    return ::nift::unexpected<::nift::Error>(::nift::Error::invalid_argument);

  impl_->callback = std::move(cb);

  impl_->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (impl_->inotify_fd < 0)
    return ::nift::unexpected<::nift::Error>(::nift::Error::io_error);

  // Recursively watch all directories.
  impl_->add_watch_recursive(fs::path(impl_->config.root.str()));

  impl_->running.store(true);
  impl_->thread = std::thread([this]() { impl_->run(); });

  return std::monostate{};
}

void NativeFileWatcher::stop() {
  if (!impl_->running.exchange(false))
    return;
  if (impl_->thread.joinable())
    impl_->thread.join();

  if (impl_->inotify_fd >= 0) {
    close(impl_->inotify_fd);
    impl_->inotify_fd = -1;
  }
  impl_->wd_to_path.clear();
  impl_->path_to_wd.clear();
}

bool NativeFileWatcher::is_running() const noexcept {
  return impl_->running.load();
}

WatchBackend NativeFileWatcher::backend() const noexcept {
  return impl_->active_backend;
}

}  // namespace nift::server

// ─── Fallback (polling) ────────────────────────────────────────────────────

#else

#include <chrono>
#include <filesystem>
#include <unordered_map>

namespace nift::server {

WatchBackend detect_best_backend() {
  return WatchBackend::Polling;
}

const char* backend_name(WatchBackend b) {
  switch (b) {
    case WatchBackend::Inotify:
      return "inotify";
    case WatchBackend::Kqueue:
      return "kqueue";
    case WatchBackend::Win32:
      return "ReadDirectoryChangesW";
    case WatchBackend::Polling:
      return "polling";
  }
  return "unknown";
}

namespace fs = std::filesystem;

struct NativeFileWatcher::Impl {
  WatcherConfig config;
  WatchCallback callback;
  std::thread thread;
  std::atomic<bool> running{false};
  std::unordered_map<std::string, std::int64_t> mtimes;
  std::mutex mu;
  WatchBackend active_backend{WatchBackend::Polling};

  bool is_ignored(const std::string& path) const {
    for (const auto& s : config.ignored_substrings) {
      if (path.find(s) != std::string::npos)
        return true;
    }
    return false;
  }

  std::vector<ChangeEvent> scan() {
    std::vector<ChangeEvent> events;
    std::unordered_map<std::string, std::int64_t> seen;

    std::error_code ec;
    fs::recursive_directory_iterator it(config.root.str(), ec);
    if (ec)
      return events;
    fs::recursive_directory_iterator end;
    for (; it != end; it.increment(ec)) {
      if (ec) {
        ec.clear();
        continue;
      }
      if (!it->is_regular_file(ec))
        continue;
      std::string p = it->path().string();
      if (is_ignored(p))
        continue;
      auto t = fs::last_write_time(it->path(), ec);
      if (ec) {
        ec.clear();
        continue;
      }
      auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch())
                      .count();
      seen[p] = static_cast<std::int64_t>(ns);
    }

    {
      std::lock_guard<std::mutex> lk(mu);
      for (const auto& [k, v] : seen) {
        auto prev = mtimes.find(k);
        if (prev == mtimes.end()) {
          events.push_back({ChangeKind::Added, ::nift::core::Path(k)});
        } else if (prev->second != v) {
          events.push_back({ChangeKind::Modified, ::nift::core::Path(k)});
        }
      }
      for (const auto& [k, v] : mtimes) {
        if (seen.find(k) == seen.end()) {
          events.push_back({ChangeKind::Removed, ::nift::core::Path(k)});
        }
      }
      mtimes = std::move(seen);
    }
    return events;
  }
};

NativeFileWatcher::NativeFileWatcher(WatcherConfig config)
    : impl_(std::make_unique<Impl>()) {
  impl_->config = std::move(config);
}

NativeFileWatcher::~NativeFileWatcher() {
  stop();
}

::nift::Expected<std::monostate, ::nift::Error> NativeFileWatcher::start(
    WatchCallback cb) {
  if (impl_->running.load())
    return std::monostate{};
  if (!cb)
    return ::nift::unexpected<::nift::Error>(::nift::Error::invalid_argument);

  impl_->callback = std::move(cb);
  (void)impl_->scan();  // baseline

  impl_->running.store(true);
  impl_->thread = std::thread([this]() {
    while (impl_->running.load()) {
      auto events = impl_->scan();
      if (!events.empty() && impl_->callback) {
        try {
          impl_->callback(events);
        } catch (...) {}
      }
      std::this_thread::sleep_for(impl_->config.poll_interval);
    }
  });
  return std::monostate{};
}

void NativeFileWatcher::stop() {
  if (!impl_->running.exchange(false))
    return;
  if (impl_->thread.joinable())
    impl_->thread.join();
}

bool NativeFileWatcher::is_running() const noexcept {
  return impl_->running.load();
}

WatchBackend NativeFileWatcher::backend() const noexcept {
  return impl_->active_backend;
}

}  // namespace nift::server

#endif
