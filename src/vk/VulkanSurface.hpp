#pragma once

#include "VulkanInstance.hpp"

struct GLFWwindow;

class VulkanSurface {
public:
   VulkanSurface(const VulkanInstance& instance, GLFWwindow* window);
   ~VulkanSurface();

   VulkanSurface(const VulkanSurface&) = delete;
   VulkanSurface& operator=(const VulkanSurface&) = delete;
   VulkanSurface(VulkanSurface&& other) noexcept;
   VulkanSurface& operator=(VulkanSurface&& other) noexcept;

   VkSurfaceKHR Get() const;

private:
   const VulkanInstance* m_instance = nullptr;
   VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

