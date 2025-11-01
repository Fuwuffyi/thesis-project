#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool {
  public:
   explicit ThreadPool(const size_t numThreads = std::thread::hardware_concurrency());
   ~ThreadPool();

   ThreadPool(const ThreadPool&) = delete;
   ThreadPool& operator=(const ThreadPool&) = delete;
   ThreadPool(ThreadPool&&) = delete;
   ThreadPool& operator=(ThreadPool&&) = delete;

   // Submit a task and get a future for the result
   template <typename F, typename... Args>
   auto Submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
      using ReturnType = std::invoke_result_t<F, Args...>;
      const auto task = std::make_shared<std::packaged_task<ReturnType()>>(
         std::bind(std::forward<F>(f), std::forward<Args>(args)...));
      std::future<ReturnType> result = task->get_future();
      {
         std::unique_lock<std::mutex> lock(m_queueMutex);
         if (m_stop.load(std::memory_order_relaxed)) {
            throw std::runtime_error("Cannot submit task to stopped ThreadPool");
         }
         m_tasks.emplace([task = std::move(task)]() { (*task)(); });
      }
      m_condition.notify_one();
      return result;
   }

   // Submit multiple tasks and wait for all to complete
   template <typename F>
   void ParallelFor(const size_t count, F&& func) {
      if (count == 0)
         return;
      std::vector<std::future<void>> futures;
      futures.reserve(count);
      for (size_t i = 0; i < count; ++i) {
         futures.push_back(Submit(func, i));
      }
      for (auto& future : futures) {
         future.wait();
      }
   }

   [[nodiscard]] size_t GetThreadCount() const noexcept { return m_threads.size(); }
   void WaitForAll();

  private:
   void WorkerThread();

   std::vector<std::thread> m_threads;
   std::queue<std::function<void()>> m_tasks;

   mutable std::mutex m_queueMutex;
   std::condition_variable m_condition;
   std::condition_variable m_waitCondition;

   std::atomic<bool> m_stop{false};
   std::atomic<size_t> m_activeTasks{0};
};
