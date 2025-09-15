#pragma once

#include <chrono>
#include <array>

class ResourceManager;
class Scene;

class PerformanceGUI final {
  public:
   PerformanceGUI() = delete;
   PerformanceGUI(const PerformanceGUI&) = delete;
   PerformanceGUI& operator=(const PerformanceGUI&) = delete;
   PerformanceGUI(PerformanceGUI&&) = delete;
   PerformanceGUI& operator=(PerformanceGUI&&) = delete;
   ~PerformanceGUI() = delete;

   static void RenderPerformanceGUI(const ResourceManager& resourceManager,
                                    const Scene& scene) noexcept;

  private:
   static constexpr size_t kFrameTimeHistorySize{120};
   static constexpr float kTargetFPS{60.0f};
   static constexpr float kMemoryMBDivisor{1024.0f * 1024.0f};

   struct PerformanceMetrics {
      std::array<float, kFrameTimeHistorySize> frameTimeHistory{};
      size_t frameTimeIndex{0};
      float averageFrameTime{0.0f};
      float minFrameTime{999.0f};
      float maxFrameTime{0.0f};
      std::chrono::steady_clock::time_point lastUpdateTime{std::chrono::steady_clock::now()};
   };

   static PerformanceMetrics s_metrics;

  private:
   [[nodiscard]] static constexpr float CalculateMemoryUsageMB(const size_t memoryUsage) noexcept;
   static void UpdateFrameTimeMetrics(const float currentFrameTime) noexcept;
   static void DrawPerformanceGraph() noexcept;
   static void DrawMemoryInfo(const ResourceManager& resourceManager) noexcept;
   static void DrawSceneInfo(const Scene& scene) noexcept;
};
