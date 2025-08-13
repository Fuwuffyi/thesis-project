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
   {{-0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
   {{0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
   {{0.5f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
   {{-0.5f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}};
const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

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
   CreateVertexBuffer();
   CreateIndexBuffer();
   CreateUniformBuffer();
   CreateDescriptorPool();
   CreateDescriptorSets();
   CreateCommandBuffers();
   CreateSynchronizationObjects();

   // Initialize ImGui
   /*
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  if (!ImGui_ImplGlfw_InitForVulkan(m_window->GetNativeWindow(), true)) {
     throw std::runtime_error("ImGUI initialization failed.");
  }
  if (!ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo *info)) {
     throw std::runtime_error("ImGUI initialization failed.");
  }
  */
}

std::vector<char> VulkanRenderer::ReadFile(const std::string &filename) {
   std::ifstream file(filename, std::ios::ate | std::ios::binary);
   if (!file.is_open()) {
      throw std::runtime_error("Failed to open file.");
   }
   const size_t fileSize = (size_t)file.tellg();
   std::vector<char> buffer(fileSize);
   file.seekg(0);
   file.read(buffer.data(), static_cast<int64_t>(fileSize));
   file.close();
   return buffer;
}

void VulkanRenderer::CreateGraphicsPipeline() {
   // Create shader modules
   const std::vector<char> vertShaderCode =
      ReadFile("resources/shaders/vk/test.vert.spv");
   const std::vector<char> fragShaderCode =
      ReadFile("resources/shaders/vk/test.frag.spv");
   const VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
   const VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);
   // Create shader stages for the programmable stages of the Pipeline
   VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
   vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
   vertShaderStageInfo.module = vertShaderModule;
   vertShaderStageInfo.pName = "main";
   VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
   fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   fragShaderStageInfo.module = fragShaderModule;
   fragShaderStageInfo.pName = "main";
   VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
      fragShaderStageInfo};
   // Setup non-programmable stages of the pipeline
   // Setup format of the vertex data
   const VkVertexInputBindingDescription bindingDescription =
      VulkanRenderer::GetVertexBindingDescription();
   const std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions =
      VulkanRenderer::GetVertexAttributeDescriptions();
   VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
   vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputInfo.vertexBindingDescriptionCount = 1;
   vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
   vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
   vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
   // Setup type of geometry to draw
   VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
   inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   inputAssembly.primitiveRestartEnable = VK_FALSE;
   // Setup dynamic states, can be changed at draw call time
   const std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR};
   VkPipelineDynamicStateCreateInfo dynamicState{};
   dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
   dynamicState.pDynamicStates = dynamicStates.data();
   // Setup viewport as dynamic (no scissor and no viewport, to define at draw
   // time)
   VkPipelineViewportStateCreateInfo viewportState{};
   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportState.viewportCount = 1;
   viewportState.scissorCount = 1;
   // Setup the rasterizer
   VkPipelineRasterizationStateCreateInfo rasterizer{};
   rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.depthClampEnable = VK_FALSE;
   rasterizer.rasterizerDiscardEnable = VK_FALSE;
   rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
   rasterizer.lineWidth = 1.0f;
   rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
   rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   rasterizer.depthBiasEnable = VK_FALSE;
   rasterizer.depthBiasConstantFactor = 0.0f;
   rasterizer.depthBiasClamp = 0.0f;
   rasterizer.depthBiasSlopeFactor = 0.0f;
   // Setup multisampling
   VkPipelineMultisampleStateCreateInfo multisampling{};
   multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.sampleShadingEnable = VK_FALSE;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   multisampling.minSampleShading = 1.0f;
   multisampling.pSampleMask = nullptr;
   multisampling.alphaToCoverageEnable = VK_FALSE;
   multisampling.alphaToOneEnable = VK_FALSE;
   // Setup color blending
   VkPipelineColorBlendAttachmentState colorBlendAttachment{};
   colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable = VK_FALSE;
   colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
   colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
   colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
   VkPipelineColorBlendStateCreateInfo colorBlending{};
   colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlending.logicOpEnable = VK_FALSE;
   colorBlending.logicOp = VK_LOGIC_OP_COPY;
   colorBlending.attachmentCount = 1;
   colorBlending.pAttachments = &colorBlendAttachment;
   colorBlending.blendConstants[0] = 0.0f;
   colorBlending.blendConstants[1] = 0.0f;
   colorBlending.blendConstants[2] = 0.0f;
   colorBlending.blendConstants[3] = 0.0f;
   // Setup a pipeline layout for uniforms
   VkPushConstantRange pushConstantRange{};
   pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   pushConstantRange.offset = 0;
   pushConstantRange.size = sizeof(glm::mat4);
   VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
   pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount = 1;
   pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
   pipelineLayoutInfo.pushConstantRangeCount = 1;
   pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
   if (vkCreatePipelineLayout(m_device.Get(), &pipelineLayoutInfo, nullptr,
                              &m_pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create pipeline layout.");
   }
   // Start setting up the full pipeline
   VkGraphicsPipelineCreateInfo pipelineInfo{};
   pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.stageCount = 2;
   pipelineInfo.pStages = shaderStages;
   pipelineInfo.pVertexInputState = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pDepthStencilState = nullptr;
   pipelineInfo.pColorBlendState = &colorBlending;
   pipelineInfo.pDynamicState = &dynamicState;
   pipelineInfo.layout = m_pipelineLayout;
   pipelineInfo.renderPass = m_renderPass.Get();
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
   pipelineInfo.basePipelineIndex = -1;
   if (vkCreateGraphicsPipelines(m_device.Get(), VK_NULL_HANDLE, 1,
                                 &pipelineInfo, nullptr,
                                 &m_graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create graphics pipeline.");
   }
   // Unload shader modules
   vkDestroyShaderModule(m_device.Get(), fragShaderModule, nullptr);
   vkDestroyShaderModule(m_device.Get(), vertShaderModule, nullptr);
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

VkShaderModule
VulkanRenderer::CreateShaderModule(const std::vector<char> &code) {
   VkShaderModuleCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   createInfo.codeSize = code.size();
   createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
   VkShaderModule shaderModule;
   if (vkCreateShaderModule(m_device.Get(), &createInfo, nullptr,
                            &shaderModule) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create shader module.");
   }
   return shaderModule;
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
   objData.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f));
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
                     m_graphicsPipeline);
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
   vkCmdPushConstants(commandBuffer, m_pipelineLayout,
                      VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(ObjectData), &objData);
   // Draw the triangle
   VkBuffer vertexBuffers[] = {m_vertexBuffer->Get()};
   VkDeviceSize offsets[] = {0};
   vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
   vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->Get(), 0,
                        VK_INDEX_TYPE_UINT16);
   vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           m_pipelineLayout, 0, 1,
                           &m_descriptorSets[m_currentFrame], 0, nullptr);
   vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0,
                    0, 0);
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

