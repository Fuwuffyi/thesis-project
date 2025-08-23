#include "VulkanTexture.hpp"

#include "../VulkanDevice.hpp"
#include "../VulkanBuffer.hpp"
#include "../VulkanCommandBuffers.hpp"

#include <stdexcept>
#include <algorithm>
#include <cmath>

#include <stb_image.h>

VulkanTexture::VulkanTexture(const VulkanDevice& device, const uint32_t width, const uint32_t height,
                             const VkFormat format, const VkImageTiling tiling, const Usage usage,
                             const MemoryType memoryType, const uint32_t mipLevels, const uint32_t arrayLayers,
                             const VkSampleCountFlagBits samples)
   :
   m_device(&device),
   m_format(format),
   m_extent{width, height, 1},
   m_tiling(tiling),
   m_usage(usage),
   m_mipLevels(mipLevels),
   m_arrayLayers(arrayLayers),
   m_samples(samples),
   m_memoryProperties(GetMemoryPropertiesFromType(memoryType))
{
   CreateImage();
   AllocateMemory();
   CreateImageView();
}

VulkanTexture::VulkanTexture(const VulkanDevice& device, const std::string& filepath,
                             const bool generateMipmaps, const bool sRGB)
   :
   m_device(&device),
   m_tiling(VK_IMAGE_TILING_OPTIMAL),
   m_usage(Usage::TransferDst | Usage::Sampled), m_arrayLayers(1),
   m_samples(VK_SAMPLE_COUNT_1_BIT),
   m_memoryProperties(GetMemoryPropertiesFromType(MemoryType::DeviceLocal))
{
   LoadFromFile(filepath, generateMipmaps, sRGB);
}

VulkanTexture::~VulkanTexture() {
   if (m_imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(m_device->Get(), m_imageView, nullptr);
   }
   if (m_image != VK_NULL_HANDLE) {
      vkDestroyImage(m_device->Get(), m_image, nullptr);
   }
   if (m_memory != VK_NULL_HANDLE) {
      vkFreeMemory(m_device->Get(), m_memory, nullptr);
   }
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
   :
   m_device(other.m_device),
   m_image(other.m_image),
   m_imageView(other.m_imageView),
   m_memory(other.m_memory),
   m_format(other.m_format),
   m_extent(other.m_extent),
   m_tiling(other.m_tiling),
   m_usage(other.m_usage),
   m_memoryProperties(other.m_memoryProperties),
   m_mipLevels(other.m_mipLevels),
   m_arrayLayers(other.m_arrayLayers),
   m_samples(other.m_samples)
{
   other.m_device = nullptr;
   other.m_image = VK_NULL_HANDLE;
   other.m_imageView = VK_NULL_HANDLE;
   other.m_memory = VK_NULL_HANDLE;
}

VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept {
   if (this != &other) {
      if (m_imageView != VK_NULL_HANDLE) {
         vkDestroyImageView(m_device->Get(), m_imageView, nullptr);
      }
      if (m_image != VK_NULL_HANDLE) {
         vkDestroyImage(m_device->Get(), m_image, nullptr);
      }
      if (m_memory != VK_NULL_HANDLE) {
         vkFreeMemory(m_device->Get(), m_memory, nullptr);
      }
      m_device = other.m_device;
      m_image = other.m_image;
      m_imageView = other.m_imageView;
      m_memory = other.m_memory;
      m_format = other.m_format;
      m_extent = other.m_extent;
      m_tiling = other.m_tiling;
      m_usage = other.m_usage;
      m_memoryProperties = other.m_memoryProperties;
      m_mipLevels = other.m_mipLevels;
      m_arrayLayers = other.m_arrayLayers;
      m_samples = other.m_samples;
      other.m_device = nullptr;
      other.m_image = VK_NULL_HANDLE;
      other.m_imageView = VK_NULL_HANDLE;
      other.m_memory = VK_NULL_HANDLE;
   }
   return *this;
}

void VulkanTexture::TransitionLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout,
                                     const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage,
                                     const uint32_t baseMipLevel, const uint32_t levelCount) {

   VulkanCommandBuffers::ExecuteImmediate(*m_device,
                                          m_device->GetCommandPool(),
                                          m_device->GetGraphicsQueue(),
                                          [&](const VkCommandBuffer& cmdBuf) {
                                          VkImageMemoryBarrier barrier{};
                                          barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                          barrier.oldLayout = oldLayout;
                                          barrier.newLayout = newLayout;
                                          barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                          barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                          barrier.image = m_image;
                                          // Pick the correct aspect mask
                                          VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
                                          if (m_format == VK_FORMAT_D32_SFLOAT ||
                                          m_format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                                          m_format == VK_FORMAT_D24_UNORM_S8_UINT) {
                                          aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
                                          if (m_format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                                          m_format == VK_FORMAT_D24_UNORM_S8_UINT) {
                                          aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
                                          }
                                          }
                                          barrier.subresourceRange.aspectMask = aspect;
                                          barrier.subresourceRange.baseMipLevel = baseMipLevel;
                                          barrier.subresourceRange.levelCount = (levelCount == VK_REMAINING_MIP_LEVELS)
                                          ? m_mipLevels - baseMipLevel
                                          : levelCount;
                                          barrier.subresourceRange.baseArrayLayer = 0;
                                          barrier.subresourceRange.layerCount = m_arrayLayers;
                                          // Access masks for common transitions
                                          if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                                          newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                                          barrier.srcAccessMask = 0;
                                          barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                          } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                                          newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                                          barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                          barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                          } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                                          newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
                                          barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                          barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                                          } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
                                          newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                                          barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                                          barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                          } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                                          newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                                          barrier.srcAccessMask = 0;
                                          barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                                          } else {
                                          throw std::invalid_argument("Unsupported layout transition.");
                                          }
                                          vkCmdPipelineBarrier(cmdBuf, srcStage, dstStage, 0,
                                                               0, nullptr, 0, nullptr, 1, &barrier);
                                          });
}

