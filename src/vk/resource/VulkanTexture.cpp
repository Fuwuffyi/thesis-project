#include "VulkanTexture.hpp"

#include "vk/VulkanCommandBuffers.hpp"
#include "vk/VulkanBuffer.hpp"
#include "vk/VulkanDevice.hpp"

#include <cmath>
#include <stb_image.h>
#include <stdexcept>

VulkanTexture::VulkanTexture(const VulkanDevice& device, const CreateInfo& info)
    : m_device(&device),
      m_width(info.width),
      m_height(info.height),
      m_depth(info.depth),
      m_format(info.format),
      m_samples(info.samples) {
   m_vkFormat = ConvertFormat(m_format);
   m_mipLevels = info.generateMipmaps
                    ? static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1
                    : 1;
   CreateImage();
   CreateImageView();
   m_sampler = CreateSampler();
}

VulkanTexture::VulkanTexture(const VulkanDevice& device, const std::string& filepath,
                             bool generateMipmaps, bool sRGB)
    : m_device(&device) {
   int32_t width, height, channels;
   stbi_set_flip_vertically_on_load(true);
   // Force 4 channels
   uint8_t* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
   if (!pixels) {
      throw std::runtime_error("Failed to load image: " + filepath);
   }
   m_width = static_cast<uint32_t>(width);
   m_height = static_cast<uint32_t>(height);
   m_depth = 1;
   m_format = sRGB ? Format::SRGB8_ALPHA8 : Format::RGBA8;
   m_vkFormat = ConvertFormat(m_format);
   m_mipLevels = generateMipmaps
                    ? static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1
                    : 1;
   // Create staging buffer
   VulkanBuffer stagingBuffer(device, m_width * m_height * 4, VulkanBuffer::Usage::TransferSrc,
                              VulkanBuffer::MemoryType::HostVisible);
   stagingBuffer.UpdateMapped(pixels, m_width * m_height * 4);
   stbi_image_free(pixels);
   CreateImage();
   TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
   CopyFromBuffer(stagingBuffer, 0);
   CreateImageView();
   m_sampler = CreateSampler();
   if (generateMipmaps) {
      GenerateMipmaps();
   } else {
      TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
   }
}

VulkanTexture::VulkanTexture(const VulkanDevice& device, uint32_t width, uint32_t height,
                             Format format, bool isDepth, uint32_t samples)
    : m_device(&device),
      m_width(width),
      m_height(height),
      m_depth(1),
      m_format(format),
      m_isDepth(isDepth),
      m_samples(samples),
      m_mipLevels(1) {
   m_vkFormat = ConvertFormat(m_format);
   CreateImage();
   CreateImageView();
}

VulkanTexture::~VulkanTexture() {
   if (m_sampler != VK_NULL_HANDLE) {
      vkDestroySampler(m_device->Get(), m_sampler, nullptr);
      m_sampler = VK_NULL_HANDLE;
   }
   if (m_imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(m_device->Get(), m_imageView, nullptr);
      m_imageView = VK_NULL_HANDLE;
   }
   if (m_image != VK_NULL_HANDLE) {
      vkDestroyImage(m_device->Get(), m_image, nullptr);
      m_image = VK_NULL_HANDLE;
   }
   if (m_imageMemory != VK_NULL_HANDLE) {
      vkFreeMemory(m_device->Get(), m_imageMemory, nullptr);
      m_imageMemory = VK_NULL_HANDLE;
   }
}

