#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;

struct AttachmentDescription {
   VkFormat format = VK_FORMAT_UNDEFINED;
   VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
   VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

struct SubpassDescription {
   VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   std::vector<VkAttachmentReference> colorAttachments;
   std::vector<VkAttachmentReference> inputAttachments;
   VkAttachmentReference* depthStencilAttachment = nullptr;
   std::vector<VkAttachmentReference> resolveAttachments;
   std::vector<uint32_t> preserveAttachments;
};

struct RenderPassDescription {
   std::vector<AttachmentDescription> attachments;
   std::vector<SubpassDescription> subpasses;
   std::vector<VkSubpassDependency> dependencies;
};

class VulkanRenderPass {
  public:
   VulkanRenderPass(const VulkanDevice& device, const RenderPassDescription& desc);
   VulkanRenderPass(const VulkanDevice& device, const std::vector<VkFormat>& colorFormats,
                    const VkFormat depthFormat = VK_FORMAT_UNDEFINED);
   ~VulkanRenderPass();

   VulkanRenderPass(const VulkanRenderPass&) = delete;
   VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;
   VulkanRenderPass(VulkanRenderPass&& other) noexcept;
   VulkanRenderPass& operator=(VulkanRenderPass&& other) noexcept;

   VkRenderPass Get() const;
   const std::vector<VkClearValue>& GetClearValues() const;

  private:
   void CreateRenderPass(const RenderPassDescription& desc);
   RenderPassDescription CreateDefaultDescription(const std::vector<VkFormat>& colorFormats,
                                                  const VkFormat depthFormat);

   const VulkanDevice* m_device = nullptr;
   VkRenderPass m_renderPass = VK_NULL_HANDLE;
   std::vector<VkClearValue> m_clearValues;
};
