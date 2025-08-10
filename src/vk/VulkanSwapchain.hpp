#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;
class VulkanSurface;
class Window;

class VulkanSwapchain {
public:
   VulkanSwapchain(const VulkanDevice& device, const VulkanSurface& surface, const Window& window);
   ~VulkanSwapchain();

   VulkanSwapchain(const VulkanSwapchain&) = delete;
   VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
   VulkanSwapchain(VulkanSwapchain&& other) noexcept;
   VulkanSwapchain& operator=(VulkanSwapchain&& other) noexcept;

   VkSwapchainKHR Get() const;
   const std::vector<VkImage>& GetImages() const;
   const std::vector<VkImageView>& GetImageViews() const;
   VkFormat GetFormat() const;
   VkExtent2D GetExtent() const;

   void Recreate();
   VkResult AcquireNextImage(const uint64_t timeout, const VkSemaphore& semaphore, uint32_t* imageIndex) const;
private:
   void CreateSwapchain();
   void CreateImageViews();
   void Cleanup();

   VkSurfaceFormatKHR ChooseFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
   VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) const;
   VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

   const VulkanDevice* m_device;
   const VulkanSurface* m_surface;
   const Window* m_window;

   VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
   VkFormat m_format;
   VkExtent2D m_extent;

   std::vector<VkImage> m_images;
   std::vector<VkImageView> m_imageViews;
};
