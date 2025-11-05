#include "vk/VulkanGPUTimer.hpp"
#include "vk/VulkanDevice.hpp"

#include <stdexcept>

VulkanGPUTimer::VulkanGPUTimer(const VulkanDevice& device) : m_device(device) {
   VkPhysicalDeviceProperties props;
   vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &props);
   m_timestampPeriod = props.limits.timestampPeriod;
   CreateQueryPools();
}

VulkanGPUTimer::~VulkanGPUTimer() { DestroyQueryPools(); }

void VulkanGPUTimer::CreateQueryPools() {
   VkQueryPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
   poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
   poolInfo.queryCount = MAX_QUERIES_PER_FRAME;
   for (FrameQueries& frame : m_frameQueries) {
      if (vkCreateQueryPool(m_device.Get(), &poolInfo, nullptr, &frame.queryPool) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create timestamp query pool");
      }
   }
}

void VulkanGPUTimer::DestroyQueryPools() {
   for (FrameQueries& frame : m_frameQueries) {
      if (frame.queryPool != VK_NULL_HANDLE) {
         vkDestroyQueryPool(m_device.Get(), frame.queryPool, nullptr);
         frame.queryPool = VK_NULL_HANDLE;
      }
   }
}

void VulkanGPUTimer::BeginFrame(const VkCommandBuffer& commandBuffer, const uint32_t frameIndex) {
   std::lock_guard<std::mutex> lock(m_mutex);
   m_currentFrame = frameIndex % MAX_FRAMES_IN_FLIGHT;
   FrameQueries& frame = m_frameQueries[m_currentFrame];
   frame.mainCommandBuffer = commandBuffer;
   frame.queries.clear();
   frame.nextQueryIndex = 0;
   vkCmdResetQueryPool(commandBuffer, frame.queryPool, 0, MAX_QUERIES_PER_FRAME);
}

void VulkanGPUTimer::Begin(const std::string& label) {
   std::lock_guard<std::mutex> lock(m_mutex);
   FrameQueries& frame = m_frameQueries[m_currentFrame];
   if (frame.nextQueryIndex + 2 > MAX_QUERIES_PER_FRAME)
      return;
   TimestampQuery& query = frame.queries[label];
   query.startQuery = frame.nextQueryIndex++;
   query.cmdBuffer = frame.mainCommandBuffer;
   query.active = true;
   query.hasResult = false;
   vkCmdWriteTimestamp(frame.mainCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frame.queryPool,
                       query.startQuery);
}

void VulkanGPUTimer::End(const std::string& label) {
   std::lock_guard<std::mutex> lock(m_mutex);
   FrameQueries& frame = m_frameQueries[m_currentFrame];
   const auto it = frame.queries.find(label);
   if (it == frame.queries.end() || !it->second.active) {
      return;
   }
   TimestampQuery& query = it->second;
   query.endQuery = frame.nextQueryIndex++;
   query.active = false;
   vkCmdWriteTimestamp(frame.mainCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                       frame.queryPool, query.endQuery);
}

void VulkanGPUTimer::BeginOnCommandBuffer(const VkCommandBuffer& cmdBuffer, const std::string& label) {
   std::lock_guard<std::mutex> lock(m_mutex);
   FrameQueries& frame = m_frameQueries[m_currentFrame];
   if (frame.nextQueryIndex + 2 > MAX_QUERIES_PER_FRAME)
      return;
   TimestampQuery& query = frame.queries[label];
   query.startQuery = frame.nextQueryIndex++;
   query.cmdBuffer = cmdBuffer;
   query.active = true;
   query.hasResult = false;
   vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frame.queryPool,
                       query.startQuery);
}

// NEW: Thread-safe method for recording on specific command buffer
void VulkanGPUTimer::EndOnCommandBuffer(const VkCommandBuffer& cmdBuffer, const std::string& label) {
   std::lock_guard<std::mutex> lock(m_mutex);
   FrameQueries& frame = m_frameQueries[m_currentFrame];
   const auto it = frame.queries.find(label);
   if (it == frame.queries.end() || !it->second.active) {
      return;
   }
   TimestampQuery& query = it->second;
   query.endQuery = frame.nextQueryIndex++;
   query.active = false;
   vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.queryPool,
                       query.endQuery);
}

void VulkanGPUTimer::EndFrame(const uint32_t frameIndex) {
   std::lock_guard<std::mutex> lock(m_mutex);
   const uint32_t resultsFrame = (frameIndex + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;
   FrameQueries& frame = m_frameQueries[resultsFrame];
   if (frame.nextQueryIndex == 0)
      return;
   std::vector<uint64_t> timestamps(frame.nextQueryIndex);
   const VkResult result =
      vkGetQueryPoolResults(m_device.Get(), frame.queryPool, 0, frame.nextQueryIndex,
                            timestamps.size() * sizeof(uint64_t), timestamps.data(),
                            sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
   if (result == VK_SUCCESS) {
      for (auto& [label, query] : frame.queries) {
         if (!query.active && query.startQuery < timestamps.size() &&
             query.endQuery < timestamps.size()) {
            const uint64_t startTime = timestamps[query.startQuery];
            const uint64_t endTime = timestamps[query.endQuery];
            // Convert to milliseconds
            query.cachedResultMs =
               static_cast<float>(endTime - startTime) * m_timestampPeriod / 1000000.0f;
            query.hasResult = true;
         }
      }
   }
}

float VulkanGPUTimer::GetElapsedMs(const std::string& label) {
   std::lock_guard<std::mutex> lock(m_mutex);
   for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      const uint32_t checkFrame =
         (m_currentFrame + MAX_FRAMES_IN_FLIGHT - 1 - i) % MAX_FRAMES_IN_FLIGHT;
      const auto& frame = m_frameQueries[checkFrame];
      const auto it = frame.queries.find(label);
      if (it != frame.queries.end() && it->second.hasResult) {
         return it->second.cachedResultMs;
      }
   }
   return 0.0f;
}

void VulkanGPUTimer::Reset() {
   std::lock_guard<std::mutex> lock(m_mutex);
   for (auto& frame : m_frameQueries) {
      frame.queries.clear();
      frame.nextQueryIndex = 0;
   }
}

bool VulkanGPUTimer::IsAvailable(const std::string& label) const {
   std::lock_guard<std::mutex> lock(m_mutex);
   for (const auto& frame : m_frameQueries) {
      const auto it = frame.queries.find(label);
      if (it != frame.queries.end() && it->second.hasResult) {
         return true;
      }
   }
   return false;
}
