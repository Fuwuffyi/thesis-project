#include "VulkanRenderer.hpp"

#include "core/Window.hpp"
#include "core/Camera.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/Node.hpp"
#include "core/scene/components/LightComponent.hpp"
#include "core/scene/components/ParticleSystemComponent.hpp"
#include "core/scene/components/RendererComponent.hpp"

#include "core/editor/PerformanceGUI.hpp"
#include "core/editor/MaterialEditor.hpp"

#include "vk/resource/VulkanMaterial.hpp"
#include "vk/resource/VulkanMesh.hpp"
#include "vk/resource/VulkanResourceFactory.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

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
   alignas(16) glm::vec3 viewPos;
};

struct LightData {
   alignas(4) uint32_t lightType;
   alignas(16) glm::vec3 position;
   alignas(16) glm::vec3 direction;
   alignas(16) glm::vec3 color;
   alignas(4) float intensity;
   alignas(4) float constant;
   alignas(4) float linear;
   alignas(4) float quadratic;
   alignas(4) float innerCone;
   alignas(4) float outerCone;
};

struct LightsData {
   alignas(4) uint32_t lightCount;
   std::array<LightData, VulkanRenderer::MAX_LIGHTS> lights;
};

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

   CreateUBOs();
   CreateMaterialDescriptorSetLayout();
   CreateMaterialDescriptorPool();

   CreateGeometryDescriptorSetLayout();
   CreateGeometryPass();
   CreateGeometryFBO();
   CreateGeometryPipeline();

   CreateLightingDescriptorSetLayout();
   CreateLightingPass();
   CreateLightingFBO();
   CreateLightingPipeline();

   CreateGizmoDescriptorSetLayout();
   CreateGizmoPipeline();

   CreateParticleDescriptorSetLayout();
   CreateParticlePipeline();
   CreateParticleInstanceBuffers();

   CreateCommandBuffers();

   SetupImgui();

   CreateDescriptorPool();
   CreateDescriptorSets();

   CreateSynchronizationObjects();

   CreateUtilityMeshes();
   CreateDefaultMaterial();

   m_window->SetResizeCallback([this](int32_t width, int32_t height) { RecreateSwapchain(); });
}

