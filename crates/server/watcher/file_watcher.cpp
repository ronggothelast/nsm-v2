/// @file file_watcher.cpp

#include "nift/server/file_watcher.hpp"

#include <chrono>
#include <filesystem>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>

namespace nift::server {

namespace fs = std::filesystem;

struct FileWatcher::Impl {
  WatcherConfig config;
  WatchCallback callback;
  std::thread thread;
  std::atomic<bool> running{false};
  std::unordered_map<std::string, std::int64_t> mtimes;
  std::mutex mu;

  bool is_ignored(const std::string& path) const {
    for (const auto& s : config.ignored_substrings) {
      if (path.find(s) != std::string::npos) return true;
    }
    return false;
  }

  std::vector<ChangeEvent> scan() {
    std::vector<ChangeEvent> events;
    std::unordered_map<std::string, std::int64_t> seen;

    std::error_code ec;
    fs::recursive_directory_iterator it(config.root.str(), ec);
    if (ec) return events;
    fs::recursive_directory_iterator end;
    for (; it != end; it.increment(ec)) {
      if (ec) {
        ec.clear();
        continue;
      }
      if (!it->is_regular_file(ec)) continue;
      std::string p = it->path().string();
      if (is_ignored(p)) continue;
      auto t = fs::last_write_time(it->path(), ec);
      if (ec) {
        ec.clear();
        continue;
      }
      auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                      t.time_since_epoch())
                      .count();
      seen[p] = static_cast<std::int64_t>(secs);
    }

    {
      std::lock_guard<std::mutex> lk(mu);
      // Detect added + modified.
      for (const auto& [k, v] : seen) {
        auto prev = mtimes.find(k);
        if (prev == mtimes.end()) {
          events.push_back({ChangeKind::Added, ::nift::core::Path(k)});
        } else if (prev->second != v) {
          events.push_back({ChangeKind::Modified, ::nift::core::Path(k)});
        }
      }
      // Detect removed.
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

FileWatcher::FileWatcher(WatcherConfig config)
    : impl_(std::make_unique<Impl>()) {
  impl_->config = std::move(config);
}

FileWatcher::~FileWatcher() { stop(); }

::nift::Expected<std::monostate, ::nift::Error> FileWatcher::start(
    WatchCallback cb) {
  if (impl_->running.load()) return std::monostate{};
  if (!cb) return ::nift::unexpected<::nift::Error>(::nift::Error::invalid_argument);
  impl_->callback = std::move(cb);

  // Initial scan to build baseline (no events fired).
  (void)impl_->scan();

  impl_->running.store(true);
  impl_->thread = std::thread([this]() {
    while (impl_->running.load()) {
      auto events = impl_->scan();
      if (!events.empty() && impl_->callback) {
        try {
          impl_->callback(events);
        } catch (...) {
          // Swallow callback exceptions — watcher is best-effort.
        }
      }
      std::this_thread::sleep_for(impl_->config.poll_interval);
    }
  });
  return std::monostate{};
}

void FileWatcher::stop() {
  if (!impl_->running.exchange(false)) return;
  if (impl_->thread.joinable()) impl_->thread.join();
}

bool FileWatcher::is_running() const noexcept { return impl_->running.load(); }

void FileWatcher::poll_once() {
  auto events = impl_->scan();
  if (!events.empty() && impl_->callback) impl_->callback(events);
}

}  // namespace nift::server
