#include "VulkanDebugMessenger.hpp"

#include <print>
#include <stdexcept>

#include "VulkanInstance.hpp"

// Callback for validation layer debug messages
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMessenger::DebugCallback(
   VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
   VkDebugUtilsMessageTypeFlagsEXT messageType,
   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
   void* pUserData) {
   std::println("Validation layer: {}", pCallbackData->pMessage);
   return VK_FALSE;
}

// Fetch messenger extension functions
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

VulkanDebugMessenger::VulkanDebugMessenger(const VulkanInstance& instance)
   :
   m_instance(&instance)
{
   // Select debug layers to log
   VkDebugUtilsMessengerCreateInfoEXT createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   createInfo.pfnUserCallback = DebugCallback;
   createInfo.pUserData = nullptr;
   // Create the debug messenger
   if (CreateDebugUtilsMessengerEXT(m_instance->Get(), &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
      throw std::runtime_error("Failed to set up debug messenger.");
   }
}

VulkanDebugMessenger::~VulkanDebugMessenger() {
   if (m_debugMessenger != VK_NULL_HANDLE && m_instance != nullptr) {
      DestroyDebugUtilsMessengerEXT(m_instance->Get(), m_debugMessenger, nullptr);
   }
}

VulkanDebugMessenger::VulkanDebugMessenger(VulkanDebugMessenger&& other) noexcept
   :
   m_instance(other.m_instance),
   m_debugMessenger(other.m_debugMessenger)
{
   other.m_instance = nullptr;
   other.m_debugMessenger = VK_NULL_HANDLE;
}

VulkanDebugMessenger& VulkanDebugMessenger::operator=(VulkanDebugMessenger&& other) noexcept {
   if (this != &other) {
      if (m_debugMessenger != VK_NULL_HANDLE && m_instance != nullptr) {
         DestroyDebugUtilsMessengerEXT(m_instance->Get(), m_debugMessenger, nullptr);
      }
      m_instance = other.m_instance;
      m_debugMessenger = other.m_debugMessenger;
      other.m_instance = nullptr;
      other.m_debugMessenger = VK_NULL_HANDLE;
   }
   return *this;
}

VkDebugUtilsMessengerEXT VulkanDebugMessenger::Get() const {
   return m_debugMessenger;
}

