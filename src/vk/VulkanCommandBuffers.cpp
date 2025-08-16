#include "VulkanCommandBuffers.hpp"

#include "VulkanDevice.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanRenderPass.hpp"

#include <stdexcept>

VulkanCommandBuffers::VulkanCommandBuffers(const VulkanDevice& device, const VkCommandPool& commandPool,
                                           const VkCommandBufferLevel level, const uint32_t count)
   :
   m_device(&device),
   m_commandPool(commandPool),
   m_commandBuffers(count, VK_NULL_HANDLE)
{
   VkCommandBufferAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool = m_commandPool;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandBufferCount = count;
   if (vkAllocateCommandBuffers(m_device->Get(), &allocInfo,
                                m_commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate command buffers.");
   }
}

VulkanCommandBuffers::~VulkanCommandBuffers() = default;


VulkanCommandBuffers::VulkanCommandBuffers(VulkanCommandBuffers&& other) noexcept
   :
   m_device(other.m_device),
   m_commandPool(other.m_commandPool),
   m_commandBuffers(std::move(other.m_commandBuffers))
{
   other.m_commandBuffers.clear();
   other.m_commandPool = VK_NULL_HANDLE;
   other.m_device = nullptr;
}

VulkanCommandBuffers& VulkanCommandBuffers::operator=(VulkanCommandBuffers&& other) noexcept {
   if (this != &other) {
      m_device = other.m_device;
      m_commandPool = other.m_commandPool;
      m_commandBuffers = std::move(other.m_commandBuffers);
      other.m_commandBuffers.clear();
      other.m_commandPool = VK_NULL_HANDLE;
      other.m_device = nullptr;
   }
   return *this;
}


const VkCommandBuffer& VulkanCommandBuffers::Get(const uint32_t index) const {
   if (index >= m_commandBuffers.size()) throw std::runtime_error("Index out of bounds in command buffers");
   return m_commandBuffers[index];
}


void VulkanCommandBuffers::Begin(const VkCommandBufferUsageFlags flags, const uint32_t index) {
   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = flags;
   beginInfo.pInheritanceInfo = nullptr;
   if (vkBeginCommandBuffer(m_commandBuffers[index], &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("Failed to begin recording command buffer.");
   }
}

void VulkanCommandBuffers::End(const uint32_t index) {
   if (vkEndCommandBuffer(m_commandBuffers[index]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to record command buffer.");
   }
}

void VulkanCommandBuffers::Reset(const uint32_t index) {
   vkResetCommandBuffer(m_commandBuffers[index], 0);
}

void VulkanCommandBuffers::BeginRenderPass(const VulkanRenderPass& renderPass, const VkFramebuffer& framebuffer,
                                           const VkExtent2D& extent, const std::vector<VkClearValue>& clearValues, const uint32_t index) {
   VkRenderPassBeginInfo renderPassInfo{};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassInfo.renderPass = renderPass.Get();
   renderPassInfo.framebuffer = framebuffer;
   renderPassInfo.renderArea.offset = { 0, 0 };
   renderPassInfo.renderArea.extent = extent;
   renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
   renderPassInfo.pClearValues = clearValues.data();
   vkCmdBeginRenderPass(m_commandBuffers[index],
                        &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffers::EndRenderPass(const uint32_t index) {
   vkCmdEndRenderPass(m_commandBuffers[index]);
}


void VulkanCommandBuffers::BindPipeline(const VkPipeline& pipeline, const VkPipelineBindPoint bindPoint, const uint32_t index) {
   vkCmdBindPipeline(m_commandBuffers[index], bindPoint, pipeline);
}

void VulkanCommandBuffers::BindDescriptorSets(const VulkanPipelineLayout& layout, const uint32_t firstSet,
                                              const std::vector<VkDescriptorSet>& descriptorSets,
                                              const VkPipelineBindPoint bindPoint, const uint32_t index) {

   vkCmdBindDescriptorSets(m_commandBuffers[index], bindPoint,
                           layout.Get(), firstSet, descriptorSets.size(),
                           descriptorSets.data(), 0, nullptr);
}


void VulkanCommandBuffers::SetViewport(const VkViewport& viewport, const uint32_t index) {
   vkCmdSetViewport(m_commandBuffers[index], 0, 1, &viewport);
}

void VulkanCommandBuffers::SetScissor(const VkRect2D& scissor, const uint32_t index) {
   vkCmdSetScissor(m_commandBuffers[index], 0, 1, &scissor);
}


void VulkanCommandBuffers::ExecuteImmediate(const VulkanDevice& device, const VkCommandPool& commandPool,
                                            const VkQueue& queue, const std::function<void(const VkCommandBuffer&)> commands) {
   VkCommandBufferAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandPool = commandPool;
   allocInfo.commandBufferCount = 1;
   VkCommandBuffer commandBuffer;
   if (vkAllocateCommandBuffers(device.Get(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate command buffer for immediate execution.");
   }

   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   vkBeginCommandBuffer(commandBuffer, &beginInfo);
   commands(commandBuffer);
   vkEndCommandBuffer(commandBuffer);
   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &commandBuffer;
   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
   vkQueueWaitIdle(queue);
   vkFreeCommandBuffers(device.Get(), commandPool, 1, &commandBuffer);
}

