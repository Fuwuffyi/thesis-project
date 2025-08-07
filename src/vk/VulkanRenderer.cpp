#include "VulkanRenderer.hpp"

#include <fstream>
#include <print>
#include <stdexcept>
#include <vector>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
   "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
   VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VulkanRenderer::VulkanRenderer(GLFWwindow* windowHandle) :
   IRenderer(windowHandle),
   m_instance(std::make_unique<VulkanInstance>(deviceExtensions, validationLayers, enableValidationLayers)),
   m_debugMessenger(std::make_unique<VulkanDebugMessenger>(*m_instance.get())),
   m_surface(std::make_unique<VulkanSurface>(*m_instance.get(), m_windowHandle))
{
   GetPhysicalDevice();
   GetLogicalDevice();
   GetSwapchain();
   GetImageViews();
   CreateRenderPass();
   CreateGraphicsPipeline();
   CreateFramebuffers();
   CreateCommandPool();
   CreateCommandBuffers();
   CreateSynchronizationObjects();

   // Initialize ImGui
   /*
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   if (!ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)m_windowHandle, true)) {
      throw std::runtime_error("ImGUI initialization failed.");
   }
   if (!ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo *info)) {
      throw std::runtime_error("ImGUI initialization failed.");
   }
   */
}

void VulkanRenderer::GetPhysicalDevice() {
   // Get device count
   uint32_t deviceCount = 0;
   vkEnumeratePhysicalDevices(m_instance->Get(), &deviceCount, nullptr);
   // If no devices, exit
   if (deviceCount == 0) {
      throw std::runtime_error("failed to find GPUs with Vulkan support.");
   }
   // Get actual devices
   std::vector<VkPhysicalDevice> devices(deviceCount);
   vkEnumeratePhysicalDevices(m_instance->Get(), &deviceCount, devices.data());
   // Setup ordered map for best score device
   std::multimap<uint32_t, VkPhysicalDevice> candidates;
   for (const VkPhysicalDevice& device : devices) {
      if (!IsDeviceSuitable(device)) continue;
      uint32_t score = RateDevice(device);
      candidates.insert(std::make_pair(score, device));
   }
   // Get the best device as selected
   if (candidates.rbegin()->first > 0) {
      m_physicalDevice = candidates.rbegin()->second;
   } else {
      throw std::runtime_error("Failed to find a suitable GPU.");
   }
}

uint32_t VulkanRenderer::RateDevice(const VkPhysicalDevice& device) {
   // Set base score to 0
   uint32_t score = 0;
   // Get device capabilities and properties
   VkPhysicalDeviceProperties deviceProperties;
   VkPhysicalDeviceFeatures deviceFeatures;
   vkGetPhysicalDeviceProperties(device, &deviceProperties);
   vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
   // Discrete gpus are better than integrated ones
   if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      score += 1000;
   }
   // Maximum texture size improves the gpu score
   score += deviceProperties.limits.maxImageDimension2D;
   // Geometry shaders are required
   if (!deviceFeatures.geometryShader) {
      score = 0;
   }
   return score;
}

void VulkanRenderer::GetQueueFamilies(const VkPhysicalDevice& device) {
   // Get the queue families off of the current device
   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(device,
                                            &queueFamilyCount,
                                            nullptr);
   std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
   vkGetPhysicalDeviceQueueFamilyProperties(device,
                                            &queueFamilyCount,
                                            queueFamilies.data());

   // Set up queue family indices
   uint32_t i = 0;
   for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
      // Check for graphics queue
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         m_queueFamilies.graphicsFamily = i;
      }
      // Check for present queue
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface->Get(),
                                           &presentSupport);
      if (presentSupport) {
         m_queueFamilies.presentFamily = i;
      }
      // If complete, early exit
      if (m_queueFamilies.HasAllValues()) break;
      ++i;
   }
}

bool VulkanRenderer::IsDeviceSuitable(const VkPhysicalDevice& device) {
   GetQueueFamilies(device);
   bool extensionsSupported = CheckDeviceExtensionSupport(device);
   bool swapChainAdequate = false;
   if (extensionsSupported) {
      SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
   }
   return m_queueFamilies.HasAllValues() && extensionsSupported && swapChainAdequate;
}

bool VulkanRenderer::CheckDeviceExtensionSupport(const VkPhysicalDevice& device) {
   uint32_t extensionCount;
   vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
   std::vector<VkExtensionProperties> availableExtensions(extensionCount);
   vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
   std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
   for (const auto& extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
   }
   return requiredExtensions.empty();
}

