#include "core/system/SystemInfo.hpp"

#include "core/Window.hpp"
#include "core/system/PerformanceMetrics.hpp"
#include "vk/VulkanDevice.hpp"

#include <thread>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#elif __linux__
#include <fstream>
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

#include <glad/gl.h>
#include <vulkan/vulkan.h>

namespace SystemInfoN {

std::string GetCPUModel() {
#ifdef _WIN32
   char cpuBrand[0x40];
   int cpuInfo[4] = {-1};
   __cpuid(cpuInfo, 0x80000000);
   unsigned int extIds = cpuInfo[0];
   memset(cpuBrand, 0, sizeof(cpuBrand));
   for (unsigned int i = 0x80000002; i <= std::min(extIds, 0x80000004u); ++i) {
      __cpuid(cpuInfo, i);
      memcpy(cpuBrand + (i - 0x80000002) * 16, cpuInfo, sizeof(cpuInfo));
   }
   return std::string(cpuBrand);
#elif __linux__
   std::ifstream cpuinfo("/proc/cpuinfo");
   std::string line;
   while (std::getline(cpuinfo, line)) {
      if (line.find("model name") != std::string::npos) {
         size_t pos = line.find(':');
         if (pos != std::string::npos) {
            return line.substr(pos + 2);
         }
      }
   }
   return "Unknown CPU";
#endif
}

uint32_t GetCPUThreadCount() { return std::thread::hardware_concurrency(); }

float GetCPUClockSpeed() {
#ifdef _WIN32
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
                    0, KEY_READ, &hKey) == ERROR_SUCCESS) {
      DWORD mhz = 0;
      DWORD size = sizeof(mhz);
      if (RegQueryValueEx(hKey, TEXT("~MHz"), nullptr, nullptr, reinterpret_cast<LPBYTE>(&mhz),
                          &size) == ERROR_SUCCESS) {
         RegCloseKey(hKey);
         return mhz / 1000.0f;
      }
      RegCloseKey(hKey);
   }
   return 0.0f;
#elif __linux__
   std::ifstream cpuinfo("/proc/cpuinfo");
   std::string line;
   while (std::getline(cpuinfo, line)) {
      if (line.find("cpu MHz") != std::string::npos) {
         size_t pos = line.find(':');
         if (pos != std::string::npos) {
            float mhz = std::stof(line.substr(pos + 2));
            return mhz / 1000.0f;
         }
      }
   }
   return 0.0f;
#else
   return 0.0f;
#endif
}

float GetCPUUtilization() {
#ifdef _WIN32
   static PDH_HQUERY query = nullptr;
   static PDH_HCOUNTER counter = nullptr;
   if (!query) {
      PdhOpenQuery(nullptr, 0, &query);
      PdhAddEnglishCounterW(query, L"\\Processor(_Total)\\% Processor Time", 0, &counter);
      PdhCollectQueryData(query);
   }
   PDH_FMT_COUNTERVALUE value;
   PdhCollectQueryData(query);
   PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value);
   return static_cast<float>(value.doubleValue);
#elif __linux__
   static unsigned long long lastTotalTime = 0;
   static unsigned long long lastIdleTime = 0;
   std::ifstream stat("/proc/stat");
   std::string line;
   std::getline(stat, line);
   unsigned long long user, nice, system, idle, iowait, irq, softirq;
   sscanf(line.c_str(), "cpu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle,
          &iowait, &irq, &softirq);
   unsigned long long totalTime = user + nice + system + idle + iowait + irq + softirq;
   unsigned long long idleTime = idle + iowait;
   float utilization = 0.0f;
   if (lastTotalTime > 0) {
      unsigned long long totalDelta = totalTime - lastTotalTime;
      unsigned long long idleDelta = idleTime - lastIdleTime;
      utilization = 100.0f * (1.0f - static_cast<float>(idleDelta) / totalDelta);
   }
   lastTotalTime = totalTime;
   lastIdleTime = idleTime;
   return utilization;
#else
   return 0.0f;
#endif
}

size_t GetSystemMemoryUsageMB() {
#ifdef _WIN32
   PROCESS_MEMORY_COUNTERS_EX pmc;
   GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                        sizeof(pmc));
   return pmc.WorkingSetSize / (1024 * 1024);
#elif __linux__
   std::ifstream status("/proc/self/status");
   std::string line;
   while (std::getline(status, line)) {
      if (line.find("VmRSS:") != std::string::npos) {
         std::istringstream iss(line);
         std::string label;
         size_t value;
         iss >> label >> value;
         return value / 1024;
      }
   }
   return 0;
#else
   return 0;
#endif
}

// Vulkan implementations
std::string GetVulkanGPUModel(const VulkanDevice& device) {
   VkPhysicalDeviceProperties props;
   vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &props);
   return std::string(props.deviceName);
}

