/// @file executor.hpp
/// @brief Work-stealing thread pool for parallel build execution.
///
/// Design:
///   - N worker threads, each owns a deque of tasks (LIFO local push/pop).
///   - When local deque empty → steal from random victim (FIFO steal-back).
///   - Tasks are `std::function<void()>` packaged via `submit()` returning future.
///
/// This is a Phase 4 placeholder — minimal, correct, and good enough to
/// drive parallel page rendering. Phase 5+ may swap in `taskflow` if
/// benchmarks demand a more sophisticated scheduler.

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace nift::build {

class WorkStealingPool {
 public:
  /// Construct a pool with `n` workers. Default = std::thread::hardware_concurrency().
  explicit WorkStealingPool(std::size_t n = 0);

  /// Joins all workers. Pending tasks complete; new submissions are rejected.
  ~WorkStealingPool();

  WorkStealingPool(const WorkStealingPool&) = delete;
  WorkStealingPool& operator=(const WorkStealingPool&) = delete;

  /// Submit a task, return a future for the result.
  template <typename F, typename... Args>
  auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

  /// Number of worker threads.
  std::size_t size() const noexcept { return workers_.size(); }

  /// Current pending task count (across all queues). Approximate.
  std::size_t pending() const noexcept;

  /// Wait until all submitted tasks have completed. Polls; for tests.
  void wait_idle();

  /// Stop accepting new tasks and join workers. Idempotent.
  void shutdown();

 private:
  using Task = std::function<void()>;

  struct Worker {
    std::deque<Task> queue;
    std::mutex mu;
  };

  void worker_loop(std::size_t idx);
  bool try_steal(std::size_t self, Task& out);

  std::vector<std::thread> workers_;
  std::vector<std::unique_ptr<Worker>> queues_;
  std::atomic<bool> stop_{false};
  std::atomic<std::size_t> active_{0};     // tasks in-flight
  std::atomic<std::size_t> next_push_{0};  // round-robin submit target
  std::condition_variable cv_;
  std::mutex global_mu_;
};

// ─── template impl ───────────────────────────────────────────────────

template <typename F, typename... Args>
auto WorkStealingPool::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
  using R = std::invoke_result_t<F, Args...>;
  auto packaged = std::make_shared<std::packaged_task<R()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  auto fut = packaged->get_future();

  if (stop_.load(std::memory_order_acquire)) {
    // Queue rejected — set future to broken_promise via destruction.
    return fut;
  }

  active_.fetch_add(1, std::memory_order_acq_rel);
  std::size_t target =
      next_push_.fetch_add(1, std::memory_order_acq_rel) % queues_.size();
  {
    std::lock_guard<std::mutex> lk(queues_[target]->mu);
    queues_[target]->queue.emplace_back([packaged]() { (*packaged)(); });
  }
  cv_.notify_one();
  return fut;
}

}  // namespace nift::build