void VulkanRenderer::CreateMaterialDescriptorSetLayout() {
   std::vector<VkDescriptorSetLayoutBinding> bindings;
   VkDescriptorSetLayoutBinding uboBinding{};
   uboBinding.binding = 16;
   uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   uboBinding.descriptorCount = 1;
   uboBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
   uboBinding.pImmutableSamplers = nullptr;
   bindings.push_back(uboBinding);
   for (uint32_t i = 0; i <= 4; ++i) {
      VkDescriptorSetLayoutBinding samplerBinding{};
      samplerBinding.binding = i;
      samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      samplerBinding.descriptorCount = 1;
      samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      samplerBinding.pImmutableSamplers = nullptr;
      bindings.push_back(samplerBinding);
   }
   VkDescriptorSetLayoutCreateInfo layoutInfo{};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
   layoutInfo.pBindings = bindings.data();
   if (vkCreateDescriptorSetLayout(m_device.Get(), &layoutInfo, nullptr,
                                   &m_materialDescriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create material descriptor set layout");
   }
}

void VulkanRenderer::CreateMaterialDescriptorPool() {
   constexpr uint32_t maxMaterials = 1000;
   std::array<VkDescriptorPoolSize, 2> poolSizes{};
   poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   poolSizes[0].descriptorCount = maxMaterials;
   poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   poolSizes[1].descriptorCount = maxMaterials * 5; // 5 textures per material
   VkDescriptorPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
   poolInfo.pPoolSizes = poolSizes.data();
   poolInfo.maxSets = maxMaterials;
   poolInfo.flags =
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Allow freeing individual sets
   if (vkCreateDescriptorPool(m_device.Get(), &poolInfo, nullptr, &m_materialDescriptorPool) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to create material descriptor pool");
   }
}

void VulkanRenderer::SetupMaterialDescriptorSets() {
   // Setup descriptor sets for all existing materials
   const auto& materials = m_resourceManager->GetAllMaterialsNamed();
   for (const auto& [material, name] : materials) {
      if (auto* vkMaterial = dynamic_cast<VulkanMaterial*>(material)) {
         if (vkMaterial->GetDescriptorSet() == VK_NULL_HANDLE) {
            vkMaterial->CreateDescriptorSet(m_materialDescriptorPool,
                                            m_materialDescriptorSetLayout);
            vkMaterial->UpdateDescriptorSet(*m_resourceManager);
         }
      }
   }
}

void VulkanRenderer::CreateGeometryDescriptorSetLayout() {
   VkDescriptorSetLayoutBinding cameraUboBinding{};
   cameraUboBinding.binding = 0;
   cameraUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   cameraUboBinding.descriptorCount = 1;
   cameraUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
   cameraUboBinding.pImmutableSamplers = nullptr;
   const std::array<VkDescriptorSetLayoutBinding, 1> bindings = {cameraUboBinding};
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
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      m_gAlbedoTexture[i] = m_resourceManager->CreateRenderTarget(
         "gbuffer_albedo_" + std::to_string(i), m_swapchain.GetExtent().width,
         m_swapchain.GetExtent().height, ITexture::Format::RGBA8);
      m_gNormalTexture[i] = m_resourceManager->CreateRenderTarget(
         "gbuffer_normal_" + std::to_string(i), m_swapchain.GetExtent().width,
         m_swapchain.GetExtent().height, ITexture::Format::RGBA16F);
      m_gDepthTexture[i] = m_resourceManager->CreateDepthTexture(
         "gbuffer_depth_" + std::to_string(i), m_swapchain.GetExtent().width,
         m_swapchain.GetExtent().height);
   }
   ITexture* albedoTex = m_resourceManager->GetTexture(m_gAlbedoTexture[0]);
   ITexture* normalTex = m_resourceManager->GetTexture(m_gNormalTexture[0]);
   ITexture* depthTex = m_resourceManager->GetTexture(m_gDepthTexture[0]);
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
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      // Create a single framebuffer for the G-buffer using the created textures' image views
      ITexture* albedoTex = m_resourceManager->GetTexture(m_gAlbedoTexture[i]);
      ITexture* normalTex = m_resourceManager->GetTexture(m_gNormalTexture[i]);
      ITexture* depthTex = m_resourceManager->GetTexture(m_gDepthTexture[i]);
      if (!albedoTex || !normalTex || !depthTex)
         throw std::runtime_error("Failed to fetch gbuffer textures for framebuffer creation.");
      VulkanTexture* vkAlbedo = reinterpret_cast<VulkanTexture*>(albedoTex);
      VulkanTexture* vkNormal = reinterpret_cast<VulkanTexture*>(normalTex);
      VulkanTexture* vkDepth = reinterpret_cast<VulkanTexture*>(depthTex);
      vkAlbedo->TransitionLayout(
         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
      vkNormal->TransitionLayout(
         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
      vkAlbedo->UpdateSamplerSettings(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
      vkNormal->UpdateSamplerSettings(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
      vkDepth->UpdateSamplerSettings(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
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
      if (vkCreateFramebuffer(m_device.Get(), &framebufferInfo, nullptr,
                              &m_geometryFramebuffers[i]) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create G-buffer framebuffer.");
      }
   }
}

void VulkanRenderer::CreateGeometryPipeline() {
   // Load shaders
   const VulkanShaderModule vertShader(m_device,
                                       std::string("resources/shaders/vk/geometry_pass.vert.spv"));
   const VulkanShaderModule fragShader(m_device,
                                       std::string("resources/shaders/vk/geometry_pass.frag.spv"));
   VkPushConstantRange modelPushConstant{};
   modelPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   modelPushConstant.offset = 0;
   modelPushConstant.size = sizeof(glm::mat4);
   m_geometryPipelineLayout = std::make_unique<VulkanPipelineLayout>(
      m_device,
      std::vector<VkDescriptorSetLayout>{m_geometryDescriptorSetLayout,
                                         m_materialDescriptorSetLayout},
      std::vector<VkPushConstantRange>{modelPushConstant});
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
      .AddScissor(VkRect2D{})
      .AddViewport(VkViewport{})
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
   cbState.attachments.push_back(cb);
   builder.SetColorBlendState(cbState);
   // Build and store pipeline
   VulkanGraphicsPipeline pipelineObj = builder.Build();
   m_geometryGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(std::move(pipelineObj));
}

void VulkanRenderer::CreateLightingDescriptorSetLayout() {
   VkDescriptorSetLayoutBinding cameraUboBinding{};
   cameraUboBinding.binding = 0;
   cameraUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   cameraUboBinding.descriptorCount = 1;
   cameraUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
   cameraUboBinding.pImmutableSamplers = nullptr;
   VkDescriptorSetLayoutBinding lightingUboBinding{};
   lightingUboBinding.binding = 1;
   lightingUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   lightingUboBinding.descriptorCount = 1;
   lightingUboBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
   lightingUboBinding.pImmutableSamplers = nullptr;
   VkDescriptorSetLayoutBinding albedoSamplerBinding{};
   albedoSamplerBinding.binding = 3;
   albedoSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   albedoSamplerBinding.descriptorCount = 1;
   albedoSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
   albedoSamplerBinding.pImmutableSamplers = nullptr;
   VkDescriptorSetLayoutBinding normalSamplerBinding{};
   normalSamplerBinding.binding = 4;
   normalSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   normalSamplerBinding.descriptorCount = 1;
   normalSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
   normalSamplerBinding.pImmutableSamplers = nullptr;
   VkDescriptorSetLayoutBinding depthSamplerBinding{};
   depthSamplerBinding.binding = 5;
   depthSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   depthSamplerBinding.descriptorCount = 1;
   depthSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
   depthSamplerBinding.pImmutableSamplers = nullptr;
   const std::array<VkDescriptorSetLayoutBinding, 5> bindings = {
      cameraUboBinding, lightingUboBinding, albedoSamplerBinding, normalSamplerBinding,
      depthSamplerBinding};
   VkDescriptorSetLayoutCreateInfo layoutInfo{};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
   layoutInfo.pBindings = bindings.data();
   if (vkCreateDescriptorSetLayout(m_device.Get(), &layoutInfo, nullptr,
                                   &m_lightingDescriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor set layout.");
   }
}

void VulkanRenderer::CreateLightingPass() {
   RenderPassDescription desc;
   AttachmentDescription colorAtt{};
   colorAtt.format = m_swapchain.GetFormat();
   colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
   colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   desc.attachments.push_back(colorAtt);
   AttachmentDescription depthAtt{};
   depthAtt.format = VK_FORMAT_D32_SFLOAT;
   depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
   depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
   depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAtt.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
   desc.attachments.push_back(depthAtt);
   SubpassDescription subpass{};
   subpass.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   VkAttachmentReference colorRef{};
   colorRef.attachment = 0;
   colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   subpass.colorAttachments.push_back(colorRef);
   VkAttachmentReference depthRef{};
   depthRef.attachment = 1;
   depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
   subpass.depthStencilAttachment = depthRef;
   desc.subpasses.push_back(subpass);
   VkSubpassDependency dep{};
   dep.srcSubpass = VK_SUBPASS_EXTERNAL;
   dep.dstSubpass = 0;
   dep.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
   dep.srcAccessMask = 0;
   dep.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
   dep.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
   desc.dependencies.push_back(dep);
   m_lightingRenderPass = std::make_unique<VulkanRenderPass>(m_device, desc);
}

void VulkanRenderer::CreateLightingFBO() {
   const auto& imageViews = m_swapchain.GetImageViews();
   m_lightingFramebuffers.resize(imageViews.size());
   for (uint32_t i = 0; i < imageViews.size(); ++i) {
      const uint32_t depthIndex = i % MAX_FRAMES_IN_FLIGHT;
      ITexture* depthTex = m_resourceManager->GetTexture(m_gDepthTexture[depthIndex]);
      if (!depthTex)
         throw std::runtime_error("Failed to get depth texture for lighting framebuffer");
      const VulkanTexture* vkDepth = reinterpret_cast<VulkanTexture*>(depthTex);
      const VkImageView attachments[2] = {imageViews[i], vkDepth->GetImageView()};
      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = m_lightingRenderPass->Get();
      framebufferInfo.attachmentCount = 2;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = m_swapchain.GetExtent().width;
      framebufferInfo.height = m_swapchain.GetExtent().height;
      framebufferInfo.layers = 1;

      if (vkCreateFramebuffer(m_device.Get(), &framebufferInfo, nullptr,
                              &m_lightingFramebuffers[i]) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create lighting framebuffer.");
      }
   }
}

void VulkanRenderer::CreateLightingPipeline() {
   // Load shaders
   const VulkanShaderModule vertShader(m_device,
                                       std::string("resources/shaders/vk/lighting_pass.vert.spv"));
   const VulkanShaderModule fragShader(m_device,
                                       std::string("resources/shaders/vk/lighting_pass.frag.spv"));
   m_lightingPipelineLayout = std::make_unique<VulkanPipelineLayout>(
      m_device, std::vector<VkDescriptorSetLayout>{m_lightingDescriptorSetLayout});
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
      .SetCullMode(VK_CULL_MODE_NONE)
      .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .DisableDepthTest()
      .AddScissor(VkRect2D{})
      .AddViewport(VkViewport{})
      .SetPipelineLayout(m_lightingPipelineLayout->Get())
      .SetRenderPass(m_lightingRenderPass->Get());
   VulkanGraphicsPipelineBuilder::RasterizationState raster{};
   raster.cullMode = VK_CULL_MODE_NONE;
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
   m_lightingGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(std::move(pipelineObj));
}

void VulkanRenderer::CreateGizmoDescriptorSetLayout() {
   VkDescriptorSetLayoutBinding cameraUboBinding{};
   cameraUboBinding.binding = 0;
   cameraUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   cameraUboBinding.descriptorCount = 1;
   cameraUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   cameraUboBinding.pImmutableSamplers = nullptr;
   const std::array<VkDescriptorSetLayoutBinding, 1> bindings = {cameraUboBinding};
   VkDescriptorSetLayoutCreateInfo layoutInfo{};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
   layoutInfo.pBindings = bindings.data();
   if (vkCreateDescriptorSetLayout(m_device.Get(), &layoutInfo, nullptr,
                                   &m_gizmoDescriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor set layout.");
   }
}

void VulkanRenderer::CreateGizmoPipeline() {
   // Load shaders
   const VulkanShaderModule vertShader(m_device,
                                       std::string("resources/shaders/vk/gizmo_pass.vert.spv"));
   const VulkanShaderModule fragShader(m_device,
                                       std::string("resources/shaders/vk/gizmo_pass.frag.spv"));
   VkPushConstantRange transformPushConstant{};
   transformPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   transformPushConstant.offset = 0;
   transformPushConstant.size = sizeof(glm::mat4);
   VkPushConstantRange colorPushConstant{};
   colorPushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
   colorPushConstant.offset = 0;
   colorPushConstant.size = sizeof(glm::vec3);
   m_gizmoPipelineLayout = std::make_unique<VulkanPipelineLayout>(
      m_device, std::vector<VkDescriptorSetLayout>{m_gizmoDescriptorSetLayout},
      std::vector<VkPushConstantRange>{transformPushConstant, colorPushConstant});
   // Build pipeline using the builder
   VulkanGraphicsPipelineBuilder builder(m_device);
   builder.SetVertexShader(vertShader.Get())
      .SetFragmentShader(fragShader.Get())
      .AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
      .AddVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position))
      .SetTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
      .SetDynamicViewportAndScissor()
      .SetCullMode(VK_CULL_MODE_NONE)
      .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .EnableDepthTest(VK_COMPARE_OP_LESS)
      .AddScissor(VkRect2D{})
      .AddViewport(VkViewport{})
      .SetPipelineLayout(m_gizmoPipelineLayout->Get())
      .SetRenderPass(m_lightingRenderPass->Get());
   VulkanGraphicsPipelineBuilder::RasterizationState raster{};
   raster.cullMode = VK_CULL_MODE_NONE;
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
   m_gizmoGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(std::move(pipelineObj));
}
void VulkanRenderer::CreateParticleDescriptorSetLayout() {
   VkDescriptorSetLayoutBinding cameraUboBinding{};
   cameraUboBinding.binding = 0;
   cameraUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   cameraUboBinding.descriptorCount = 1;
   cameraUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   cameraUboBinding.pImmutableSamplers = nullptr;
   const std::array<VkDescriptorSetLayoutBinding, 1> bindings = {cameraUboBinding};
   VkDescriptorSetLayoutCreateInfo layoutInfo{};
   layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
   layoutInfo.pBindings = bindings.data();
   if (vkCreateDescriptorSetLayout(m_device.Get(), &layoutInfo, nullptr,
                                   &m_particleDescriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create particle descriptor set layout.");
   }
}

void VulkanRenderer::CreateParticlePipeline() {
   // Load shaders
   const VulkanShaderModule vertShader(m_device,
                                       std::string("resources/shaders/vk/particle_pass.vert.spv"));
   const VulkanShaderModule fragShader(m_device,
                                       std::string("resources/shaders/vk/particle_pass.frag.spv"));
   m_particlePipelineLayout = std::make_unique<VulkanPipelineLayout>(
      m_device, std::vector<VkDescriptorSetLayout>{m_particleDescriptorSetLayout});
   // Build pipeline
   VulkanGraphicsPipelineBuilder builder(m_device);
   builder.SetVertexShader(vertShader.Get())
      .SetFragmentShader(fragShader.Get())
      // Vertex attributes (per-vertex)
      .AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
      .AddVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position))
      .AddVertexAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal))
      .AddVertexAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv))
      // Instance attributes
      .AddVertexBinding(1, sizeof(ParticleInstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
      // Mat4 particle transform
      .AddVertexAttribute(3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0)
      .AddVertexAttribute(4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(glm::vec4))
      .AddVertexAttribute(5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sizeof(glm::vec4))
      .AddVertexAttribute(6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 3 * sizeof(glm::vec4))
      // Color
      .AddVertexAttribute(7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(glm::mat4))
      .SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
      .SetDynamicViewportAndScissor()
      .SetCullMode(VK_CULL_MODE_NONE)
      .SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .EnableDepthTest(VK_COMPARE_OP_LESS)
      .AddScissor(VkRect2D{})
      .AddViewport(VkViewport{})
      .SetPipelineLayout(m_particlePipelineLayout->Get())
      .SetRenderPass(m_lightingRenderPass->Get());
   VulkanGraphicsPipelineBuilder::RasterizationState raster{};
   raster.cullMode = VK_CULL_MODE_NONE;
   raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   builder.SetRasterizationState(raster);
   VulkanGraphicsPipelineBuilder::MultisampleState ms{};
   ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   builder.SetMultisampleState(ms);
   VulkanGraphicsPipelineBuilder::DepthStencilState ds{};
   ds.depthTestEnable = VK_TRUE;
   ds.depthWriteEnable = VK_FALSE;
   ds.depthCompareOp = VK_COMPARE_OP_LESS;
   builder.SetDepthStencilState(ds);
   // Configure alpha blending
   VulkanGraphicsPipelineBuilder::ColorBlendAttachmentState cb{};
   cb.blendEnable = VK_TRUE;
   cb.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   cb.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   cb.colorBlendOp = VK_BLEND_OP_ADD;
   cb.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   cb.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   cb.alphaBlendOp = VK_BLEND_OP_ADD;
   cb.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   VulkanGraphicsPipelineBuilder::ColorBlendState cbState{};
   cbState.attachments.push_back(cb);
   builder.SetColorBlendState(cbState);
   // Build pipeline
   VulkanGraphicsPipeline pipelineObj = builder.Build();
   m_particleGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(std::move(pipelineObj));
}

void VulkanRenderer::CreateParticleInstanceBuffers() {
   m_particleInstanceCapacity = 100000;
   const VkDeviceSize bufferSize = m_particleInstanceCapacity * sizeof(ParticleInstanceData);
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      m_particleInstanceBuffers[i] = std::make_unique<VulkanBuffer>(
         m_device, bufferSize, VulkanBuffer::Usage::Vertex, VulkanBuffer::MemoryType::CPUToGPU);
      m_particleInstanceBuffers[i]->Map();
   }
}

