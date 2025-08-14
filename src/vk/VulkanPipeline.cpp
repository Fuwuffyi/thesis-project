#include "VulkanPipeline.hpp"

#include "VulkanDevice.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

// VulkanShaderModule
std::vector<char> VulkanShaderModule::ReadFile(const std::string& filename) {
   std::ifstream file(filename, std::ios::ate | std::ios::binary);
   if (!file.is_open()) {
      throw std::runtime_error("Failed to open shader file: " + filename);
   }
   const size_t fileSize = static_cast<size_t>(file.tellg());
   std::vector<char> buffer(fileSize);
   file.seekg(0);
   file.read(buffer.data(), fileSize);
   return buffer;
}

VulkanShaderModule::VulkanShaderModule(const VulkanDevice& device, const std::vector<char>& code)
   :
   m_device(&device)
{
   VkShaderModuleCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   createInfo.codeSize = code.size();
   createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
   if (vkCreateShaderModule(m_device->Get(), &createInfo,
                            nullptr, &m_shaderModule) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create shader module");
   }
}

VulkanShaderModule::VulkanShaderModule(const VulkanDevice& device, const std::string& filepath)
   :
   VulkanShaderModule(device, ReadFile(filepath)) {}

VulkanShaderModule::~VulkanShaderModule() {
   if (m_shaderModule != VK_NULL_HANDLE) {
      vkDestroyShaderModule(m_device->Get(), m_shaderModule, nullptr);
   }
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept {
   *this = std::move(other);
}

VulkanShaderModule& VulkanShaderModule::operator=(VulkanShaderModule&& other) noexcept {
   if (this != &other) {
      m_device = other.m_device;
      m_shaderModule = other.m_shaderModule;
      other.m_device = nullptr;
      other.m_shaderModule = VK_NULL_HANDLE;
   }
   return *this;
}

VkShaderModule VulkanShaderModule::Get() const {
   return m_shaderModule;
}

// VulkanPipelineLayout
VulkanPipelineLayout::VulkanPipelineLayout(
   const VulkanDevice& device,
   const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
   const std::vector<VkPushConstantRange>& pushConstantRanges)
   :
   m_device(&device)
{
   VkPipelineLayoutCreateInfo info{};
   info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   info.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
   info.pSetLayouts = descriptorSetLayouts.data();
   info.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
   info.pPushConstantRanges = pushConstantRanges.data();
   if (vkCreatePipelineLayout(m_device->Get(), &info, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create pipeline layout");
   }
}

VulkanPipelineLayout::~VulkanPipelineLayout() {
   if (m_pipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(m_device->Get(), m_pipelineLayout, nullptr);
   }
}

VulkanPipelineLayout::VulkanPipelineLayout(VulkanPipelineLayout&& other) noexcept {
   *this = std::move(other);
}

VulkanPipelineLayout& VulkanPipelineLayout::operator=(VulkanPipelineLayout&& other) noexcept {
   if (this != &other) {
      m_device = other.m_device;
      m_pipelineLayout = other.m_pipelineLayout;
      other.m_device = nullptr;
      other.m_pipelineLayout = VK_NULL_HANDLE;
   }
   return *this;
}

// VulkanGraphicsPipeline
VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanDevice& device, const VkPipeline& pipeline, const VkPipelineLayout& layout)
   :
   m_device(&device),
   m_pipeline(pipeline),
   m_layout(layout)
{}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
   if (m_pipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(m_device->Get(), m_pipeline, nullptr);
   }
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept {
   *this = std::move(other);
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::operator=(VulkanGraphicsPipeline&& other) noexcept {
   if (this != &other) {
      m_device = other.m_device;
      m_pipeline = other.m_pipeline;
      m_layout = other.m_layout;
      other.m_device = nullptr;
      other.m_pipeline = VK_NULL_HANDLE;
      other.m_layout = VK_NULL_HANDLE;
   }
   return *this;
}

VkPipeline VulkanGraphicsPipeline::GetPipeline() const { return m_pipeline; }
VkPipelineLayout VulkanGraphicsPipeline::GetLayout() const { return m_layout; }

// VulkanGraphicsPipelineBuilder
VulkanGraphicsPipelineBuilder::VulkanGraphicsPipelineBuilder(const VulkanDevice& device)
   :
   m_device(&device)
{}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::AddShaderStage(
   const VkShaderStageFlagBits stage, const VkShaderModule& module, const std::string& entryPoint) {
   m_shaderStages.push_back({ stage, module, entryPoint });
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetVertexShader(
   const VkShaderModule& module, const std::string& entryPoint) {
   return AddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, module, entryPoint);
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetFragmentShader(
   const VkShaderModule& module, const std::string& entryPoint) {
   return AddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, module, entryPoint);
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetVertexInput(const VertexInputState& vertexInput) {
   m_vertexInput = vertexInput;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::AddVertexBinding(
   const uint32_t binding, const uint32_t stride, const VkVertexInputRate& inputRate) {
   m_vertexInput.bindings.push_back({ binding, stride, inputRate });
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::AddVertexAttribute(
   const uint32_t location, const uint32_t binding, const VkFormat format, const uint32_t offset) {
   m_vertexInput.attributes.push_back({ location, binding, format, offset });
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetInputAssembly(const InputAssemblyState& inputAssembly) {
   m_inputAssembly = inputAssembly;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetTopology(const VkPrimitiveTopology& topology) {
   m_inputAssembly.topology = topology;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetViewportState(const ViewportState& viewportState) {
   m_viewport = viewportState;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::AddViewport(const VkViewport& viewport) {
   m_viewport.viewports.push_back(viewport);
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::AddScissor(const VkRect2D& scissor) {
   m_viewport.scissors.push_back(scissor);
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetRasterizationState(const RasterizationState& rasterization) {
   m_rasterization = rasterization;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetPolygonMode(const VkPolygonMode mode) {
   m_rasterization.polygonMode = mode;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetCullMode(const VkCullModeFlags cullMode) {
   m_rasterization.cullMode = cullMode;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetFrontFace(const VkFrontFace frontFace) {
   m_rasterization.frontFace = frontFace;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetMultisampleState(const MultisampleState& multisample) {
   m_multisample = multisample;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetSampleCount(const VkSampleCountFlagBits samples) {
   m_multisample.rasterizationSamples = samples;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDepthStencilState(const DepthStencilState& depthStencil) {
   m_depthStencil = depthStencil;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::EnableDepthTest(const VkCompareOp compareOp) {
   m_depthStencil.depthTestEnable = VK_TRUE;
   m_depthStencil.depthCompareOp = compareOp;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::DisableDepthTest() {
   m_depthStencil.depthTestEnable = VK_FALSE;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetColorBlendState(const ColorBlendState& colorBlend) {
   m_colorBlend = colorBlend;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::AddColorBlendAttachment(const ColorBlendAttachmentState& attachment) {
   m_colorBlend.attachments.push_back(attachment);
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDefaultColorBlending() {
   ColorBlendAttachmentState attachment{};
   attachment.blendEnable = VK_TRUE;
   attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   attachment.colorBlendOp = VK_BLEND_OP_ADD;
   attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   attachment.alphaBlendOp = VK_BLEND_OP_ADD;
   m_colorBlend.attachments = { attachment };
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::AddDynamicState(const VkDynamicState state) {
   m_dynamicStates.push_back(state);
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDynamicViewportAndScissor() {
   AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
   AddDynamicState(VK_DYNAMIC_STATE_SCISSOR);
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetPipelineLayout(const VkPipelineLayout& layout) {
   m_pipelineLayout = layout;
   return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetRenderPass(const VkRenderPass& renderPass, const uint32_t subpass) {
   m_renderPass = renderPass;
   m_subpass = subpass;
   return *this;
}

VulkanGraphicsPipeline VulkanGraphicsPipelineBuilder::Build() {
   ValidateState();
   // Shader stages
   std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
   shaderStages.reserve(m_shaderStages.size());
   for (const ShaderStage& s : m_shaderStages) {
      VkPipelineShaderStageCreateInfo stage{};
      stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      stage.stage = s.stage;
      stage.module = s.module;
      stage.pName = s.entryPoint.c_str();
      shaderStages.push_back(stage);
   }
   // Vertex input
   VkPipelineVertexInputStateCreateInfo vertexInput{};
   vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInput.bindings.size());
   vertexInput.pVertexBindingDescriptions = m_vertexInput.bindings.data();
   vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexInput.attributes.size());
   vertexInput.pVertexAttributeDescriptions = m_vertexInput.attributes.data();
   // Input assembly
   VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
   inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssembly.topology = m_inputAssembly.topology;
   inputAssembly.primitiveRestartEnable = m_inputAssembly.primitiveRestartEnable;
   // Viewport
   VkPipelineViewportStateCreateInfo viewportState{};
   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportState.viewportCount = static_cast<uint32_t>(m_viewport.viewports.size());
   viewportState.pViewports = m_viewport.viewports.data();
   viewportState.scissorCount = static_cast<uint32_t>(m_viewport.scissors.size());
   viewportState.pScissors = m_viewport.scissors.data();
   // Rasterization
   VkPipelineRasterizationStateCreateInfo raster{};
   raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   raster.depthClampEnable = m_rasterization.depthClampEnable;
   raster.rasterizerDiscardEnable = m_rasterization.rasterizerDiscardEnable;
   raster.polygonMode = m_rasterization.polygonMode;
   raster.cullMode = m_rasterization.cullMode;
   raster.frontFace = m_rasterization.frontFace;
   raster.depthBiasEnable = m_rasterization.depthBiasEnable;
   raster.depthBiasConstantFactor = m_rasterization.depthBiasConstantFactor;
   raster.depthBiasClamp = m_rasterization.depthBiasClamp;
   raster.depthBiasSlopeFactor = m_rasterization.depthBiasSlopeFactor;
   raster.lineWidth = m_rasterization.lineWidth;
   // Multisampling
   VkPipelineMultisampleStateCreateInfo ms{};
   ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   ms.rasterizationSamples = m_multisample.rasterizationSamples;
   ms.sampleShadingEnable = m_multisample.sampleShadingEnable;
   ms.minSampleShading = m_multisample.minSampleShading;
   ms.pSampleMask = m_multisample.pSampleMask;
   ms.alphaToCoverageEnable = m_multisample.alphaToCoverageEnable;
   ms.alphaToOneEnable = m_multisample.alphaToOneEnable;
   // Depth stencil
   VkPipelineDepthStencilStateCreateInfo ds{};
   ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   ds.depthTestEnable = m_depthStencil.depthTestEnable;
   ds.depthWriteEnable = m_depthStencil.depthWriteEnable;
   ds.depthCompareOp = m_depthStencil.depthCompareOp;
   ds.depthBoundsTestEnable = m_depthStencil.depthBoundsTestEnable;
   ds.stencilTestEnable = m_depthStencil.stencilTestEnable;
   ds.front = m_depthStencil.front;
   ds.back = m_depthStencil.back;
   ds.minDepthBounds = m_depthStencil.minDepthBounds;
   ds.maxDepthBounds = m_depthStencil.maxDepthBounds;
   // Color blend attachments
   std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
   blendAttachments.reserve(m_colorBlend.attachments.size());
   for (const ColorBlendAttachmentState& a : m_colorBlend.attachments) {
      VkPipelineColorBlendAttachmentState att{};
      att.blendEnable = a.blendEnable;
      att.srcColorBlendFactor = a.srcColorBlendFactor;
      att.dstColorBlendFactor = a.dstColorBlendFactor;
      att.colorBlendOp = a.colorBlendOp;
      att.srcAlphaBlendFactor = a.srcAlphaBlendFactor;
      att.dstAlphaBlendFactor = a.dstAlphaBlendFactor;
      att.alphaBlendOp = a.alphaBlendOp;
      att.colorWriteMask = a.colorWriteMask;
      blendAttachments.push_back(att);
   }
   VkPipelineColorBlendStateCreateInfo cb{};
   cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   cb.logicOpEnable = m_colorBlend.logicOpEnable;
   cb.logicOp = m_colorBlend.logicOp;
   cb.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
   cb.pAttachments = blendAttachments.data();
   std::copy(std::begin(m_colorBlend.blendConstants),
             std::end(m_colorBlend.blendConstants), cb.blendConstants);
   // Dynamic state
   VkPipelineDynamicStateCreateInfo dyn{};
   dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dyn.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
   dyn.pDynamicStates = m_dynamicStates.data();
   // Pipeline create info
   VkGraphicsPipelineCreateInfo pipelineInfo{};
   pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
   pipelineInfo.pStages = shaderStages.data();
   pipelineInfo.pVertexInputState = &vertexInput;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &raster;
   pipelineInfo.pMultisampleState = &ms;
   pipelineInfo.pDepthStencilState = &ds;
   pipelineInfo.pColorBlendState = &cb;
   pipelineInfo.pDynamicState = &dyn;
   pipelineInfo.layout = m_pipelineLayout;
   pipelineInfo.renderPass = m_renderPass;
   pipelineInfo.subpass = m_subpass;
   pipelineInfo.basePipelineHandle = m_basePipeline;
   pipelineInfo.basePipelineIndex = m_basePipelineIndex;
   VkPipeline pipeline;
   if (vkCreateGraphicsPipelines(m_device->Get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create graphics pipeline");
   }
   return VulkanGraphicsPipeline(*m_device, pipeline, m_pipelineLayout);
}

void VulkanGraphicsPipelineBuilder::ValidateState() const {
   if (m_shaderStages.empty()) {
      throw std::runtime_error("No shader stages set");
   }
   if (m_pipelineLayout == VK_NULL_HANDLE) {
      throw std::runtime_error("Pipeline layout not set");
   }
   if (m_renderPass == VK_NULL_HANDLE) {
      throw std::runtime_error("Render pass not set");
   }
}

