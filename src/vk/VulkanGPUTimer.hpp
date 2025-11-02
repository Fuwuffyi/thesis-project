#pragma once

#include "core/system/IGPUTimer.hpp"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <array>

class VulkanDevice;
class VulkanCommandBuffers;

class VulkanGPUTimer : public IGPUTimer {
  public:
   explicit VulkanGPUTimer(const VulkanDevice& device);
   ~VulkanGPUTimer() override;
   void Begin(const std::string& label) override;
   void End(const std::string& label) override;
   [[nodiscard]] float GetElapsedMs(const std::string& label) override;
   void Reset() override;
   [[nodiscard]] bool IsAvailable(const std::string& label) const override;
   void BeginFrame(const VkCommandBuffer commandBuffer, const uint32_t frameIndex);
   void EndFrame(const uint32_t frameIndex);

  private:
   struct TimestampQuery {
      uint32_t startQuery{0};
      uint32_t endQuery{0};
      bool active{false};
      float cachedResultMs{0.0f};
      bool hasResult{false};
   };

   struct FrameQueries {
      VkQueryPool queryPool{VK_NULL_HANDLE};
      std::unordered_map<std::string, TimestampQuery> queries;
      uint32_t nextQueryIndex{0};
      VkCommandBuffer activeCommandBuffer{VK_NULL_HANDLE};
   };

   static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
   static constexpr uint32_t MAX_QUERIES_PER_FRAME = 64;

   const VulkanDevice& m_device;
   std::array<FrameQueries, MAX_FRAMES_IN_FLIGHT> m_frameQueries;
   uint32_t m_currentFrame{0};
   float m_timestampPeriod{1.0f};

   void CreateQueryPools();
   void DestroyQueryPools();
};
