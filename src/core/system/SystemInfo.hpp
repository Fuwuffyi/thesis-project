#pragma once

#include "core/system/PerformanceLogger.hpp"
#include "core/GraphicsAPI.hpp"

#include <string>

class Window;
class VulkanDevice;

namespace SystemInfo {

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
float GetVulkanGPUUtilization(const VulkanDevice& device);

std::string GetOpenGLGPUModel();
size_t GetOpenGLVRAMMB();
std::string GetOpenGLDriverVersion();
std::string GetOpenGLAPIVersion();
size_t GetOpenGLMemoryUsageMB();
float GetOpenGLGPUUtilization();

PerformanceLogger::SystemInfo BuildSystemInfo(GraphicsAPI api, const Window& window,
                                              const void* devicePtr = nullptr);
} // namespace SystemInfo
