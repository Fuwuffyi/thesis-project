#pragma once

#include "../core/IRenderer.hpp"
#include "VulkanInstance.hpp"
#include "VulkanDebugMessenger.hpp"
#include "VulkanSurface.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanCommandBuffers.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanBuffer.hpp"

#include "core/editor/MaterialEditor.hpp"
#include "core/resource/ResourceManager.hpp"
#include "resource/VulkanTexture.hpp"

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

struct CameraData {
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
};

struct ObjectData {
   alignas(16) glm::mat4 model;
};

class VulkanRenderer : public IRenderer {
  public:
   VulkanRenderer(Window* window);
   ~VulkanRenderer();
   void RenderFrame() override;

  private:
   void SetupImgui() override;
   void RenderImgui() override;
   void DestroyImgui() override;
   // Functions to set up a graphics pipeline
   void CreateGraphicsPipeline();
   // Descriptor set for pipeline
   void CreateDescriptorSetLayout();
   void CreateDescriptorPool();
   void CreateDescriptorSets();
   // Functions to set up framebuffers
   void CreateFramebuffers();
   // Functions to set up command pool
   void CreateCommandBuffers();
   void RecordCommandBuffer(const uint32_t imageIndex);
   // Functions to set up synchronization for drawing
   void CreateSynchronizationObjects();
   // Functions to setup swapchain recreation
   void RecreateSwapchain();
   void CleanupSwapchain();
   // Setup for depth texture
   void CreateDepthResources();
   VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,
                                const VkImageTiling& tiling,
                                const VkFormatFeatureFlags features) const;
   VkFormat FindDepthFormat() const;
   bool HasStencilComponent(const VkFormat& format) const;
   [[nodiscard]] ResourceManager* GetResourceManager() const noexcept override;
   // TODO: Remove once scene impl complete
   // Functions to create textures
   // Testing mesh
   void CreateTestResources();
   void CreateDefaultMaterial();
   void CreateUniformBuffer();
   void UpdateUniformBuffer(const uint32_t currentImage);

  private:
   constexpr static uint32_t MAX_FRAMES_IN_FLIGHT = 2;
   uint32_t m_currentFrame = 0;
   VulkanInstance m_instance;
#ifndef NDEBUG
   VulkanDebugMessenger m_debugMessenger;
#endif
   VulkanSurface m_surface;
   VulkanDevice m_device;
   VulkanSwapchain m_swapchain;
   VulkanRenderPass m_renderPass;
   std::vector<VkFramebuffer> m_swapchainFramebuffers;
   VkDescriptorSetLayout m_descriptorSetLayout;
   std::unique_ptr<VulkanPipelineLayout> m_pipelineLayout;
   std::unique_ptr<VulkanGraphicsPipeline> m_graphicsPipeline;
   // TODO: Remove unique ptrs in favour of stack variables once other abstractions are implemented
   std::unique_ptr<VulkanCommandBuffers> m_commandBuffers;
   std::vector<VkSemaphore> m_imageAvailableSemaphores;
   std::vector<VkSemaphore> m_renderFinishedSemaphores;
   std::vector<VkFence> m_inFlightFences;
   TextureHandle m_depthTexture;
   TextureHandle m_texture;
   std::vector<std::unique_ptr<VulkanBuffer>> m_uniformBuffers;
   std::vector<void*> m_uniformBuffersMapped;
   VkDescriptorPool m_descriptorPool;
   std::vector<VkDescriptorSet> m_descriptorSets;
   std::unique_ptr<ResourceManager> m_resourceManager;
   std::unique_ptr<MaterialEditor> m_materialEditor;
};