void VulkanRenderer::CreateCommandBuffers() {
   m_commandBuffers = std::make_unique<VulkanCommandBuffers>(
      m_device, m_device.GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT);
}

void VulkanRenderer::RecordCommandBuffer(const uint32_t imageIndex) {
   // Setup record
   m_commandBuffers->Begin(0, m_currentFrame);
   // Calculate dynamic viewport and scissor
   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(m_swapchain.GetExtent().width);
   viewport.height = static_cast<float>(m_swapchain.GetExtent().height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = m_swapchain.GetExtent();
   if (m_activeScene) [[likely]] {
      m_activeScene->UpdateScene(m_deltaTime);
      m_activeScene->UpdateTransforms();
   }
   // GEOMETRY PASS
   {
      std::vector<VkClearValue> clearValues(3);
      clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
      clearValues[1].color = {{0.5f, 0.5f, 1.0f, 0.0f}};
      clearValues[2].depthStencil = {1.0f, 0};
      m_commandBuffers->BeginRenderPass(*m_geometryRenderPass,
                                        m_geometryFramebuffers[m_currentFrame],
                                        m_swapchain.GetExtent(), clearValues, m_currentFrame);
      m_commandBuffers->BindPipeline(m_geometryGraphicsPipeline->GetPipeline(),
                                     VK_PIPELINE_BIND_POINT_GRAPHICS, m_currentFrame);
      vkCmdBindDescriptorSets(m_commandBuffers->Get(m_currentFrame),
                              VK_PIPELINE_BIND_POINT_GRAPHICS, m_geometryPipelineLayout->Get(), 0,
                              1, &m_geometryDescriptorSets[m_currentFrame], 0, nullptr);
      m_commandBuffers->SetViewport(viewport, m_currentFrame);
      m_commandBuffers->SetScissor(scissor, m_currentFrame);
      // Draw the scene
      if (m_activeScene) {
         // Update transforms
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
                  vkCmdPushConstants(m_commandBuffers->Get(m_currentFrame),
                                     m_geometryPipelineLayout->Get(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                                     sizeof(glm::mat4), &worldTransform->GetTransformMatrix());
               }
               if (IMaterial* material = m_resourceManager->GetMaterial(renderer->GetMaterial())) {
                  VulkanMaterial* vkMaterial = reinterpret_cast<VulkanMaterial*>(material);
                  if (vkMaterial->GetDescriptorSet() == VK_NULL_HANDLE) {
                     vkMaterial->CreateDescriptorSet(m_materialDescriptorPool,
                                                     m_materialDescriptorSetLayout);
                  }
                  vkMaterial->Bind(0, *m_resourceManager);
                  const VkDescriptorSet materialDescSet = vkMaterial->GetDescriptorSet();
                  vkCmdBindDescriptorSets(
                     m_commandBuffers->Get(m_currentFrame), VK_PIPELINE_BIND_POINT_GRAPHICS,
                     m_geometryPipelineLayout->Get(), 1, 1, &materialDescSet, 0, nullptr);
               }
               if (const IMesh* mesh = m_resourceManager->GetMesh(renderer->GetMesh())) {
                  const VulkanMesh* vkMesh = reinterpret_cast<const VulkanMesh*>(mesh);
                  vkMesh->Draw(m_commandBuffers->Get(m_currentFrame));
               }
            }
         });
      }
      m_commandBuffers->EndRenderPass(m_currentFrame);
   }
   // Transition G-buffer textures from attachment layouts to shader-read-only layouts
   {
      std::vector<VkImageMemoryBarrier> layoutTransitions;
      layoutTransitions.reserve(3);
      ITexture* albedoTex = m_resourceManager->GetTexture(m_gAlbedoTexture[m_currentFrame]);
      ITexture* normalTex = m_resourceManager->GetTexture(m_gNormalTexture[m_currentFrame]);
      ITexture* depthTex = m_resourceManager->GetTexture(m_gDepthTexture[m_currentFrame]);
      if (albedoTex && normalTex && depthTex) {
         VulkanTexture* vkAlbedo = reinterpret_cast<VulkanTexture*>(albedoTex);
         VulkanTexture* vkNormal = reinterpret_cast<VulkanTexture*>(normalTex);
         VulkanTexture* vkDepth = reinterpret_cast<VulkanTexture*>(depthTex);
         // Albedo texture transition
         VkImageMemoryBarrier albedoBarrier{};
         albedoBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
         albedoBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
         albedoBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         albedoBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         albedoBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         albedoBarrier.image = vkAlbedo->GetImage();
         albedoBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
         albedoBarrier.subresourceRange.baseMipLevel = 0;
         albedoBarrier.subresourceRange.levelCount = 1;
         albedoBarrier.subresourceRange.baseArrayLayer = 0;
         albedoBarrier.subresourceRange.layerCount = 1;
         albedoBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
         albedoBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
         layoutTransitions.push_back(albedoBarrier);
         // Normal texture transition
         VkImageMemoryBarrier normalBarrier{};
         normalBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
         normalBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
         normalBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         normalBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         normalBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         normalBarrier.image = vkNormal->GetImage();
         normalBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
         normalBarrier.subresourceRange.baseMipLevel = 0;
         normalBarrier.subresourceRange.levelCount = 1;
         normalBarrier.subresourceRange.baseArrayLayer = 0;
         normalBarrier.subresourceRange.layerCount = 1;
         normalBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
         normalBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
         layoutTransitions.push_back(normalBarrier);
         // Depth texture transition
         VkImageMemoryBarrier depthBarrier{};
         depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
         depthBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
         depthBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         depthBarrier.image = vkDepth->GetImage();
         depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
         depthBarrier.subresourceRange.baseMipLevel = 0;
         depthBarrier.subresourceRange.levelCount = 1;
         depthBarrier.subresourceRange.baseArrayLayer = 0;
         depthBarrier.subresourceRange.layerCount = 1;
         depthBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
         depthBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
         layoutTransitions.push_back(depthBarrier);
         // Execute the pipeline barrier
         vkCmdPipelineBarrier(m_commandBuffers->Get(m_currentFrame),
                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // srcStageMask
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                              static_cast<uint32_t>(layoutTransitions.size()),
                              layoutTransitions.data());
      }
   }
   // LIGHTING PASS
   {
      std::vector<VkClearValue> clearValues(2);
      clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
      clearValues[1].depthStencil = {1.0f, 0};
      m_commandBuffers->BeginRenderPass(*m_lightingRenderPass, m_lightingFramebuffers[imageIndex],
                                        m_swapchain.GetExtent(), clearValues, m_currentFrame);
      m_commandBuffers->BindPipeline(m_lightingGraphicsPipeline->GetPipeline(),
                                     VK_PIPELINE_BIND_POINT_GRAPHICS, m_currentFrame);
      vkCmdBindDescriptorSets(m_commandBuffers->Get(m_currentFrame),
                              VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightingPipelineLayout->Get(), 0,
                              1, &m_lightingDescriptorSets[m_currentFrame], 0, nullptr);
      m_commandBuffers->SetViewport(viewport, m_currentFrame);
      m_commandBuffers->SetScissor(scissor, m_currentFrame);

      if (const IMesh* mesh = m_resourceManager->GetMesh(m_fullscreenQuad)) {
         const VulkanMesh* vkMesh = reinterpret_cast<const VulkanMesh*>(mesh);
         vkMesh->Draw(m_commandBuffers->Get(m_currentFrame));
      }
   }
   // GIZMO PASS
   {
      m_commandBuffers->BindPipeline(m_gizmoGraphicsPipeline->GetPipeline(),
                                     VK_PIPELINE_BIND_POINT_GRAPHICS, m_currentFrame);
      vkCmdBindDescriptorSets(m_commandBuffers->Get(m_currentFrame),
                              VK_PIPELINE_BIND_POINT_GRAPHICS, m_gizmoPipelineLayout->Get(), 0, 1,
                              &m_gizmoDescriptorSets[m_currentFrame], 0, nullptr);
      m_commandBuffers->SetViewport(viewport, m_currentFrame);
      m_commandBuffers->SetScissor(scissor, m_currentFrame);
      if (m_activeScene) {
         m_activeScene->ForEachNode([&](const Node* node) {
            // Skip inactive nodes
            if (!node->IsActive())
               return;
            if (const auto* lightComp = node->GetComponent<LightComponent>()) {
               // If has position, load it in
               if (const Transform* worldTransform = node->GetWorldTransform()) {
                  // Set up transformation matrix for rendering
                  vkCmdPushConstants(m_commandBuffers->Get(m_currentFrame),
                                     m_gizmoPipelineLayout->Get(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                                     sizeof(glm::mat4), &worldTransform->GetTransformMatrix());
               }
               vkCmdPushConstants(m_commandBuffers->Get(m_currentFrame),
                                  m_gizmoPipelineLayout->Get(), VK_SHADER_STAGE_FRAGMENT_BIT,
                                  sizeof(glm::mat4), sizeof(glm::vec3), &lightComp->GetColor());
               if (const IMesh* mesh = m_resourceManager->GetMesh(m_lineCube)) {
                  const VulkanMesh* vkMesh = reinterpret_cast<const VulkanMesh*>(mesh);
                  vkMesh->Draw(m_commandBuffers->Get(m_currentFrame));
               }
            }
         });
      }
   }
   // PARTICLE PASS
   {
      // Check particle buffer size
      if (m_activeScene) {
         uint32_t maxParticles = 0;
         m_activeScene->ForEachNode([&](const Node* node) {
            if (auto* ps = node->GetComponent<ParticleSystemComponent>()) {
               maxParticles = std::max(maxParticles, ps->GetActiveParticleCount());
            }
         });
         if (maxParticles > m_particleInstanceCapacity) {
            vkDeviceWaitIdle(m_device.Get()); // Wait before reallocation
            m_particleInstanceCapacity = maxParticles * 2;
            // Reallocate all buffers
            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
               const VkDeviceSize newSize =
                  m_particleInstanceCapacity * sizeof(ParticleInstanceData);
               m_particleInstanceBuffers[i] =
                  std::make_unique<VulkanBuffer>(m_device, newSize, VulkanBuffer::Usage::Vertex,
                                                 VulkanBuffer::MemoryType::CPUToGPU);
               m_particleInstanceBuffers[i]->Map();
            }
         }
      }
      if (m_activeScene) {
         m_commandBuffers->BindPipeline(m_particleGraphicsPipeline->GetPipeline(),
                                        VK_PIPELINE_BIND_POINT_GRAPHICS, m_currentFrame);
         vkCmdBindDescriptorSets(m_commandBuffers->Get(m_currentFrame),
                                 VK_PIPELINE_BIND_POINT_GRAPHICS, m_particlePipelineLayout->Get(),
                                 0, 1, &m_particleDescriptorSets[m_currentFrame], 0, nullptr);
         m_commandBuffers->SetViewport(viewport, m_currentFrame);
         m_commandBuffers->SetScissor(scissor, m_currentFrame);
         RenderParticlesInstanced(imageIndex);
      }
   }
   // Render ImGUI
   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_commandBuffers->Get(m_currentFrame));
   m_commandBuffers->EndRenderPass(m_currentFrame);
   m_commandBuffers->End(m_currentFrame);
}

