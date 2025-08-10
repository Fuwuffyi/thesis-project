#include "VulkanSwapchain.hpp"

#include "VulkanDevice.hpp"
#include "VulkanSurface.hpp"
#include "../core/Window.hpp"

#include <limits>
#include <algorithm>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VulkanSwapchain::VulkanSwapchain(const VulkanDevice& device, const VulkanSurface& surface, const Window& window)
   :
   m_device(&device),
   m_surface(&surface),
   m_window(&window)
{
   CreateSwapchain();
   CreateImageViews();
}

VulkanSwapchain::~VulkanSwapchain() {
   Cleanup();
}

VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& other) noexcept
   :
   m_device(other.m_device),
   m_surface(other.m_surface),
   m_window(other.m_window),
   m_swapchain(other.m_swapchain),
   m_format(other.m_format),
   m_extent(other.m_extent),
   m_images(other.m_images),
   m_imageViews(other.m_imageViews)
{
   other.m_device = nullptr;
   other.m_surface = nullptr;
   other.m_window = nullptr;
   other.m_swapchain = VK_NULL_HANDLE;
   other.m_format = VK_FORMAT_UNDEFINED;
   other.m_extent = { 0, 0 };
   other.m_images.clear();
   other.m_imageViews.clear();
}

VulkanSwapchain& VulkanSwapchain::operator=(VulkanSwapchain&& other) noexcept {
   if (this != &other) {
      m_device = other.m_device;
      m_surface = other.m_surface;
      m_window = other.m_window;
      m_swapchain = other.m_swapchain;
      m_format = other.m_format;
      m_extent = other.m_extent;
      m_images = other.m_images;
      m_imageViews = other.m_imageViews;
      other.m_device = nullptr;
      other.m_surface = nullptr;
      other.m_window = nullptr;
      other.m_swapchain = VK_NULL_HANDLE;
      other.m_format = VK_FORMAT_UNDEFINED;
      other.m_extent = { 0, 0 };
      other.m_images.clear();
      other.m_imageViews.clear();
   }
   return *this;
}

void VulkanSwapchain::Recreate() {
   vkDeviceWaitIdle(m_device->Get());
   Cleanup();
   CreateSwapchain();
   CreateImageViews();
}

VkResult VulkanSwapchain::AcquireNextImage(const uint64_t timeout, const VkSemaphore& semaphore, uint32_t* imageIndex) const {
   return vkAcquireNextImageKHR(m_device->Get(), m_swapchain, timeout,
                                semaphore, VK_NULL_HANDLE, imageIndex);
}

void VulkanSwapchain::CreateSwapchain() {
   const SwapChainSupportDetails swapChainSupport = VulkanDevice::QuerySwapChainSupport(
      m_device->GetPhysicalDevice(),
      m_surface->Get()
   );
   // Get data to create the swapchain
   const VkSurfaceFormatKHR surfaceFormat = ChooseFormat(swapChainSupport.formats);
   const VkPresentModeKHR presentMode = ChoosePresentMode(swapChainSupport.presentModes);
   const VkExtent2D extent = ChooseExtent(swapChainSupport.capabilities);
   uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
   if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
   }
   // Create the swapchain
   VkSwapchainCreateInfoKHR createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   createInfo.surface = m_surface->Get();
   createInfo.minImageCount = imageCount;
   createInfo.imageFormat = surfaceFormat.format;
   createInfo.imageColorSpace = surfaceFormat.colorSpace;
   createInfo.imageExtent = extent;
   createInfo.imageArrayLayers = 1;
   createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   const uint32_t queueFamilyIndices[] = {
      m_device->GetGraphicsQueueFamily(),
      m_device->GetPresentQueueFamily()
   };
   if (m_device->GetGraphicsQueueFamily() != m_device->GetPresentQueueFamily()) {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
   } else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;
      createInfo.pQueueFamilyIndices = nullptr;
   }
   createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
   createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   createInfo.presentMode = presentMode;
   createInfo.clipped = VK_TRUE;
   createInfo.oldSwapchain = VK_NULL_HANDLE;
   if (vkCreateSwapchainKHR(m_device->Get(), &createInfo,
                            nullptr, &m_swapchain) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create swapchain.");
   }
   vkGetSwapchainImagesKHR(m_device->Get(), m_swapchain,
                           &imageCount, nullptr);
   m_images.resize(imageCount);
   vkGetSwapchainImagesKHR(m_device->Get(), m_swapchain,
                           &imageCount, m_images.data());
   m_format = surfaceFormat.format;
   m_extent = extent;
}

void VulkanSwapchain::CreateImageViews() {
   m_imageViews.resize(m_images.size());
   for (size_t i = 0; i < m_images.size(); ++i) {
      VkImageViewCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image = m_images[i];
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = m_format;
      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;
      if (vkCreateImageView(m_device->Get(), &createInfo,
                            nullptr, &m_imageViews[i]) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create image views.");
      }
   }
}

void VulkanSwapchain::Cleanup() {
   for (const VkImageView& imageView : m_imageViews) {
      vkDestroyImageView(m_device->Get(), imageView, nullptr);
   }
   vkDestroySwapchainKHR(m_device->Get(), m_swapchain, nullptr);
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseFormat(const std::vector<VkSurfaceFormatKHR>& formats) const{
   // Select an SRGB non-linear format for the surface
   for (const VkSurfaceFormatKHR& availableFormat : formats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
         return availableFormat;
      }
   }
   // Otherwise, get the first one
   return formats[0];
}

VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) const {
   for (const VkPresentModeKHR& availablePresentMode : modes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
         return availablePresentMode;
      }
   }
   return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
   if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
   } else {
      VkExtent2D actualExtent = {
         m_window->GetWidth(),
         m_window->GetHeight() 
      };
      actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                      capabilities.maxImageExtent.width);
      actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                       capabilities.maxImageExtent.height);
      return actualExtent;
   }
}

VkSwapchainKHR VulkanSwapchain::Get() const {
   return m_swapchain;
}

const std::vector<VkImage>& VulkanSwapchain::GetImages() const {
   return m_images;
}

const std::vector<VkImageView>& VulkanSwapchain::GetImageViews() const {
   return m_imageViews;
}

VkFormat VulkanSwapchain::GetFormat() const {
   return m_format;
}

VkExtent2D VulkanSwapchain::GetExtent() const {
   return m_extent;
}

