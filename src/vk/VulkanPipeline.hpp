#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class VulkanDevice;

class VulkanShaderModule {
  public:
   VulkanShaderModule(const VulkanDevice& device, const std::vector<char>& code);
   VulkanShaderModule(const VulkanDevice& device, const std::string& filepath);
   ~VulkanShaderModule();

   VulkanShaderModule(const VulkanShaderModule&) = delete;
   VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;
   VulkanShaderModule(VulkanShaderModule&& other) noexcept;
   VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

   VkShaderModule Get() const;

  private:
   static std::vector<char> ReadFile(const std::string& filename);

  private:
   const VulkanDevice* m_device = nullptr;
   VkShaderModule m_shaderModule = VK_NULL_HANDLE;
};

class VulkanPipelineLayout {
  public:
   VulkanPipelineLayout(const VulkanDevice& device,
                        const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
                        const std::vector<VkPushConstantRange>& pushConstantRanges = {});
   ~VulkanPipelineLayout();

   VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
   VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;
   VulkanPipelineLayout(VulkanPipelineLayout&& other) noexcept;
   VulkanPipelineLayout& operator=(VulkanPipelineLayout&& other) noexcept;

   VkPipelineLayout Get() const { return m_pipelineLayout; }

  private:
   const VulkanDevice* m_device = nullptr;
   VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};

class VulkanGraphicsPipeline {
  public:
   VulkanGraphicsPipeline(const VulkanDevice& device, const VkPipeline& pipeline,
                          const VkPipelineLayout& layout);
   ~VulkanGraphicsPipeline();

   VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
   VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;
   VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept;
   VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&& other) noexcept;

   VkPipeline GetPipeline() const;
   VkPipelineLayout GetLayout() const;

  private:
   const VulkanDevice* m_device = nullptr;
   VkPipeline m_pipeline = VK_NULL_HANDLE;
   VkPipelineLayout m_layout = VK_NULL_HANDLE;
};

class VulkanGraphicsPipelineBuilder {
  public:
   struct ShaderStage {
      VkShaderStageFlagBits stage;
      VkShaderModule module;
      std::string entryPoint = "main";
   };

   struct VertexInputState {
      std::vector<VkVertexInputBindingDescription> bindings;
      std::vector<VkVertexInputAttributeDescription> attributes;
   };

   struct InputAssemblyState {
      VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      VkBool32 primitiveRestartEnable = VK_FALSE;
   };

   struct ViewportState {
      std::vector<VkViewport> viewports;
      std::vector<VkRect2D> scissors;
   };

   struct RasterizationState {
      VkBool32 depthClampEnable = VK_FALSE;
      VkBool32 rasterizerDiscardEnable = VK_FALSE;
      VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
      VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
      VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
      VkBool32 depthBiasEnable = VK_FALSE;
      float depthBiasConstantFactor = 0.0f;
      float depthBiasClamp = 0.0f;
      float depthBiasSlopeFactor = 0.0f;
      float lineWidth = 1.0f;
   };

   struct MultisampleState {
      VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
      VkBool32 sampleShadingEnable = VK_FALSE;
      float minSampleShading = 1.0f;
      const VkSampleMask* pSampleMask = nullptr;
      VkBool32 alphaToCoverageEnable = VK_FALSE;
      VkBool32 alphaToOneEnable = VK_FALSE;
   };

   struct DepthStencilState {
      VkBool32 depthTestEnable = VK_TRUE;
      VkBool32 depthWriteEnable = VK_TRUE;
      VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
      VkBool32 depthBoundsTestEnable = VK_FALSE;
      VkBool32 stencilTestEnable = VK_FALSE;
      VkStencilOpState front = {};
      VkStencilOpState back = {};
      float minDepthBounds = 0.0f;
      float maxDepthBounds = 1.0f;
   };

   struct ColorBlendAttachmentState {
      VkBool32 blendEnable = VK_FALSE;
      VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
      VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
      VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
      VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   };

   struct ColorBlendState {
      VkBool32 logicOpEnable = VK_FALSE;
      VkLogicOp logicOp = VK_LOGIC_OP_COPY;
      std::vector<ColorBlendAttachmentState> attachments;
      float blendConstants[4] = {0.0f, 0.0f, 0.0f, 0.0f};
   };

