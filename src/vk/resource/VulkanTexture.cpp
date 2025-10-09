#include "vk/resource/VulkanTexture.hpp"

#include "vk/VulkanDevice.hpp"
#include "vk/VulkanBuffer.hpp"
#include "vk/VulkanCommandBuffers.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <stb_image.h>

#include <stdexcept>
#include <cstring>
#include <cmath>
#include <algorithm>

VulkanTexture::VulkanTexture(const VulkanDevice& device, const CreateInfo& info)
    : m_device(&device),
      m_width(info.width),
      m_height(info.height),
      m_depth(info.depth),
      m_format((info.format == Format::RGB8) ? Format::RGBA8 : info.format),
      m_isDepth(info.format == Format::Depth24 || info.format == Format::Depth32F),
      m_samples(info.samples),
      m_mipLevels(
         info.generateMipmaps
            ? static_cast<uint32_t>(std::floor(std::log2(std::max(info.width, info.height)))) + 1
            : 1) {
   m_vkFormat = ConvertFormat(m_format);
   CreateImage();
   CreateImageView();
   UpdateSamplerSettings(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
}

VulkanTexture::VulkanTexture(const VulkanDevice& device, const std::string& filepath,
                             const bool generateMipmaps, const bool sRGB)
    : m_device(&device) {
   int texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, 0);
   if (!pixels)
      throw std::runtime_error("Failed to load texture image: " + filepath);

   m_width = static_cast<uint32_t>(texWidth);
   m_height = static_cast<uint32_t>(texHeight);
   m_depth = 1;
   m_format = (texChannels == 4) ? Format::RGBA8 : Format::RGB8;
   m_vkFormat = ConvertFormat(m_format);
   m_mipLevels = generateMipmaps
                    ? static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1
                    : 1;
   VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_width) * m_height * BytesPerPixel(m_format);

   VulkanBuffer staging(*m_device, imageSize, VulkanBuffer::Usage::TransferSrc,
                        VulkanBuffer::MemoryType::CPUToGPU);
   staging.Map();
   staging.Update(pixels, imageSize);
   stbi_image_free(pixels);
   CreateImage();
   CopyFromBuffer(staging);
   if (generateMipmaps)
      GenerateMipmaps();
   else
      TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
   CreateImageView();
   UpdateSamplerSettings(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
}

