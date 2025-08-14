#include "VulkanRenderer.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "../core/Vertex.hpp"
#include "../core/Window.hpp"
#include "../core/Camera.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <fstream>
#include <print>
#include <stdexcept>
#include <vector>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char *> validationLayers = {
   "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
   VK_KHR_SWAPCHAIN_EXTENSION_NAME};

// Testing mesh
const std::vector<Vertex> vertices = {
   {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
   {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
   {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
   {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
   {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {0.0f, 0.0f}},
   {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {1.0f, 0.0f}},
   {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {1.0f, 1.0f}},
   {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {0.0f, 1.0f}},
   {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.0f, 0.0f}, {0.0f, 0.0f}},
   {{-0.5f, -0.5f,  0.5f}, {0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}},
   {{-0.5f,  0.5f,  0.5f}, {0.5f, 0.0f, 0.0f}, {1.0f, 1.0f}},
   {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.0f, 0.0f}, {0.0f, 1.0f}},
   {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
   {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
   {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
   {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
   {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
   {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
   {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
   {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
   {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.5f, 0.0f}, {0.0f, 0.0f}},
   {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.5f, 0.0f}, {1.0f, 0.0f}},
   {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.5f, 0.0f}, {1.0f, 1.0f}},
   {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f}},
};

const std::vector<uint16_t> indices = {
   0, 1, 2, 2, 3, 0,
   4, 5, 6, 6, 7, 4,
   8, 9, 10, 10, 11, 8,
   12, 13, 14, 14, 15, 12,
   16, 17, 18, 18, 19, 16,
   20, 21, 22, 22, 23, 20
};

VulkanRenderer::VulkanRenderer(Window *windowHandle)
   : IRenderer(windowHandle),
   m_instance(deviceExtensions, validationLayers, enableValidationLayers),
   m_debugMessenger(m_instance),
   m_surface(m_instance, m_window->GetNativeWindow()),
   m_device(m_instance, m_surface, deviceExtensions, validationLayers,
            enableValidationLayers),
   m_swapchain(m_device, m_surface, *m_window),
   m_renderPass(m_device, m_swapchain.GetFormat()) {
   CreateDescriptorSetLayout();
   CreateGraphicsPipeline();
   CreateFramebuffers();
   CreateCommandPool();
   SetupImgui();
   CreateTextureImage();
   CreateVertexBuffer();
   CreateIndexBuffer();
   CreateUniformBuffer();
   CreateDescriptorPool();
   CreateDescriptorSets();
   CreateCommandBuffers();
   CreateSynchronizationObjects();
}

void VulkanRenderer::CreateGraphicsPipeline() {
   const VulkanShaderModule vertShader(m_device, "resources/shaders/vk/test.vert.spv");
   const VulkanShaderModule fragShader(m_device, "resources/shaders/vk/test.frag.spv");
   VkPushConstantRange pushConstantRange{};
   pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   pushConstantRange.offset = 0;
   pushConstantRange.size = sizeof(glm::mat4);
   m_pipelineLayout = std::make_unique<VulkanPipelineLayout>(
      m_device,
      std::vector<VkDescriptorSetLayout>{ m_descriptorSetLayout },
      std::vector<VkPushConstantRange>{ pushConstantRange }
   );
   VulkanGraphicsPipelineBuilder builder(m_device);
   builder
      // Set the shaders
      .SetVertexShader(vertShader.Get())
      .SetFragmentShader(fragShader.Get())
      // Set vertex locations
      .AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
      .AddVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position))
      .AddVertexAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal))
      .AddVertexAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv))
      // Setup type of drawing
      .SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
      // Make viewport and scissor dynamic
      .AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
      .AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
      .AddViewport(VkViewport{}).AddScissor(VkRect2D{})
      // Set layout and render pass
      .SetPipelineLayout(m_pipelineLayout->Get())
      .SetRenderPass(m_renderPass.Get());
   VulkanGraphicsPipelineBuilder::RasterizationState raster{};
   raster.cullMode = VK_CULL_MODE_BACK_BIT;
   raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   builder.SetRasterizationState(raster);
   VulkanGraphicsPipelineBuilder::MultisampleState ms{};
   ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   builder.SetMultisampleState(ms);
   VulkanGraphicsPipelineBuilder::ColorBlendAttachmentState cb{};
   cb.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   VulkanGraphicsPipelineBuilder::ColorBlendState cbState{};
   cbState.attachments.push_back(cb);
   builder.SetColorBlendState(cbState);
   m_graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(
      builder.Build()
   );
}