void VulkanRenderer::RenderParticlesInstanced(const uint32_t imageIndex) {
   if (!m_activeScene)
      return;
   const IMesh* mesh = m_resourceManager->GetMesh(m_fullscreenQuad);
   if (!mesh)
      return;
   const VulkanMesh* vkMesh = reinterpret_cast<const VulkanMesh*>(mesh);
   m_activeScene->ForEachNode([&](const Node* node) {
      if (!node->IsActive())
         return;
      const auto* particles = node->GetComponent<ParticleSystemComponent>();
      if (!particles)
         return;
      const uint32_t activeCount = particles->GetActiveParticleCount();
      if (activeCount == 0)
         return;
      // Upload instance data
      const auto& instanceData = particles->GetInstanceData();
      const VkDeviceSize dataSize = activeCount * sizeof(ParticleInstanceData);
      m_particleInstanceBuffers[m_currentFrame]->Update(instanceData.data(), dataSize);
      // Bind and draw
      const VkBuffer vertexBuffers[] = {vkMesh->GetVertexBuffer(),
                                        m_particleInstanceBuffers[m_currentFrame]->Get()};
      const VkDeviceSize offsets[] = {0, 0};
      vkCmdBindVertexBuffers(m_commandBuffers->Get(m_currentFrame), 0, 2, vertexBuffers, offsets);
      vkCmdBindIndexBuffer(m_commandBuffers->Get(m_currentFrame), vkMesh->GetIndexBuffer(), 0,
                           vkMesh->GetIndexType());
      vkCmdDrawIndexed(m_commandBuffers->Get(m_currentFrame),
                       static_cast<uint32_t>(vkMesh->GetIndexCount()), activeCount, 0, 0, 0);
   });
}