void VulkanTexture::CopyFromBuffer(const VulkanBuffer& buffer, const uint32_t mipLevel) {
   VulkanCommandBuffers::ExecuteImmediate(*m_device,
                                          m_device->GetCommandPool(),
                                          m_device->GetGraphicsQueue(),
                                          [&](const VkCommandBuffer& cmdBuf) {
                                          VkBufferImageCopy region{};
                                          region.bufferOffset = 0;
                                          region.bufferRowLength = 0;
                                          region.bufferImageHeight = 0;
                                          region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                          region.imageSubresource.mipLevel = mipLevel;
                                          region.imageSubresource.baseArrayLayer = 0;
                                          region.imageSubresource.layerCount = 1;
                                          region.imageOffset = {0, 0, 0};
                                          // Calculate extent for this mip level
                                          uint32_t mipWidth = std::max(1u, m_extent.width >> mipLevel);
                                          uint32_t mipHeight = std::max(1u, m_extent.height >> mipLevel);
                                          region.imageExtent = {mipWidth, mipHeight, 1};

                                          vkCmdCopyBufferToImage(cmdBuf, buffer.Get(), m_image,
                                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                                          });
}

void VulkanTexture::GenerateMipmaps() {
   // Check if image format supports linear blitting
   if (!FormatSupportsBlitting(m_format)) {
      throw std::runtime_error("Format does not support linear blitting for mipmap generation.");
   }
   VulkanCommandBuffers::ExecuteImmediate(*m_device,
                                          m_device->GetCommandPool(),
                                          m_device->GetGraphicsQueue(),
                                          [&](const VkCommandBuffer& cmdBuf) {
                                          VkImageMemoryBarrier barrier{};
                                          barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                          barrier.image = m_image;
                                          barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                          barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                          barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                          barrier.subresourceRange.baseArrayLayer = 0;
                                          barrier.subresourceRange.layerCount = 1;
                                          barrier.subresourceRange.levelCount = 1;
                                          int32_t mipWidth = static_cast<int32_t>(m_extent.width);
                                          int32_t mipHeight = static_cast<int32_t>(m_extent.height);
                                          for (uint32_t i = 1; i < m_mipLevels; ++i) {
                                          // Transition level i-1 to SRC for reading
                                          barrier.subresourceRange.baseMipLevel = i - 1;
                                          barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                                          barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                                          barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                          barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                                          vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                                                               VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                                               0, nullptr, 0, nullptr, 1, &barrier);

                                          // Blit from level i-1 to level i
                                          VkImageBlit blit{};
                                          blit.srcOffsets[0] = {0, 0, 0};
                                          blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                                          blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                          blit.srcSubresource.mipLevel = i - 1;
                                          blit.srcSubresource.baseArrayLayer = 0;
                                          blit.srcSubresource.layerCount = 1;
                                          blit.dstOffsets[0] = {0, 0, 0};
                                          blit.dstOffsets[1] = {
                                          mipWidth > 1 ? mipWidth / 2 : 1,
                                          mipHeight > 1 ? mipHeight / 2 : 1,
                                          1
                                          };
                                          blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                          blit.dstSubresource.mipLevel = i;
                                          blit.dstSubresource.baseArrayLayer = 0;
                                          blit.dstSubresource.layerCount = 1;
                                          vkCmdBlitImage(cmdBuf, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                         m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                         1, &blit, VK_FILTER_LINEAR);

                                          barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                                          barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                          barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                                          barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                          vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                                               0, nullptr, 0, nullptr, 1, &barrier);

                                          if (mipWidth > 1) mipWidth /= 2;
                                          if (mipHeight > 1) mipHeight /= 2;
                                          }
                                          // Transition the last mip level to SHADER_READ_ONLY
                                          barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
                                          barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                                          barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                          barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                          barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                          vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                                               0, nullptr, 0, nullptr, 1, &barrier);
                                          });
}

