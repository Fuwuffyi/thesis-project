#include "core/ThreadPool.hpp"
#include <atomic>

ThreadPool::ThreadPool() : ThreadPool(1) {}

ThreadPool::ThreadPool(size_t numThreads) {
   if (numThreads == 0)
      numThreads = 1;
   m_threads.reserve(numThreads);
   for (size_t i = 0; i < numThreads; ++i) {
      m_threads.emplace_back([this]() { WorkerThread(); });
   }
}

ThreadPool::~ThreadPool() {
   {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_stop.store(true, std::memory_order_release);
   }
   m_condition.notify_all();

   for (auto& thread : m_threads) {
      if (thread.joinable()) {
         thread.join();
      }
   }
}

void ThreadPool::Submit(const std::function<void()> task) {
   {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      if (m_stop.load(std::memory_order_relaxed)) {
         throw std::runtime_error("Cannot submit task to stopped ThreadPool");
      }
      m_activeTasks.fetch_add(1, std::memory_order_release);
      m_tasks.emplace(std::move(task));
   }
   m_condition.notify_one();
}

size_t ThreadPool::GetThreadCount() const noexcept { return m_threads.size(); }

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
         }
      }
      if (task) {
         task();
         const size_t remaining = m_activeTasks.fetch_sub(1, std::memory_order_acq_rel) - 1;
         if (remaining == 0) {
            m_waitCondition.notify_all();
         }
      }
   }
}

void ThreadPool::WaitForAll() {
   std::unique_lock<std::mutex> lock(m_queueMutex);
   m_waitCondition.wait(lock,
                        [this]() { return m_activeTasks.load(std::memory_order_acquire) == 0; });
}
