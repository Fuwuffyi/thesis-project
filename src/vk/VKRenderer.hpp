#pragma once

#include "../core/IRenderer.hpp"
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
   std::optional<uint32_t> graphicsFamily;

   bool HasAllValues() {
      return graphicsFamily.has_value();
   }
};

class VKRenderer : public IRenderer {
public:
   VKRenderer(GLFWwindow* windowHandle);
   ~VKRenderer();
   void RenderFrame() override;
private:
   // Callback for validation layers
   static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData);

   // Functions to setup vulkan instance
   void CreateInstance();
   std::vector<const char*> GetRequiredExtensions() const;
   // Functions to setup vulkan validation layers (debug info)
   bool CheckValidationLayerSupport() const;
   void GetDebugMessenger();
   // Functions to setup the physical device
   void GetPhysicalDevice();
   uint32_t RateDevice(const VkPhysicalDevice& device);
   void GetQueueFamilies(const VkPhysicalDevice& device);
   bool IsDeviceSuitable(const VkPhysicalDevice& device);
   // Functions to setup the logical device
   void GetLogicalDevice();
private:
   VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
   VkInstance m_instance = VK_NULL_HANDLE;
   VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
   VkDevice m_logicalDevice = VK_NULL_HANDLE;
   QueueFamilyIndices m_queueFamilies{};
};