VulkanTexture::VulkanTexture(const VulkanDevice& device, const uint32_t width,
                             const uint32_t height, const Format format, const bool isDepth,
                             const uint32_t samples)
    : m_device(&device),
      m_width(width),
      m_height(height),
      m_depth(1),
      m_format((format == Format::RGB8) ? Format::RGBA8 : format),
      m_isDepth(isDepth),
      m_samples(samples),
      m_mipLevels(1) {
   m_vkFormat = ConvertFormat(m_format);
   CreateImage();
   CreateImageView();
   UpdateSamplerSettings(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
}

VulkanTexture::VulkanTexture(const VulkanDevice& device, const Format format,
                             const glm::vec4& color)
    : m_device(&device),
      m_width(1),
      m_height(1),
      m_depth(1),
      m_format((format == Format::RGB8) ? Format::RGBA8 : format),
      m_isDepth(false),
      m_samples(1),
      m_mipLevels(1) {
   m_vkFormat = ConvertFormat(m_format);
   CreateImage();
   VulkanBuffer staging(*m_device, BytesPerPixel(m_format), VulkanBuffer::Usage::TransferSrc,
                        VulkanBuffer::MemoryType::CPUToGPU);
   staging.Map();
   staging.Update(&color[0], BytesPerPixel(m_format));
   CopyFromBuffer(staging);
   TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
   CreateImageView();
   UpdateSamplerSettings(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
}

VulkanTexture::~VulkanTexture() {
   if (m_sampler)
      vkDestroySampler(m_device->Get(), m_sampler, nullptr);
   if (m_imageView)
      vkDestroyImageView(m_device->Get(), m_imageView, nullptr);
   if (m_image != VK_NULL_HANDLE)
      vmaDestroyImage(m_device->GetAllocator(), m_image, m_allocation);
   if (m_imguiDescriptorSet != VK_NULL_HANDLE)
      ImGui_ImplVulkan_RemoveTexture(m_imguiDescriptorSet);
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept { *this = std::move(other); }

VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept {
   if (this != &other) {
      if (m_sampler)
         vkDestroySampler(m_device->Get(), m_sampler, nullptr);
      if (m_imageView)
         vkDestroyImageView(m_device->Get(), m_imageView, nullptr);
      if (m_image != VK_NULL_HANDLE)
         vmaDestroyImage(m_device->GetAllocator(), m_image, m_allocation);
      if (m_imguiDescriptorSet != VK_NULL_HANDLE)
         ImGui_ImplVulkan_RemoveTexture(m_imguiDescriptorSet);
      m_device = other.m_device;
      m_image = std::exchange(other.m_image, VK_NULL_HANDLE);
      m_allocation = std::exchange(other.m_allocation, nullptr);
      m_imageView = std::exchange(other.m_imageView, VK_NULL_HANDLE);
      m_sampler = std::exchange(other.m_sampler, VK_NULL_HANDLE);
      m_vkFormat = other.m_vkFormat;
      m_width = other.m_width;
      m_height = other.m_height;
      m_depth = other.m_depth;
      m_mipLevels = other.m_mipLevels;
      m_samples = other.m_samples;
      m_format = other.m_format;
      m_isDepth = other.m_isDepth;
   }
   return *this;
}

size_t VulkanTexture::GetMemoryUsage() const noexcept {
   return static_cast<size_t>(m_width) * m_height * m_depth * BytesPerPixel(m_format);
}

VkFormat VulkanTexture::ConvertFormat(const Format fmt) const {
   switch (fmt) {
      case Format::R8:
         return VK_FORMAT_R8_UNORM;
      case Format::RG8:
         return VK_FORMAT_R8G8_UNORM;
      case Format::RGB8:
         return VK_FORMAT_R8G8B8_UNORM;
      case Format::RGBA8:
         return VK_FORMAT_R8G8B8A8_UNORM;
      case Format::SRGB8_ALPHA8:
         return VK_FORMAT_R8G8B8A8_SRGB;
      case Format::RGBA16F:
         return VK_FORMAT_R16G16B16A16_SFLOAT;
      case Format::RGBA32F:
         return VK_FORMAT_R32G32B32A32_SFLOAT;
      case Format::Depth24:
         return VK_FORMAT_D24_UNORM_S8_UINT;
      case Format::Depth32F:
         return VK_FORMAT_D32_SFLOAT;
      default:
         throw std::runtime_error("Unsupported format");
   }
}

constexpr size_t VulkanTexture::BytesPerPixel(const Format fmt) noexcept {
   switch (fmt) {
      case Format::R8:
         return 1;
      case Format::RG8:
         return 2;
      case Format::RGB8:
         return 3;
      case Format::RGBA8:
         return 4;
      case Format::SRGB8_ALPHA8:
         return 4;
      case Format::RGBA16F:
         return 8;
      case Format::RGBA32F:
         return 16;
      case Format::Depth24:
         return 4;
      case Format::Depth32F:
         return 4;
      default:
         return 4;
   }
}

VkSampler VulkanTexture::CreateSampler(const VkFilter minF, const VkFilter magF,
                                       const VkSamplerAddressMode addr, bool enableAnisotropy,
                                       const float maxAnisotropy) const {
   VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
   info.magFilter = magF;
   info.minFilter = minF;
   info.addressModeU = addr;
   info.addressModeV = addr;
   info.addressModeW = addr;
   info.anisotropyEnable = enableAnisotropy ? VK_TRUE : VK_FALSE;
   info.maxAnisotropy = enableAnisotropy ? maxAnisotropy : 1.0f;
   info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
   info.unnormalizedCoordinates = VK_FALSE;
   info.compareEnable = VK_FALSE;
   info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   info.minLod = 0.0f;
   info.maxLod = static_cast<float>(m_mipLevels);

   VkSampler sampler;
   if (vkCreateSampler(m_device->Get(), &info, nullptr, &sampler) != VK_SUCCESS)
      throw std::runtime_error("Failed to create sampler");
   return sampler;
}

void VulkanTexture::UpdateSamplerSettings(const VkFilter minFilter, const VkFilter magFilter,
                                          const VkSamplerAddressMode addressMode,
                                          const bool enableAnisotropy, const float maxAnisotropy) {
   if (m_sampler != VK_NULL_HANDLE)
      vkDestroySampler(m_device->Get(), m_sampler, nullptr);
   m_sampler = CreateSampler(minFilter, magFilter, addressMode, enableAnisotropy, maxAnisotropy);
   m_descriptorSetDirty = true;
}

void VulkanTexture::CreateImage() {
   VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.extent = {m_width, m_height, m_depth};
   imageInfo.mipLevels = m_mipLevels;
   imageInfo.arrayLayers = 1;
   imageInfo.format = m_vkFormat;
   imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
   if (m_mipLevels > 1)
      imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
   if (m_isDepth) {
      imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
   } else {
      imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   }
   imageInfo.samples = static_cast<VkSampleCountFlagBits>(m_samples);
   imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   VmaAllocationCreateInfo allocInfo{};
   allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
   if (vmaCreateImage(m_device->GetAllocator(), &imageInfo, &allocInfo, &m_image, &m_allocation,
                      nullptr) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image with VMA");
   }
}

void VulkanTexture::CreateImageView() {
   VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
   viewInfo.image = m_image;
   viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   viewInfo.format = m_vkFormat;
   viewInfo.subresourceRange.aspectMask =
      m_isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
   viewInfo.subresourceRange.baseMipLevel = 0;
   viewInfo.subresourceRange.levelCount = m_mipLevels;
   viewInfo.subresourceRange.baseArrayLayer = 0;
   viewInfo.subresourceRange.layerCount = 1;

   if (vkCreateImageView(m_device->Get(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS)
      throw std::runtime_error("Failed to create image view");
}

void VulkanTexture::TransitionLayout(const VkImageLayout oldL, const VkImageLayout newL,
                                     const VkPipelineStageFlags srcStage,
                                     const VkPipelineStageFlags dstStage, const uint32_t baseMip,
                                     const uint32_t levelCount) {
   VulkanCommandBuffers::ExecuteImmediate(
      *m_device, m_device->GetCommandPool(), m_device->GetGraphicsQueue(),
      [&](VkCommandBuffer cmd) {
         VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
         barrier.oldLayout = oldL;
         barrier.newLayout = newL;
         barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         barrier.image = m_image;
         barrier.subresourceRange.aspectMask =
            m_isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
         barrier.subresourceRange.baseMipLevel = baseMip;
         barrier.subresourceRange.levelCount = levelCount;
         barrier.subresourceRange.baseArrayLayer = 0;
         barrier.subresourceRange.layerCount = 1;
         if (oldL == VK_IMAGE_LAYOUT_UNDEFINED) {
            barrier.srcAccessMask = 0;
         } else if (oldL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         } else if (oldL == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
         }
         if (newL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         } else if (newL == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
         }
         vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
      });
}

void VulkanTexture::CopyFromBuffer(const VulkanBuffer& buffer, const uint32_t mipLevel) {
   VulkanCommandBuffers::ExecuteImmediate(
      *m_device, m_device->GetCommandPool(), m_device->GetGraphicsQueue(),
      [&](VkCommandBuffer cmd) {
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
         region.imageExtent = {m_width >> mipLevel, m_height >> mipLevel, m_depth};

         TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                          mipLevel, 1);

         vkCmdCopyBufferToImage(cmd, buffer.Get(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                &region);
      });
}

void VulkanTexture::GenerateMipmaps() {
   VulkanCommandBuffers::ExecuteImmediate(
      *m_device, m_device->GetCommandPool(), m_device->GetGraphicsQueue(),
      [&](VkCommandBuffer cmd) {
         int32_t mipW = static_cast<int32_t>(m_width);
         int32_t mipH = static_cast<int32_t>(m_height);

         for (uint32_t i = 1; i < m_mipLevels; i++) {
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.image = m_image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipW, mipH, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;

            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {std::max(mipW / 2, 1), std::max(mipH / 2, 1), 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                                 1, &barrier);

            mipW = std::max(mipW / 2, 1);
            mipH = std::max(mipH / 2, 1);
         }

         VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
         barrier.image = m_image;
         barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
         barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
         barrier.subresourceRange.levelCount = 1;
         barrier.subresourceRange.baseArrayLayer = 0;
         barrier.subresourceRange.layerCount = 1;
         barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
         barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

         vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                              &barrier);
      });
}