void VulkanRenderer::CreateSynchronizationObjects() {
   const uint32_t imageCount = m_swapchain.GetImages().size();
   m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
   m_renderFinishedSemaphores.resize(imageCount);
   m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
   VkSemaphoreCreateInfo semaphoreInfo{};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
   VkFenceCreateInfo fenceInfo{};
   fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
   // Create per-frame resources
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      if (vkCreateSemaphore(m_device.Get(), &semaphoreInfo, nullptr,
                            &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(m_device.Get(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create per-frame synchronization objects");
      }
   }
   // Create per-image resources
   for (uint32_t i = 0; i < imageCount; ++i) {
      if (vkCreateSemaphore(m_device.Get(), &semaphoreInfo, nullptr,
                            &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create per-image synchronization objects");
      }
   }
}

void VulkanRenderer::RecreateSwapchain() {
   vkDeviceWaitIdle(m_device.Get());
   CleanupSwapchain();
   m_swapchain.Recreate();
   CreateGeometryPass();
   CreateGeometryFBO();
   CreateLightingPass();
   CreateLightingFBO();
   UpdateDescriptorSets();
   if (m_activeCamera) {
      m_activeCamera->SetAspectRatio(static_cast<float>(m_swapchain.GetExtent().width) /
                                     static_cast<float>(m_swapchain.GetExtent().height));
   }
}

void VulkanRenderer::CleanupSwapchain() {
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vkDestroyFramebuffer(m_device.Get(), m_geometryFramebuffers[i], nullptr);
   }
   for (VkFramebuffer framebuffer : m_lightingFramebuffers) {
      vkDestroyFramebuffer(m_device.Get(), framebuffer, nullptr);
   }
   m_lightingFramebuffers.clear();
}

void VulkanRenderer::CreateUBOs() {
   // Camera buffers
   const VkDeviceSize cameraBufferSize = sizeof(CameraData);
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      m_cameraUniformBuffers[i] =
         std::make_unique<VulkanBuffer>(m_device, cameraBufferSize, VulkanBuffer::Usage::Uniform,
                                        VulkanBuffer::MemoryType::CPUToGPU);
      m_cameraUniformBuffers[i]->Map();
   }
   // Light buffers
   const VkDeviceSize lightBufferSize = sizeof(LightsData);
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      m_lightsUniformBuffers[i] =
         std::make_unique<VulkanBuffer>(m_device, lightBufferSize, VulkanBuffer::Usage::Uniform,
                                        VulkanBuffer::MemoryType::CPUToGPU);
      m_lightsUniformBuffers[i]->Map();
   }
}

