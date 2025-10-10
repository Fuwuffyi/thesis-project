#include "core/editor/PerformanceGUI.hpp"

#include "core/resource/ResourceManager.hpp"
#include "core/scene/Scene.hpp"

#include <algorithm>
#include <numeric>
#include <ranges>

#include <imgui.h>

// Window configuration
constexpr ImVec2 kWindowPosition{5.0f, 5.0f};

constexpr ImGuiWindowFlags kOverlayWindowFlags =
   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
   ImGuiWindowFlags_NoMove;

// Colors for performance indicators
constexpr ImVec4 kGoodPerformanceColor{0.0f, 1.0f, 0.0f, 1.0f};
constexpr ImVec4 kWarningPerformanceColor{1.0f, 1.0f, 0.0f, 1.0f};
constexpr ImVec4 kBadPerformanceColor{1.0f, 0.0f, 0.0f, 1.0f};

// Performance thresholds
constexpr float kGoodFPSThreshold = 55.0f;
constexpr float kWarningFPSThreshold = 30.0f;
constexpr float kHighMemoryThresholdMB = 2048.0f;
constexpr float kCriticalMemoryThresholdMB = 4096.0f;

// Graph configuration
constexpr float kGraphHeight = 80.0f;
constexpr ImVec2 kGraphSize{0.0f, kGraphHeight};

PerformanceGUI::PerformanceMetrics PerformanceGUI::s_metrics{};

void PerformanceGUI::RenderPerformanceGUI(const ResourceManager& resourceManager,
                                          const Scene& scene) noexcept {
   ImGui::SetNextWindowPos(kWindowPosition, ImGuiCond_Always);
   if (ImGui::Begin("Performance Overlay", nullptr, kOverlayWindowFlags)) {
      const ImGuiIO& io = ImGui::GetIO();
      const float framerate = io.Framerate;
      const float frameTime = 1000.0f / std::max(framerate, 0.001f); // Avoid division by zero
      // Update performance metrics
      UpdateFrameTimeMetrics(frameTime);
      // Display FPS with color coding
      const ImVec4 fpsColor = framerate >= kGoodFPSThreshold      ? kGoodPerformanceColor
                              : framerate >= kWarningFPSThreshold ? kWarningPerformanceColor
                                                                  : kBadPerformanceColor;
      ImGui::TextColored(fpsColor, "FPS: %.1f", framerate);
      ImGui::Text("Frame: %.3f ms (avg: %.3f ms)", frameTime, s_metrics.averageFrameTime);
      ImGui::Text("Min/Max: %.3f/%.3f ms", s_metrics.minFrameTime, s_metrics.maxFrameTime);
      DrawMemoryInfo(resourceManager);
      DrawSceneInfo(scene);
      // Performance graph
      if (ImGui::CollapsingHeader("Performance Graph")) {
         DrawPerformanceGraph();
      }
      // Reset stats button
      if (ImGui::Button("Reset Stats")) {
         s_metrics = PerformanceMetrics{};
      }
   }
   ImGui::End();
}

constexpr float PerformanceGUI::CalculateMemoryUsageMB(const size_t memoryUsage) noexcept {
   return static_cast<float>(memoryUsage) / kMemoryMBDivisor;
}

void PerformanceGUI::UpdateFrameTimeMetrics(const float currentFrameTime) noexcept {
   // Update frame time history
   s_metrics.frameTimeHistory[s_metrics.frameTimeIndex] = currentFrameTime;
   s_metrics.frameTimeIndex = (s_metrics.frameTimeIndex + 1) % kFrameTimeHistorySize;
   // Calculate statistics only every few frames to reduce overhead
   const auto currentTime = std::chrono::steady_clock::now();
   const auto timeSinceUpdate =
      std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - s_metrics.lastUpdateTime);
   if (timeSinceUpdate.count() > 100) { // Update every 100ms
      const auto validFrameTimes = std::span{s_metrics.frameTimeHistory};
      // Calculate average (excluding zeros for incomplete history)
      auto nonZeroFrameTimes =
         validFrameTimes | std::views::filter([](float ft) { return ft > 0.0f; });
      if (!std::ranges::empty(nonZeroFrameTimes)) {
         const size_t count = std::ranges::distance(nonZeroFrameTimes);
         const float sum =
            std::accumulate(nonZeroFrameTimes.begin(), nonZeroFrameTimes.end(), 0.0f);
         s_metrics.averageFrameTime = sum / static_cast<float>(count);
         // Update min/max
         const auto [minIt, maxIt] = std::ranges::minmax_element(nonZeroFrameTimes);
         s_metrics.minFrameTime = *minIt;
         s_metrics.maxFrameTime = *maxIt;
      }
      s_metrics.lastUpdateTime = currentTime;
   }
}

void PerformanceGUI::DrawPerformanceGraph() noexcept {
   // Create a view of valid frame times (non-zero)
   auto validFrameTimes =
      s_metrics.frameTimeHistory | std::views::filter([](float ft) { return ft > 0.0f; });
   if (std::ranges::empty(validFrameTimes)) {
      ImGui::Text("No frame time data available");
      return;
   }
   // Convert to vector
   std::vector<float> frameTimesVec;
   frameTimesVec.reserve(kFrameTimeHistorySize);
   std::ranges::copy(validFrameTimes, std::back_inserter(frameTimesVec));
   if (!frameTimesVec.empty()) {
      const auto [minTime, maxTime] = std::ranges::minmax_element(frameTimesVec);
      const float scaleMin = std::max(0.0f, *minTime - 1.0f);
      const float scaleMax = *maxTime + 1.0f;
      const std::string overlayText = std::format("Frame Time: {:.2f}ms", frameTimesVec.back());
      ImGui::PlotLines("Frame Times", frameTimesVec.data(), static_cast<int>(frameTimesVec.size()),
                       0, overlayText.c_str(), scaleMin, scaleMax, kGraphSize);
      // Add target frame time line indicator
      const float targetFrameTime = 1000.0f / kTargetFPS;
      ImGui::Text("Target: %.2f ms (%.1f FPS)", targetFrameTime, kTargetFPS);
   }
}

void PerformanceGUI::DrawMemoryInfo(const ResourceManager& resourceManager) noexcept {
   const float memoryUsageMB = CalculateMemoryUsageMB(resourceManager.GetTotalMemoryUsage());
   const size_t resourceCount = resourceManager.GetResourceCount();
   // Color code memory usage
   const ImVec4 memoryColor = memoryUsageMB < kHighMemoryThresholdMB ? kGoodPerformanceColor
                              : memoryUsageMB < kCriticalMemoryThresholdMB
                                 ? kWarningPerformanceColor
                                 : kBadPerformanceColor;

   ImGui::TextColored(memoryColor, "Resource MEM: %.3f MB", memoryUsageMB);
   ImGui::Text("Resource Count: %zu", resourceCount);
   // Memory usage bar
   if (memoryUsageMB > 0.0f) {
      const float memoryProgress = std::min(memoryUsageMB / kCriticalMemoryThresholdMB, 1.0f);
      ImGui::ProgressBar(memoryProgress, ImVec2(-1.0f, 0.0f),
                         std::format("{:.1f} MB", memoryUsageMB).c_str());
   }
}

void PerformanceGUI::DrawSceneInfo(const Scene& scene) noexcept {
   const size_t nodeCount = scene.GetNodeCount();
   ImGui::Text("Scene Nodes: %zu", nodeCount);
}
