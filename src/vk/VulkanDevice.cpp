#include "VulkanDevice.hpp"

#include <vulkan/vulkan_core.h>
#include <map>
#include <set>
#include <stdexcept>

#include "VulkanInstance.hpp"
#include "VulkanSurface.hpp"

VulkanDevice::VulkanDevice(const VulkanInstance& instance, const VulkanSurface& surface,
                           const std::vector<const char*>& requiredExtensions, const std::vector<const char*>& validationLayers,
                           const bool enableValidation)
   :
   m_instance(&instance),
   m_surface(&surface)
{
   CreatePhysicalDevice(requiredExtensions);
   FindQueueFamilies();
   CreateLogicalDevice(requiredExtensions, validationLayers, enableValidation);
   GetDeviceQueues();
}

VulkanDevice::~VulkanDevice() {
   if (m_device != VK_NULL_HANDLE) {
      vkDestroyDevice(m_device, nullptr);
   }
}

void VulkanDevice::CreatePhysicalDevice(const std::vector<const char*>& requiredExtensions) {
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
      if (!IsDeviceSuitable(device, m_surface->Get(), requiredExtensions)) continue;
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

void VulkanDevice::CreateLogicalDevice(const std::vector<const char*>& requiredExtensions,
                                       const std::vector<const char*>& validationLayers,
                                       const bool enableValidation) {
   // Specifies the queues to create for vulkan
   std::set<uint32_t> uniqueQueueFamilies;
   if (m_queueFamilies.graphicsFamily.has_value()) uniqueQueueFamilies.insert(m_queueFamilies.graphicsFamily.value());
   if (m_queueFamilies.presentFamily.has_value()) uniqueQueueFamilies.insert(m_queueFamilies.presentFamily.value());
   // Setup create info for all families
   std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
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
   VkPhysicalDeviceFeatures deviceFeatures{};
   deviceFeatures.samplerAnisotropy = VK_TRUE;
   // Create the logical device
   VkDeviceCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
   createInfo.pQueueCreateInfos = queueCreateInfos.data();
   createInfo.pEnabledFeatures = &deviceFeatures;
   // Set enabled extensioms
   createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
   createInfo.ppEnabledExtensionNames = requiredExtensions.data();
   // Set validation layers if debug
   if (enableValidation) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
   } else {
      createInfo.enabledLayerCount = 0;
   }
   if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr,
                      &m_device) != VK_SUCCESS) {
      throw std::runtime_error("failed to create logical device!");
   }
   vkGetDeviceQueue(m_device, m_queueFamilies.graphicsFamily.value(),
                    0, &m_graphicsQueue);
   vkGetDeviceQueue(m_device, m_queueFamilies.presentFamily.value(),
                    0, &m_presentQueue);
}

void VulkanDevice::FindQueueFamilies() {
   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
   std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
   vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());
   for (uint32_t i = 0; i < queueFamilyCount; ++i) {
      const VkQueueFamilyProperties& queueFamily = queueFamilies[i];
      // Graphics queue
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         m_queueFamilies.graphicsFamily = i;
      }
      // Present queue
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface->Get(), &presentSupport);
      if (presentSupport) {
         m_queueFamilies.presentFamily = i;
      }
   }
}

void VulkanDevice::GetDeviceQueues() {
   if (m_queueFamilies.graphicsFamily.has_value()) {
      vkGetDeviceQueue(m_device, m_queueFamilies.graphicsFamily.value(), 0, &m_graphicsQueue);
   }
   if (m_queueFamilies.presentFamily.has_value()) {
      vkGetDeviceQueue(m_device, m_queueFamilies.presentFamily.value(), 0, &m_presentQueue);
   }
}

