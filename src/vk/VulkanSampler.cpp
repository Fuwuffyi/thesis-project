#include "VulkanSampler.hpp"
#include "VulkanDevice.hpp"

#include <stdexcept>

VulkanSampler::VulkanSampler(const VulkanDevice& device, const SamplerCreateInfo& createInfo)
:
   m_device(&device)
{
   VkPhysicalDeviceProperties properties{};
   vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &properties);
   VkSamplerCreateInfo samplerInfo{};
   samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   samplerInfo.magFilter = createInfo.magFilter;
   samplerInfo.minFilter = createInfo.minFilter;
   samplerInfo.addressModeU = createInfo.addressModeU;
   samplerInfo.addressModeV = createInfo.addressModeV;
   samplerInfo.addressModeW = createInfo.addressModeW;
   samplerInfo.anisotropyEnable = createInfo.anisotropyEnable;
   samplerInfo.maxAnisotropy = createInfo.anisotropyEnable ? 
      std::min(createInfo.maxAnisotropy, properties.limits.maxSamplerAnisotropy) : 1.0f;
   samplerInfo.borderColor = createInfo.borderColor;
   samplerInfo.unnormalizedCoordinates = createInfo.unnormalizedCoordinates;
   samplerInfo.compareEnable = createInfo.compareEnable;
   samplerInfo.compareOp = createInfo.compareOp;
   samplerInfo.mipmapMode = createInfo.mipmapMode;
   samplerInfo.mipLodBias = createInfo.mipLodBias;
   samplerInfo.minLod = createInfo.minLod;
   samplerInfo.maxLod = createInfo.maxLod;
   if (vkCreateSampler(device.Get(), &samplerInfo,
                       nullptr, &m_sampler) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create texture sampler.");
   }
}

VulkanSampler::~VulkanSampler() {
   if (m_sampler != VK_NULL_HANDLE) {
      vkDestroySampler(m_device->Get(), m_sampler, nullptr);
   }
}

VulkanSampler::VulkanSampler(VulkanSampler&& other) noexcept
:
   m_device(other.m_device),
   m_sampler(other.m_sampler)
{
   other.m_device = nullptr;
   other.m_sampler = VK_NULL_HANDLE;
}

VulkanSampler& VulkanSampler::operator=(VulkanSampler&& other) noexcept {
   if (this != &other) {
      if (m_sampler != VK_NULL_HANDLE) {
         vkDestroySampler(m_device->Get(), m_sampler, nullptr);
      }
      m_device = other.m_device;
      m_sampler = other.m_sampler;
      other.m_device = nullptr;
      other.m_sampler = VK_NULL_HANDLE;
   }
   return *this;
}

VulkanSampler VulkanSampler::CreateLinear(const VulkanDevice& device, const float maxLod) {
   SamplerCreateInfo createInfo{};
   createInfo.magFilter = VK_FILTER_LINEAR;
   createInfo.minFilter = VK_FILTER_LINEAR;
   createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   createInfo.anisotropyEnable = VK_FALSE;
   createInfo.maxLod = maxLod;
   return VulkanSampler(device, createInfo);
}

VulkanSampler VulkanSampler::CreateNearest(const VulkanDevice& device, const float maxLod) {
   SamplerCreateInfo createInfo{};
   createInfo.magFilter = VK_FILTER_NEAREST;
   createInfo.minFilter = VK_FILTER_NEAREST;
   createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
   createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   createInfo.anisotropyEnable = VK_FALSE;
   createInfo.maxLod = maxLod;
   return VulkanSampler(device, createInfo);
}

VulkanSampler VulkanSampler::CreateAnisotropic(const VulkanDevice& device, const float maxAnisotropy, const float maxLod) {
   SamplerCreateInfo createInfo{};
   createInfo.magFilter = VK_FILTER_LINEAR;
   createInfo.minFilter = VK_FILTER_LINEAR;
   createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   createInfo.anisotropyEnable = VK_TRUE;
   createInfo.maxAnisotropy = maxAnisotropy;
   createInfo.maxLod = maxLod;
   return VulkanSampler(device, createInfo);
}

