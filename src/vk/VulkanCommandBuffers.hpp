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

   void BindDescriptorSet(const VulkanPipelineLayout& layout, const uint32_t firstSet,
                          const VkDescriptorSet& descriptorSet,
                          const VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                          const uint32_t index = 0);

   void SetViewport(const VkViewport& viewport, const uint32_t index = 0);
   void SetScissor(const VkRect2D& scissor, const uint32_t index = 0);

   void PushConstants(const VulkanPipelineLayout& layout, const VkShaderStageFlags stageFlags,
                      const uint32_t offset, const uint32_t size, const void* pValues,
                      const uint32_t index = 0);

   template <typename T>
   void PushConstantsTyped(const VulkanPipelineLayout& layout, const VkShaderStageFlags stageFlags,
                           const T& data, const uint32_t index = 0) {
      PushConstants(layout, stageFlags, 0, sizeof(T), &data, index);
   }

   void BindVertexBuffers(const uint32_t firstBinding, const std::vector<VkBuffer>& buffers,
                          const std::vector<VkDeviceSize>& offsets, const uint32_t index = 0);

   void BindIndexBuffer(const VkBuffer buffer, const VkDeviceSize offset,
                        const VkIndexType indexType, const uint32_t index = 0);

   void DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount,
                    const uint32_t firstIndex, const int32_t vertexOffset,
                    const uint32_t firstInstance, const uint32_t index = 0);

   void PipelineBarrier(const VkPipelineStageFlags srcStageMask,
                        const VkPipelineStageFlags dstStageMask,
                        const VkDependencyFlags dependencyFlags,
                        const std::vector<VkMemoryBarrier>& memoryBarriers,
                        const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarriers,
                        const std::vector<VkImageMemoryBarrier>& imageMemoryBarriers,
                        const uint32_t index = 0);

   static void ExecuteImmediate(const VulkanDevice& device, const VkCommandPool& commandPool,
                                const VkQueue& queue,
                                const std::function<void(const VkCommandBuffer&)> commands);

  private:
   const VulkanDevice* m_device;
   VkCommandPool m_commandPool = VK_NULL_HANDLE;
   std::vector<VkCommandBuffer> m_commandBuffers;
};