VulkanDevice::VulkanDevice(VulkanDevice&& other) noexcept
   :
   m_instance(other.m_instance),
   m_surface(other.m_surface),
   m_physicalDevice(other.m_physicalDevice),
   m_device(other.m_device),
   m_queueFamilies(other.m_queueFamilies),
   m_graphicsQueue(other.m_graphicsQueue),
   m_presentQueue(other.m_presentQueue)
{
   other.m_instance = nullptr;
   other.m_surface = nullptr;
   other.m_physicalDevice = VK_NULL_HANDLE;
   other.m_device = VK_NULL_HANDLE;
   other.m_queueFamilies = {};
   other.m_graphicsQueue = VK_NULL_HANDLE;
   other.m_presentQueue = VK_NULL_HANDLE;
}

VulkanDevice& VulkanDevice::operator=(VulkanDevice&& other) noexcept {
   if (this != &other) {
      m_instance = other.m_instance;
      m_surface = other.m_surface;
      m_physicalDevice = other.m_physicalDevice;
      m_device = other.m_device;
      m_queueFamilies = other.m_queueFamilies;
      m_graphicsQueue = other.m_graphicsQueue;
      m_presentQueue = other.m_presentQueue;
      other.m_instance = nullptr;
      other.m_surface = nullptr;
      other.m_physicalDevice = VK_NULL_HANDLE;
      other.m_device = VK_NULL_HANDLE;
      other.m_queueFamilies = {};
      other.m_graphicsQueue = VK_NULL_HANDLE;
      other.m_presentQueue = VK_NULL_HANDLE;
   }
   return *this;
}

uint32_t VulkanDevice::RateDevice(const VkPhysicalDevice& device) {
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

bool VulkanDevice::IsDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface, const std::vector<const char*>& requiredExtensions) {
   // Temporarily store queue families for evaluation
   QueueFamilyIndices indices;
   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
   std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
   for (uint32_t i = 0; i < queueFamilyCount; ++i) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         indices.graphicsFamily = i;
      }
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
      if (presentSupport) {
         indices.presentFamily = i;
      }
      if (indices.HasAllValues()) break;
   }
   bool extensionsSupported = CheckDeviceExtensionSupport(device, requiredExtensions);
   bool swapChainAdequate = false;
   if (extensionsSupported) {
      SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
   }
   VkPhysicalDeviceFeatures supportedFeatures;
   vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
   return indices.HasAllValues() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool VulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& requiredExtensions) {
   uint32_t extensionCount;
   vkEnumerateDeviceExtensionProperties(device, nullptr,
                                        &extensionCount, nullptr);
   std::vector<VkExtensionProperties> availableExtensions(extensionCount);
   vkEnumerateDeviceExtensionProperties(device, nullptr,
                                        &extensionCount, availableExtensions.data());
   std::set<std::string> required(requiredExtensions.begin(), requiredExtensions.end());
   for (const auto& extension : availableExtensions) {
      required.erase(extension.extensionName);
   }
   return required.empty();
}

SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
   // Get the surface capabilities
   SwapChainSupportDetails details;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                             &details.capabilities);
   // Query supported surface formats
   uint32_t formatCount;
   vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface,
                                        &formatCount, nullptr);
   if (formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface,
                                           &formatCount, details.formats.data());
   }
   // Query supported presentation modes
   uint32_t presentModeCount;
   vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                             &presentModeCount, nullptr);

   if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                                &presentModeCount, details.presentModes.data());
   }
   return details;
}

const VkDevice& VulkanDevice::Get() const {
   return m_device;
}

const VkPhysicalDevice& VulkanDevice::GetPhysicalDevice() const {
   return m_physicalDevice;
}

const VkQueue& VulkanDevice::GetGraphicsQueue() const {
   return m_graphicsQueue;
}

const VkQueue& VulkanDevice::GetPresentQueue() const {
   return m_presentQueue;
}

const QueueFamilyIndices& VulkanDevice::GetQueueFamilies() const {
   return m_queueFamilies;
}

uint32_t VulkanDevice::GetGraphicsQueueFamily() const {
   return m_queueFamilies.graphicsFamily.value();
}

uint32_t VulkanDevice::GetPresentQueueFamily() const {
   return m_queueFamilies.presentFamily.value();
}

