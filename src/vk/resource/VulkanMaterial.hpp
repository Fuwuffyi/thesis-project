#pragma once

#include "core/resource/MaterialInstance.hpp"
#include "vk/VulkanBuffer.hpp"

#include <memory>

class VulkanDevice;
class ResourceManager;

class VulkanMaterial final : public MaterialInstance {
  public:
   VulkanMaterial(const VulkanDevice& device, const MaterialTemplate& materialTemplate);
   ~VulkanMaterial() override = default;

   // MaterialInstance implementation
   void Bind(const uint32_t bindingPoint, const ResourceManager& resourceManager) override;
   void UpdateUBO() override;
   void* GetNativeHandle() const noexcept override;

   void CreateDescriptorSet(const VkDescriptorPool pool, const VkDescriptorSetLayout layout);
   VkDescriptorSet GetDescriptorSet() const noexcept { return m_descriptorSet; }
   void UpdateDescriptorSet(const ResourceManager& resourceManager);

  private:
   const VulkanDevice* m_device;
   std::unique_ptr<VulkanBuffer> m_uniformBuffer;
   VkDescriptorSet m_descriptorSet{VK_NULL_HANDLE};
   bool m_descriptorsDirty{true};
};
