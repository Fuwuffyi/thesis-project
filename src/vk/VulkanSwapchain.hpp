#pragma once

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

#include <vector>

class VulkanDevice;
class VulkanSurface;
class Window;

class VulkanSwapchain {
  public:
   VulkanSwapchain(const VulkanDevice& device, const VulkanSurface& surface, const Window& window);
   ~VulkanSwapchain();

   // Move semantics
   VulkanSwapchain(VulkanSwapchain&& other) noexcept;
   VulkanSwapchain& operator=(VulkanSwapchain&& other) noexcept;

   // Delete copy semantics
   VulkanSwapchain(const VulkanSwapchain&) = delete;
   VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

   void Recreate();
   VkResult AcquireNextImage(const uint64_t timeout, const VkSemaphore& semaphore,
                             uint32_t* imageIndex) const;

   // Getters
   VkSwapchainKHR Get() const;
   const std::vector<VkImage>& GetImages() const;
   const std::vector<VkImageView>& GetImageViews() const;
   VkFormat GetFormat() const;
   VkExtent2D GetExtent() const;

  private:
   void CreateSwapchain();
   void CreateImageViews();
   void Cleanup();

   const VulkanDevice* m_device;
   const VulkanSurface* m_surface;
   const Window* m_window;

   vkb::Swapchain m_vkbSwapchain;
   VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
   VkFormat m_format{VK_FORMAT_UNDEFINED};
   VkExtent2D m_extent{};
   std::vector<VkImage> m_images;
   std::vector<VkImageView> m_imageViews;
   bool m_ownsSwapchain{false};
};