void VulkanRenderer::GetLogicalDevice() {
   // Specifies the queues to create for vulkan
   std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
   std::set<uint32_t> uniqueQueueFamilies = {
      m_queueFamilies.graphicsFamily.value(),
      m_queueFamilies.presentFamily.value()
   };
   // Setup create info for all families
   float queuePriority = 1.0f; // Set priority to 1.0f (can change based on queue for finer control)
   for (uint32_t queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.emplace_back(queueCreateInfo);
   }
   // Specifies the used device features
   // TODO: Specify used features
   VkPhysicalDeviceFeatures deviceFeatures{};
   // Create the logical device
   VkDeviceCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
   createInfo.pQueueCreateInfos = queueCreateInfos.data();
   createInfo.pEnabledFeatures = &deviceFeatures;
   // Set enabled extensioms
   createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
   createInfo.ppEnabledExtensionNames = deviceExtensions.data();
   // Set validation layers if debug
   if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
   } else {
      createInfo.enabledLayerCount = 0;
   }
   if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr,
                      &m_logicalDevice) != VK_SUCCESS) {
      throw std::runtime_error("failed to create logical device!");
   }
   vkGetDeviceQueue(m_logicalDevice, m_queueFamilies.graphicsFamily.value(),
                    0, &m_graphicsQueue);
   vkGetDeviceQueue(m_logicalDevice, m_queueFamilies.presentFamily.value(),
                    0, &m_presentQueue);
}

void VulkanRenderer::GetSwapchain() {
   SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice);
   VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
   VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
   VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);
   uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
   if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
   }
   VkSwapchainCreateInfoKHR createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   createInfo.surface = m_surface->Get();
   createInfo.minImageCount = imageCount;
   createInfo.imageFormat = surfaceFormat.format;
   createInfo.imageColorSpace = surfaceFormat.colorSpace;
   createInfo.imageExtent = extent;
   createInfo.imageArrayLayers = 1;
   createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   uint32_t queueFamilyIndices[] = {m_queueFamilies.graphicsFamily.value(), m_queueFamilies.presentFamily.value()};
   if (m_queueFamilies.graphicsFamily != m_queueFamilies.presentFamily) {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
   } else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;
      createInfo.pQueueFamilyIndices = nullptr;
   }
   createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
   createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   createInfo.presentMode = presentMode;
   createInfo.clipped = VK_TRUE;
   createInfo.oldSwapchain = VK_NULL_HANDLE;
   if (vkCreateSwapchainKHR(m_logicalDevice, &createInfo,
                            nullptr, &m_swapchain) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create swap chain.");
   }
   vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain,
                           &imageCount, nullptr);
   m_swapchainImages.resize(imageCount);
   vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain,
                           &imageCount, m_swapchainImages.data());
   m_swapchainImageFormat = surfaceFormat.format;
   m_swapchainExtent = extent;
}

SwapChainSupportDetails VulkanRenderer::QuerySwapChainSupport(const VkPhysicalDevice& device) const {
   // Get the surface capabilities
   SwapChainSupportDetails details;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface->Get(),
                                             &details.capabilities);
   // Query supported surface formats
   uint32_t formatCount;
   vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface->Get(),
                                        &formatCount, nullptr);
   if (formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface->Get(),
                                           &formatCount, details.formats.data());
   }
   // Query supported presentation modes
   uint32_t presentModeCount;
   vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface->Get(),
                                             &presentModeCount, nullptr);

   if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface->Get(),
                                                &presentModeCount, details.presentModes.data());
   }
   return details;
}

VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
   // Select an SRGB non-linear format for the surface
   for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
         return availableFormat;
      }
   }
   // Otherwise, get the first one
   return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const {
   for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
         return availablePresentMode;
      }
   }
   return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
   if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
   } else {
      int32_t width, height;
      glfwGetFramebufferSize(m_windowHandle, &width, &height);
      VkExtent2D actualExtent = {
         static_cast<uint32_t>(width),
         static_cast<uint32_t>(height)
      };
      actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                      capabilities.maxImageExtent.width);
      actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                       capabilities.maxImageExtent.height);
      return actualExtent;
   }
}

void VulkanRenderer::GetImageViews() {
   m_swapchainImageViews.resize(m_swapchainImages.size());
   for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
      VkImageViewCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image = m_swapchainImages[i];
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = m_swapchainImageFormat;
      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;
      if (vkCreateImageView(m_logicalDevice, &createInfo,
                            nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create image views.");
      }
   }
}

