#pragma once

#include "core/resource/ITexture.hpp"

#include <vulkan/vulkan.h>
#include <string>

class VulkanDevice;
class VulkanBuffer;

class VulkanTexture : public ITexture {
  public:
   VulkanTexture(const VulkanDevice& device, const CreateInfo& info);
   VulkanTexture(const VulkanDevice& device, const std::string& filepath,
                 const bool generateMipmaps, const bool sRGB);
   VulkanTexture(const VulkanDevice& device, const uint32_t width, const uint32_t height,
                 const Format format, const bool isDepth = false, const uint32_t samples = 1);
   ~VulkanTexture();

   VulkanTexture(const VulkanTexture&) = delete;
   VulkanTexture& operator=(const VulkanTexture&) = delete;
   VulkanTexture(VulkanTexture&& other) noexcept;
   VulkanTexture& operator=(VulkanTexture&& other) noexcept;

   ResourceType GetType() const override;
   size_t GetMemoryUsage() const override;
   bool IsValid() const override;

   uint32_t GetWidth() const override;
   uint32_t GetHeight() const override;
   uint32_t GetDepth() const override;
   Format GetFormat() const override;
   void Bind(uint32_t unit = 0) const override;
   void* GetNativeHandle() const override;

   VkImage GetImage() const;
   VkImageView GetImageView() const;
   VkFormat GetVkFormat() const;

   void TransitionLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout,
                         const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage,
                         const uint32_t baseMipLevel = 0,
                         const uint32_t levelCount = VK_REMAINING_MIP_LEVELS);

  private:
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

  private:
   const VulkanDevice* m_device = nullptr;
   VkImage m_image = VK_NULL_HANDLE;
   VkImageView m_imageView = VK_NULL_HANDLE;
   VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
   VkSampler m_sampler = VK_NULL_HANDLE;
   VkFormat m_vkFormat;

   uint32_t m_width = 0;
   uint32_t m_height = 0;
   uint32_t m_depth = 1;
   Format m_format = Format::RGBA8;
   bool m_isDepth = false;
   uint32_t m_samples = 1;
   uint32_t m_mipLevels = 1;
};
