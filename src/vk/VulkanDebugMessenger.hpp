#pragma once

#include <vulkan/vulkan.h>

// Forward declaration of Vulkan Instance class
class VulkanInstance;

class VulkanDebugMessenger {
public:
   VulkanDebugMessenger(const VulkanInstance& instance);
   ~VulkanDebugMessenger();

   VulkanDebugMessenger(const VulkanDebugMessenger&) = delete;
   VulkanDebugMessenger& operator=(const VulkanDebugMessenger&) = delete;
   VulkanDebugMessenger(VulkanDebugMessenger&& other) noexcept;
   VulkanDebugMessenger& operator=(VulkanDebugMessenger&& other) noexcept;

   VkDebugUtilsMessengerEXT Get() const;

private:
   const VulkanInstance* m_instance = nullptr;
   VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

   // Debug function to print message
   static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData);
};

