#pragma once

#include <vulkan/vulkan.h>

class VulkanDevice;

struct SamplerCreateInfo {
   VkFilter magFilter = VK_FILTER_LINEAR;
   VkFilter minFilter = VK_FILTER_LINEAR;
   VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   float mipLodBias = 0.0f;
   VkBool32 anisotropyEnable = VK_TRUE;
   float maxAnisotropy = 1.0f;
   VkBool32 compareEnable = VK_FALSE;
   VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
   float minLod = 0.0f;
   float maxLod = VK_LOD_CLAMP_NONE;
   VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
   VkBool32 unnormalizedCoordinates = VK_FALSE;
};

class VulkanSampler {
  public:
   VulkanSampler(const VulkanDevice& device, const SamplerCreateInfo& createInfo = {});
   ~VulkanSampler();

   VulkanSampler(const VulkanSampler&) = delete;
   VulkanSampler& operator=(const VulkanSampler&) = delete;
   VulkanSampler(VulkanSampler&& other) noexcept;
   VulkanSampler& operator=(VulkanSampler&& other) noexcept;

   VkSampler Get() const { return m_sampler; }

   static VulkanSampler CreateLinear(const VulkanDevice& device,
                                     const float maxLod = VK_LOD_CLAMP_NONE);
   static VulkanSampler CreateNearest(const VulkanDevice& device,
                                      const float maxLod = VK_LOD_CLAMP_NONE);
   static VulkanSampler CreateAnisotropic(const VulkanDevice& device, const float maxAnisotropy,
                                          const float maxLod = VK_LOD_CLAMP_NONE);

  private:
   const VulkanDevice* m_device = nullptr;
   VkSampler m_sampler = VK_NULL_HANDLE;
};
