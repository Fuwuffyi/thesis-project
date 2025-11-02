#pragma once

#include "core/system/PerformanceMetrics.hpp"

class ResourceManager;
class Scene;

class PerformanceGUI final {
  public:
   PerformanceGUI() = delete;
   static void RenderPerformanceGUI(const ResourceManager& resourceManager, const Scene& scene,
                                    const PerformanceMetrics& currentMetrics) noexcept;
   static void ResetStats() noexcept;

  private:
   static constexpr float kTargetFPS{60.0f};
   static constexpr float kMemoryMBDivisor{1024.0f * 1024.0f};
   static constexpr float kGoodFPSThreshold = 55.0f;
   static constexpr float kWarningFPSThreshold = 30.0f;
   static constexpr float kHighMemoryThresholdMB = 2048.0f;
   static constexpr float kCriticalMemoryThresholdMB = 4096.0f;

   static PerformanceStatistics s_stats;
   static FrameTimeHistory<120> s_history;

   [[nodiscard]] static constexpr float CalculateMemoryUsageMB(const size_t memoryUsage) noexcept;
   static void DrawMemoryInfo(const ResourceManager& resourceManager,
                              const PerformanceMetrics& metrics) noexcept;
   static void DrawSceneInfo(const Scene& scene) noexcept;
   static void DrawPerformanceGraph() noexcept;
   static void DrawRenderPassTimings(const PerformanceMetrics& metrics) noexcept;
};
