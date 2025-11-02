#pragma once

#include "core/IRenderer.hpp"
#include "vk/VulkanInstance.hpp"
#include "vk/VulkanSurface.hpp"
#include "vk/VulkanDevice.hpp"
#include "vk/VulkanSwapchain.hpp"
#include "vk/VulkanPipeline.hpp"
#include "vk/VulkanCommandBuffers.hpp"
#include "vk/VulkanRenderPass.hpp"
#include "vk/VulkanBuffer.hpp"

#include "core/ThreadPool.hpp"
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

class VulkanRenderer : public IRenderer {
  public:
   VulkanRenderer(Window* window);
   ~VulkanRenderer();
   void RenderFrame() override;

   [[nodiscard]] const VulkanDevice& GetDevice() const noexcept;

  private:
   void SetupImgui() override;
   void RenderImgui() override;
   void DestroyImgui() override;

   void CreateUBOs();

   void UpdateCameraUBO(const uint32_t currentImage);
   void UpdateLightsUBO(const uint32_t currentImage);

   // Material setup
   void CreateMaterialDescriptorSetLayout();
   void CreateMaterialDescriptorPool();
   void SetupMaterialDescriptorSets();

   // Geometry Pass
   void CreateGeometryDescriptorSetLayout();
   void CreateGeometryFBO();
   void CreateGeometryPass();
   void CreateGeometryPipeline();

   // Lighting Pass
   void CreateLightingDescriptorSetLayout();
   void CreateLightingFBO();
   void CreateLightingPass();
   void CreateLightingPipeline();

   // Gizmo Pass
   void CreateGizmoDescriptorSetLayout();
   void CreateGizmoPipeline();

   // Particle pass
   void CreateParticleDescriptorSetLayout();
   void CreateParticlePipeline();
   void CreateParticleInstanceBuffers();

   // Descriptor set for pipeline
   void CreateDescriptorSets();
   void UpdateDescriptorSets();
   void CreateDescriptorPool();
   // Functions to set up command pool
   void CreateCommandBuffers();
   void RecordCommandBuffer(const uint32_t imageIndex);
   void RenderParticlesInstanced(const uint32_t imageIndex);

   void RenderGeometryPass(const VkViewport& viewport, const VkRect2D& scissor);
   void TransitionGBufferLayouts();
   void RenderLightingPass(const uint32_t imageIndex, const VkViewport& viewport,
                           const VkRect2D& scissor);
   void RenderGizmoPass(const VkViewport& viewport, const VkRect2D& scissor);
   void RenderParticlePass(const uint32_t imageIndex, const VkViewport& viewport,
                           const VkRect2D& scissor);
   void ResizeParticleBuffers(const size_t newCapacity);
   // Functions to set up synchronization for drawing
   void CreateSynchronizationObjects();
   // Functions to setup swapchain recreation
   void RecreateSwapchain();
   void CleanupSwapchain();

   void CreateUtilityMeshes();
   void CreateDefaultMaterial();

   [[nodiscard]] ResourceManager* GetResourceManager() const noexcept override;

  public:
   constexpr static uint32_t MAX_LIGHTS{256};

  private:
   // TODO: Remove unique ptrs in favour of stack variables once other abstractions are implemented
   constexpr static uint32_t MAX_FRAMES_IN_FLIGHT{2};
   uint32_t m_currentFrame{0};

   double m_lastFrameTime{0};
   float m_deltaTime{0};

   VulkanInstance m_instance;
   VulkanSurface m_surface;
   VulkanDevice m_device;

   VulkanSwapchain m_swapchain;

   std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> m_cameraUniformBuffers;
   std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> m_lightsUniformBuffers;

   MeshHandle m_fullscreenQuad;
   MeshHandle m_lineCube;

   // G-Buffer
   std::array<TextureHandle, MAX_FRAMES_IN_FLIGHT> m_gDepthTexture;
   std::array<TextureHandle, MAX_FRAMES_IN_FLIGHT> m_gAlbedoTexture; // RGB color + A AO
   std::array<TextureHandle, MAX_FRAMES_IN_FLIGHT>
      m_gNormalTexture; // RG encoded normal + B roughness + A metallic

   // Geometry pass
   std::unique_ptr<VulkanRenderPass> m_geometryRenderPass;
   std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> m_geometryFramebuffers;
   VkDescriptorSetLayout m_geometryDescriptorSetLayout;
   std::unique_ptr<VulkanPipelineLayout> m_geometryPipelineLayout;
   std::unique_ptr<VulkanGraphicsPipeline> m_geometryGraphicsPipeline;
   std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_geometryDescriptorSets;

   // Lighting pass
   std::unique_ptr<VulkanRenderPass> m_lightingRenderPass;
   std::vector<VkFramebuffer> m_lightingFramebuffers;
   VkDescriptorSetLayout m_lightingDescriptorSetLayout;
   std::unique_ptr<VulkanPipelineLayout> m_lightingPipelineLayout;
   std::unique_ptr<VulkanGraphicsPipeline> m_lightingGraphicsPipeline;
   std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_lightingDescriptorSets;

   // Material descriptor stuff
   VkDescriptorSetLayout m_materialDescriptorSetLayout;
   VkDescriptorPool m_materialDescriptorPool;

   // Gizmo pass
   VkDescriptorSetLayout m_gizmoDescriptorSetLayout;
   std::unique_ptr<VulkanPipelineLayout> m_gizmoPipelineLayout;
   std::unique_ptr<VulkanGraphicsPipeline> m_gizmoGraphicsPipeline;
   std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_gizmoDescriptorSets;

   // Particle pass
   std::unique_ptr<VulkanPipelineLayout> m_particlePipelineLayout;
   std::unique_ptr<VulkanGraphicsPipeline> m_particleGraphicsPipeline;
   VkDescriptorSetLayout m_particleDescriptorSetLayout;
   std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_particleDescriptorSets;
   std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> m_particleInstanceBuffers;
   size_t m_particleInstanceCapacity{0};

   std::unique_ptr<VulkanCommandBuffers> m_commandBuffers; // Per frame in flight
   std::vector<VkCommandPool> m_threadCommandPools;
   std::vector<std::array<std::unique_ptr<VulkanCommandBuffers>, MAX_FRAMES_IN_FLIGHT>>
      m_secondaryCommandBuffers; // Per thread per frame (per frame in flight)
   std::vector<VkSemaphore> m_imageAvailableSemaphores;
   std::vector<VkSemaphore> m_renderFinishedSemaphores;
   std::vector<VkFence> m_inFlightFences;
   TextureHandle m_depthTexture;
   VkDescriptorPool m_descriptorPool;
   std::unique_ptr<ResourceManager> m_resourceManager;
   std::unique_ptr<MaterialEditor> m_materialEditor;

   std::unique_ptr<ThreadPool> m_threadPool;
   uint32_t m_numRenderThreads{0};
};
