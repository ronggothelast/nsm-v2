/// @file test_native_watcher.cpp

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "nift/server/native_watcher.hpp"

using namespace nift::server;
namespace fs = std::filesystem;

// Helper: create a temp directory with some files.
static fs::path make_temp_dir(const std::string& prefix) {
  auto dir = fs::temp_directory_path() / (prefix + std::to_string(getpid()));
  fs::create_directories(dir);
  return dir;
}

static void write_file(const fs::path& p, const std::string& content) {
  std::ofstream ofs(p);
  ofs << content;
}

TEST_CASE("NativeFileWatcher: detect_best_backend returns valid backend",
          "[server][native_watcher]") {
  auto b = detect_best_backend();
  // On Linux should be Inotify, on others Polling (kqueue/Win32 deferred).
#if defined(__linux__)
  CHECK(b == WatchBackend::Inotify);
#else
  CHECK(b == WatchBackend::Polling);
#endif
}

TEST_CASE("NativeFileWatcher: backend_name returns non-empty",
          "[server][native_watcher]") {
  for (auto b : {WatchBackend::Inotify, WatchBackend::Kqueue, WatchBackend::Win32,
                 WatchBackend::Polling}) {
    CHECK(std::string(backend_name(b)).size() > 0);
  }
}

TEST_CASE("NativeFileWatcher: start and stop", "[server][native_watcher]") {
  auto dir = make_temp_dir("nift_native_test_");
  write_file(dir / "a.html", "<h1>Hello</h1>");

  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(dir.string());
  cfg.poll_interval = std::chrono::milliseconds(100);

  NativeFileWatcher watcher(std::move(cfg));
  CHECK_FALSE(watcher.is_running());

  auto result = watcher.start([](const std::vector<ChangeEvent>&) {});
  CHECK(result.has_value());
  CHECK(watcher.is_running());

  watcher.stop();
  CHECK_FALSE(watcher.is_running());

  fs::remove_all(dir);
}

TEST_CASE("NativeFileWatcher: detects file addition", "[server][native_watcher]") {
  auto dir = make_temp_dir("nift_native_add_");
  write_file(dir / "existing.html", "existing");

  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(dir.string());
  cfg.poll_interval = std::chrono::milliseconds(100);

  std::vector<ChangeEvent> received;
  std::mutex mu;

  NativeFileWatcher watcher(std::move(cfg));
  watcher.start([&](const std::vector<ChangeEvent>& events) {
    std::lock_guard<std::mutex> lk(mu);
    received.insert(received.end(), events.begin(), events.end());
  });

  // Wait a moment for initial scan.
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Add a new file.
  write_file(dir / "new.html", "new content");

  // Wait for detection.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  watcher.stop();

  bool found_add = false;
  {
    std::lock_guard<std::mutex> lk(mu);
    for (const auto& e : received) {
      if (e.kind == ChangeKind::Added &&
          e.path.str().find("new.html") != std::string::npos) {
        found_add = true;
      }
    }
  }
  CHECK(found_add);

  fs::remove_all(dir);
}

TEST_CASE("NativeFileWatcher: detects file modification", "[server][native_watcher]") {
  auto dir = make_temp_dir("nift_native_mod_");
  auto file = dir / "mod.html";
  write_file(file, "original");

  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(dir.string());
  cfg.poll_interval = std::chrono::milliseconds(100);

  std::vector<ChangeEvent> received;
  std::mutex mu;

  NativeFileWatcher watcher(std::move(cfg));
  watcher.start([&](const std::vector<ChangeEvent>& events) {
    std::lock_guard<std::mutex> lk(mu);
    received.insert(received.end(), events.begin(), events.end());
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Modify the file.
  write_file(file, "modified content");

  // Wait longer for macOS CI (polling fallback is slower).
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  watcher.stop();

  bool found_mod = false;
  {
    std::lock_guard<std::mutex> lk(mu);
    for (const auto& e : received) {
      if (e.kind == ChangeKind::Modified &&
          e.path.str().find("mod.html") != std::string::npos) {
        found_mod = true;
      }
    }
  }
  CHECK(found_mod);

  fs::remove_all(dir);
}

TEST_CASE("NativeFileWatcher: detects file removal", "[server][native_watcher]") {
  auto dir = make_temp_dir("nift_native_rm_");
  auto file = dir / "rm.html";
  write_file(file, "to be removed");

  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(dir.string());
  cfg.poll_interval = std::chrono::milliseconds(100);

  std::vector<ChangeEvent> received;
  std::mutex mu;

  NativeFileWatcher watcher(std::move(cfg));
  watcher.start([&](const std::vector<ChangeEvent>& events) {
    std::lock_guard<std::mutex> lk(mu);
    received.insert(received.end(), events.begin(), events.end());
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Remove the file.
  fs::remove(file);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  watcher.stop();

  bool found_rm = false;
  {
    std::lock_guard<std::mutex> lk(mu);
    for (const auto& e : received) {
      if (e.kind == ChangeKind::Removed &&
          e.path.str().find("rm.html") != std::string::npos) {
        found_rm = true;
      }
    }
  }
  CHECK(found_rm);

  fs::remove_all(dir);
}

TEST_CASE("NativeFileWatcher: ignores configured substrings",
          "[server][native_watcher]") {
  auto dir = make_temp_dir("nift_native_ignore_");
  auto git_dir = dir / ".git";
  fs::create_directories(git_dir);

  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(dir.string());
  cfg.ignored_substrings = {".git"};

  std::vector<ChangeEvent> received;
  std::mutex mu;

  NativeFileWatcher watcher(std::move(cfg));
  watcher.start([&](const std::vector<ChangeEvent>& events) {
    std::lock_guard<std::mutex> lk(mu);
    received.insert(received.end(), events.begin(), events.end());
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Add file in .git dir.
  write_file(git_dir / "config", "git config");

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  watcher.stop();

  bool found_git = false;
  {
    std::lock_guard<std::mutex> lk(mu);
    for (const auto& e : received) {
      if (e.path.str().find(".git") != std::string::npos) {
        found_git = true;
      }
    }
  }
  CHECK_FALSE(found_git);

  fs::remove_all(dir);
}

TEST_CASE("NativeFileWatcher: backend reports correctly", "[server][native_watcher]") {
  auto dir = make_temp_dir("nift_native_backend_");

  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(dir.string());

  NativeFileWatcher watcher(std::move(cfg));

#if defined(__linux__)
  CHECK(watcher.backend() == WatchBackend::Inotify);
#else
  CHECK(watcher.backend() == WatchBackend::Polling);
#endif

  fs::remove_all(dir);
}
