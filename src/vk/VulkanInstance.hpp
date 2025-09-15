#pragma once

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

class VulkanInstance {
  public:
   VulkanInstance();
   ~VulkanInstance();

   VulkanInstance(VulkanInstance&& other) noexcept;
   VulkanInstance& operator=(VulkanInstance&& other) noexcept;
   VulkanInstance(const VulkanInstance&) = delete;
   VulkanInstance& operator=(const VulkanInstance&) = delete;

   VkInstance Get() const;
   const vkb::Instance& GetVkbInstance() const;

  private:
   vkb::Instance m_vkbInstance;
   VkInstance m_instance{VK_NULL_HANDLE};
   bool m_ownsInstance{false};
};
