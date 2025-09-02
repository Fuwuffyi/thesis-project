#include "VulkanRenderPass.hpp"

#include "VulkanDevice.hpp"

#include <stdexcept>
#include <vulkan/vulkan_core.h>

VulkanRenderPass::VulkanRenderPass(const VulkanDevice& device, const RenderPassDescription& desc)
    : m_device(&device) {
   CreateRenderPass(desc);
}

VulkanRenderPass::VulkanRenderPass(const VulkanDevice& device, const VkFormat colorFormat,
                                   const VkFormat depthFormat)
    : m_device(&device) {
   RenderPassDescription desc = CreateDefaultDescription(colorFormat, depthFormat);
   CreateRenderPass(desc);
}

VulkanRenderPass::~VulkanRenderPass() {
   if (m_renderPass != VK_NULL_HANDLE && m_device) {
      vkDestroyRenderPass(m_device->Get(), m_renderPass, nullptr);
   }
}

VulkanRenderPass::VulkanRenderPass(VulkanRenderPass&& other) noexcept
    : m_device(other.m_device),
      m_renderPass(other.m_renderPass),
      m_clearValues(std::move(other.m_clearValues)) {
   other.m_device = nullptr;
   other.m_renderPass = VK_NULL_HANDLE;
}

VulkanRenderPass& VulkanRenderPass::operator=(VulkanRenderPass&& other) noexcept {
   if (this != &other) {
      m_device = other.m_device;
      m_renderPass = other.m_renderPass;
      m_clearValues = std::move(other.m_clearValues);
      other.m_device = nullptr;
      other.m_renderPass = VK_NULL_HANDLE;
   }
   return *this;
}

VkRenderPass VulkanRenderPass::Get() const { return m_renderPass; }

const std::vector<VkClearValue>& VulkanRenderPass::GetClearValues() const { return m_clearValues; }

void VulkanRenderPass::CreateRenderPass(const RenderPassDescription& desc) {
   // Convert attachment descriptions
   std::vector<VkAttachmentDescription> attachments;
   attachments.reserve(desc.attachments.size());
   for (const AttachmentDescription& att : desc.attachments) {
      VkAttachmentDescription vkAtt{};
      vkAtt.format = att.format;
      vkAtt.samples = att.samples;
      vkAtt.loadOp = att.loadOp;
      vkAtt.storeOp = att.storeOp;
      vkAtt.stencilLoadOp = att.stencilLoadOp;
      vkAtt.stencilStoreOp = att.stencilStoreOp;
      vkAtt.initialLayout = att.initialLayout;
      vkAtt.finalLayout = att.finalLayout;
      attachments.push_back(vkAtt);
      // Add clear value
      VkClearValue clearValue{};
      if (att.format == VK_FORMAT_D32_SFLOAT || att.format == VK_FORMAT_D24_UNORM_S8_UINT ||
          att.format == VK_FORMAT_D16_UNORM) {
         clearValue.depthStencil = {1.0f, 0};
      } else {
         clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
      }
      m_clearValues.push_back(clearValue);
   }
   // Convert subpass descriptions
   std::vector<VkSubpassDescription> subpasses;
   subpasses.reserve(desc.subpasses.size());
   for (const SubpassDescription& sub : desc.subpasses) {
      VkSubpassDescription vkSub{};
      vkSub.pipelineBindPoint = sub.bindPoint;
      vkSub.colorAttachmentCount = static_cast<uint32_t>(sub.colorAttachments.size());
      vkSub.pColorAttachments =
         sub.colorAttachments.empty() ? nullptr : sub.colorAttachments.data();
      vkSub.inputAttachmentCount = static_cast<uint32_t>(sub.inputAttachments.size());
      vkSub.pInputAttachments =
         sub.inputAttachments.empty() ? nullptr : sub.inputAttachments.data();
      vkSub.pDepthStencilAttachment = sub.depthStencilAttachment;
      vkSub.pResolveAttachments =
         sub.resolveAttachments.empty() ? nullptr : sub.resolveAttachments.data();
      vkSub.preserveAttachmentCount = static_cast<uint32_t>(sub.preserveAttachments.size());
      vkSub.pPreserveAttachments =
         sub.preserveAttachments.empty() ? nullptr : sub.preserveAttachments.data();
      subpasses.push_back(vkSub);
   }
   // Create render pass
   VkRenderPassCreateInfo renderPassInfo{};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
   renderPassInfo.pAttachments = attachments.data();
   renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
   renderPassInfo.pSubpasses = subpasses.data();
   renderPassInfo.dependencyCount = static_cast<uint32_t>(desc.dependencies.size());
   renderPassInfo.pDependencies = desc.dependencies.empty() ? nullptr : desc.dependencies.data();
   if (vkCreateRenderPass(m_device->Get(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create render pass.");
   }
}

RenderPassDescription VulkanRenderPass::CreateDefaultDescription(const VkFormat colorFormat,
                                                                 const VkFormat depthFormat) {
   RenderPassDescription desc;
   // Color attachment
   AttachmentDescription colorAtt{};
   colorAtt.format = colorFormat;
   colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
   colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   desc.attachments.push_back(colorAtt);
   // Depth attachment
   if (depthFormat != VK_FORMAT_UNDEFINED) {
      AttachmentDescription depthAtt{};
      depthAtt.format = depthFormat;
      depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
      depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      desc.attachments.push_back(depthAtt);
   }
   // Subpass
   SubpassDescription subpass{};
   subpass.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   VkAttachmentReference colorRef{};
   colorRef.attachment = 0;
   colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   subpass.colorAttachments.push_back(colorRef);
   if (depthFormat != VK_FORMAT_UNDEFINED) {
      subpass.depthStencilAttachment =
         new VkAttachmentReference{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
   }
   desc.subpasses.push_back(subpass);
   // Dependencies
   VkSubpassDependency dependency{};
   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass = 0;
   dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.srcAccessMask = 0;
   dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
   if (depthFormat != VK_FORMAT_UNDEFINED) {
      dependency.srcStageMask =
         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependency.dstStageMask =
         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependency.dstAccessMask =
         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
   }
   desc.dependencies.push_back(dependency);
   return desc;
}
