#include "VulkanRenderer.hpp"

#include "core/Window.hpp"
#include "core/Camera.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/Node.hpp"
#include "core/scene/components/RendererComponent.hpp"

#include "core/editor/PerformanceGUI.hpp"
#include "core/editor/MaterialEditor.hpp"

#include "vk/resource/VulkanMesh.hpp"
#include "vk/resource/VulkanResourceFactory.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <GLFW/glfw3.h>
#include <memory>
#include <print>
#include <stdexcept>
#include <vector>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

struct CameraData {
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
};

struct ObjectData {
   alignas(16) glm::mat4 model;
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VulkanRenderer::VulkanRenderer(Window* windowHandle)
    : IRenderer(windowHandle),
      m_instance(),
      m_surface(m_instance, m_window->GetNativeWindow()),
      m_device(m_instance, m_surface),
      m_swapchain(m_device, m_surface, *m_window) {
   m_resourceManager =
      std::make_unique<ResourceManager>(std::make_unique<VulkanResourceFactory>(m_device));
   m_materialEditor =
      std::make_unique<MaterialEditor>(m_resourceManager.get(), GraphicsAPI::Vulkan);

   CreateGeometryDescriptorSetLayout();
   CreateGeometryPass();
   CreateGeometryFBO();
   CreateGeometryPipeline();

   CreateCommandBuffers();

   CreateDepthResources();

   SetupImgui();
   CreateTestResources();
   CreateUniformBuffer();
   CreateDescriptorPool();
   CreateDescriptorSets();
   CreateSynchronizationObjects();

   CreateDefaultMaterial();

   m_window->SetResizeCallback([this](int32_t width, int32_t height) { RecreateSwapchain(); });
}

void VulkanRenderer::CreateGeometryDescriptorSetLayout() {
   VkDescriptorSetLayoutBinding uboLayoutBinding{};
   uboLayoutBinding.binding = 0;
   uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   uboLayoutBinding.descriptorCount = 1;
   uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   uboLayoutBinding.pImmutableSamplers = nullptr;
   VkDescriptorSetLayoutBinding samplerLayoutBinding{};
   samplerLayoutBinding.binding = 1;
   samplerLayoutBinding.descriptorCount = 1;
   samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   samplerLayoutBinding.pImmutableSamplers = nullptr;
   samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
   const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                                 samplerLayoutBinding};
   VkDescriptorSetLayoutCreateInfo layoutInfo{};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
   layoutInfo.pBindings = bindings.data();
   if (vkCreateDescriptorSetLayout(m_device.Get(), &layoutInfo, nullptr,
                                   &m_geometryDescriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor set layout.");
   }
}

void VulkanRenderer::CreateGeometryPass() {
   m_gAlbedoTexture = m_resourceManager->CreateRenderTarget(
      "gbuffer_albedo", m_swapchain.GetExtent().width, m_swapchain.GetExtent().height,
      ITexture::Format::RGBA8);
   m_gNormalTexture = m_resourceManager->CreateRenderTarget(
      "gbuffer_normal", m_swapchain.GetExtent().width, m_swapchain.GetExtent().height,
      ITexture::Format::RGBA16F);
   m_gDepthTexture = m_resourceManager->CreateDepthTexture(
      "gbuffer_depth", m_swapchain.GetExtent().width, m_swapchain.GetExtent().height);
   ITexture* albedoTex = m_resourceManager->GetTexture(m_gAlbedoTexture);
   ITexture* normalTex = m_resourceManager->GetTexture(m_gNormalTexture);
   ITexture* depthTex = m_resourceManager->GetTexture(m_gDepthTexture);
   if (!albedoTex || !normalTex || !depthTex) {
      throw std::runtime_error("Failed to get G-buffer textures");
   }
   VulkanTexture* vkAlbedo = reinterpret_cast<VulkanTexture*>(albedoTex);
   VulkanTexture* vkNormal = reinterpret_cast<VulkanTexture*>(normalTex);
   VulkanTexture* vkDepth = reinterpret_cast<VulkanTexture*>(depthTex);
   m_geometryRenderPass = std::make_unique<VulkanRenderPass>(
      m_device, std::vector<VkFormat>{vkAlbedo->GetVkFormat(), vkNormal->GetVkFormat()},
      vkDepth->GetVkFormat());
}

void VulkanRenderer::CreateGeometryFBO() {
   // Create a single framebuffer for the G-buffer using the created textures' image views
   ITexture* albedoTex = m_resourceManager->GetTexture(m_gAlbedoTexture);
   ITexture* normalTex = m_resourceManager->GetTexture(m_gNormalTexture);
   ITexture* depthTex = m_resourceManager->GetTexture(m_gDepthTexture);
   if (!albedoTex || !normalTex || !depthTex)
      throw std::runtime_error("Failed to fetch gbuffer textures for framebuffer creation.");
   VulkanTexture* vkAlbedo = reinterpret_cast<VulkanTexture*>(albedoTex);
   VulkanTexture* vkNormal = reinterpret_cast<VulkanTexture*>(normalTex);
   VulkanTexture* vkDepth = reinterpret_cast<VulkanTexture*>(depthTex);
   VkImageView attachments[3] = {vkAlbedo->GetImageView(), vkNormal->GetImageView(),
                                 vkDepth->GetImageView()};
   VkFramebufferCreateInfo framebufferInfo{};
   framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
   framebufferInfo.renderPass = m_geometryRenderPass->Get();
   framebufferInfo.attachmentCount = 3;
   framebufferInfo.pAttachments = attachments;
   framebufferInfo.width = m_swapchain.GetExtent().width;
   framebufferInfo.height = m_swapchain.GetExtent().height;
   framebufferInfo.layers = 1;
   VkFramebuffer fb;
   if (vkCreateFramebuffer(m_device.Get(), &framebufferInfo, nullptr, &fb) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create G-buffer framebuffer.");
   }
   m_geometryFramebuffers.clear();
   m_geometryFramebuffers.push_back(fb);
}

void VulkanRenderer::CreateGeometryPipeline() {
   // Load shaders
   const VulkanShaderModule vertShader(m_device,
                                       std::string("resources/shaders/vk/geometry_pass.vert.spv"));
   const VulkanShaderModule fragShader(m_device,
                                       std::string("resources/shaders/vk/geometry_pass.frag.spv"));
   VkPushConstantRange pushConstantRange{};
   pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   pushConstantRange.offset = 0;
   pushConstantRange.size = sizeof(glm::mat4);
   m_geometryPipelineLayout = std::make_unique<VulkanPipelineLayout>(
      m_device, std::vector<VkDescriptorSetLayout>{m_geometryDescriptorSetLayout},
      std::vector<VkPushConstantRange>{pushConstantRange});
   // Build pipeline using the builder
   VulkanGraphicsPipelineBuilder builder(m_device);
   builder.SetVertexShader(vertShader.Get())
      .SetFragmentShader(fragShader.Get())
      .AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
      .AddVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position))
      .AddVertexAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal))
      .AddVertexAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv))
      .SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
      .SetDynamicViewportAndScissor()
      .SetCullMode(VK_CULL_MODE_BACK_BIT)
      .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .EnableDepthTest(VK_COMPARE_OP_LESS)
      .SetPipelineLayout(m_geometryPipelineLayout->Get())
      .SetRenderPass(m_geometryRenderPass->Get());
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
   // Build and store pipeline
   VulkanGraphicsPipeline pipelineObj = builder.Build();
   m_geometryGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(std::move(pipelineObj));
}