size_t GetVulkanVRAMMB(const VulkanDevice& device) {
   VkPhysicalDeviceMemoryProperties memProps;
   vkGetPhysicalDeviceMemoryProperties(device.GetPhysicalDevice(), &memProps);

   size_t vramSize = 0;
   for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
      if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
         vramSize += memProps.memoryHeaps[i].size;
      }
   }
   return vramSize / (1024 * 1024);
}

std::string GetVulkanDriverVersion(const VulkanDevice& device) {
   VkPhysicalDeviceProperties props;
   vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &props);
   const uint32_t major = VK_VERSION_MAJOR(props.driverVersion);
   const uint32_t minor = VK_VERSION_MINOR(props.driverVersion);
   const uint32_t patch = VK_VERSION_PATCH(props.driverVersion);
   std::ostringstream oss;
   oss << major << "." << minor << "." << patch;
   return oss.str();
}

std::string GetVulkanAPIVersion(const VulkanDevice& device) {
   VkPhysicalDeviceProperties props;
   vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &props);
   const uint32_t major = VK_API_VERSION_MAJOR(props.apiVersion);
   const uint32_t minor = VK_API_VERSION_MINOR(props.apiVersion);
   const uint32_t patch = VK_API_VERSION_PATCH(props.apiVersion);
   std::ostringstream oss;
   oss << "Vulkan " << major << "." << minor << "." << patch;
   return oss.str();
}

size_t GetVulkanMemoryUsageMB(const VulkanDevice& device) {
   VmaAllocator allocator = device.GetAllocator();
   if (!allocator)
      return 0;
   VmaTotalStatistics stats;
   vmaCalculateStatistics(allocator, &stats);
   return stats.total.statistics.allocationBytes / (1024 * 1024);
}

float GetVulkanGPUUtilization(const VulkanDevice& device) {
   // GPU utilization is not directly available through Vulkan
   // Would require platform-specific extensions or external monitoring
   return 0.0f;
}

// OpenGL implementations
std::string GetOpenGLGPUModel() {
   const GLubyte* renderer = glGetString(GL_RENDERER);
   return renderer ? std::string(reinterpret_cast<const char*>(renderer)) : "Unknown GPU";
}

size_t GetOpenGLVRAMMB() {
   // OpenGL doesn't provide a standard way to query total VRAM
   // This uses vendor-specific extensions
   GLint memKB = 0;
   // NVIDIA
   if (GLAD_GL_NVX_gpu_memory_info) {
      glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &memKB);
      if (memKB > 0)
         return memKB / 1024;
   }
   // AMD
   if (GLAD_GL_ATI_meminfo) {
      GLint memInfo[4];
      glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, memInfo);
      return memInfo[0] / 1024;
   }
   return 0;
}

std::string GetOpenGLDriverVersion() {
   const GLubyte* version = glGetString(GL_VERSION);
   return version ? std::string(reinterpret_cast<const char*>(version)) : "Unknown";
}

std::string GetOpenGLAPIVersion() {
   int32_t major = 0, minor = 0;
   glGetIntegerv(GL_MAJOR_VERSION, &major);
   glGetIntegerv(GL_MINOR_VERSION, &minor);
   std::ostringstream oss;
   oss << "OpenGL " << major << "." << minor;
   return oss.str();
}

size_t GetOpenGLMemoryUsageMB() {
   int32_t memKB = 0;
   // NVIDIA
   if (GLAD_GL_NVX_gpu_memory_info) {
      int32_t totalMem, availMem;
      glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalMem);
      glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availMem);
      return (totalMem - availMem) / 1024;
   }
   // AMD
   if (GLAD_GL_ATI_meminfo) {
      int32_t memInfo[4];
      glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, memInfo);
      // memInfo[0] is free memory in KB
      return memInfo[0] / 1024;
   }
   return 0;
}

float GetOpenGLGPUUtilization() {
   // Not available through standard OpenGL
   return 0.0f;
}

SystemInfo BuildSystemInfo(const GraphicsAPI api, const Window& window, const void* devicePtr) {
   SystemInfo info;
   // CPU info
   info.cpuModel = GetCPUModel();
   info.threadCount = GetCPUThreadCount();
   info.clockSpeedGHz = GetCPUClockSpeed();
   // Window info
   info.windowWidth = window.GetWidth();
   info.windowHeight = window.GetHeight();
   // GPU info based on API
   if (api == GraphicsAPI::Vulkan && devicePtr) {
      const auto* device = static_cast<const VulkanDevice*>(devicePtr);
      info.gpuModel = GetVulkanGPUModel(*device);
      info.vramMB = GetVulkanVRAMMB(*device);
      info.driverVersion = GetVulkanDriverVersion(*device);
      info.apiVersion = GetVulkanAPIVersion(*device);
   } else if (api == GraphicsAPI::OpenGL) {
      info.gpuModel = GetOpenGLGPUModel();
      info.vramMB = GetOpenGLVRAMMB();
      info.driverVersion = GetOpenGLDriverVersion();
      info.apiVersion = GetOpenGLAPIVersion();
   }
   return info;
}

} // namespace SystemInfoN