VulkanTexture::VulkanTexture(const VulkanDevice& device, const ITexture::Format format,
                             const glm::vec4& color)
    : m_device(&device),
      m_width(1),
      m_height(1),
      m_depth(1),
      m_format(format),
      m_isDepth(false),
      m_samples(1),
      m_mipLevels(1) {
   m_format = Format::RGBA8;
   m_vkFormat = ConvertFormat(m_format);

   CreateImage();
   CreateImageView();
   m_sampler = CreateSampler();

   uint8_t pixelData[4] = {static_cast<uint8_t>(glm::clamp(color.r * 255.0f, 0.0f, 255.0f)),
                           static_cast<uint8_t>(glm::clamp(color.g * 255.0f, 0.0f, 255.0f)),
                           static_cast<uint8_t>(glm::clamp(color.b * 255.0f, 0.0f, 255.0f)),
                           static_cast<uint8_t>(glm::clamp(color.a * 255.0f, 0.0f, 255.0f))};

   VulkanBuffer stagingBuffer(device, sizeof(pixelData), VulkanBuffer::Usage::TransferSrc,
                              VulkanBuffer::MemoryType::HostVisible);
   stagingBuffer.UpdateMapped(pixelData, sizeof(pixelData));

   TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
   CopyFromBuffer(stagingBuffer, 0);
   TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
    : m_device(other.m_device),
      m_image(other.m_image),
      m_imageView(other.m_imageView),
      m_imageMemory(other.m_imageMemory),
      m_vkFormat(other.m_vkFormat),
      m_width(other.m_width),
      m_height(other.m_height),
      m_depth(other.m_depth),
      m_format(other.m_format),
      m_isDepth(other.m_isDepth),
      m_samples(other.m_samples),
      m_mipLevels(other.m_mipLevels) {
   other.m_image = VK_NULL_HANDLE;
   other.m_imageView = VK_NULL_HANDLE;
   other.m_imageMemory = VK_NULL_HANDLE;
   other.m_sampler = VK_NULL_HANDLE;
   other.m_device = nullptr;
}

VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept {
   if (this != &other) {
      // Destroy any existing Vulkan resources
      if (m_sampler != VK_NULL_HANDLE) {
         vkDestroySampler(m_device->Get(), m_sampler, nullptr);
      }
      if (m_imageView != VK_NULL_HANDLE) {
         vkDestroyImageView(m_device->Get(), m_imageView, nullptr);
      }
      if (m_image != VK_NULL_HANDLE) {
         vkDestroyImage(m_device->Get(), m_image, nullptr);
      }
      if (m_imageMemory != VK_NULL_HANDLE) {
         vkFreeMemory(m_device->Get(), m_imageMemory, nullptr);
      }
      // Move all members
      m_device = other.m_device;
      m_image = other.m_image;
      m_imageView = other.m_imageView;
      m_imageMemory = other.m_imageMemory;
      m_sampler = other.m_sampler;
      m_vkFormat = other.m_vkFormat;
      m_width = other.m_width;
      m_height = other.m_height;
      m_depth = other.m_depth;
      m_format = other.m_format;
      m_isDepth = other.m_isDepth;
      m_samples = other.m_samples;
      m_mipLevels = other.m_mipLevels;
      // Null out the moved-from object
      other.m_device = nullptr;
      other.m_image = VK_NULL_HANDLE;
      other.m_imageView = VK_NULL_HANDLE;
      other.m_imageMemory = VK_NULL_HANDLE;
      other.m_sampler = VK_NULL_HANDLE;
   }
   return *this;
}

ResourceType VulkanTexture::GetType() const { return ResourceType::Texture; }

size_t VulkanTexture::GetMemoryUsage() const {
   size_t bytesPerPixel = 4;
   switch (m_format) {
      case Format::R8:
         bytesPerPixel = 1;
         break;
      case Format::RG8:
         bytesPerPixel = 2;
         break;
      case Format::RGB8:
         bytesPerPixel = 3;
         break;
      case Format::RGBA8:
         break;
      case Format::SRGB8_ALPHA8:
         bytesPerPixel = 4;
         break;
      case Format::RGBA16F:
         bytesPerPixel = 8;
         break;
      case Format::RGBA32F:
         bytesPerPixel = 16;
         break;
      case Format::Depth24:
         bytesPerPixel = 3;
         break;
      case Format::Depth32F:
         bytesPerPixel = 4;
         break;
   }
   size_t totalSize = 0;
   for (uint32_t i = 0; i < m_mipLevels; ++i) {
      const uint32_t mipWidth = std::max(1u, m_width >> i);
      const uint32_t mipHeight = std::max(1u, m_height >> i);
      totalSize += mipWidth * mipHeight * m_depth * bytesPerPixel;
   }
   return totalSize * m_samples;
}

bool VulkanTexture::IsValid() const {
   return m_image != VK_NULL_HANDLE && m_imageView != VK_NULL_HANDLE;
}

void VulkanTexture::CreateImage() {
   VkImageCreateInfo imageInfo{};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.extent.width = m_width;
   imageInfo.extent.height = m_height;
   imageInfo.extent.depth = m_depth;
   imageInfo.mipLevels = m_mipLevels;
   imageInfo.arrayLayers = 1;
   imageInfo.format = m_vkFormat;
   imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   imageInfo.samples = static_cast<VkSampleCountFlagBits>(m_samples);
   imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   if (m_isDepth) {
      imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
   } else {
      imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      if (m_mipLevels > 1) {
         imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
      }
   }
   if (vkCreateImage(m_device->Get(), &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image.");
   }
   // Allocate memory for the image
   VkMemoryRequirements memRequirements;
   vkGetImageMemoryRequirements(m_device->Get(), m_image, &memRequirements);
   VkMemoryAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize = memRequirements.size;
   // Find proper memory type for device local memory
   allocInfo.memoryTypeIndex =
      FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   if (vkAllocateMemory(m_device->Get(), &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate image memory.");
   }
   vkBindImageMemory(m_device->Get(), m_image, m_imageMemory, 0);
}

void VulkanTexture::CreateImageView() {
   VkImageViewCreateInfo viewInfo{};
   viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   viewInfo.image = m_image;
   viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   viewInfo.format = m_vkFormat;
   viewInfo.subresourceRange.aspectMask =
      m_isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
   viewInfo.subresourceRange.baseMipLevel = 0;
   viewInfo.subresourceRange.levelCount = m_mipLevels;
   viewInfo.subresourceRange.baseArrayLayer = 0;
   viewInfo.subresourceRange.layerCount = 1;
   if (vkCreateImageView(m_device->Get(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create texture image view!");
   }
}

VkSampler VulkanTexture::CreateSampler() {
   VkSamplerCreateInfo samplerInfo{};
   samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   samplerInfo.magFilter = VK_FILTER_LINEAR;
   samplerInfo.minFilter = VK_FILTER_LINEAR;
   samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.anisotropyEnable = VK_TRUE;
   VkPhysicalDeviceProperties properties{};
   vkGetPhysicalDeviceProperties(m_device->GetPhysicalDevice(), &properties);
   samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
   samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
   samplerInfo.unnormalizedCoordinates = VK_FALSE;
   samplerInfo.compareEnable = VK_FALSE;
   samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
   samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   samplerInfo.mipLodBias = 0.0f;
   samplerInfo.minLod = 0.0f;
   samplerInfo.maxLod = static_cast<float>(m_mipLevels);
   if (vkCreateSampler(m_device->Get(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create texture sampler!");
   }
   return m_sampler;
}

void VulkanTexture::TransitionLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout,
                                     const VkPipelineStageFlags srcStage,
                                     const VkPipelineStageFlags dstStage,
                                     const uint32_t baseMipLevel, const uint32_t levelCount) {
   VulkanCommandBuffers::ExecuteImmediate(
      *m_device, m_device->GetCommandPool(), m_device->GetGraphicsQueue(),
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
         if (m_isDepth) {
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (m_vkFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                m_vkFormat == VK_FORMAT_D24_UNORM_S8_UINT) {
               aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
         }
         barrier.subresourceRange.aspectMask = aspect;
         barrier.subresourceRange.baseMipLevel = baseMipLevel;
         barrier.subresourceRange.levelCount =
            (levelCount == VK_REMAINING_MIP_LEVELS) ? m_mipLevels - baseMipLevel : levelCount;
         barrier.subresourceRange.baseArrayLayer = 0;
         barrier.subresourceRange.layerCount = 1;
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
         vkCmdPipelineBarrier(cmdBuf, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
      });
}

void VulkanTexture::CopyFromBuffer(const VulkanBuffer& buffer, const uint32_t mipLevel) {
   VulkanCommandBuffers::ExecuteImmediate(
      *m_device, m_device->GetCommandPool(), m_device->GetGraphicsQueue(),
      [&](const VkCommandBuffer& cmdBuf) {
         VkBufferImageCopy region{};
         region.bufferOffset = 0;
         region.bufferRowLength = 0;
         region.bufferImageHeight = 0;
         region.imageSubresource.aspectMask =
            m_isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
         region.imageSubresource.mipLevel = mipLevel;
         region.imageSubresource.baseArrayLayer = 0;
         region.imageSubresource.layerCount = 1;
         region.imageOffset = {0, 0, 0};
         // Calculate extent for this mip level
         uint32_t mipWidth = std::max(1u, m_width >> mipLevel);
         uint32_t mipHeight = std::max(1u, m_height >> mipLevel);
         region.imageExtent = {mipWidth, mipHeight, 1};
         vkCmdCopyBufferToImage(cmdBuf, buffer.Get(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                1, &region);
      });
}

void VulkanTexture::GenerateMipmaps() {
   // Check if image format supports linear blitting
   if (!FormatSupportsBlitting(m_vkFormat)) {
      throw std::runtime_error("Format does not support linear blitting for mipmap generation.");
   }
   VulkanCommandBuffers::ExecuteImmediate(
      *m_device, m_device->GetCommandPool(), m_device->GetGraphicsQueue(),
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
         int32_t mipWidth = static_cast<int32_t>(m_width);
         int32_t mipHeight = static_cast<int32_t>(m_height);
         for (uint32_t i = 1; i < m_mipLevels; ++i) {
            // Transition level i-1 to SRC for reading
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &barrier);
            // Blit from level i-1 to level i
            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                                  mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            vkCmdBlitImage(cmdBuf, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                                 1, &barrier);
            if (mipWidth > 1)
               mipWidth /= 2;
            if (mipHeight > 1)
               mipHeight /= 2;
         }
         barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
         barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
         barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
         vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                              &barrier);
      });
}

bool VulkanTexture::FormatSupportsBlitting(const VkFormat format) const {
   VkFormatProperties formatProperties;
   vkGetPhysicalDeviceFormatProperties(m_device->GetPhysicalDevice(), format, &formatProperties);

   return (formatProperties.optimalTilingFeatures &
           VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
}

VkFormat VulkanTexture::ConvertFormat(const Format format) {
   switch (format) {
      case Format::R8:
         return VK_FORMAT_R8_UNORM;
      case Format::RG8:
         return VK_FORMAT_R8G8_UNORM;
      case Format::RGB8:
         return VK_FORMAT_R8G8B8_UNORM;
      case Format::RGBA8:
         return VK_FORMAT_R8G8B8A8_UNORM;
      case Format::RGBA16F:
         return VK_FORMAT_R16G16B16A16_SFLOAT;
      case Format::RGBA32F:
         return VK_FORMAT_R32G32B32A32_SFLOAT;
      case Format::SRGB8_ALPHA8:
         return VK_FORMAT_R8G8B8A8_SRGB;
      case Format::Depth24:
         return VK_FORMAT_D24_UNORM_S8_UINT;
      case Format::Depth32F:
         return VK_FORMAT_D32_SFLOAT;
      default:
         return VK_FORMAT_R8G8B8A8_UNORM;
   }
}

uint32_t VulkanTexture::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
   VkPhysicalDeviceMemoryProperties memProperties;
   vkGetPhysicalDeviceMemoryProperties(m_device->GetPhysicalDevice(), &memProperties);

   for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
         return i;
      }
   }

   throw std::runtime_error("Failed to find suitable memory type!");
}

uint32_t VulkanTexture::GetWidth() const { return m_width; }

uint32_t VulkanTexture::GetHeight() const { return m_height; }

uint32_t VulkanTexture::GetDepth() const { return m_depth; }

ITexture::Format VulkanTexture::GetFormat() const { return m_format; }

void VulkanTexture::Bind(const uint32_t unit) const {
   throw std::runtime_error("Bind method not implemented for vulkan texture.");
}

void* VulkanTexture::GetNativeHandle() const { return reinterpret_cast<void*>(m_image); }

VkImage VulkanTexture::GetImage() const { return m_image; }

VkImageView VulkanTexture::GetImageView() const { return m_imageView; }

VkFormat VulkanTexture::GetVkFormat() const { return m_vkFormat; }

VkSampler VulkanTexture::GetSampler() const { return m_sampler; }