void VulkanRenderer::UpdateCameraUBO(const uint32_t currentImage) {
   if (!m_activeCamera)
      return;
   const CameraData camData{.view = m_activeCamera->GetViewMatrix(),
                            .proj = m_activeCamera->GetProjectionMatrix(),
                            .viewPos = m_activeCamera->GetTransform().GetPosition()};
   m_cameraUniformBuffers[currentImage]->Update(&camData, sizeof(CameraData));
}

void VulkanRenderer::UpdateLightsUBO(const uint32_t currentImage) {
   if (!m_activeScene)
      return;
   LightsData lightsData{};
   lightsData.lightCount = 0;
   m_activeScene->ForEachNode([&](const Node* node) {
      if (!node->IsActive() || lightsData.lightCount >= MAX_LIGHTS) [[unlikely]]
         return;
      const auto* lightComp = node->GetComponent<LightComponent>();
      if (lightComp) [[likely]] {
         auto& light = lightsData.lights[lightsData.lightCount];
         light.lightType = static_cast<uint32_t>(lightComp->GetType());
         light.color = lightComp->GetColor();
         light.intensity = lightComp->GetIntensity();
         light.constant = lightComp->GetConstant();
         light.linear = lightComp->GetLinear();
         light.quadratic = lightComp->GetQuadratic();
         const auto* transform = node->GetWorldTransform();
         light.position = transform->GetPosition();
         light.direction = transform->GetForward();
         light.innerCone = lightComp->GetInnerCone();
         light.outerCone = lightComp->GetOuterCone();
         ++lightsData.lightCount;
      }
   });
   m_lightsUniformBuffers[currentImage]->Update(&lightsData, sizeof(LightsData));
}

void VulkanRenderer::CreateDescriptorPool() {
   std::array<VkDescriptorPoolSize, 2> poolSizes{};
   poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   poolSizes[0].descriptorCount =
      static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 3; // Camera/Lighting/Particle UBO
   poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   poolSizes[1].descriptorCount =
      static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 3; // G-Buffer textures
   VkDescriptorPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
   poolInfo.pPoolSizes = poolSizes.data();
   poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) *
                      4; // Geometry + Lighting + Gizmo + Particle sets
   if (vkCreateDescriptorPool(m_device.Get(), &poolInfo, nullptr, &m_descriptorPool) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor pool.");
   }
}

