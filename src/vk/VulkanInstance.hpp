#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VulkanInstance {
public:
   VulkanInstance(const std::vector<const char*>& extensions,
                  const std::vector<const char*>& validationLayers,
                  bool enableValidation);
   ~VulkanInstance();

   VulkanInstance(const VulkanInstance&) = delete;
   VulkanInstance& operator=(const VulkanInstance&) = delete;
   VulkanInstance(VulkanInstance&& other) noexcept;
   VulkanInstance& operator=(VulkanInstance&& other) noexcept;

   VkInstance Get() const;
private:
   VkInstance m_instance = VK_NULL_HANDLE;

   std::vector<const char*> GetRequiredExtensions(bool enableValidation) const;
   bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers) const;
};

