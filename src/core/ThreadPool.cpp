#include "core/ThreadPool.hpp"
#include <atomic>

ThreadPool::ThreadPool(size_t numThreads) {
   m_threads.reserve(numThreads);
   for (size_t i = 0; i < numThreads; ++i) {
      m_threads.emplace_back([this]() { WorkerThread(); });
   }
}

ThreadPool::~ThreadPool() {
   {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_stop = true;
   }
   m_condition.notify_all();
   for (std::thread& thread : m_threads) {
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
         m_condition.wait(lock, [this]() { return m_stop || !m_tasks.empty(); });
         if (m_stop && m_tasks.empty()) {
            return;
         }
         if (!m_tasks.empty()) {
            task = std::move(m_tasks.front());
            m_tasks.pop();
            m_activeTasks.fetch_add(1);
         }
      }
      if (task) {
         task();
         m_activeTasks.fetch_sub(1);
      }
   }
}

void ThreadPool::WaitForAll() {
   while (m_activeTasks > 0 || !m_tasks.empty()) {
      std::this_thread::yield();
   }
}
