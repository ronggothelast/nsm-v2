/// @file executor.cpp

#include "nift/build/executor.hpp"

#include <chrono>

namespace nift::build {

WorkStealingPool::WorkStealingPool(std::size_t n) {
  if (n == 0) {
    n = std::thread::hardware_concurrency();
    if (n == 0)
      n = 2;
  }
  queues_.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    queues_.emplace_back(std::make_unique<Worker>());
  }
  workers_.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    workers_.emplace_back([this, i] { this->worker_loop(i); });
  }
}

WorkStealingPool::~WorkStealingPool() {
  shutdown();
}

void WorkStealingPool::shutdown() {
  if (stop_.exchange(true))
    return;  // already stopped
  cv_.notify_all();
  for (auto& t : workers_) {
    if (t.joinable())
      t.join();
  }
}

std::size_t WorkStealingPool::pending() const noexcept {
  std::size_t total = 0;
  for (const auto& w : queues_) {
    std::lock_guard<std::mutex> lk(w->mu);
    total += w->queue.size();
  }
  return total;
}

void WorkStealingPool::wait_idle() {
  using namespace std::chrono_literals;
  while (active_.load(std::memory_order_acquire) > 0) {
    std::this_thread::sleep_for(100us);
  }
}

bool WorkStealingPool::try_steal(std::size_t self, Task& out) {
  std::size_t n = queues_.size();
  if (n <= 1)
    return false;
  // Random start to avoid cache-line contention on the first victim.
  thread_local std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<std::size_t> dist(0, n - 1);
  std::size_t start = dist(rng);
  for (std::size_t i = 0; i < n; ++i) {
    std::size_t v = (start + i) % n;
    if (v == self)
      continue;
    std::lock_guard<std::mutex> lk(queues_[v]->mu);
    if (!queues_[v]->queue.empty()) {
      out = std::move(queues_[v]->queue.front());
      queues_[v]->queue.pop_front();
      return true;
    }
  }
  return false;
}

void WorkStealingPool::worker_loop(std::size_t idx) {
  while (!stop_.load(std::memory_order_acquire)) {
    Task task;
    bool got = false;

    // 1) Try local LIFO pop.
    {
      std::lock_guard<std::mutex> lk(queues_[idx]->mu);
      if (!queues_[idx]->queue.empty()) {
        task = std::move(queues_[idx]->queue.back());
        queues_[idx]->queue.pop_back();
        got = true;
      }
    }

    // 2) Otherwise, steal.
    if (!got)
      got = try_steal(idx, task);

    if (got) {
      try {
        task();
      } catch (...) {
        // Tasks are wrapped in packaged_task — exceptions land in the future.
      }
      active_.fetch_sub(1, std::memory_order_acq_rel);
      continue;
    }

    // 3) Idle wait.
    std::unique_lock<std::mutex> lk(global_mu_);
    cv_.wait_for(lk, std::chrono::milliseconds(1), [this] {
      return stop_.load(std::memory_order_acquire) ||
             active_.load(std::memory_order_acquire) > 0;
    });
  }

  // Drain on shutdown — empty local queue.
  while (true) {
    Task task;
    {
      std::lock_guard<std::mutex> lk(queues_[idx]->mu);
      if (queues_[idx]->queue.empty())
        break;
      task = std::move(queues_[idx]->queue.back());
      queues_[idx]->queue.pop_back();
    }
    try {
      task();
    } catch (...) {}
    active_.fetch_sub(1, std::memory_order_acq_rel);
  }
}

}  // namespace nift::build
