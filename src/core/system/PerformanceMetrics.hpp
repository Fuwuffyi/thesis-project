#pragma once

#include <chrono>
#include <string>
#include <array>

struct PerformanceMetrics {
   // Frame timing
   float frameTimeMs{0.0f};
   float cpuTimeMs{0.0f};
   float gpuTimeMs{0.0f};
   // Render pass timings
   float geometryPassMs{0.0f};
   float lightingPassMs{0.0f};
   float gizmoPassMs{0.0f};
   float particlePassMs{0.0f};
   float imguiPassMs{0.0f};
   // Memory usage
   size_t vramUsageMB{0};
   size_t systemMemUsageMB{0};
   // Utilization
   float gpuUtilization{0.0f};
   float cpuUtilization{0.0f};

   [[nodiscard]] float GetFPS() const noexcept;
   [[nodiscard]] float GetTotalRenderPassTime() const noexcept;
};

// System information structure
struct SystemInfo {
   std::string cpuModel;
   uint32_t threadCount{0};
   float clockSpeedGHz{0.0f};
   std::string gpuModel;
   size_t vramMB{0};
   std::string driverVersion;
   std::string apiVersion;
   uint32_t windowWidth{0};
   uint32_t windowHeight{0};
};

// Statistical tracking for performance metrics
struct PerformanceStatistics {
   float avgFPS{0.0f};
   float minFPS{999999.0f};
   float maxFPS{0.0f};
   float avgFrameTime{0.0f};
   float minFrameTime{999999.0f};
   float maxFrameTime{0.0f};
   uint64_t totalFrames{0};
   float totalRunTimeSeconds{0.0f};
   // Per-pass averages
   float avgGeometryPassMs{0.0f};
   float avgLightingPassMs{0.0f};
   float avgGizmoPassMs{0.0f};
   float avgParticlePassMs{0.0f};
   float avgImguiPassMs{0.0f};

   void Update(const PerformanceMetrics& metrics) noexcept;
   void Reset() noexcept;
};

// Frame time history for GUI graphing
template <size_t Size = 120>
struct FrameTimeHistory {
   std::array<float, Size> frameTimeHistory{};
   size_t frameTimeIndex{0};
   std::chrono::steady_clock::time_point lastUpdateTime{std::chrono::steady_clock::now()};

   void AddSample(float frameTime) noexcept {
      frameTimeHistory[frameTimeIndex] = frameTime;
      frameTimeIndex = (frameTimeIndex + 1) % Size;
   }

   [[nodiscard]] const std::array<float, Size>& GetHistory() const noexcept {
      return frameTimeHistory;
   }
};