VkVertexInputBindingDescription VulkanRenderer::GetVertexBindingDescription() {
   VkVertexInputBindingDescription bindingDescription{};
   bindingDescription.binding = 0;
   bindingDescription.stride = sizeof(Vertex);
   bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
   return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3>
VulkanRenderer::GetVertexAttributeDescriptions() {
   std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
   attributeDescriptions[0].binding = 0;
   attributeDescriptions[0].location = 0;
   attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
   attributeDescriptions[0].offset = offsetof(Vertex, position);
   attributeDescriptions[1].binding = 0;
   attributeDescriptions[1].location = 1;
   attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
   attributeDescriptions[1].offset = offsetof(Vertex, normal);
   attributeDescriptions[2].binding = 0;
   attributeDescriptions[2].location = 2;
   attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
   attributeDescriptions[2].offset = offsetof(Vertex, uv);
   return attributeDescriptions;
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
   vkDestroyPipeline(m_device.Get(), m_graphicsPipeline, nullptr);
   vkDestroyPipelineLayout(m_device.Get(), m_pipelineLayout, nullptr);
   for (size_t i = 0; i < m_renderFinishedSemaphores.size(); ++i) {
      vkDestroySemaphore(m_device.Get(), m_renderFinishedSemaphores[i], nullptr);
   }
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vkDestroySemaphore(m_device.Get(), m_imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(m_device.Get(), m_inFlightFences[i], nullptr);
   }
   vkDestroyCommandPool(m_device.Get(), m_commandPool, nullptr);
   /*
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  */
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

   /*
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::ShowDemoWindow();
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_commandBuffer);
  */

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
