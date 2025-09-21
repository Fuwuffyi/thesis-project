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
      m_uniformBuffer->Map();
      m_uniformBuffer->Update(uboData, uboSize);
   }

   ClearDirty();
}

void* VulkanMaterial::GetNativeHandle() const noexcept {
   if (m_uniformBuffer) {
      return reinterpret_cast<void*>(m_uniformBuffer->Get());
   }
   return nullptr;
}
