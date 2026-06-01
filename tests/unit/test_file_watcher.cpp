/// @file test_file_watcher.cpp

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include "nift/server/file_watcher.hpp"

using namespace nift::server;
namespace fs = std::filesystem;

namespace {

struct TempDir {
  fs::path root;
  TempDir() {
    root = fs::temp_directory_path() / ("nift_watcher_" + std::to_string(std::rand()));
    fs::create_directories(root);
  }
  ~TempDir() {
    std::error_code ec;
    fs::remove_all(root, ec);
  }
  fs::path write(const std::string& name, const std::string& body) {
    auto p = root / name;
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out.write(body.data(), static_cast<std::streamsize>(body.size()));
    return p;
  }
};

}  // namespace

TEST_CASE("FileWatcher: detects added file", "[server][watcher]") {
  TempDir d;
  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(d.root.string());

  std::atomic<int> events{0};
  std::atomic<int> added{0};
  FileWatcher w(cfg);
  REQUIRE(w.start([&](const std::vector<ChangeEvent>& evs) {
    for (const auto& e : evs) {
      events.fetch_add(1);
      if (e.kind == ChangeKind::Added)
        added.fetch_add(1);
    }
  }));

  // Wait for baseline scan.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  d.write("a.txt", "hello");
  w.poll_once();

  CHECK(added.load() >= 1);
  w.stop();
}

TEST_CASE("FileWatcher: detects modified file", "[server][watcher]") {
  TempDir d;
  d.write("a.txt", "v1");

  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(d.root.string());

  std::atomic<int> modified{0};
  FileWatcher w(cfg);
  REQUIRE(w.start([&](const std::vector<ChangeEvent>& evs) {
    for (const auto& e : evs) {
      if (e.kind == ChangeKind::Modified)
        modified.fetch_add(1);
    }
  }));

  // Touch with different mtime — sleep ensures filesystem timestamp changes.
  std::this_thread::sleep_for(std::chrono::seconds(1));
  d.write("a.txt", "v2 different content");
  w.poll_once();

  CHECK(modified.load() >= 1);
  w.stop();
}

TEST_CASE("FileWatcher: detects removed file", "[server][watcher]") {
  TempDir d;
  auto p = d.write("doomed.txt", "x");

  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(d.root.string());

  std::atomic<int> removed{0};
  FileWatcher w(cfg);
  REQUIRE(w.start([&](const std::vector<ChangeEvent>& evs) {
    for (const auto& e : evs) {
      if (e.kind == ChangeKind::Removed)
        removed.fetch_add(1);
    }
  }));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  fs::remove(p);
  w.poll_once();
  CHECK(removed.load() >= 1);
  w.stop();
}

TEST_CASE("FileWatcher: ignores .git", "[server][watcher]") {
  TempDir d;
  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(d.root.string());

  std::atomic<int> events{0};
  FileWatcher w(cfg);
  REQUIRE(w.start([&](const std::vector<ChangeEvent>& evs) {
    events.fetch_add(static_cast<int>(evs.size()));
  }));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  d.write(".git/HEAD", "ref: refs/heads/main");
  w.poll_once();
  CHECK(events.load() == 0);
  w.stop();
}

TEST_CASE("FileWatcher: stop is idempotent", "[server][watcher]") {
  TempDir d;
  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(d.root.string());
  FileWatcher w(cfg);
  REQUIRE(w.start([](const std::vector<ChangeEvent>&) {}));
  w.stop();
  w.stop();
  SUCCEED("no hang");
}

TEST_CASE("FileWatcher: rejects null callback", "[server][watcher]") {
  TempDir d;
  WatcherConfig cfg;
  cfg.root = ::nift::core::Path(d.root.string());
  FileWatcher w(cfg);
  auto r = w.start({});
  CHECK_FALSE(r);
}
