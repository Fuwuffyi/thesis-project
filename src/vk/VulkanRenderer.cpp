#include "VulkanRenderer.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "core/Vertex.hpp"
#include "core/Window.hpp"
#include "core/Camera.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/Node.hpp"
#include "core/scene/components/RendererComponent.hpp"
#include "core/editor/MaterialEditor.hpp"

#include "vk/resource/VulkanMesh.hpp"
#include "vk/resource/VulkanResourceFactory.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>
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

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VulkanRenderer::VulkanRenderer(Window* windowHandle)
    : IRenderer(windowHandle),
      m_instance(deviceExtensions, validationLayers, enableValidationLayers),
#ifndef NDEBUG
      m_debugMessenger(m_instance),
#endif
      m_surface(m_instance, m_window->GetNativeWindow()),
      m_device(m_instance, m_surface, deviceExtensions, validationLayers, enableValidationLayers),
      m_swapchain(m_device, m_surface, *m_window),
      m_renderPass(m_device, m_swapchain.GetFormat(), FindDepthFormat()) {
   m_resourceManager =
      std::make_unique<ResourceManager>(std::make_unique<VulkanResourceFactory>(m_device));
   m_materialEditor = std::make_unique<MaterialEditor>(m_resourceManager.get());
   CreateDescriptorSetLayout();
   CreateGraphicsPipeline();
   CreateDepthResources();
   CreateFramebuffers();
   SetupImgui();
   CreateTestResources();
   CreateUniformBuffer();
   CreateDescriptorPool();
   CreateDescriptorSets();
   CreateCommandBuffers();
   CreateSynchronizationObjects();

   m_window->SetResizeCallback([this](int32_t width, int32_t height) { RecreateSwapchain(); });
}

void VulkanRenderer::CreateGraphicsPipeline() {
   const VulkanShaderModule vertShader(m_device, "resources/shaders/vk/test.vert.spv");
   const VulkanShaderModule fragShader(m_device, "resources/shaders/vk/test.frag.spv");
   VkPushConstantRange pushConstantRange{};
   pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   pushConstantRange.offset = 0;
   pushConstantRange.size = sizeof(glm::mat4);
   m_pipelineLayout = std::make_unique<VulkanPipelineLayout>(
      m_device, std::vector<VkDescriptorSetLayout>{m_descriptorSetLayout},
      std::vector<VkPushConstantRange>{pushConstantRange});
   VulkanGraphicsPipelineBuilder builder(m_device);
   builder
      // Set the shaders
      .SetVertexShader(vertShader.Get())
      .SetFragmentShader(fragShader.Get())
      // Setup depth testing
      .EnableDepthTest(VK_COMPARE_OP_LESS)
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
      .AddViewport(VkViewport{})
      .AddScissor(VkRect2D{})
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
   m_graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(builder.Build());
}

void VulkanRenderer::CreateDescriptorSetLayout() {
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
   if (vkCreateDescriptorSetLayout(m_device.Get(), &layoutInfo, nullptr, &m_descriptorSetLayout) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor set layout.");
   }
}

void VulkanRenderer::CreateFramebuffers() {
   // Create framebuffers from the given swapchain images
   m_swapchainFramebuffers.resize(m_swapchain.GetImageViews().size());
   for (size_t i = 0; i < m_swapchain.GetImageViews().size(); ++i) {
      std::vector<VkImageView> attachments;
      attachments.reserve(2);
      attachments.push_back(m_swapchain.GetImageViews()[i]);
      ITexture* depthTexture = m_resourceManager->GetTexture(m_depthTexture);
      if (depthTexture) {
         VulkanTexture* vkDepthTexture = reinterpret_cast<VulkanTexture*>(depthTexture);
         attachments.push_back(vkDepthTexture->GetImageView());
      }
      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = m_renderPass.Get();
      framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
      framebufferInfo.pAttachments = attachments.data();
      framebufferInfo.width = m_swapchain.GetExtent().width;
      framebufferInfo.height = m_swapchain.GetExtent().height;
      framebufferInfo.layers = 1;
      if (vkCreateFramebuffer(m_device.Get(), &framebufferInfo, nullptr,
                              &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create a framebuffer.");
      }
   }
}

void VulkanRenderer::CreateCommandBuffers() {
   m_commandBuffers = std::make_unique<VulkanCommandBuffers>(
      m_device, m_device.GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT);
}

void VulkanRenderer::RecordCommandBuffer(const uint32_t imageIndex) {
   // Rotate the object for fun
   static auto startTime = std::chrono::high_resolution_clock::now();
   auto currentTime = std::chrono::high_resolution_clock::now();
   float time =
      std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
   // Setup record
   m_commandBuffers->Begin(0, m_currentFrame);
   // Start a render pass
   m_commandBuffers->BeginRenderPass(m_renderPass, m_swapchainFramebuffers[imageIndex],
                                     m_swapchain.GetExtent(),
                                     {{{0.0f, 0.0f, 0.0f, 1.0f}}, {{1.0f, 0.0f}}}, m_currentFrame);
   m_commandBuffers->BindPipeline(m_graphicsPipeline->GetPipeline(),
                                  VK_PIPELINE_BIND_POINT_GRAPHICS, m_currentFrame);
   // Set the dynamic viewport and scissor
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
   // Set triangle position/rotation/scale
   vkCmdBindDescriptorSets(m_commandBuffers->Get(m_currentFrame), VK_PIPELINE_BIND_POINT_GRAPHICS,
                           m_pipelineLayout->Get(), 0, 1, &m_descriptorSets[m_currentFrame], 0,
                           nullptr);
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
               vkCmdPushConstants(m_commandBuffers->Get(m_currentFrame), m_pipelineLayout->Get(),
                                  VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectData), &objData);
            }
            // Render the mesh
            if (renderer->IsMultiMesh()) {
               for (const auto& subMeshRenderer : renderer->GetSubMeshRenderers()) {
                  if (!subMeshRenderer.visible)
                     continue;
                  if (const IMesh* mesh = m_resourceManager->GetMesh(subMeshRenderer.mesh)) {
                     const VulkanMesh* vkMesh = reinterpret_cast<const VulkanMesh*>(mesh);
                     vkMesh->Draw(m_commandBuffers->Get(m_currentFrame));
                  }
               }
            } else {
               if (const IMesh* mesh = m_resourceManager->GetMesh(renderer->GetMesh())) {
                  const VulkanMesh* vkMesh = reinterpret_cast<const VulkanMesh*>(mesh);
                  vkMesh->Draw(m_commandBuffers->Get(m_currentFrame));
               }
            }
         }
      });
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
   CreateFramebuffers();
   if (m_activeCamera) {
      m_activeCamera->SetAspectRatio(static_cast<float>(m_swapchain.GetExtent().width) /
                                     static_cast<float>(m_swapchain.GetExtent().height));
   }
}

