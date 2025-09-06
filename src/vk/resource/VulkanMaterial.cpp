#include "vk/resource/VulkanMaterial.hpp"

#include "core/resource/ResourceManager.hpp"
#include "vk/VulkanDevice.hpp"
#include "vk/resource/VulkanTexture.hpp"

#include <stdexcept>

VulkanMaterial::VulkanMaterial(const VulkanDevice& device, const MaterialTemplate& materialTemplate)
    : MaterialInstance(materialTemplate), m_device(&device) {
   // Create uniform buffer if UBO size > 0
   const size_t uboSize = GetUBOSize();
   if (uboSize > 0) {
      m_uniformBuffer = std::make_unique<VulkanBuffer>(
         device, uboSize, VulkanBuffer::Usage::Uniform, VulkanBuffer::MemoryType::HostVisible);
   }
}

void VulkanMaterial::Bind(const uint32_t bindingPoint, const ResourceManager& resourceManager) {
   // Update UBO data if dirty
   UpdateUBO();

   // NOTE: In Vulkan, "binding" is typically handled through descriptor sets
   // The actual binding happens during command buffer recording via vkCmdBindDescriptorSets
}

void VulkanMaterial::UpdateUBO() {
   if (!IsUBODirty() || !m_uniformBuffer) {
      return;
   }

   // Update UBO data in the base class
   UpdateUBOData();

   // Upload data to GPU
   const void* uboData = GetUBOData();
   const size_t uboSize = GetUBOSize();

   if (uboData && uboSize > 0) {
      m_uniformBuffer->UpdateMapped(uboData, uboSize);
   }

   ClearDirty();
}

void* VulkanMaterial::GetNativeHandle() const {
   if (m_uniformBuffer) {
      return reinterpret_cast<void*>(m_uniformBuffer->Get());
   }
   return nullptr;
}

VkBuffer VulkanMaterial::GetUniformBuffer() const {
   return m_uniformBuffer ? m_uniformBuffer->Get() : VK_NULL_HANDLE;
}

std::vector<VkDescriptorSet> VulkanMaterial::GetTextureDescriptorSets(
   const ResourceManager& resourceManager, VkDevice device, VkDescriptorPool descriptorPool,
   VkDescriptorSetLayout textureLayout) const {
   std::vector<VkDescriptorSet> descriptorSets;
   const auto& textureDescriptors = m_template->GetTextures();
   for (const auto& [textureName, descriptor] : textureDescriptors) {
      const TextureHandle& textureHandle = GetTexture(textureName);
      // Try to get the texture from the handle first
      ITexture* texture = nullptr;
      if (textureHandle.IsValid()) {
         texture = resourceManager.GetTexture(textureHandle);
      }
      // If no texture is set or texture is invalid, try to use default
      if (!texture && descriptor.defaultTexture.IsValid()) {
         texture = resourceManager.GetTexture(descriptor.defaultTexture);
      }
      if (texture && texture->IsValid()) {
         VulkanTexture* vkTexture = reinterpret_cast<VulkanTexture*>(texture);
         // Ensure texture has descriptor set
         vkTexture->CreateDescriptorSet(device, descriptorPool, textureLayout);
         vkTexture->UpdateDescriptorSet(device);
         VkDescriptorSet textureDescSet = vkTexture->GetDescriptorSet();
         if (textureDescSet != VK_NULL_HANDLE) {
            descriptorSets.push_back(textureDescSet);
         }
      }
   }
   return descriptorSets;
}