void VulkanRenderer::CreateDescriptorSets() {
   // Allocate geometry descriptor sets
   std::vector<VkDescriptorSetLayout> geometryLayouts(MAX_FRAMES_IN_FLIGHT,
                                                      m_geometryDescriptorSetLayout);
   VkDescriptorSetAllocateInfo geometryAllocInfo{};
   geometryAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   geometryAllocInfo.descriptorPool = m_descriptorPool;
   geometryAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   geometryAllocInfo.pSetLayouts = geometryLayouts.data();
   if (vkAllocateDescriptorSets(m_device.Get(), &geometryAllocInfo,
                                m_geometryDescriptorSets.data()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate geometry descriptor sets.");
   }
   // Allocate gizmo descriptor sets
   std::vector<VkDescriptorSetLayout> gizmoLayouts(MAX_FRAMES_IN_FLIGHT,
                                                   m_gizmoDescriptorSetLayout);
   VkDescriptorSetAllocateInfo gizmoAllocInfo{};
   gizmoAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   gizmoAllocInfo.descriptorPool = m_descriptorPool;
   gizmoAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   gizmoAllocInfo.pSetLayouts = gizmoLayouts.data();
   if (vkAllocateDescriptorSets(m_device.Get(), &gizmoAllocInfo, m_gizmoDescriptorSets.data()) !=
       VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate geometry descriptor sets.");
   }
   // Allocate lighting descriptor sets
   std::vector<VkDescriptorSetLayout> lightingLayouts(MAX_FRAMES_IN_FLIGHT,
                                                      m_lightingDescriptorSetLayout);
   VkDescriptorSetAllocateInfo lightingAllocInfo{};
   lightingAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   lightingAllocInfo.descriptorPool = m_descriptorPool;
   lightingAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   lightingAllocInfo.pSetLayouts = lightingLayouts.data();
   if (vkAllocateDescriptorSets(m_device.Get(), &lightingAllocInfo,
                                m_lightingDescriptorSets.data()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate lighting descriptor sets.");
   }
   // Allocate particle descriptor sets
   std::vector<VkDescriptorSetLayout> particleLayouts(MAX_FRAMES_IN_FLIGHT,
                                                      m_particleDescriptorSetLayout);
   VkDescriptorSetAllocateInfo particleAllocInfo{};
   particleAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   particleAllocInfo.descriptorPool = m_descriptorPool;
   particleAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
   particleAllocInfo.pSetLayouts = particleLayouts.data();

   if (vkAllocateDescriptorSets(m_device.Get(), &particleAllocInfo,
                                m_particleDescriptorSets.data()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate particle descriptor sets.");
   }
   // Update geometry descriptor sets
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      VkDescriptorBufferInfo cameraBufferInfo{};
      cameraBufferInfo.buffer = m_cameraUniformBuffers[i]->Get();
      cameraBufferInfo.offset = 0;
      cameraBufferInfo.range = sizeof(CameraData);
      VkWriteDescriptorSet cameraDescriptorWrite{};
      cameraDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      cameraDescriptorWrite.dstSet = m_geometryDescriptorSets[i];
      cameraDescriptorWrite.dstBinding = 0;
      cameraDescriptorWrite.dstArrayElement = 0;
      cameraDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      cameraDescriptorWrite.descriptorCount = 1;
      cameraDescriptorWrite.pBufferInfo = &cameraBufferInfo;
      vkUpdateDescriptorSets(m_device.Get(), 1, &cameraDescriptorWrite, 0, nullptr);
   }
   // Update gizmo descriptor sets
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      VkDescriptorBufferInfo cameraBufferInfo{};
      cameraBufferInfo.buffer = m_cameraUniformBuffers[i]->Get();
      cameraBufferInfo.offset = 0;
      cameraBufferInfo.range = sizeof(CameraData);
      VkWriteDescriptorSet cameraDescriptorWrite{};
      cameraDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      cameraDescriptorWrite.dstSet = m_gizmoDescriptorSets[i];
      cameraDescriptorWrite.dstBinding = 0;
      cameraDescriptorWrite.dstArrayElement = 0;
      cameraDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      cameraDescriptorWrite.descriptorCount = 1;
      cameraDescriptorWrite.pBufferInfo = &cameraBufferInfo;
      vkUpdateDescriptorSets(m_device.Get(), 1, &cameraDescriptorWrite, 0, nullptr);
   }
   // Update particle descriptor sets
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      VkDescriptorBufferInfo cameraBufferInfo{};
      cameraBufferInfo.buffer = m_cameraUniformBuffers[i]->Get();
      cameraBufferInfo.offset = 0;
      cameraBufferInfo.range = sizeof(CameraData);
      VkWriteDescriptorSet cameraDescriptorWrite{};
      cameraDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      cameraDescriptorWrite.dstSet = m_particleDescriptorSets[i];
      cameraDescriptorWrite.dstBinding = 0;
      cameraDescriptorWrite.dstArrayElement = 0;
      cameraDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      cameraDescriptorWrite.descriptorCount = 1;
      cameraDescriptorWrite.pBufferInfo = &cameraBufferInfo;
      vkUpdateDescriptorSets(m_device.Get(), 1, &cameraDescriptorWrite, 0, nullptr);
   }
   UpdateDescriptorSets();
}

