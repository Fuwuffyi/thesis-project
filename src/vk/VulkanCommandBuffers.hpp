#pragma once

#include <vulkan/vulkan.h>
#include <functional>

class VulkanDevice;
class VulkanPipelineLayout;
class VulkanRenderPass;

class VulkanCommandBuffers {
  public:
   VulkanCommandBuffers(const VulkanDevice& device, const VkCommandPool& commandPool,
                        const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                        const uint32_t count = 1);
   ~VulkanCommandBuffers();

   VulkanCommandBuffers(const VulkanCommandBuffers&) = delete;
   VulkanCommandBuffers& operator=(const VulkanCommandBuffers&) = delete;
   VulkanCommandBuffers(VulkanCommandBuffers&& other) noexcept;
   VulkanCommandBuffers& operator=(VulkanCommandBuffers&& other) noexcept;

   const VkCommandBuffer& Get(const uint32_t index = 0) const;

   void Begin(const VkCommandBufferUsageFlags flags = 0, const uint32_t index = 0);
   void End(const uint32_t index = 0);
   void Reset(const uint32_t index = 0);

   void BeginRenderPass(const VulkanRenderPass& renderPass, const VkFramebuffer& framebuffer,
                        const VkExtent2D& extent, const std::vector<VkClearValue>& clearValues,
                        const uint32_t index = 0);
   void EndRenderPass(const uint32_t index = 0);

   void BindPipeline(const VkPipeline& pipeline,
                     const VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                     const uint32_t index = 0);
   void BindDescriptorSets(const VulkanPipelineLayout& layout, const uint32_t firstSet,
                           const std::vector<VkDescriptorSet>& descriptorSets,
                           const VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                           const uint32_t index = 0);

   void SetViewport(const VkViewport& viewport, const uint32_t index = 0);
   void SetScissor(const VkRect2D& scissor, const uint32_t index = 0);

   static void ExecuteImmediate(const VulkanDevice& device, const VkCommandPool& commandPool,
                                const VkQueue& queue,
                                const std::function<void(const VkCommandBuffer&)> commands);

  private:
   const VulkanDevice* m_device;
   VkCommandPool m_commandPool = VK_NULL_HANDLE;
   std::vector<VkCommandBuffer> m_commandBuffers;
};
