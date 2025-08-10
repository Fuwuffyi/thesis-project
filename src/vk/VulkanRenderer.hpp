#pragma once

#include "../core/IRenderer.hpp"
#include "VulkanInstance.hpp"
#include "VulkanDebugMessenger.hpp"
#include "VulkanSurface.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <optional>
#include <vector>

struct UniformBufferObject {
   alignas(16) glm::mat4 model;
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
};

class VulkanRenderer : public IRenderer {
public:
   VulkanRenderer(Window* window);
   ~VulkanRenderer();
   void RenderFrame() override;
private:
   // Functions to set up a render pass
   void CreateRenderPass();
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
   // Functions to set up memory and buffers
   uint32_t FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags& properties) const;
   void CreateBuffer(const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties,
                     VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
   void CopyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, const VkDeviceSize& size) const;
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
   std::vector<VkFramebuffer> m_swapchainFramebuffers;
   VkRenderPass m_renderPass;
   VkDescriptorSetLayout m_descriptorSetLayout;
   VkPipelineLayout m_pipelineLayout;
   VkPipeline m_graphicsPipeline;
   VkCommandPool m_commandPool;
   std::vector<VkCommandBuffer> m_commandBuffers;
   std::vector<VkSemaphore> m_imageAvailableSemaphores;
   std::vector<VkSemaphore> m_renderFinishedSemaphores;
   std::vector<VkFence> m_inFlightFences;
   VkBuffer m_vertexBuffer;
   VkDeviceMemory m_vertexBufferMemory;
   VkBuffer m_indexBuffer;
   VkDeviceMemory m_indexBufferMemory;
   std::vector<VkBuffer> m_uniformBuffers;
   std::vector<VkDeviceMemory> m_uniformBuffersMemory;
   std::vector<void*> m_uniformBuffersMapped;
   VkDescriptorPool m_descriptorPool;
   std::vector<VkDescriptorSet> m_descriptorSets;
};