  public:
   VulkanGraphicsPipelineBuilder(const VulkanDevice& device);
   // Shader stages
   VulkanGraphicsPipelineBuilder& AddShaderStage(const VkShaderStageFlagBits stage,
                                                 const VkShaderModule& module,
                                                 const std::string& entryPoint = "main");
   VulkanGraphicsPipelineBuilder& SetVertexShader(const VkShaderModule& module,
                                                  const std::string& entryPoint = "main");
   VulkanGraphicsPipelineBuilder& SetFragmentShader(const VkShaderModule& module,
                                                    const std::string& entryPoint = "main");
   // Vertex input
   VulkanGraphicsPipelineBuilder& SetVertexInput(const VertexInputState& vertexInput);
   VulkanGraphicsPipelineBuilder& AddVertexBinding(const uint32_t binding, const uint32_t stride,
                                                   const VkVertexInputRate& inputRate);
   VulkanGraphicsPipelineBuilder& AddVertexAttribute(const uint32_t location,
                                                     const uint32_t binding, const VkFormat format,
                                                     const uint32_t offset);
   // Input assembly
   VulkanGraphicsPipelineBuilder& SetInputAssembly(const InputAssemblyState& inputAssembly);
   VulkanGraphicsPipelineBuilder& SetTopology(const VkPrimitiveTopology& topology);
   // Viewport
   VulkanGraphicsPipelineBuilder& SetViewportState(const ViewportState& viewportState);
   VulkanGraphicsPipelineBuilder& AddViewport(const VkViewport& viewport);
   VulkanGraphicsPipelineBuilder& AddScissor(const VkRect2D& scissor);
   // Rasterization
   VulkanGraphicsPipelineBuilder& SetRasterizationState(const RasterizationState& rasterization);
   VulkanGraphicsPipelineBuilder& SetPolygonMode(const VkPolygonMode mode);
   VulkanGraphicsPipelineBuilder& SetCullMode(const VkCullModeFlags cullMode);
   VulkanGraphicsPipelineBuilder& SetFrontFace(const VkFrontFace frontFace);
   // Multisampling
   VulkanGraphicsPipelineBuilder& SetMultisampleState(const MultisampleState& multisample);
   VulkanGraphicsPipelineBuilder& SetSampleCount(const VkSampleCountFlagBits samples);
   // Depth/Stencil
   VulkanGraphicsPipelineBuilder& SetDepthStencilState(const DepthStencilState& depthStencil);
   VulkanGraphicsPipelineBuilder& EnableDepthTest(const VkCompareOp compareOp = VK_COMPARE_OP_LESS);
   VulkanGraphicsPipelineBuilder& DisableDepthTest();
   // Color blending
   VulkanGraphicsPipelineBuilder& SetColorBlendState(const ColorBlendState& colorBlend);
   VulkanGraphicsPipelineBuilder& AddColorBlendAttachment(
      const ColorBlendAttachmentState& attachment);
   VulkanGraphicsPipelineBuilder& SetDefaultColorBlending();
   // Dynamic state
   VulkanGraphicsPipelineBuilder& AddDynamicState(const VkDynamicState state);
   VulkanGraphicsPipelineBuilder& SetDynamicViewportAndScissor();
   // Pipeline layout and render pass
   VulkanGraphicsPipelineBuilder& SetPipelineLayout(const VkPipelineLayout& layout);
   VulkanGraphicsPipelineBuilder& SetRenderPass(const VkRenderPass& renderPass,
                                                const uint32_t subpass = 0);
   // Build the pipeline
   VulkanGraphicsPipeline Build();

  private:
   void ValidateState() const;
   VkPipelineVertexInputStateCreateInfo GetVertexInputStateCreateInfo() const;
   VkPipelineInputAssemblyStateCreateInfo GetInputAssemblyStateCreateInfo() const;
   VkPipelineViewportStateCreateInfo GetViewportStateCreateInfo() const;
   VkPipelineRasterizationStateCreateInfo GetRasterizationStateCreateInfo() const;
   VkPipelineMultisampleStateCreateInfo GetMultisampleStateCreateInfo() const;
   VkPipelineDepthStencilStateCreateInfo GetDepthStencilStateCreateInfo() const;
   VkPipelineColorBlendStateCreateInfo GetColorBlendStateCreateInfo() const;
   VkPipelineDynamicStateCreateInfo GetDynamicStateCreateInfo() const;

  private:
   const VulkanDevice* m_device = nullptr;
   // Pipeline state
   std::vector<ShaderStage> m_shaderStages;
   VertexInputState m_vertexInput;
   InputAssemblyState m_inputAssembly;
   ViewportState m_viewport;
   RasterizationState m_rasterization;
   MultisampleState m_multisample;
   DepthStencilState m_depthStencil;
   ColorBlendState m_colorBlend;
   std::vector<VkDynamicState> m_dynamicStates;
   // Pipeline configuration
   VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
   VkRenderPass m_renderPass = VK_NULL_HANDLE;
   uint32_t m_subpass = 0;
   // Optional base pipeline
   VkPipeline m_basePipeline = VK_NULL_HANDLE;
   int32_t m_basePipelineIndex = -1;
   // Temporary storage for Vulkan structs (needed for create info)
   mutable std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageCreateInfos;
   mutable std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachments;
};
