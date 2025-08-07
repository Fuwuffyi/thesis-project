#pragma once

#include "../core/IRenderer.hpp"
#include "VulkanInstance.hpp"
#include "VulkanSurface.hpp"
#include "VulkanDebugMessenger.hpp"
#include <memory>
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

struct SwapChainSupportDetails {
   VkSurfaceCapabilitiesKHR capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool HasAllValues() {
      return graphicsFamily.has_value() && presentFamily.has_value();
   }
};

class VulkanRenderer : public IRenderer {
public:
   VulkanRenderer(GLFWwindow* windowHandle);
   ~VulkanRenderer();
   void RenderFrame() override;
private:
   // Functions to setup the physical device
   void GetPhysicalDevice();
   uint32_t RateDevice(const VkPhysicalDevice& device);
   void GetQueueFamilies(const VkPhysicalDevice& device);
   bool IsDeviceSuitable(const VkPhysicalDevice& device);
   bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device);
   // Functions to setup the logical device
   void GetLogicalDevice();
   // Functions to set up Swapchain
   void GetSwapchain();
   SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& device) const;
   VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
   VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
   VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
   // Functions to set up image views
   void GetImageViews();
   // Functions to set up a render pass
   void CreateRenderPass();
   // Functions to set up a graphics pipeline
   static std::vector<char> ReadFile(const std::string& filename);
   void CreateGraphicsPipeline();
   VkShaderModule CreateShaderModule(const std::vector<char>& code);
   // Functions to set up framebuffers
   void CreateFramebuffers();
   // Functions to set up command pool
   void CreateCommandPool();
   void CreateCommandBuffer();
   void RecordCommandBuffer(const VkCommandBuffer& commandBuffer, const uint32_t imageIndex);
private:
   std::unique_ptr<VulkanInstance> m_instance;
   std::unique_ptr<VulkanDebugMessenger> m_debugMessenger;
   std::unique_ptr<VulkanSurface> m_surface;
   VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
   VkDevice m_logicalDevice = VK_NULL_HANDLE;
   QueueFamilyIndices m_queueFamilies{};
   VkSwapchainKHR m_swapchain;
   std::vector<VkImage> m_swapchainImages;
   VkFormat m_swapchainImageFormat;
   VkExtent2D m_swapchainExtent;
   std::vector<VkImageView> m_swapchainImageViews;
   std::vector<VkFramebuffer> m_swapchainFramebuffers;
   VkQueue m_graphicsQueue;
   VkQueue m_presentQueue;
   VkRenderPass m_renderPass;
   VkPipelineLayout m_pipelineLayout;
   VkPipeline m_graphicsPipeline;
   VkCommandPool m_commandPool;
   VkCommandBuffer m_commandBuffer;
};

