#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <optional>

class VulkanInstance;
class VulkanSurface;

struct QueueFamilyIndices {
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool HasAllValues() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

class VulkanDevice {
  public:
   VulkanDevice(const VulkanInstance& instance, const VulkanSurface& surface);
   ~VulkanDevice();

   // Move semantics
   VulkanDevice(VulkanDevice&& other) noexcept;
   VulkanDevice& operator=(VulkanDevice&& other) noexcept;

   // Delete copy semantics
   VulkanDevice(const VulkanDevice&) = delete;
   VulkanDevice& operator=(const VulkanDevice&) = delete;

   // Getters
   const VkDevice& Get() const;
   const VkPhysicalDevice& GetPhysicalDevice() const;
   const VkQueue& GetGraphicsQueue() const;
   const VkQueue& GetPresentQueue() const;
   const VkCommandPool& GetCommandPool() const;
   const QueueFamilyIndices& GetQueueFamilies() const;
   const VmaAllocator& GetAllocator() const;
   uint32_t GetGraphicsQueueFamily() const;
   uint32_t GetPresentQueueFamily() const;

  private:
   void CreateCommandPool();
   void CreateAllocator(const VulkanInstance& instance,
                        const VkPhysicalDeviceBufferDeviceAddressFeatures& features);

  private:
   vkb::Device m_vkbDevice;
   VkDevice m_device{VK_NULL_HANDLE};
   VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
   VkQueue m_graphicsQueue{VK_NULL_HANDLE};
   VkQueue m_presentQueue{VK_NULL_HANDLE};
   VkCommandPool m_commandPool{VK_NULL_HANDLE};
   QueueFamilyIndices m_queueFamilies{};
   VmaAllocator m_allocator{VK_NULL_HANDLE};
   bool m_ownsDevice{false};
};