uint32_t VulkanTexture::CalculateMipLevels(const uint32_t width, const uint32_t height) {
   return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

VkImageView VulkanTexture::CreateImageView(const VulkanDevice& device, const VkImage image,
                                           const VkFormat format, const VkImageAspectFlags aspectFlags,
                                           const uint32_t mipLevels, const uint32_t arrayLayers) {
   VkImageViewCreateInfo viewInfo{};
   viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   viewInfo.image = image;
   viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   viewInfo.format = format;
   viewInfo.subresourceRange.aspectMask = aspectFlags;
   viewInfo.subresourceRange.baseMipLevel = 0;
   viewInfo.subresourceRange.levelCount = mipLevels;
   viewInfo.subresourceRange.baseArrayLayer = 0;
   viewInfo.subresourceRange.layerCount = arrayLayers;
   VkImageView imageView;
   if (vkCreateImageView(device.Get(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image view.");
   }
   return imageView;
}

void VulkanTexture::CreateImage() {
   VkImageCreateInfo imageInfo{};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.extent = m_extent;
   imageInfo.mipLevels = m_mipLevels;
   imageInfo.arrayLayers = m_arrayLayers;
   imageInfo.format = m_format;
   imageInfo.tiling = m_tiling;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   imageInfo.usage = static_cast<VkImageUsageFlags>(m_usage);
   imageInfo.samples = m_samples;
   imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   if (vkCreateImage(m_device->Get(), &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image.");
   }
}

void VulkanTexture::AllocateMemory() {
   VkMemoryRequirements memRequirements;
   vkGetImageMemoryRequirements(m_device->Get(), m_image, &memRequirements);
   VkMemoryAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize = memRequirements.size;
   allocInfo.memoryTypeIndex = VulkanBuffer::FindMemoryType(
      m_device->GetPhysicalDevice(), memRequirements.memoryTypeBits, m_memoryProperties);
   if (vkAllocateMemory(m_device->Get(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate image memory.");
   }
   vkBindImageMemory(m_device->Get(), m_image, m_memory, 0);
}

void VulkanTexture::CreateImageView() {
   VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
   // Determine aspect flags based on format
   if (m_format == VK_FORMAT_D32_SFLOAT || m_format == VK_FORMAT_D32_SFLOAT_S8_UINT || 
      m_format == VK_FORMAT_D24_UNORM_S8_UINT) {
      aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
      if (m_format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_format == VK_FORMAT_D24_UNORM_S8_UINT) {
         aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
   }
   m_imageView = CreateImageView(*m_device, m_image, m_format, aspectFlags, 
                                 m_mipLevels, m_arrayLayers);
}

void VulkanTexture::LoadFromFile(const std::string& filepath, const bool generateMipmaps, const bool sRGB) {
   int32_t texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   if (!pixels) throw std::runtime_error("Failed to load texture image: " + filepath);
   m_extent = { (uint32_t)texWidth, (uint32_t)texHeight, 1 };
   m_format = sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
   if (generateMipmaps) { m_mipLevels = CalculateMipLevels(m_extent.width, m_extent.height);
      m_usage = Usage::TransferSrc | Usage::TransferDst | Usage::Sampled;
   } else { m_mipLevels = 1; m_usage = Usage::TransferDst | Usage::Sampled; }
   // Create the VkImage and allocate/bind memory before any transition/copy
   CreateImage();
   AllocateMemory();
   // Stage pixels
   const VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);
   VulkanBuffer stagingBuffer(*m_device, imageSize, VulkanBuffer::Usage::TransferSrc,
                              VulkanBuffer::MemoryType::HostVisible);
   stagingBuffer.Update(pixels, imageSize);
   stbi_image_free(pixels);
   // Transitions/copy/mips
   TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
   CopyFromBuffer(stagingBuffer, 0);
   if (generateMipmaps) {
      GenerateMipmaps();
   } else {
      TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
   }
   // Create the view after the image exists
   CreateImageView();
}

VkMemoryPropertyFlags VulkanTexture::GetMemoryPropertiesFromType(const MemoryType type) const {
   switch (type) {
      case MemoryType::DeviceLocal:
         return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      case MemoryType::HostVisible:
         return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      case MemoryType::HostCoherent:
         return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      default:
         return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   }
}

bool VulkanTexture::FormatSupportsBlitting(const VkFormat format) const {
   VkFormatProperties formatProperties;
   vkGetPhysicalDeviceFormatProperties(m_device->GetPhysicalDevice(), format,
                                       &formatProperties);

   return (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
}

VkImage VulkanTexture::Get() const {
   return m_image;
}

VkImageView VulkanTexture::GetView() const {
   return m_imageView;
}

VkDeviceMemory VulkanTexture::GetMemory() const {
   return m_memory;
}

VkFormat VulkanTexture::GetFormat() const {
   return m_format;
}

VkExtent3D VulkanTexture::GetExtent() const {
   return m_extent;
}

uint32_t VulkanTexture::GetMipLevels() const {
   return m_mipLevels;
}

uint32_t VulkanTexture::GetArrayLayers() const {
   return m_arrayLayers;
}

