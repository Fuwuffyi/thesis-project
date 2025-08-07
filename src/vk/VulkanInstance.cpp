#include "VulkanInstance.hpp"

#include <cstring>
#include <GLFW/glfw3.h>
#include <stdexcept>

VulkanInstance::VulkanInstance(const std::vector<const char*>& extensions,
                               const std::vector<const char*>& validationLayers,
                               const bool enableValidation) {
   // Check for validation (debug) layers
   if (enableValidation && !CheckValidationLayerSupport(validationLayers)) {
      throw std::runtime_error("Validation layers requested, but not available.");
   }
   // Setup the information for the application
   VkApplicationInfo appInfo{};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "Thesis Project";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName = "No Engine";
   appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.apiVersion = VK_API_VERSION_1_4;
   // Get required extensions for GLFW and others
   const std::vector<const char *> requiredExtensions = GetRequiredExtensions(enableValidation);
   // Create instance information
   VkInstanceCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   createInfo.pApplicationInfo = &appInfo;
   createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
   createInfo.ppEnabledExtensionNames = requiredExtensions.data();
   // Set debug layers if enabled
   if (enableValidation) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
   } else {
      createInfo.enabledLayerCount = 0;
   }
   // Create the vulkan instance
   if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create Vulkan instance.");
   }
}

VulkanInstance::~VulkanInstance() {
   if (m_instance != VK_NULL_HANDLE) {
      vkDestroyInstance(m_instance, nullptr);
   }
}

VulkanInstance::VulkanInstance(VulkanInstance&& other) noexcept 
: m_instance(other.m_instance) {
   other.m_instance = VK_NULL_HANDLE;
}

VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other) noexcept {
   if (this != &other) {
      if (m_instance != VK_NULL_HANDLE) {
         vkDestroyInstance(m_instance, nullptr);
      }
      m_instance = other.m_instance;
      other.m_instance = VK_NULL_HANDLE;
   }
   return *this;
}

std::vector<const char*> VulkanInstance::GetRequiredExtensions(const bool enableValidation) const {
   uint32_t glfwExtensionCount = 0;
   const char** glfwExtensions;
   // Get glfw extensions
   glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
   std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
   // Add debug extensions
   if (enableValidation) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
   }
   return extensions;
}

bool VulkanInstance::CheckValidationLayerSupport(const std::vector<const char*>& validationLayers) const {
   // Get available validation layers
   uint32_t layerCount;
   vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
   std::vector<VkLayerProperties> availableLayers(layerCount);
   vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
   // Check if the required layers are contained
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

VkInstance VulkanInstance::Get() const {
   return m_instance;
}