void VulkanRenderer::CreateCommandBuffers() {
   m_commandBuffers = std::make_unique<VulkanCommandBuffers>(
      m_device, m_device.GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT);
}

void VulkanRenderer::RecordCommandBuffer(const uint32_t imageIndex) {
   // Setup record
   m_commandBuffers->Begin(0, m_currentFrame);
   // Start a render pass
   // Clear values for attachments
   std::vector<VkClearValue> clearValues(3);
   clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // albedo
   clearValues[1].color = {{0.5f, 0.5f, 1.0f, 0.0f}}; // normal
   clearValues[2].depthStencil = {1.0f, 0};
   m_commandBuffers->BeginRenderPass(*m_geometryRenderPass, m_geometryFramebuffers[0],
                                     m_swapchain.GetExtent(), clearValues, m_currentFrame);

   m_commandBuffers->BindPipeline(m_geometryGraphicsPipeline->GetPipeline(),
                                  VK_PIPELINE_BIND_POINT_GRAPHICS, m_currentFrame);
   vkCmdBindDescriptorSets(m_commandBuffers->Get(m_currentFrame), VK_PIPELINE_BIND_POINT_GRAPHICS,
                           m_geometryPipelineLayout->Get(), 0, 1, &m_descriptorSets[m_currentFrame],
                           0, nullptr);
   // Set dynamic viewport and scissor
   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(m_swapchain.GetExtent().width);
   viewport.height = static_cast<float>(m_swapchain.GetExtent().height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   m_commandBuffers->SetViewport(viewport, m_currentFrame);
   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = m_swapchain.GetExtent();
   m_commandBuffers->SetScissor(scissor, m_currentFrame);
   // Draw the scene
   if (m_activeScene) {
      // Update transforms
      m_activeScene->UpdateTransforms();
      m_activeScene->ForEachNode([&](const Node* node) {
         // Skip inactive nodes
         if (!node->IsActive())
            return;
         if (const auto* renderer = node->GetComponent<RendererComponent>()) {
            // If not visible, do not render
            if (!renderer->IsVisible() || !renderer->HasMesh())
               return;
            // If has position, load it in
            if (const Transform* worldTransform = node->GetWorldTransform()) {
               // Set up transformation matrix for rendering
               ObjectData objData{worldTransform->GetTransformMatrix()};
               vkCmdPushConstants(m_commandBuffers->Get(m_currentFrame),
                                  m_geometryPipelineLayout->Get(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                                  sizeof(ObjectData), &objData);
            }
            if (const IMesh* mesh = m_resourceManager->GetMesh(renderer->GetMesh())) {
               const VulkanMesh* vkMesh = reinterpret_cast<const VulkanMesh*>(mesh);
               vkMesh->Draw(m_commandBuffers->Get(m_currentFrame));
            }
         }
      });
   }

   // TODO: Currently blitting color to screen
   {
      // Acquire handles
      ITexture* albedoTex = m_resourceManager->GetTexture(m_gAlbedoTexture);
      if (!albedoTex) {
         m_commandBuffers->End(m_currentFrame);
         throw std::runtime_error("Missing albedo texture for blit.");
      }
      VulkanTexture* vkAlbedo = reinterpret_cast<VulkanTexture*>(albedoTex);
      // Transition albedo image to TRANSFER_SRC_OPTIMAL
      vkAlbedo->TransitionLayout(
         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT /* mip level*/);
      // Transition swapchain image (the destination) from PRESENT_SRC to TRANSFER_DST
      VkImage swapchainImage = m_swapchain.GetImages()[imageIndex];
      // Helper function: transition image layout for swapchain image. We do it manually here.
      VkImageMemoryBarrier barrierFromPresentToTransferDst{};
      barrierFromPresentToTransferDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrierFromPresentToTransferDst.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      barrierFromPresentToTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrierFromPresentToTransferDst.oldLayout =
         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // driver may have undefined
      barrierFromPresentToTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrierFromPresentToTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrierFromPresentToTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrierFromPresentToTransferDst.image = swapchainImage;
      barrierFromPresentToTransferDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrierFromPresentToTransferDst.subresourceRange.baseMipLevel = 0;
      barrierFromPresentToTransferDst.subresourceRange.levelCount = 1;
      barrierFromPresentToTransferDst.subresourceRange.baseArrayLayer = 0;
      barrierFromPresentToTransferDst.subresourceRange.layerCount = 1;
      vkCmdPipelineBarrier(m_commandBuffers->Get(m_currentFrame), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &barrierFromPresentToTransferDst);
      // Blit region
      VkImageBlit blit{};
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.srcSubresource.layerCount = 1;
      blit.srcSubresource.mipLevel = 0;
      blit.srcOffsets[0] = {0, 0, 0};
      blit.srcOffsets[1] = {static_cast<int32_t>(m_swapchain.GetExtent().width),
                            static_cast<int32_t>(m_swapchain.GetExtent().height), 1};
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.baseArrayLayer = 0;
      blit.dstSubresource.layerCount = 1;
      blit.dstSubresource.mipLevel = 0;
      blit.dstOffsets[0] = {0, 0, 0};
      blit.dstOffsets[1] = {static_cast<int32_t>(m_swapchain.GetExtent().width),
                            static_cast<int32_t>(m_swapchain.GetExtent().height), 1};
      vkCmdBlitImage(m_commandBuffers->Get(m_currentFrame), vkAlbedo->GetImage(),
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchainImage,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
      // Transition swapchain image to PRESENT_SRC
      VkImageMemoryBarrier barrierToPresent{};
      barrierToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrierToPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrierToPresent.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      barrierToPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrierToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      barrierToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrierToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrierToPresent.image = swapchainImage;
      barrierToPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrierToPresent.subresourceRange.baseMipLevel = 0;
      barrierToPresent.subresourceRange.levelCount = 1;
      barrierToPresent.subresourceRange.baseArrayLayer = 0;
      barrierToPresent.subresourceRange.layerCount = 1;
      vkCmdPipelineBarrier(m_commandBuffers->Get(m_currentFrame), VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &barrierToPresent);
      // Transition albedo back to SHADER_READ_ONLY_OPTIMAL for future sampling
      vkAlbedo->TransitionLayout(
         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT /* mip level */);
   }

   // TODO: Clean up imgui stuff
   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_commandBuffers->Get(m_currentFrame));
   m_commandBuffers->EndRenderPass(m_currentFrame);
   m_commandBuffers->End(m_currentFrame);
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
      vkCreateSemaphore(m_device.Get(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
      vkCreateFence(m_device.Get(), &fenceInfo, nullptr, &m_inFlightFences[i]);
   }
   // Create per-image render-finished semaphores
   for (size_t i = 0; i < imageCount; ++i) {
      vkCreateSemaphore(m_device.Get(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
   }
}

void VulkanRenderer::RecreateSwapchain() {
   vkDeviceWaitIdle(m_device.Get());
   CleanupSwapchain();
   m_swapchain.Recreate();
   CreateDepthResources();
   CreateGeometryPass();
   CreateGeometryFBO();
   if (m_activeCamera) {
      m_activeCamera->SetAspectRatio(static_cast<float>(m_swapchain.GetExtent().width) /
                                     static_cast<float>(m_swapchain.GetExtent().height));
   }
}

void VulkanRenderer::CleanupSwapchain() {
   for (const VkFramebuffer& framebuffer : m_geometryFramebuffers) {
      vkDestroyFramebuffer(m_device.Get(), framebuffer, nullptr);
   }
   m_geometryFramebuffers.clear();
}

void VulkanRenderer::CreateDepthResources() {
   m_depthTexture = m_resourceManager->CreateDepthTexture(
      "depth_texture", m_swapchain.GetExtent().width, m_swapchain.GetExtent().height);
   ITexture* depthTexture = m_resourceManager->GetTexture(m_depthTexture);
   if (depthTexture) {
      VulkanTexture* vkDepthTexture = reinterpret_cast<VulkanTexture*>(depthTexture);
      vkDepthTexture->TransitionLayout(
         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
   }
}

void VulkanRenderer::CreateTestResources() {
   m_texture = m_resourceManager->LoadTexture(
      "test_texture", "resources/textures/wood_tile_01_BaseColor.png", true, true);
}

void VulkanRenderer::CreateUniformBuffer() {
   const VkDeviceSize bufferSize = sizeof(CameraData);
   m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      m_uniformBuffers[i] = std::make_unique<VulkanBuffer>(
         m_device, bufferSize, VulkanBuffer::Usage::Uniform, VulkanBuffer::MemoryType::HostVisible);
   }
}

void VulkanRenderer::UpdateUniformBuffer(const uint32_t currentImage) {
   if (!m_activeCamera)
      return;
   CameraData camData{};
   m_activeCamera->SetAspectRatio(static_cast<float>(m_swapchain.GetExtent().width) /
                                  static_cast<float>(m_swapchain.GetExtent().height));
   camData.view = m_activeCamera->GetViewMatrix();
   camData.proj = m_activeCamera->GetProjectionMatrix();
   m_uniformBuffers[currentImage]->UpdateMapped(&camData, sizeof(CameraData));
}

void VulkanRenderer::CreateDescriptorPool() {
   std::array<VkDescriptorPoolSize, 2> poolSizes{};
   poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   VkDescriptorPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
   poolInfo.pPoolSizes = poolSizes.data();
   poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   if (vkCreateDescriptorPool(m_device.Get(), &poolInfo, nullptr, &m_descriptorPool) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor pool.");
   }
}

void VulkanRenderer::CreateDescriptorSets() {
   std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_geometryDescriptorSetLayout);
   VkDescriptorSetAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfo.descriptorPool = m_descriptorPool;
   allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   allocInfo.pSetLayouts = layouts.data();
   m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
   if (vkAllocateDescriptorSets(m_device.Get(), &allocInfo, m_descriptorSets.data()) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate descriptor sets.");
   }
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = m_uniformBuffers[i]->Get();
      bufferInfo.offset = 0;
      bufferInfo.range = sizeof(CameraData);
      VkDescriptorImageInfo imageInfo{};
      ITexture* texture = m_resourceManager->GetTexture(m_texture);
      if (texture) {
         VulkanTexture* vkTexture = reinterpret_cast<VulkanTexture*>(texture);
         imageInfo = vkTexture->GetDescriptorImageInfo();
      }
      std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = m_descriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo = &bufferInfo;
      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = m_descriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pImageInfo = &imageInfo;
      vkUpdateDescriptorSets(m_device.Get(), static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
   }
}

VulkanRenderer::~VulkanRenderer() {
   vkDeviceWaitIdle(m_device.Get());
   CleanupSwapchain();
   vkDestroyDescriptorPool(m_device.Get(), m_descriptorPool, nullptr);
   vkDestroyDescriptorSetLayout(m_device.Get(), m_geometryDescriptorSetLayout, nullptr);
   for (size_t i = 0; i < m_renderFinishedSemaphores.size(); ++i) {
      vkDestroySemaphore(m_device.Get(), m_renderFinishedSemaphores[i], nullptr);
   }
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vkDestroySemaphore(m_device.Get(), m_imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(m_device.Get(), m_inFlightFences[i], nullptr);
   }
   DestroyImgui();
}

void VulkanRenderer::SetupImgui() {
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   if (!ImGui_ImplGlfw_InitForVulkan(m_window->GetNativeWindow(), true)) {
      throw std::runtime_error("ImGUI initialization failed.");
   }
   VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
   VkDescriptorPoolCreateInfo pool_info{};
   pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
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
   imguiInfo.RenderPass = m_geometryRenderPass->Get();
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
   const ImGuiViewport* viewport = ImGui::GetMainViewport();
   m_activeScene->DrawInspector(*m_materialEditor);
   // Render material editor
   m_materialEditor->DrawMaterialBrowser();
   m_materialEditor->DrawMaterialProperties();
   m_materialEditor->DrawTextureBrowser();
   // FPS Overlay
   PerformanceGUI::RenderPerformanceGUI(*m_resourceManager.get(), *m_activeScene);
   // Imgui render end
   ImGui::Render();
}

void VulkanRenderer::RenderFrame() {
   // Wait for previous farme
   vkWaitForFences(m_device.Get(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
   // Get the next image of the swapchain
   uint32_t imageIndex;
   const VkResult nextImageResult = m_swapchain.AcquireNextImage(
      UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], &imageIndex);
   if (nextImageResult == VK_ERROR_OUT_OF_DATE_KHR || nextImageResult == VK_SUBOPTIMAL_KHR) {
      RecreateSwapchain();
      return;
   } else if (nextImageResult != VK_SUCCESS) {
      throw std::runtime_error("Failed to acquire swap chain image.");
   }
   vkResetFences(m_device.Get(), 1, &m_inFlightFences[m_currentFrame]);
   // Setup command buffer to draw the triangle
   m_commandBuffers->Reset(m_currentFrame);
   UpdateUniformBuffer(m_currentFrame);
   RenderImgui();
   RecordCommandBuffer(imageIndex);
   // Submit the command buffer
   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
   VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores = waitSemaphores;
   submitInfo.pWaitDstStageMask = waitStages;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &m_commandBuffers->Get(m_currentFrame);
   VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[imageIndex]};
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
   const VkResult presentResult = vkQueuePresentKHR(m_device.GetPresentQueue(), &presentInfo);
   if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
      RecreateSwapchain();
   } else if (presentResult != VK_SUCCESS) {
      throw std::runtime_error("Failed to present swap chain image.");
   }
   // Increase frame counter
   m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

ResourceManager* VulkanRenderer::GetResourceManager() const noexcept {
   return m_resourceManager.get();
}

void VulkanRenderer::CreateDefaultMaterial() {
   auto defaultMat = m_resourceManager->CreateMaterial("default_pbr", "PBR");
   if (IMaterial* material = m_resourceManager->GetMaterial(defaultMat)) {
      material->SetParameter("albedo", glm::vec3(0.8f, 0.8f, 0.8f));
      material->SetParameter("metallic", 0.0f);
      material->SetParameter("roughness", 0.8f);
      material->SetParameter("ao", 1.0f);
   }
}