void VulkanRenderer::UpdateDescriptorSets() {
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      std::vector<VkWriteDescriptorSet> descriptorWrites;
      // Camera UBO
      VkDescriptorBufferInfo cameraBufferInfo{};
      cameraBufferInfo.buffer = m_cameraUniformBuffers[i]->Get();
      cameraBufferInfo.offset = 0;
      cameraBufferInfo.range = sizeof(CameraData);
      VkWriteDescriptorSet cameraWrite{};
      cameraWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      cameraWrite.dstSet = m_lightingDescriptorSets[i];
      cameraWrite.dstBinding = 0;
      cameraWrite.dstArrayElement = 0;
      cameraWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      cameraWrite.descriptorCount = 1;
      cameraWrite.pBufferInfo = &cameraBufferInfo;
      descriptorWrites.push_back(cameraWrite);
      // Lighting UBO
      VkDescriptorBufferInfo lightBufferInfo{};
      lightBufferInfo.buffer = m_lightsUniformBuffers[i]->Get();
      lightBufferInfo.offset = 0;
      lightBufferInfo.range = sizeof(LightsData);
      VkWriteDescriptorSet lightWrite{};
      lightWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      lightWrite.dstSet = m_lightingDescriptorSets[i];
      lightWrite.dstBinding = 1;
      lightWrite.dstArrayElement = 0;
      lightWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      lightWrite.descriptorCount = 1;
      lightWrite.pBufferInfo = &lightBufferInfo;
      descriptorWrites.push_back(lightWrite);
      // G-Buffer textures
      ITexture* albedoTex = m_resourceManager->GetTexture(m_gAlbedoTexture[i]);
      ITexture* normalTex = m_resourceManager->GetTexture(m_gNormalTexture[i]);
      ITexture* depthTex = m_resourceManager->GetTexture(m_gDepthTexture[i]);
      if (albedoTex && normalTex && depthTex) {
         VulkanTexture* vkAlbedo = reinterpret_cast<VulkanTexture*>(albedoTex);
         VulkanTexture* vkNormal = reinterpret_cast<VulkanTexture*>(normalTex);
         VulkanTexture* vkDepth = reinterpret_cast<VulkanTexture*>(depthTex);
         // Albedo texture
         VkDescriptorImageInfo albedoImageInfo{};
         albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         albedoImageInfo.imageView = vkAlbedo->GetImageView();
         albedoImageInfo.sampler = vkAlbedo->GetSampler();
         VkWriteDescriptorSet albedoWrite{};
         albedoWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
         albedoWrite.dstSet = m_lightingDescriptorSets[i];
         albedoWrite.dstBinding = 3;
         albedoWrite.dstArrayElement = 0;
         albedoWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
         albedoWrite.descriptorCount = 1;
         albedoWrite.pImageInfo = &albedoImageInfo;
         descriptorWrites.push_back(albedoWrite);
         // Normal texture
         VkDescriptorImageInfo normalImageInfo{};
         normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         normalImageInfo.imageView = vkNormal->GetImageView();
         normalImageInfo.sampler = vkNormal->GetSampler();
         VkWriteDescriptorSet normalWrite{};
         normalWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
         normalWrite.dstSet = m_lightingDescriptorSets[i];
         normalWrite.dstBinding = 4;
         normalWrite.dstArrayElement = 0;
         normalWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
         normalWrite.descriptorCount = 1;
         normalWrite.pImageInfo = &normalImageInfo;
         descriptorWrites.push_back(normalWrite);
         // Depth texture
         VkDescriptorImageInfo depthImageInfo{};
         depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         depthImageInfo.imageView = vkDepth->GetImageView();
         depthImageInfo.sampler = vkDepth->GetSampler();
         VkWriteDescriptorSet depthWrite{};
         depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
         depthWrite.dstSet = m_lightingDescriptorSets[i];
         depthWrite.dstBinding = 5;
         depthWrite.dstArrayElement = 0;
         depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
         depthWrite.descriptorCount = 1;
         depthWrite.pImageInfo = &depthImageInfo;
         descriptorWrites.push_back(depthWrite);
      }
      vkUpdateDescriptorSets(m_device.Get(), static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
   }
}

VulkanRenderer::~VulkanRenderer() {
   vkDeviceWaitIdle(m_device.Get());
   CleanupSwapchain();
   if (m_materialDescriptorPool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(m_device.Get(), m_materialDescriptorPool, nullptr);
   }
   if (m_materialDescriptorSetLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(m_device.Get(), m_materialDescriptorSetLayout, nullptr);
   }
   vkDestroyDescriptorPool(m_device.Get(), m_descriptorPool, nullptr);
   vkDestroyDescriptorSetLayout(m_device.Get(), m_geometryDescriptorSetLayout, nullptr);
   vkDestroyDescriptorSetLayout(m_device.Get(), m_lightingDescriptorSetLayout, nullptr);
   for (uint32_t i = 0; i < m_renderFinishedSemaphores.size(); ++i) {
      vkDestroySemaphore(m_device.Get(), m_renderFinishedSemaphores[i], nullptr);
   }
   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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
   imguiInfo.RenderPass = m_lightingRenderPass->Get();
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
   // Calculate delta time
   const double currentTime = glfwGetTime();
   m_deltaTime = static_cast<float>(currentTime - m_lastFrameTime);
   m_lastFrameTime = currentTime;
   ImGuiIO& io = ImGui::GetIO();
   io.DeltaTime = m_deltaTime;
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

   UpdateCameraUBO(m_currentFrame);
   UpdateLightsUBO(m_currentFrame);

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

void VulkanRenderer::CreateUtilityMeshes() {
   // Create fullscreen quad for lighting pass
   constexpr std::array<Vertex, 4> quadVerts = {{
      {glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f)},
      {glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f)},
      {glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 1.0f)},
      {glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 1.0f)},
   }};
   constexpr std::array<uint32_t, 6> quadInds = {0, 1, 2, 2, 3, 0};
   const std::vector<Vertex> quadVertVec(quadVerts.begin(), quadVerts.end());
   const std::vector<uint32_t> quadIndVec(quadInds.begin(), quadInds.end());
   m_fullscreenQuad = m_resourceManager->LoadMesh("quad", quadVertVec, quadIndVec);
   // Create wireframe cube for gizmos
   constexpr std::array<Vertex, 10> cubeVerts = {{
      // Cube vertices
      {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(0, 0)},
      {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(1, 0)},
      {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(1, 1)},
      {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(0, 1)},
      {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(0, 0)},
      {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(1, 0)},
      {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(1, 1)},
      {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(0, 1)},
      // Arrow line vertices
      {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(0, 0)},
      {glm::vec3(0.0f, 0.0f, -0.8f), glm::vec3(0, 0, -1), glm::vec2(0, 1)}
   }};
   constexpr std::array<uint32_t, 26> cubeInds = {0, 1, 1, 5, 5, 4, 4, 0, 3, 2, 2, 6, 6,
                                                  7, 7, 3, 0, 3, 1, 2, 5, 6, 4, 7, 8, 9};
   const std::vector<Vertex> cubeVertVec(cubeVerts.begin(), cubeVerts.end());
   const std::vector<uint32_t> cubeIndVec(cubeInds.begin(), cubeInds.end());
   m_lineCube = m_resourceManager->LoadMesh("unit_cube", cubeVertVec, cubeIndVec);
}

void VulkanRenderer::CreateDefaultMaterial() {
   auto defaultMat = m_resourceManager->CreateMaterial("default_pbr", "PBR");
   if (IMaterial* material = m_resourceManager->GetMaterial(defaultMat)) {
      material->SetParameter("albedo", glm::vec3(1.0f));
      material->SetParameter("metallic", 1.0f);
      material->SetParameter("roughness", 1.0f);
      material->SetParameter("ao", 1.0f);
   }
}
