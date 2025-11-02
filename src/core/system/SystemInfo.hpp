#pragma once

#include "core/GraphicsAPI.hpp"
#include "core/system/PerformanceMetrics.hpp"

#include <string>

class Window;
class VulkanDevice;

namespace SystemInfoN {

std::string GetCPUModel();
uint32_t GetCPUThreadCount();
float GetCPUClockSpeed();

float GetCPUUtilization();

size_t GetSystemMemoryUsageMB();

std::string GetVulkanGPUModel(const VulkanDevice& device);
size_t GetVulkanVRAMMB(const VulkanDevice& device);
std::string GetVulkanDriverVersion(const VulkanDevice& device);
std::string GetVulkanAPIVersion(const VulkanDevice& device);
size_t GetVulkanMemoryUsageMB(const VulkanDevice& device);

std::string GetOpenGLGPUModel();
size_t GetOpenGLVRAMMB();
std::string GetOpenGLDriverVersion();
std::string GetOpenGLAPIVersion();
size_t GetOpenGLMemoryUsageMB();

SystemInfo BuildSystemInfo(const GraphicsAPI api, const Window& window, const void* devicePtr = nullptr);
} // namespace SystemInfoN