void VulkanRenderer::CreateRenderPass() {
   VkAttachmentDescription colorAttachment{};
   colorAttachment.format = m_swapchainImageFormat;
   colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
   colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   VkAttachmentReference colorAttachmentRef{};
   colorAttachmentRef.attachment = 0;
   colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   VkSubpassDescription subpass{};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &colorAttachmentRef;
   VkSubpassDependency dependency{};
   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass = 0;
   dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.srcAccessMask = 0;
   dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
   VkRenderPassCreateInfo renderPassInfo{};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = 1;
   renderPassInfo.pAttachments = &colorAttachment;
   renderPassInfo.subpassCount = 1;
   renderPassInfo.pSubpasses = &subpass;
   renderPassInfo.dependencyCount = 1;
   renderPassInfo.pDependencies = &dependency;
   if (vkCreateRenderPass(m_logicalDevice, &renderPassInfo,
                          nullptr, &m_renderPass) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create render pass.");
   }
}

std::vector<char> VulkanRenderer::ReadFile(const std::string& filename) {
   std::ifstream file(filename, std::ios::ate | std::ios::binary);
   if (!file.is_open()) {
      throw std::runtime_error("Failed to open file.");
   }
   const size_t fileSize = (size_t) file.tellg();
   std::vector<char> buffer(fileSize);
   file.seekg(0);
   file.read(buffer.data(), static_cast<int64_t>(fileSize));
   file.close();
   return buffer;
}

void VulkanRenderer::CreateGraphicsPipeline() {
   // Create shader modules
   const std::vector<char> vertShaderCode = ReadFile("resources/shaders/triangle.vert.spv");
   const std::vector<char> fragShaderCode = ReadFile("resources/shaders/triangle.frag.spv");
   const VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
   const VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);
   // Create shader stages for the programmable stages of the Pipeline
   VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
   vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
   vertShaderStageInfo.module = vertShaderModule;
   vertShaderStageInfo.pName = "main";
   VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
   fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   fragShaderStageInfo.module = fragShaderModule;
   fragShaderStageInfo.pName = "main";
   VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
   // Setup non-programmable stages of the pipeline
   // Setup format of the vertex data
   VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
   vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputInfo.vertexBindingDescriptionCount = 0;
   vertexInputInfo.pVertexBindingDescriptions = nullptr;
   vertexInputInfo.vertexAttributeDescriptionCount = 0;
   vertexInputInfo.pVertexAttributeDescriptions = nullptr;
   // Setup type of geometry to draw
   VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
   inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   inputAssembly.primitiveRestartEnable = VK_FALSE;
   // Setup dynamic states, can be changed at draw call time
   const std::vector<VkDynamicState> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
   };
   VkPipelineDynamicStateCreateInfo dynamicState{};
   dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
   dynamicState.pDynamicStates = dynamicStates.data();
   // Setup viewport as dynamic (no scissor and no viewport, to define at draw time)
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
   rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
   rasterizer.depthBiasEnable = VK_FALSE;
   rasterizer.depthBiasConstantFactor = 0.0f;
   rasterizer.depthBiasClamp = 0.0f;
   rasterizer.depthBiasSlopeFactor = 0.0f;
   // Setup multisampling
   VkPipelineMultisampleStateCreateInfo multisampling{};
   multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.sampleShadingEnable = VK_FALSE;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   multisampling.minSampleShading = 1.0f;
   multisampling.pSampleMask = nullptr;
   multisampling.alphaToCoverageEnable = VK_FALSE;
   multisampling.alphaToOneEnable = VK_FALSE;
   // Setup color blending
   VkPipelineColorBlendAttachmentState colorBlendAttachment{};
   colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable = VK_FALSE;
   colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
   colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
   colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
   VkPipelineColorBlendStateCreateInfo colorBlending{};
   colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlending.logicOpEnable = VK_FALSE;
   colorBlending.logicOp = VK_LOGIC_OP_COPY;
   colorBlending.attachmentCount = 1;
   colorBlending.pAttachments = &colorBlendAttachment;
   colorBlending.blendConstants[0] = 0.0f;
   colorBlending.blendConstants[1] = 0.0f;
   colorBlending.blendConstants[2] = 0.0f;
   colorBlending.blendConstants[3] = 0.0f;
   // Setup a pipeline layout for uniforms
   VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
   pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount = 0; // Optional
   pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
   pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
   pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
   if (vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo,
                              nullptr, &m_pipelineLayout) != VK_SUCCESS) {
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
   pipelineInfo.renderPass = m_renderPass;
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
   pipelineInfo.basePipelineIndex = -1;
   if (vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1,
                                 &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create graphics pipeline.");
   }
   // Unload shader modules
   vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
   vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& code) {
   VkShaderModuleCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   createInfo.codeSize = code.size();
   createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
   VkShaderModule shaderModule;
   if (vkCreateShaderModule(m_logicalDevice, &createInfo,
                            nullptr, &shaderModule) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create shader module.");
   }
   return shaderModule;
}

