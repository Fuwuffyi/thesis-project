#include "vk/VulkanSwapchain.hpp"
#include <VkBootstrap.h>

#include "vk/VulkanDevice.hpp"
#include "vk/VulkanSurface.hpp"

#include "core/Window.hpp"

#include <stdexcept>

VulkanSwapchain::VulkanSwapchain(const VulkanDevice& device, const VulkanSurface& surface,
                                 const Window& window)
    : m_device(&device), m_surface(&surface), m_window(&window) {
   CreateSwapchain();
   CreateImageViews();
}

VulkanSwapchain::~VulkanSwapchain() { Cleanup(); }

VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& other) noexcept
    : m_device(other.m_device),
      m_surface(other.m_surface),
      m_window(other.m_window),
      m_vkbSwapchain(std::move(other.m_vkbSwapchain)),
      m_swapchain(other.m_swapchain),
      m_format(other.m_format),
      m_extent(other.m_extent),
      m_images(std::move(other.m_images)),
      m_imageViews(std::move(other.m_imageViews)),
      m_ownsSwapchain(other.m_ownsSwapchain) {
   other.m_device = nullptr;
   other.m_surface = nullptr;
   other.m_window = nullptr;
   other.m_swapchain = VK_NULL_HANDLE;
   other.m_format = VK_FORMAT_UNDEFINED;
   other.m_extent = {0, 0};
   other.m_ownsSwapchain = false;
}

VulkanSwapchain& VulkanSwapchain::operator=(VulkanSwapchain&& other) noexcept {
   if (this != &other) {
      Cleanup();
      m_device = other.m_device;
      m_surface = other.m_surface;
      m_window = other.m_window;
      m_vkbSwapchain = std::move(other.m_vkbSwapchain);
      m_swapchain = other.m_swapchain;
      m_format = other.m_format;
      m_extent = other.m_extent;
      m_images = std::move(other.m_images);
      m_imageViews = std::move(other.m_imageViews);
      m_ownsSwapchain = other.m_ownsSwapchain;
      other.m_device = nullptr;
      other.m_surface = nullptr;
      other.m_window = nullptr;
      other.m_swapchain = VK_NULL_HANDLE;
      other.m_format = VK_FORMAT_UNDEFINED;
      other.m_extent = {0, 0};
      other.m_ownsSwapchain = false;
   }
   return *this;
}

void VulkanSwapchain::Recreate() {
   vkDeviceWaitIdle(m_device->Get());
   Cleanup();
   CreateSwapchain();
   CreateImageViews();
}

VkResult VulkanSwapchain::AcquireNextImage(const uint64_t timeout, const VkSemaphore& semaphore,
                                           uint32_t* imageIndex) const {
   return vkAcquireNextImageKHR(m_device->Get(), m_swapchain, timeout, semaphore, VK_NULL_HANDLE,
                                imageIndex);
}

void VulkanSwapchain::CreateSwapchain() {
   // vk-bootstrap dramatically simplifies swapchain creation
   vkb::SwapchainBuilder swapchain_builder{m_device->GetPhysicalDevice(), m_device->Get(),
                                           m_surface->Get()};
   const auto swap_ret =
      swapchain_builder
         .set_desired_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
         .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
         .set_desired_extent(m_window->GetWidth(), m_window->GetHeight())
         .add_fallback_format({VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
         .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)
         .build();
   if (!swap_ret) {
      throw std::runtime_error("Failed to create swapchain: " + swap_ret.error().message());
   }
   m_vkbSwapchain = swap_ret.value();
   m_swapchain = m_vkbSwapchain.swapchain;
   m_format = m_vkbSwapchain.image_format;
   m_extent = m_vkbSwapchain.extent;
   m_images = m_vkbSwapchain.get_images().value();
   m_ownsSwapchain = true;
}

void VulkanSwapchain::CreateImageViews() {
   const auto image_views_ret = m_vkbSwapchain.get_image_views();
   if (!image_views_ret) {
      throw std::runtime_error("Failed to create swapchain image views: " +
                               image_views_ret.error().message());
   }
   m_imageViews = image_views_ret.value();
}

void VulkanSwapchain::Cleanup() {
   for (VkImageView imageView : m_imageViews) {
      vkDestroyImageView(m_device->Get(), imageView, nullptr);
   }
   m_imageViews.clear();
   if (m_ownsSwapchain) {
      vkb::destroy_swapchain(m_vkbSwapchain);
      m_ownsSwapchain = false;
   }
   m_swapchain = VK_NULL_HANDLE;
   m_images.clear();
}

VkSwapchainKHR VulkanSwapchain::Get() const { return m_swapchain; }

const std::vector<VkImage>& VulkanSwapchain::GetImages() const { return m_images; }

const std::vector<VkImageView>& VulkanSwapchain::GetImageViews() const { return m_imageViews; }

VkFormat VulkanSwapchain::GetFormat() const { return m_format; }

VkExtent2D VulkanSwapchain::GetExtent() const { return m_extent; }
