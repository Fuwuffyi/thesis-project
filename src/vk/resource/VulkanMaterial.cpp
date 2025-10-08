#include "vk/resource/VulkanMaterial.hpp"

#include "core/resource/ResourceManager.hpp"
#include "vk/VulkanDevice.hpp"
#include "vk/resource/VulkanTexture.hpp"

VulkanMaterial::VulkanMaterial(const VulkanDevice& device, const MaterialTemplate& materialTemplate)
    : MaterialInstance(materialTemplate), m_device(&device) {
   // Create uniform buffer if UBO size > 0
   const size_t uboSize = GetUBOSize();
   if (uboSize > 0) {
      m_uniformBuffer = std::make_unique<VulkanBuffer>(
         device, uboSize, VulkanBuffer::Usage::Uniform, VulkanBuffer::MemoryType::CPUToGPU);
      m_uniformBuffer->Map();
   }
}

void VulkanMaterial::Bind(const uint32_t bindingPoint, const ResourceManager& resourceManager) {
   // Update UBO data if dirty
   UpdateUBO();
   if (m_descriptorsDirty && m_descriptorSet != VK_NULL_HANDLE) {
      UpdateDescriptorSet(resourceManager);
      m_descriptorsDirty = false;
   }
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
      m_uniformBuffer->Map();
      m_uniformBuffer->Update(uboData, uboSize);
   }
   ClearDirty();
   m_descriptorsDirty = true;
}

void VulkanMaterial::CreateDescriptorSet(const VkDescriptorPool pool,
                                         const VkDescriptorSetLayout layout) {
   VkDescriptorSetAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfo.descriptorPool = pool;
   allocInfo.descriptorSetCount = 1;
   allocInfo.pSetLayouts = &layout;
   if (vkAllocateDescriptorSets(m_device->Get(), &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate material descriptor set");
   }
   m_descriptorsDirty = true;
}

void VulkanMaterial::UpdateDescriptorSet(const ResourceManager& resourceManager) {
   if (m_descriptorSet == VK_NULL_HANDLE) {
      return;
   }
   std::vector<VkWriteDescriptorSet> writes;
   std::vector<VkDescriptorBufferInfo> bufferInfos;
   std::vector<VkDescriptorImageInfo> imageInfos;
   // Reserve space to avoid reallocation
   const auto& textureDescriptors = m_template->GetTextures();
   bufferInfos.reserve(1);
   imageInfos.reserve(textureDescriptors.size());
   // Update UBO if present
   if (m_uniformBuffer && GetUBOSize() > 0) {
      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = m_uniformBuffer->Get();
      bufferInfo.offset = 0;
      bufferInfo.range = GetUBOSize();
      bufferInfos.push_back(bufferInfo);
      VkWriteDescriptorSet write{};
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = m_descriptorSet;
      write.dstBinding = 0; // UBO binding
      write.dstArrayElement = 0;
      write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write.descriptorCount = 1;
      write.pBufferInfo = &bufferInfos.back();
      writes.push_back(write);
   }
   // Update textures
   for (const auto& [textureName, descriptor] : textureDescriptors) {
      ITexture* texture = nullptr;
      const TextureHandle& th = GetTexture(textureName);
      if (th.IsValid()) {
         texture = resourceManager.GetTexture(th);
      }
      if (!texture && descriptor.defaultTexture.IsValid()) {
         texture = resourceManager.GetTexture(descriptor.defaultTexture);
      }
      if (texture && texture->IsValid()) {
         const VulkanTexture* vkTexture = reinterpret_cast<const VulkanTexture*>(texture);
         VkDescriptorImageInfo imageInfo{};
         imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         imageInfo.imageView = vkTexture->GetImageView();
         imageInfo.sampler = vkTexture->GetSampler();
         imageInfos.push_back(imageInfo);
         VkWriteDescriptorSet write{};
         write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
         write.dstSet = m_descriptorSet;
         write.dstBinding = descriptor.bindingSlot;
         write.dstArrayElement = 0;
         write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
         write.descriptorCount = 1;
         write.pImageInfo = &imageInfos.back();
         writes.push_back(write);
      }
   }
   if (!writes.empty()) {
      vkUpdateDescriptorSets(m_device->Get(), static_cast<uint32_t>(writes.size()), writes.data(),
                             0, nullptr);
   }
}

void* VulkanMaterial::GetNativeHandle() const noexcept {
   if (m_uniformBuffer) {
      return reinterpret_cast<void*>(m_uniformBuffer->Get());
   }
   return nullptr;
}