void VulkanRenderer::CreateDescriptorSetLayout() {
   VkDescriptorSetLayoutBinding uboLayoutBinding{};
   uboLayoutBinding.binding = 0;
   uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   uboLayoutBinding.descriptorCount = 1;
   uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   uboLayoutBinding.pImmutableSamplers = nullptr;
   VkDescriptorSetLayoutCreateInfo layoutInfo{};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = 1;
   layoutInfo.pBindings = &uboLayoutBinding;
   if (vkCreateDescriptorSetLayout(m_device.Get(), &layoutInfo, nullptr,
                                   &m_descriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor set layout.");
   }
}

void VulkanRenderer::CreateFramebuffers() {
   // Create framebuffers from the given swapchain images
   m_swapchainFramebuffers.resize(m_swapchain.GetImageViews().size());
   for (size_t i = 0; i < m_swapchain.GetImageViews().size(); ++i) {
      const VkImageView attachments[] = {m_swapchain.GetImageViews()[i]};
      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = m_renderPass.Get();
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = m_swapchain.GetExtent().width;
      framebufferInfo.height = m_swapchain.GetExtent().height;
      framebufferInfo.layers = 1;
      if (vkCreateFramebuffer(m_device.Get(), &framebufferInfo, nullptr,
                              &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create a framebuffer.");
      }
   }
}

void VulkanRenderer::CreateCommandPool() {
   VkCommandPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   poolInfo.queueFamilyIndex = m_device.GetGraphicsQueueFamily();
   if (vkCreateCommandPool(m_device.Get(), &poolInfo, nullptr, &m_commandPool) !=
      VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool!");
   }
}

void VulkanRenderer::CreateCommandBuffers() {
   m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
   VkCommandBufferAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool = m_commandPool;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
   if (vkAllocateCommandBuffers(m_device.Get(), &allocInfo,
                                m_commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate command buffers.");
   }
}

void VulkanRenderer::RecordCommandBuffer(const VkCommandBuffer &commandBuffer,
                                         const uint32_t imageIndex) {
   // Rotate the object for fun
   static auto startTime = std::chrono::high_resolution_clock::now();
   auto currentTime = std::chrono::high_resolution_clock::now();
   float time = std::chrono::duration<float, std::chrono::seconds::period>(
      currentTime - startTime)
      .count();
   ObjectData objData {};
   objData.model = glm::mat4(1.0f);
   // Setup record
   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = 0;
   beginInfo.pInheritanceInfo = nullptr;
   if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("Failed to begin recording command buffer.");
   }
   // Start a render pass
   VkRenderPassBeginInfo renderPassInfo{};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassInfo.renderPass = m_renderPass.Get();
   renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
   renderPassInfo.renderArea.offset = {0, 0};
   renderPassInfo.renderArea.extent = m_swapchain.GetExtent();
   // Clear color to black
   VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
   renderPassInfo.clearValueCount = 1;
   renderPassInfo.pClearValues = &clearColor;
   vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                        VK_SUBPASS_CONTENTS_INLINE);
   vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                     m_graphicsPipeline->GetPipeline());
   // Set the dynamic viewport and scissor
   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(m_swapchain.GetExtent().width);
   viewport.height = static_cast<float>(m_swapchain.GetExtent().height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = m_swapchain.GetExtent();
   vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
   // Set triangle position/rotation/scale
   vkCmdPushConstants(commandBuffer, m_pipelineLayout->Get(),
                      VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(ObjectData), &objData);
   // Draw the triangle
   VkBuffer vertexBuffers[] = {m_vertexBuffer->Get()};
   VkDeviceSize offsets[] = {0};
   vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
   vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->Get(), 0,
                        VK_INDEX_TYPE_UINT16);
   vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           m_pipelineLayout->Get(), 0, 1,
                           &m_descriptorSets[m_currentFrame], 0, nullptr);
   vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0,
                    0, 0);
   // TODO: Clean up imgui stuff
   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_commandBuffers[m_currentFrame]);
   vkCmdEndRenderPass(commandBuffer);
   if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("Failed to record command buffer.");
   }
}

