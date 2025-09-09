#pragma once

#include "core/resource/ITexture.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <string>

class VulkanDevice;
class VulkanBuffer;

class VulkanTexture : public ITexture {
  public:
   // Existing constructors
   VulkanTexture(const VulkanDevice& device, const CreateInfo& info);
   VulkanTexture(const VulkanDevice& device, const std::string& filepath,
                 const bool generateMipmaps, const bool sRGB);
   VulkanTexture(const VulkanDevice& device, const uint32_t width, const uint32_t height,
                 const Format format, const bool isDepth = false, const uint32_t samples = 1);
   VulkanTexture(const VulkanDevice& device, const ITexture::Format format, const glm::vec4& color);
   ~VulkanTexture();

   VulkanTexture(const VulkanTexture&) = delete;
   VulkanTexture& operator=(const VulkanTexture&) = delete;
   VulkanTexture(VulkanTexture&& other) noexcept;
   VulkanTexture& operator=(VulkanTexture&& other) noexcept;

   // ITexture implementation
   ResourceType GetType() const noexcept override;
   size_t GetMemoryUsage() const noexcept override;
   bool IsValid() const noexcept override;
   uint32_t GetWidth() const noexcept override;
   uint32_t GetHeight() const noexcept override;
   uint32_t GetDepth() const noexcept override;
   Format GetFormat() const noexcept override;
   void Bind(uint32_t unit = 0) const noexcept override;
   void* GetNativeHandle() const noexcept override;

   // Vulkan-specific accessors
   VkImage GetImage() const;
   VkImageView GetImageView() const;
   VkFormat GetVkFormat() const;
   VkSampler GetSampler() const;

   // Descriptor management within texture
   VkDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }
   void CreateDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool,
                            VkDescriptorSetLayout layout);
   void UpdateDescriptorSet(VkDevice device);
   VkDescriptorImageInfo GetDescriptorImageInfo() const;

   // Texture state transitions
   void TransitionLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout,
                         const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage,
                         const uint32_t baseMipLevel = 0,
                         const uint32_t levelCount = VK_REMAINING_MIP_LEVELS);

   // Enhanced sampler configuration
   void UpdateSamplerSettings(VkFilter minFilter, VkFilter magFilter,
                              VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                              bool enableAnisotropy = true, float maxAnisotropy = 16.0f);

  private:
   // Existing private methods
   void CopyFromBuffer(const VulkanBuffer& buffer, const uint32_t mipLevel = 0);
   void GenerateMipmaps();
   VkSampler CreateSampler();
   VkFormat ConvertFormat(const Format format);
   void CreateImage();
   void AllocateMemory();
   void CreateImageView();
   uint32_t FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties);
   void LoadFromFile(const std::string& filepath, const bool generateMipmaps, const bool sRGB);
   bool FormatSupportsBlitting(const VkFormat format) const;

   // Enhanced sampler creation with custom parameters
   VkSampler CreateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode,
                           bool enableAnisotropy, float maxAnisotropy);

  private:
   const VulkanDevice* m_device{nullptr};
   VkImage m_image{VK_NULL_HANDLE};
   VkImageView m_imageView{VK_NULL_HANDLE};
   VkDeviceMemory m_imageMemory{VK_NULL_HANDLE};
   VkSampler m_sampler{VK_NULL_HANDLE};
   VkFormat m_vkFormat;

   // Descriptor set managed by texture
   VkDescriptorSet m_descriptorSet{VK_NULL_HANDLE};
   bool m_descriptorNeedsUpdate{true};

   uint32_t m_width = 0;
   uint32_t m_height = 0;
   uint32_t m_depth = 1;
   Format m_format = Format::RGBA8;
   bool m_isDepth = false;
   uint32_t m_samples = 1;
   uint32_t m_mipLevels = 1;

   // Enhanced sampler state tracking
   VkFilter m_minFilter = VK_FILTER_LINEAR;
   VkFilter m_magFilter = VK_FILTER_LINEAR;
   VkSamplerAddressMode m_addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   bool m_anisotropyEnabled = true;
   float m_maxAnisotropy = 16.0f;
};