void VulkanRenderer::CleanupSwapchain() {
   for (const VkFramebuffer& framebuffer : m_swapchainFramebuffers) {
      vkDestroyFramebuffer(m_device.Get(), framebuffer, nullptr);
   }
}

VkFormat VulkanRenderer::FindSupportedFormat(const std::vector<VkFormat>& candidates,
                                             const VkImageTiling& tiling,
                                             const VkFormatFeatureFlags features) const {
   for (const VkFormat& format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(m_device.GetPhysicalDevice(), format, &props);
      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
         return format;
      } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                 (props.optimalTilingFeatures & features) == features) {
         return format;
      }
   }
   throw std::runtime_error("Failed to find supported format.");
}

VkFormat VulkanRenderer::FindDepthFormat() const {
   return FindSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool VulkanRenderer::HasStencilComponent(const VkFormat& format) const {
   return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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
   m_texture = m_resourceManager->LoadTexture("test_texture", "resources/textures/bricks_color.jpg",
                                              true, true);
   m_textureSampler =
      std::make_unique<VulkanSampler>(VulkanSampler::CreateAnisotropic(m_device, 16.0f));
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
   std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
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
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      ITexture* texture = m_resourceManager->GetTexture(m_texture);
      if (texture) {
         VulkanTexture* vkTexture = reinterpret_cast<VulkanTexture*>(texture);
         imageInfo.imageView = vkTexture->GetImageView();
      }
      imageInfo.sampler = m_textureSampler->Get();
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
   vkDestroyDescriptorSetLayout(m_device.Get(), m_descriptorSetLayout, nullptr);
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
   const ImGuiViewport* viewport = ImGui::GetMainViewport();
   m_activeScene->DrawInspector(*m_materialEditor);
   // FPS Overlay
   {
      ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
      ImGui::SetNextWindowBgAlpha(0.35f);
      ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoSavedSettings |
                               ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
      ImGui::Begin("FPS Overlay", nullptr, flags);
      ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
      ImGui::Text("Frame: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
      ImGui::Text(
         "Resource MEM: %.3f MB",
         static_cast<float>(m_resourceManager->GetTotalMemoryUsage()) / (1024.0f * 1024.0f));
      ImGui::End();
   }
   // Show textures
   {
      const uint32_t columns = 4;
      const uint32_t imgSize = 128;
      ImGui::SetNextWindowPos(
         ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - 600));
      ImGui::SetNextWindowSize(ImVec2(columns * imgSize, 600));
      ImGui::SetNextWindowBgAlpha(0.35f);
      ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                               ImGuiWindowFlags_NoSavedSettings |
                               ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
      if (ImGui::Begin("Texture Browser", nullptr, flags)) {
         ImGui::BeginChild("TextureScrollRegion", ImVec2(0, 0), false,
                           ImGuiWindowFlags_HorizontalScrollbar);
         ImGui::Columns(columns, nullptr, false);
         const auto namedTextures = m_resourceManager->GetAllTexturesNamed();
         for (const auto& tex : namedTextures) {
            if (tex.first) {
               // TODO: Render textures
            }
            ImGui::NextColumn();
         }
         ImGui::Columns(1);
         ImGui::EndChild();
      }
      ImGui::End();
   }
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
