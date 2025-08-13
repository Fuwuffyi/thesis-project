#pragma once

#include "../core/IRenderer.hpp"
#include "VulkanInstance.hpp"
#include "VulkanDebugMessenger.hpp"
#include "VulkanSurface.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanBuffer.hpp"

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <optional>
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
   // Functions to set up a graphics pipeline
   static std::vector<char> ReadFile(const std::string& filename);
   void CreateGraphicsPipeline();
   VkShaderModule CreateShaderModule(const std::vector<char>& code);
   // Descriptor set for pipeline
   void CreateDescriptorSetLayout();
   void CreateDescriptorPool();
   void CreateDescriptorSets();
   // Functions to set up framebuffers
   void CreateFramebuffers();
   // Functions to set up command pool
   void CreateCommandPool();
   void CreateCommandBuffers();
   void RecordCommandBuffer(const VkCommandBuffer& commandBuffer, const uint32_t imageIndex);
   // Functions to set up synchronization for drawing
   void CreateSynchronizationObjects();
   // Functions to setup swapchain recreation
   void RecreateSwapchain();
   void CleanupSwapchain();
   // Functions to set up vertex attributes
   static VkVertexInputBindingDescription GetVertexBindingDescription();
   static std::array<VkVertexInputAttributeDescription, 3> GetVertexAttributeDescriptions();
   // Testing mesh
   void CreateVertexBuffer();
   void CreateIndexBuffer();
   void CreateUniformBuffer();
   void UpdateUniformBuffer(const uint32_t currentImage);
private:
   constexpr static uint32_t MAX_FRAMES_IN_FLIGHT = 2;
   uint32_t m_currentFrame = 0;
   VulkanInstance m_instance;
   VulkanDebugMessenger m_debugMessenger;
   VulkanSurface m_surface;
   VulkanDevice m_device;
   VulkanSwapchain m_swapchain;
   VulkanRenderPass m_renderPass;
   std::vector<VkFramebuffer> m_swapchainFramebuffers;
   VkDescriptorSetLayout m_descriptorSetLayout;
   VkPipelineLayout m_pipelineLayout;
   VkPipeline m_graphicsPipeline;
   VkCommandPool m_commandPool;
   std::vector<VkCommandBuffer> m_commandBuffers;
   std::vector<VkSemaphore> m_imageAvailableSemaphores;
   std::vector<VkSemaphore> m_renderFinishedSemaphores;
   std::vector<VkFence> m_inFlightFences;
   // TODO: Remove unique ptrs in favour of stack variables once other abstractions are implemented
   std::unique_ptr<VulkanBuffer> m_vertexBuffer;
   std::unique_ptr<VulkanBuffer> m_indexBuffer;
   std::vector<std::unique_ptr<VulkanBuffer>> m_uniformBuffers;
   std::vector<void*> m_uniformBuffersMapped;
   VkDescriptorPool m_descriptorPool;
   std::vector<VkDescriptorSet> m_descriptorSets;
};

