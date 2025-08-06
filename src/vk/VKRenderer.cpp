#include "VKRenderer.hpp"

#include <print>
#include <stdexcept>
#include <vector>
#include <map>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

const std::vector<const char*> validationLayers = {
   "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

VKRenderer::VKRenderer(GLFWwindow* windowHandle) : IRenderer(windowHandle) {
   CreateInstance();
   GetDebugMessenger();
   GetPhysicalDevice();
   GetLogicalDevice();
}

VKAPI_ATTR VkBool32 VKAPI_CALL VKRenderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                         void* pUserData) {
   std::println("Validation layer: {}", pCallbackData->pMessage);
   return VK_FALSE;
}

void VKRenderer::CreateInstance() {
   // Check for available debug validation layers
   if (enableValidationLayers && !CheckValidationLayerSupport()) {
      throw std::runtime_error("Validation layers requested, but not available.");
   }
   // Information about the application
   VkApplicationInfo appInfo{};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "Thesis Project";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName = "No Engine";
   appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.apiVersion = VK_API_VERSION_1_4;
   // Setup extensions
   VkInstanceCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   createInfo.pApplicationInfo = &appInfo;
   // Set required vulkan extensions
   auto extensions = GetRequiredExtensions();
   createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
   createInfo.ppEnabledExtensionNames = extensions.data();
   // Set up validation layers
   if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
   } else {
      createInfo.enabledLayerCount = 0;
   }
   // Create the instance
   if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create Vulkan instance.");
   }
}

std::vector<const char*> VKRenderer::GetRequiredExtensions() const {
   // Get glfw required extensions
   uint32_t glfwExtensionCount = 0;
   const char** glfwExtensions;
   glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
   std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
   // Add debug extension if debug enabled
   if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
   }
   return extensions;
}

bool VKRenderer::CheckValidationLayerSupport() const {
   // Get validation layer values
   uint32_t layerCount;
   vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
   std::vector<VkLayerProperties> availableLayers(layerCount);
   vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
   // Check for the required validation layer
   for (const char* layerName : validationLayers) {
      bool layerFound = false;
      for (const VkLayerProperties& layerProperties : availableLayers) {
         if (strcmp(layerName, layerProperties.layerName) == 0) {
            layerFound = true;
            break;
         }
      }
      if (!layerFound) {
         return false;
      }
   }
   return true;
}

// Load the debug messenger functions using the proc address, since it is an extension function
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
   auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
   if (func != nullptr) {
      func(instance, debugMessenger, pAllocator);
   }
}

void VKRenderer::GetDebugMessenger() {
   // Setup struct to hold debug messenger information
   VkDebugUtilsMessengerCreateInfoEXT createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   createInfo.pfnUserCallback = VKRenderer::DebugCallback;
   createInfo.pUserData = nullptr;
   if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
      throw std::runtime_error("Failed to set up debug messenger.");
   }
}

void VKRenderer::GetPhysicalDevice() {
   // Get device count
   uint32_t deviceCount = 0;
   vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
   // If no devices, exit
   if (deviceCount == 0) {
      throw std::runtime_error("failed to find GPUs with Vulkan support.");
   }
   // Get actual devices
   std::vector<VkPhysicalDevice> devices(deviceCount);
   vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
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

uint32_t VKRenderer::RateDevice(const VkPhysicalDevice& device) {
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

void VKRenderer::GetQueueFamilies(const VkPhysicalDevice& device) {
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
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         m_queueFamilies.graphicsFamily = i;
      }
      if (m_queueFamilies.HasAllValues()) break;
      ++i;
   }
}

bool VKRenderer::IsDeviceSuitable(const VkPhysicalDevice& device) {
   GetQueueFamilies(device);
   return m_queueFamilies.HasAllValues();
}

void VKRenderer::GetLogicalDevice() {
   // Specifies the queues to create for vulkan
   VkDeviceQueueCreateInfo queueCreateInfo{};
   queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
   queueCreateInfo.queueFamilyIndex = m_queueFamilies.graphicsFamily.value();
   queueCreateInfo.queueCount = 1;
   // Set priority to 1 as we only have one queue (useful to influence the queue submit scheduling)
   float queuePriority = 1.0f;
   queueCreateInfo.pQueuePriorities = &queuePriority;
   // Specifies the used device features
   // TODO: Specify used features
   VkPhysicalDeviceFeatures deviceFeatures{};
   // Create the logical device
   VkDeviceCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   createInfo.pQueueCreateInfos = &queueCreateInfo;
   createInfo.queueCreateInfoCount = 1;
   createInfo.pEnabledFeatures = &deviceFeatures;
   // Set enabled extensioms
   createInfo.enabledExtensionCount = 0;
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
}

VKRenderer::~VKRenderer() {
   vkDestroyDevice(m_logicalDevice, nullptr);
   if (enableValidationLayers) {
      DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
   }
   vkDestroyInstance(m_instance, nullptr);
   ImGui_ImplVulkan_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

void VKRenderer::RenderFrame() {
   // ImGui_ImplVulkan_NewFrame();
   // ImGui_ImplGlfw_NewFrame();
   // ImGui::NewFrame();
   // ImGui::ShowDemoWindow();
   // ImGui::Render();
   // ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}

