#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

class VulkanInstance;
class VulkanSurface;

struct QueueFamilyIndices {
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool HasAllValues() {
      return graphicsFamily.has_value() && presentFamily.has_value();
   }
};

struct SwapChainSupportDetails {
   VkSurfaceCapabilitiesKHR capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR> presentModes;
};

class VulkanDevice {
public:
   VulkanDevice(const VulkanInstance& instance, const VulkanSurface& surface,
                const std::vector<const char*>& requiredExtensions, const std::vector<const char*>& validationLayers,
                const bool enableValidation);
   ~VulkanDevice();

   VulkanDevice(const VulkanDevice&) = delete;
   VulkanDevice& operator=(const VulkanDevice&) = delete;
   VulkanDevice(VulkanDevice&& other) noexcept;
   VulkanDevice& operator=(VulkanDevice&& other) noexcept;

   const VkPhysicalDevice& GetPhysicalDevice() const;
   const VkDevice& Get() const;
   const VkQueue& GetGraphicsQueue() const;
   const VkQueue& GetPresentQueue() const;
   const QueueFamilyIndices& GetQueueFamilies() const;
   uint32_t GetGraphicsQueueFamily() const;
   uint32_t GetPresentQueueFamily() const;

private:
   // Object creation
   void CreatePhysicalDevice(const std::vector<const char*>& requiredExtensions);
   void CreateLogicalDevice(const std::vector<const char*>& requiredExtensions,
                            const std::vector<const char*>& validationLayers,
                            const bool enableValidation);
   void FindQueueFamilies();
   void GetDeviceQueues();

   // Device evaluation
   static uint32_t RateDevice(const VkPhysicalDevice& device);
   static bool IsDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface, const std::vector<const char*>& requiredExtensions);
   static bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& requiredExtensions);
public:
   // TODO: Move to private? when swapchain fixed maybe, unsure.
   static SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
private:
   // Dependencies
   const VulkanInstance* m_instance;
   const VulkanSurface* m_surface;
   // Core objects
   VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
   VkDevice m_device = VK_NULL_HANDLE;
   // Queue management
   QueueFamilyIndices m_queueFamilies{};
   VkQueue m_graphicsQueue = VK_NULL_HANDLE;
   VkQueue m_presentQueue = VK_NULL_HANDLE;
};
