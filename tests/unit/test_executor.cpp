/// @file test_executor.cpp

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <vector>

#include "nift/build/executor.hpp"

using namespace nift::build;

TEST_CASE("WorkStealingPool: default size is hardware_concurrency",
          "[build][executor]") {
  WorkStealingPool pool;
  CHECK(pool.size() >= 1);
}

TEST_CASE("WorkStealingPool: explicit size", "[build][executor]") {
  WorkStealingPool pool(4);
  CHECK(pool.size() == 4);
}

TEST_CASE("WorkStealingPool: single task returns value", "[build][executor]") {
  WorkStealingPool pool(2);
  auto fut = pool.submit([] { return 42; });
  CHECK(fut.get() == 42);
}

TEST_CASE("WorkStealingPool: void task", "[build][executor]") {
  WorkStealingPool pool(2);
  std::atomic<int> counter{0};
  auto fut = pool.submit([&] { counter.fetch_add(1); });
  fut.wait();
  CHECK(counter.load() == 1);
}

TEST_CASE("WorkStealingPool: 1000 tasks parallel", "[build][executor]") {
  WorkStealingPool pool(4);
  std::atomic<int> counter{0};
  std::vector<std::future<void>> futs;
  for (int i = 0; i < 1000; ++i) {
    futs.push_back(pool.submit([&] { counter.fetch_add(1); }));
  }
  for (auto& f : futs)
    f.wait();
  CHECK(counter.load() == 1000);
}

TEST_CASE("WorkStealingPool: tasks with arguments", "[build][executor]") {
  WorkStealingPool pool(2);
  auto fut = pool.submit([](int a, int b) { return a + b; }, 3, 4);
  CHECK(fut.get() == 7);
}

TEST_CASE("WorkStealingPool: exception propagates via future", "[build][executor]") {
  WorkStealingPool pool(2);
  auto fut = pool.submit([] { throw std::runtime_error("boom"); });
  CHECK_THROWS_AS(fut.get(), std::runtime_error);
}

TEST_CASE("WorkStealingPool: shutdown joins cleanly", "[build][executor]") {
  WorkStealingPool pool(4);
  for (int i = 0; i < 100; ++i) {
    pool.submit([] { std::this_thread::sleep_for(std::chrono::microseconds(10)); });
  }
  // Destructor calls shutdown(); just need no hang/crash.
  pool.shutdown();
  pool.shutdown();  // idempotent
  SUCCEED("shutdown completed without hang");
}

TEST_CASE("WorkStealingPool: parallel speedup over serial", "[build][executor]") {
  // Sanity check: 4-worker pool finishes 8 × 10ms tasks in < 4 × 10ms.
  WorkStealingPool pool(4);
  auto t0 = std::chrono::steady_clock::now();
  std::vector<std::future<void>> futs;
  for (int i = 0; i < 8; ++i) {
    futs.push_back(pool.submit(
        [] { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }));
  }
  for (auto& f : futs)
    f.wait();
  auto elapsed = std::chrono::steady_clock::now() - t0;
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
  // Serial would take 80ms; with 4 workers expect ≤ 40ms (with slack for noise).
  CHECK(ms.count() < 60);
}
