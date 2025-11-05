#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
  public:
   ThreadPool();
   explicit ThreadPool(const size_t numThreads);
   ~ThreadPool();

   ThreadPool(const ThreadPool&) = delete;
   ThreadPool& operator=(const ThreadPool&) = delete;
   ThreadPool(ThreadPool&&) = delete;
   ThreadPool& operator=(ThreadPool&&) = delete;

   void Submit(const std::function<void()> task);

   [[nodiscard]] size_t GetThreadCount() const noexcept;
   void WaitForAll();

  private:
   void WorkerThread();

   std::vector<std::thread> m_threads;
   std::queue<std::function<void()>> m_tasks;

   std::mutex m_queueMutex;
   std::condition_variable m_condition;
   std::condition_variable m_waitCondition;

   std::atomic<bool> m_stop{false};
   std::atomic<size_t> m_activeTasks{0};
};
