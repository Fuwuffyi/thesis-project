#pragma once

#include "core/resource/ITexture.hpp"
#include "vk/VulkanDevice.hpp"

#include <glm/glm.hpp>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>
#include <utility>

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
   VulkanTexture(const VulkanDevice& device, const Format format, const glm::vec4& color);
   ~VulkanTexture();

   VulkanTexture(const VulkanTexture&) = delete;
   VulkanTexture& operator=(const VulkanTexture&) = delete;
   VulkanTexture(VulkanTexture&&) noexcept;
   VulkanTexture& operator=(VulkanTexture&&) noexcept;

   [[nodiscard]] ResourceType GetType() const noexcept override { return ResourceType::Texture; }
   [[nodiscard]] size_t GetMemoryUsage() const noexcept override;
   [[nodiscard]] bool IsValid() const noexcept override { return m_image && m_imageView; }
   [[nodiscard]] uint32_t GetWidth() const noexcept override { return m_width; }
   [[nodiscard]] uint32_t GetHeight() const noexcept override { return m_height; }
   [[nodiscard]] uint32_t GetDepth() const noexcept override { return m_depth; }
   [[nodiscard]] Format GetFormat() const noexcept override { return m_format; }
   void Bind(const uint32_t) const noexcept override { std::unreachable(); }
   [[nodiscard]] void* GetNativeHandle() const noexcept override {
      return reinterpret_cast<void*>(m_image);
   }

   [[nodiscard]] VkImage GetImage() const noexcept { return m_image; }
   [[nodiscard]] VkImageView GetImageView() const noexcept { return m_imageView; }
   [[nodiscard]] VkFormat GetVkFormat() const noexcept { return m_vkFormat; }
   [[nodiscard]] VkSampler GetSampler() const noexcept { return m_sampler; }
   [[nodiscard]] VkDescriptorSet GetImguiDescriptor() const noexcept {
      if (!m_descriptorSetDirty)
         return m_imguiDescriptorSet;
      if (m_imguiDescriptorSet != VK_NULL_HANDLE)
         ImGui_ImplVulkan_RemoveTexture(m_imguiDescriptorSet);
      m_imguiDescriptorSet = ImGui_ImplVulkan_AddTexture(m_sampler, m_imageView,
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      return m_imguiDescriptorSet;
   }
   void UpdateSamplerSettings(
      const VkFilter minFilter, const VkFilter magFilter,
      const VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      const bool enableAnisotropy = true, const float maxAnisotropy = 16.0f);

   void TransitionLayout(const VkImageLayout oldL, const VkImageLayout newL,
                         const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage,
                         const uint32_t baseMip = 0,
                         const uint32_t levelCount = VK_REMAINING_MIP_LEVELS);

  private:
   [[nodiscard]] VkFormat ConvertFormat(const Format fmt) const;
   [[nodiscard]] static constexpr size_t BytesPerPixel(const Format fmt) noexcept;
   [[nodiscard]] VkSampler CreateSampler(const VkFilter minF, const VkFilter magF,
                                         const VkSamplerAddressMode addr, bool enableAnisotropy,
                                         const float maxAnisotropy) const;
   void CreateImage();
   void CreateImageView();
   void CopyFromBuffer(const VulkanBuffer& buffer, const uint32_t mipLevel = 0);
   void GenerateMipmaps();

  private:
   const VulkanDevice* m_device{nullptr};
   VkImage m_image{VK_NULL_HANDLE};
   VkImageView m_imageView{VK_NULL_HANDLE};
   VkSampler m_sampler{VK_NULL_HANDLE};
   VmaAllocation m_allocation{nullptr};
   VkFormat m_vkFormat;

   bool m_descriptorSetDirty{true};
   mutable VkDescriptorSet m_imguiDescriptorSet{VK_NULL_HANDLE};

   uint32_t m_width{};
   uint32_t m_height{};
   uint32_t m_depth{1};
   uint32_t m_mipLevels{1};
   uint32_t m_samples{1};
   Format m_format{Format::RGBA8};
   bool m_isDepth{};
};
