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
   void* GetNativeHandle() const override;

   // Vulkan-specific methods
   VkBuffer GetUniformBuffer() const;

   // Get all texture descriptor sets for binding
   std::vector<VkDescriptorSet> GetTextureDescriptorSets(const ResourceManager& resourceManager,
                                                         VkDevice device,
                                                         VkDescriptorPool descriptorPool,
                                                         VkDescriptorSetLayout textureLayout) const;

  private:
   const VulkanDevice* m_device;
   std::unique_ptr<VulkanBuffer> m_uniformBuffer;
};
