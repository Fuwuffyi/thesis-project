#pragma once

#include <vulkan/vulkan.h>
#include <string>

class VulkanDevice;
class VulkanBuffer;

class VulkanImage {
public:
   enum class Usage : VkImageUsageFlags {
      ColorAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      DepthStencilAttachment = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      Sampled = VK_IMAGE_USAGE_SAMPLED_BIT,
      Storage = VK_IMAGE_USAGE_STORAGE_BIT,
      TransferSrc = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      TransferDst = VK_IMAGE_USAGE_TRANSFER_DST_BIT
   };

   enum class MemoryType {
      DeviceLocal,
      HostVisible,
      HostCoherent
   };

   VulkanImage(const VulkanDevice& device, const uint32_t width, const uint32_t height,
               const VkFormat format, const VkImageTiling tiling, const Usage usage,
               const MemoryType memoryType, const uint32_t mipLevels = 1,
               const uint32_t arrayLayers = 1, const VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

   VulkanImage(const VulkanDevice& device, const std::string& filepath,
               const bool generateMipmaps = true, const bool sRGB = true);
   ~VulkanImage();

   VulkanImage(const VulkanImage&) = delete;
   VulkanImage& operator=(const VulkanImage&) = delete;
   VulkanImage(VulkanImage&& other) noexcept;
   VulkanImage& operator=(VulkanImage&& other) noexcept;

   VkImage Get() const;
   VkImageView GetView() const;
   VkDeviceMemory GetMemory() const;
   VkFormat GetFormat() const;
   VkExtent3D GetExtent() const;
   uint32_t GetMipLevels() const;
   uint32_t GetArrayLayers() const;

   void TransitionLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout,
                         const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage,
                         const uint32_t baseMipLevel = 0, const uint32_t levelCount = VK_REMAINING_MIP_LEVELS);
   void CopyFromBuffer(const VulkanBuffer& buffer, const uint32_t mipLevel = 0);
   void GenerateMipmaps();

   static uint32_t CalculateMipLevels(const uint32_t width, const uint32_t height);

   static VkImageView CreateImageView(const VulkanDevice& device, const VkImage image,
                                      const VkFormat format, const VkImageAspectFlags aspectFlags,
                                      const uint32_t mipLevels = 1, const uint32_t arrayLayers = 1);

private:
   void CreateImage();
   void AllocateMemory();
   void CreateImageView();
   void LoadFromFile(const std::string& filepath, const bool generateMipmaps, const bool sRGB);
   VkMemoryPropertyFlags GetMemoryPropertiesFromType(const MemoryType type) const;
   bool FormatSupportsBlitting(const VkFormat format) const;

private:
   const VulkanDevice* m_device = nullptr;
   VkImage m_image = VK_NULL_HANDLE;
   VkImageView m_imageView = VK_NULL_HANDLE;
   VkDeviceMemory m_memory = VK_NULL_HANDLE;
   VkFormat m_format;
   VkExtent3D m_extent;
   VkImageTiling m_tiling;
   Usage m_usage;
   VkMemoryPropertyFlags m_memoryProperties;
   uint32_t m_mipLevels;
   uint32_t m_arrayLayers;
   VkSampleCountFlagBits m_samples;
};

// Bitwise operators for Usage enum
inline VulkanImage::Usage operator|(VulkanImage::Usage a, VulkanImage::Usage b) {
   return static_cast<VulkanImage::Usage>(
      static_cast<VkImageUsageFlags>(a) | static_cast<VkImageUsageFlags>(b)
   );
}

