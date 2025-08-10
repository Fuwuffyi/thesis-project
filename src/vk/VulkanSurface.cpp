#include "VulkanSurface.hpp"

#include "VulkanInstance.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>

VulkanSurface::VulkanSurface(const VulkanInstance& instance, GLFWwindow* window)
   :
   m_instance(&instance)
{
   if (glfwCreateWindowSurface(m_instance->Get(), window,
                               nullptr, &m_surface) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create window surface.");
   }
}

VulkanSurface::~VulkanSurface() {
   if (m_surface != VK_NULL_HANDLE && m_instance != nullptr) {
      vkDestroySurfaceKHR(m_instance->Get(), m_surface, nullptr);
   }
}

VulkanSurface::VulkanSurface(VulkanSurface&& other) noexcept 
   :
   m_instance(other.m_instance),
   m_surface(other.m_surface)
{
   other.m_instance = nullptr;
   other.m_surface = VK_NULL_HANDLE;
}

VulkanSurface& VulkanSurface::operator=(VulkanSurface&& other) noexcept {
   if (this != &other) {
      if (m_surface != VK_NULL_HANDLE && m_instance != nullptr) {
         vkDestroySurfaceKHR(m_instance->Get(), m_surface, nullptr);
      }
      m_instance = other.m_instance;
      m_surface = other.m_surface;
      other.m_instance = nullptr;
      other.m_surface = VK_NULL_HANDLE;
   }
   return *this;
}

VkSurfaceKHR VulkanSurface::Get() const {
   return m_surface;
}

