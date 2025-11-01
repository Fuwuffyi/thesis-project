#include "core/ThreadPool.hpp"
#include <atomic>

ThreadPool::ThreadPool(size_t numThreads) {
   if (numThreads == 0) {
      numThreads = 1;
   }
   m_threads.reserve(numThreads);
   for (size_t i = 0; i < numThreads; ++i) {
      m_threads.emplace_back([this]() { WorkerThread(); });
   }
}

ThreadPool::~ThreadPool() {
   m_stop.store(true, std::memory_order_release);
   m_condition.notify_all();
   for (auto& thread : m_threads) {
      if (thread.joinable()) {
         thread.join();
      }
   }
}

void ThreadPool::WorkerThread() {
   while (true) {
      std::function<void()> task;
      {
         std::unique_lock<std::mutex> lock(m_queueMutex);
         m_condition.wait(
            lock, [this]() { return m_stop.load(std::memory_order_acquire) || !m_tasks.empty(); });
         if (m_stop.load(std::memory_order_acquire) && m_tasks.empty()) {
            return;
         }
         if (!m_tasks.empty()) {
            task = std::move(m_tasks.front());
            m_tasks.pop();
            m_activeTasks.fetch_add(1, std::memory_order_relaxed);
         }
      }
      if (task) {
         task();
         const size_t remaining = m_activeTasks.fetch_sub(1, std::memory_order_release) - 1;
         if (remaining == 0) {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (m_tasks.empty()) {
               m_waitCondition.notify_all();
            }
         }
      }
   }
}

void ThreadPool::WaitForAll() {
   std::unique_lock<std::mutex> lock(m_queueMutex);
   m_waitCondition.wait(lock, [this]() {
      return m_tasks.empty() && m_activeTasks.load(std::memory_order_acquire) == 0;
   });
}