void VulkanRenderer::CreateFramebuffers() {
   // Create framebuffers from the given swapchain images
   m_swapchainFramebuffers.resize(m_swapchainImageViews.size());
   for (size_t i = 0; i < m_swapchainImageViews.size(); ++i) {
      const VkImageView attachments[] = {
         m_swapchainImageViews[i]
      };
      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = m_renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = m_swapchainExtent.width;
      framebufferInfo.height = m_swapchainExtent.height;
      framebufferInfo.layers = 1;
      if (vkCreateFramebuffer(m_logicalDevice, &framebufferInfo,
                              nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
         throw std::runtime_error("Failed to create a framebuffer.");
      }
   }
}

void VulkanRenderer::CreateCommandPool() {
   VkCommandPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   poolInfo.queueFamilyIndex = m_queueFamilies.graphicsFamily.value();
   if (vkCreateCommandPool(m_logicalDevice, &poolInfo,
                           nullptr, &m_commandPool) != VK_SUCCESS) {
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
   if (vkAllocateCommandBuffers(m_logicalDevice,
                                &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate command buffers.");
   }
}

void VulkanRenderer::RecordCommandBuffer(const VkCommandBuffer& commandBuffer, const uint32_t imageIndex) {
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
   renderPassInfo.renderPass = m_renderPass;
   renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
   renderPassInfo.renderArea.offset = {0, 0};
   renderPassInfo.renderArea.extent = m_swapchainExtent;
   // Clear color to black
   VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
   renderPassInfo.clearValueCount = 1;
   renderPassInfo.pClearValues = &clearColor;
   vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
   vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
   // Set the dynamic viewport and scissor
   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(m_swapchainExtent.width);
   viewport.height = static_cast<float>(m_swapchainExtent.height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = m_swapchainExtent;
   vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
   // Draw the triangle
   vkCmdDraw(commandBuffer, 3, 1, 0, 0);
   vkCmdEndRenderPass(commandBuffer);
   if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("Failed to record command buffer.");
   }
}

void VulkanRenderer::CreateSynchronizationObjects() {
   m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
   m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
   m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
   VkSemaphoreCreateInfo semaphoreInfo{};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
   VkFenceCreateInfo fenceInfo{};
   fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      if (vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
         vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
         vkCreateFence(m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {

         throw std::runtime_error("failed to create synchronization objects for a frame!");
      }
   }
}

void VulkanRenderer::RecreateSwapchain() {
   vkDeviceWaitIdle(m_logicalDevice);
   GetSwapchain();
   GetImageViews();
   CreateFramebuffers();
}

void VulkanRenderer::CleanupSwapchain() {
   for (const VkFramebuffer& framebuffer : m_swapchainFramebuffers) {
      vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
   }
   for (const VkImageView& imageView : m_swapchainImageViews) {
      vkDestroyImageView(m_logicalDevice, imageView, nullptr);
   }
   vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
}

VulkanRenderer::~VulkanRenderer() {
   vkDeviceWaitIdle(m_logicalDevice);
   CleanupSwapchain();
   vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
   vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
   vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
   }
   vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
   vkDestroyDevice(m_logicalDevice, nullptr);
   /*
   ImGui_ImplVulkan_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
   */
}

void VulkanRenderer::RenderFrame() {
   // Wait for previous farme
   vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
   // Get the next image of the swapchain
   uint32_t imageIndex;
   const VkResult nextImageResult = vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, UINT64_MAX,
                                                          m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
   if (nextImageResult == VK_ERROR_OUT_OF_DATE_KHR || nextImageResult == VK_SUBOPTIMAL_KHR) {
      RecreateSwapchain();
      return;
   } else if (nextImageResult != VK_SUCCESS) {
      throw std::runtime_error("Failed to acquire swap chain image.");
   }
   vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame]);
   // Setup command buffer to draw the triangle
   vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
   RecordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);
   // Submit the command buffer
   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
   VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores = waitSemaphores;
   submitInfo.pWaitDstStageMask = waitStages;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
   VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
   submitInfo.signalSemaphoreCount = 1;
   submitInfo.pSignalSemaphores = signalSemaphores;
   if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
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
   VkSwapchainKHR swapChains[] = {m_swapchain};
   presentInfo.swapchainCount = 1;
   presentInfo.pSwapchains = swapChains;
   presentInfo.pImageIndices = &imageIndex;
   presentInfo.pResults = nullptr;
   const VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);
   if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
      RecreateSwapchain();
   } else if (presentResult != VK_SUCCESS) {
      throw std::runtime_error("Failed to present swap chain image.");
   }
   // Increase frame counter
   m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