void VulkanRenderer::CreateSynchronizationObjects() {
   size_t imageCount = m_swapchain.GetImages().size();
   m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
   m_renderFinishedSemaphores.resize(imageCount);
   m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
   VkSemaphoreCreateInfo semaphoreInfo{};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
   VkFenceCreateInfo fenceInfo{};
   fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
   // Create per-frame image-available semaphores & fences
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vkCreateSemaphore(m_device.Get(), &semaphoreInfo, nullptr,
                        &m_imageAvailableSemaphores[i]);
      vkCreateFence(m_device.Get(), &fenceInfo, nullptr,
                    &m_inFlightFences[i]);
   }
   // Create per-image render-finished semaphores
   for (size_t i = 0; i < imageCount; ++i) {
      vkCreateSemaphore(m_device.Get(), &semaphoreInfo, nullptr,
                        &m_renderFinishedSemaphores[i]);
   }
}

void VulkanRenderer::RecreateSwapchain() {
   m_swapchain.Recreate();
   CreateFramebuffers();
}

void VulkanRenderer::CleanupSwapchain() {
   for (const VkFramebuffer &framebuffer : m_swapchainFramebuffers) {
      vkDestroyFramebuffer(m_device.Get(), framebuffer, nullptr);
   }
}

void VulkanRenderer::CreateImage(const uint32_t width, const uint32_t height, const VkFormat format,
                                 const VkImageTiling tiling, const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties,
                                 VkImage& image, VkDeviceMemory& imageMemory) {
   VkImageCreateInfo imageInfo{};
   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.imageType = VK_IMAGE_TYPE_2D;
   imageInfo.extent.width = width;
   imageInfo.extent.height = height;
   imageInfo.extent.depth = 1;
   imageInfo.mipLevels = 1;
   imageInfo.arrayLayers = 1;
   imageInfo.format = format;
   imageInfo.tiling = tiling;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   imageInfo.usage = usage;
   imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
   imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   if (vkCreateImage(m_device.Get(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image.");
   }
   VkMemoryRequirements memRequirements;
   vkGetImageMemoryRequirements(m_device.Get(), image, &memRequirements);
   VkMemoryAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize = memRequirements.size;
   allocInfo.memoryTypeIndex = VulkanBuffer::FindMemoryType(m_device.GetPhysicalDevice(), memRequirements.memoryTypeBits, properties);
   if (vkAllocateMemory(m_device.Get(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate image memory.");
   }
   vkBindImageMemory(m_device.Get(), image, imageMemory, 0);
}

void VulkanRenderer::CreateTextureImage() {
   // Load the image in a buffer
   int32_t texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load("resources/textures/texture_base.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   const VkDeviceSize imageSize = texWidth * texHeight * 4;
   if (!pixels) {
      throw std::runtime_error("Failed to load texture image.");
   }
   auto stagingBuffer = std::make_unique<VulkanBuffer>(
      m_device, imageSize, VulkanBuffer::Usage::TransferSrc,
      VulkanBuffer::MemoryType::HostVisible);
   stagingBuffer->Update(pixels, imageSize);
   stbi_image_free(pixels);
   CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory);
}

void VulkanRenderer::CreateVertexBuffer() {
   const VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
   auto stagingBuffer = std::make_unique<VulkanBuffer>(
      m_device, bufferSize, VulkanBuffer::Usage::TransferSrc,
      VulkanBuffer::MemoryType::HostVisible);
   stagingBuffer->Update(vertices.data(), bufferSize);
   m_vertexBuffer = std::make_unique<VulkanBuffer>(
      m_device, bufferSize,
      static_cast<VulkanBuffer::Usage>(
         static_cast<VkBufferUsageFlags>(VulkanBuffer::Usage::TransferDst) |
         static_cast<VkBufferUsageFlags>(VulkanBuffer::Usage::Vertex)),
      VulkanBuffer::MemoryType::DeviceLocal);
   VulkanBuffer::CopyBuffer(m_device, m_commandPool, *stagingBuffer,
                            *m_vertexBuffer, bufferSize);
}

void VulkanRenderer::CreateIndexBuffer() {
   const VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();
   auto stagingBuffer = std::make_unique<VulkanBuffer>(
      m_device, bufferSize, VulkanBuffer::Usage::TransferSrc,
      VulkanBuffer::MemoryType::HostVisible);
   stagingBuffer->Update(indices.data(), bufferSize);
   m_indexBuffer = std::make_unique<VulkanBuffer>(
      m_device, bufferSize,
      static_cast<VulkanBuffer::Usage>(
         static_cast<VkBufferUsageFlags>(VulkanBuffer::Usage::TransferDst) |
         static_cast<VkBufferUsageFlags>(VulkanBuffer::Usage::Index)),
      VulkanBuffer::MemoryType::DeviceLocal);
   VulkanBuffer::CopyBuffer(m_device, m_commandPool, *stagingBuffer,
                            *m_indexBuffer, bufferSize);
}

void VulkanRenderer::CreateUniformBuffer() {
   const VkDeviceSize bufferSize = sizeof(CameraData);
   m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      m_uniformBuffers[i] = std::make_unique<VulkanBuffer>(
         m_device, bufferSize, VulkanBuffer::Usage::Uniform,
         VulkanBuffer::MemoryType::HostVisible);
   }
}

void VulkanRenderer::UpdateUniformBuffer(const uint32_t currentImage) {
   if (!m_activeCamera)
      return;
   CameraData camData {};
   m_activeCamera->SetAspectRatio(
      static_cast<float>(m_swapchain.GetExtent().width) /
      static_cast<float>(m_swapchain.GetExtent().height));
   camData.view = m_activeCamera->GetViewMatrix();
   camData.proj = m_activeCamera->GetProjectionMatrix();
   m_uniformBuffers[currentImage]->UpdateMapped(&camData, sizeof(CameraData));
}

void VulkanRenderer::CreateDescriptorPool() {
   VkDescriptorPoolSize poolSize{};
   poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   VkDescriptorPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   poolInfo.poolSizeCount = 1;
   poolInfo.pPoolSizes = &poolSize;
   poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   if (vkCreateDescriptorPool(m_device.Get(), &poolInfo, nullptr,
                              &m_descriptorPool) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor pool.");
   }
}

void VulkanRenderer::CreateDescriptorSets() {
   std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                              m_descriptorSetLayout);
   VkDescriptorSetAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfo.descriptorPool = m_descriptorPool;
   allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   allocInfo.pSetLayouts = layouts.data();
   m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
   if (vkAllocateDescriptorSets(m_device.Get(), &allocInfo,
                                m_descriptorSets.data()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate descriptor sets.");
   }
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = m_uniformBuffers[i]->Get();
      bufferInfo.offset = 0;
      bufferInfo.range = sizeof(CameraData);
      VkWriteDescriptorSet descriptorWrite{};
      descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrite.dstSet = m_descriptorSets[i];
      descriptorWrite.dstBinding = 0;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pBufferInfo = &bufferInfo;
      descriptorWrite.pImageInfo = nullptr;
      descriptorWrite.pTexelBufferView = nullptr;
      vkUpdateDescriptorSets(m_device.Get(), 1, &descriptorWrite, 0, nullptr);
   }
}

VulkanRenderer::~VulkanRenderer() {
   vkDeviceWaitIdle(m_device.Get());
   CleanupSwapchain();
   vkDestroyDescriptorPool(m_device.Get(), m_descriptorPool, nullptr);
   vkDestroyDescriptorSetLayout(m_device.Get(), m_descriptorSetLayout, nullptr);
   for (size_t i = 0; i < m_renderFinishedSemaphores.size(); ++i) {
      vkDestroySemaphore(m_device.Get(), m_renderFinishedSemaphores[i], nullptr);
   }
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vkDestroySemaphore(m_device.Get(), m_imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(m_device.Get(), m_inFlightFences[i], nullptr);
   }
   vkDestroyCommandPool(m_device.Get(), m_commandPool, nullptr);
   DestroyImgui();
}

void VulkanRenderer::SetupImgui() {
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   if (!ImGui_ImplGlfw_InitForVulkan(m_window->GetNativeWindow(), true)) {
      throw std::runtime_error("ImGUI initialization failed.");
   }
   VkDescriptorPoolSize pool_sizes[] =
      {
         { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
         { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
         { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
         { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
         { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
         { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
         { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
      };
   VkDescriptorPoolCreateInfo pool_info{};
   pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // ImGui needs this
   pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
   pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
   pool_info.pPoolSizes = pool_sizes;
   VkDescriptorPool imguiPool;
   if (vkCreateDescriptorPool(m_device.Get(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create ImGui descriptor pool.");
   }
   ImGui_ImplVulkan_InitInfo imguiInfo{};
   imguiInfo.Instance = m_instance.Get();
   imguiInfo.PhysicalDevice = m_device.GetPhysicalDevice();
   imguiInfo.Device = m_device.Get();
   imguiInfo.QueueFamily = m_device.GetGraphicsQueueFamily();
   imguiInfo.Queue = m_device.GetGraphicsQueue();
   imguiInfo.PipelineCache = VK_NULL_HANDLE;
   imguiInfo.DescriptorPool = imguiPool;
   imguiInfo.MinImageCount = static_cast<uint32_t>(m_swapchain.GetImages().size());
   imguiInfo.ImageCount = static_cast<uint32_t>(m_swapchain.GetImages().size());
   imguiInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
   imguiInfo.Allocator = nullptr;
   imguiInfo.RenderPass = m_renderPass.Get();
   imguiInfo.CheckVkResultFn = [](VkResult err) {
      if (err != VK_SUCCESS) {
         throw std::runtime_error("ImGui Vulkan call failed.");
      }
   };
   if (!ImGui_ImplVulkan_Init(&imguiInfo)) {
      throw std::runtime_error("ImGUI initialization failed.");
   }
}

void VulkanRenderer::DestroyImgui() {
   ImGui_ImplVulkan_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

void VulkanRenderer::RenderImgui() {
   ImGui_ImplVulkan_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();
   // FPS Overlay
   {
      ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
      ImGui::SetNextWindowBgAlpha(0.35f);
      ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
         ImGuiWindowFlags_AlwaysAutoResize |
         ImGuiWindowFlags_NoSavedSettings |
         ImGuiWindowFlags_NoFocusOnAppearing |
         ImGuiWindowFlags_NoNav;
      ImGui::Begin("FPS Overlay", nullptr, flags);
      ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
      ImGui::Text("Frame: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
      ImGui::End();
   }
   ImGui::Render();
}

void VulkanRenderer::RenderFrame() {
   // Wait for previous farme
   vkWaitForFences(m_device.Get(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE,
                   UINT64_MAX);
   // Get the next image of the swapchain
   uint32_t imageIndex;
   const VkResult nextImageResult = m_swapchain.AcquireNextImage(
      UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], &imageIndex);
   if (nextImageResult == VK_ERROR_OUT_OF_DATE_KHR ||
      nextImageResult == VK_SUBOPTIMAL_KHR) {
      RecreateSwapchain();
      return;
   } else if (nextImageResult != VK_SUCCESS) {
      throw std::runtime_error("Failed to acquire swap chain image.");
   }
   vkResetFences(m_device.Get(), 1, &m_inFlightFences[m_currentFrame]);
   // Setup command buffer to draw the triangle
   vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
   UpdateUniformBuffer(m_currentFrame);
   RenderImgui();
   RecordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);
   // Submit the command buffer
   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
   VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores = waitSemaphores;
   submitInfo.pWaitDstStageMask = waitStages;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
   VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[imageIndex] };
   submitInfo.signalSemaphoreCount = 1;
   submitInfo.pSignalSemaphores = signalSemaphores;
   if (vkQueueSubmit(m_device.GetGraphicsQueue(), 1, &submitInfo,
                     m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
   }
   // Finish frame and present
   VkPresentInfoKHR presentInfo{};
   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
   presentInfo.waitSemaphoreCount = 1;
   presentInfo.pWaitSemaphores = signalSemaphores;
   VkSwapchainKHR swapChains[] = {m_swapchain.Get()};
   presentInfo.swapchainCount = 1;
   presentInfo.pSwapchains = swapChains;
   presentInfo.pImageIndices = &imageIndex;
   presentInfo.pResults = nullptr;
   const VkResult presentResult =
      vkQueuePresentKHR(m_device.GetPresentQueue(), &presentInfo);
   if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
      presentResult == VK_SUBOPTIMAL_KHR) {
      RecreateSwapchain();
   } else if (presentResult != VK_SUCCESS) {
      throw std::runtime_error("Failed to present swap chain image.");
   }
   // Increase frame counter
   m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

