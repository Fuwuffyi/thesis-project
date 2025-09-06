#include "vk/resource/VulkanMaterial.hpp"

#include "core/resource/ResourceManager.hpp"

#include "vk/VulkanDevice.hpp"
#include "vk/resource/VulkanTexture.hpp"

#include <stdexcept>
#include <array>

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
   // TODO: In Vulkan, "binding" is typically handled through descriptor sets
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

void VulkanMaterial::UpdateDescriptorSets(const ResourceManager& resourceManager, VkDevice device,
                                          VkDescriptorPool descriptorPool,
                                          VkDescriptorSetLayout descriptorSetLayout) {
   if (!m_descriptorSetNeedsUpdate) {
      return;
   }

   // Allocate descriptor set if needed
   if (m_descriptorSet == VK_NULL_HANDLE) {
      VkDescriptorSetAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool = descriptorPool;
      allocInfo.descriptorSetCount = 1;
      allocInfo.pSetLayouts = &descriptorSetLayout;

      if (vkAllocateDescriptorSets(device, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
         throw std::runtime_error("Failed to allocate descriptor set for material");
      }
   }
   std::vector<VkWriteDescriptorSet> descriptorWrites;
   std::vector<VkDescriptorBufferInfo> bufferInfos;
   std::vector<VkDescriptorImageInfo> imageInfos;
   // Update uniform buffer descriptor
   if (m_uniformBuffer) {
      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = m_uniformBuffer->Get();
      bufferInfo.offset = 0;
      bufferInfo.range = GetUBOSize();
      bufferInfos.push_back(bufferInfo);
      VkWriteDescriptorSet descriptorWrite{};
      descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrite.dstSet = m_descriptorSet;
      descriptorWrite.dstBinding = 0; // Assuming UBO is at binding 0
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pBufferInfo = &bufferInfos.back();
      descriptorWrites.push_back(descriptorWrite);
   }
   // Update texture descriptors
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
      // Create descriptor for the texture
      if (texture && texture->IsValid()) {
         VulkanTexture* vkTexture = reinterpret_cast<VulkanTexture*>(texture);
         VkDescriptorImageInfo imageInfo{};
         imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         imageInfo.imageView = vkTexture->GetImageView();
         imageInfo.sampler = vkTexture->GetSampler();
         imageInfos.push_back(imageInfo);
         VkWriteDescriptorSet descriptorWrite{};
         descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
         descriptorWrite.dstSet = m_descriptorSet;
         descriptorWrite.dstBinding = descriptor.bindingSlot;
         descriptorWrite.dstArrayElement = 0;
         descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
         descriptorWrite.descriptorCount = 1;
         descriptorWrite.pImageInfo = &imageInfos.back();
         descriptorWrites.push_back(descriptorWrite);
      }
   }
   // Update all descriptors at once
   if (!descriptorWrites.empty()) {
      vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
   }
   m_descriptorSetNeedsUpdate = false;
}
