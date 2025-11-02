#include "core/editor/PerformanceGUI.hpp"

#include "core/resource/ResourceManager.hpp"
#include "core/scene/Scene.hpp"

#include <imgui.h>
#include <algorithm>
#include <ranges>
#include <format>

constexpr ImVec2 kWindowPosition{5.0f, 5.0f};
constexpr ImGuiWindowFlags kOverlayWindowFlags =
   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
   ImGuiWindowFlags_NoMove;

constexpr ImVec4 kGoodPerformanceColor{0.0f, 1.0f, 0.0f, 1.0f};
constexpr ImVec4 kWarningPerformanceColor{1.0f, 1.0f, 0.0f, 1.0f};
constexpr ImVec4 kBadPerformanceColor{1.0f, 0.0f, 0.0f, 1.0f};
constexpr float kGraphHeight = 80.0f;
constexpr ImVec2 kGraphSize{0.0f, kGraphHeight};

PerformanceStatistics PerformanceGUI::s_stats{};
FrameTimeHistory<1024> PerformanceGUI::s_history{};

void PerformanceGUI::RenderPerformanceGUI(const ResourceManager& resourceManager,
                                          const Scene& scene,
                                          const PerformanceMetrics& currentMetrics) noexcept {
   ImGui::SetNextWindowPos(kWindowPosition, ImGuiCond_Always);
   if (ImGui::Begin("Performance Overlay", nullptr, kOverlayWindowFlags)) {
      // Update statistics
      s_stats.Update(currentMetrics);
      s_history.AddSample(currentMetrics.frameTimeMs);
      const float fps = currentMetrics.GetFPS();
      // Display FPS with color coding
      const ImVec4 fpsColor = fps >= kGoodFPSThreshold      ? kGoodPerformanceColor
                              : fps >= kWarningFPSThreshold ? kWarningPerformanceColor
                                                            : kBadPerformanceColor;
      ImGui::TextColored(fpsColor, "FPS: %.1f (avg: %.1f)", fps, s_stats.avgFPS);
      ImGui::Text("Frame: %.3f ms (avg: %.3f ms)", currentMetrics.frameTimeMs,
                  s_stats.avgFrameTime);
      ImGui::Text("Min/Max: %.3f/%.3f ms", s_stats.minFrameTime, s_stats.maxFrameTime);
      ImGui::Separator();
      // CPU/GPU breakdown
      ImGui::Text("CPU: %.3f ms (%.1f%%)", currentMetrics.cpuTimeMs, currentMetrics.cpuUtilization);
      ImGui::Text("GPU: %.3f ms", currentMetrics.gpuTimeMs);
      ImGui::Separator();
      DrawMemoryInfo(resourceManager, currentMetrics);
      DrawSceneInfo(scene);
      // Performance graph
      if (ImGui::CollapsingHeader("Performance Graph")) {
         DrawPerformanceGraph();
      }
      // Render pass timings
      if (ImGui::CollapsingHeader("Render Pass Timings")) {
         DrawRenderPassTimings(currentMetrics);
      }
      // Reset stats button
      if (ImGui::Button("Reset Stats")) {
         ResetStats();
      }
   }
   ImGui::End();
}

void PerformanceGUI::ResetStats() noexcept {
   s_stats.Reset();
   s_history = FrameTimeHistory<1024>{};
}

constexpr float PerformanceGUI::CalculateMemoryUsageMB(const size_t memoryUsage) noexcept {
   return static_cast<float>(memoryUsage) / kMemoryMBDivisor;
}

void PerformanceGUI::DrawPerformanceGraph() noexcept {
   const auto& history = s_history.GetHistory();
   auto validFrameTimes = history | std::views::filter([](float ft) { return ft > 0.0f; });
   if (std::ranges::empty(validFrameTimes)) {
      ImGui::Text("No frame time data available");
      return;
   }
   std::vector<float> frameTimesVec;
   frameTimesVec.reserve(120);
   std::ranges::copy(validFrameTimes, std::back_inserter(frameTimesVec));
   if (!frameTimesVec.empty()) {
      const auto [minTime, maxTime] = std::ranges::minmax_element(frameTimesVec);
      const float scaleMin = std::max(0.0f, *minTime - 1.0f);
      const float scaleMax = *maxTime + 1.0f;
      const std::string overlayText = std::format("Frame Time: {:.2f}ms", frameTimesVec.back());
      ImGui::PlotLines("Frame Times", frameTimesVec.data(), static_cast<int>(frameTimesVec.size()),
                       0, overlayText.c_str(), scaleMin, scaleMax, kGraphSize);
      const float targetFrameTime = 1000.0f / kTargetFPS;
      ImGui::Text("Target: %.2f ms (%.1f FPS)", targetFrameTime, kTargetFPS);
   }
}

void PerformanceGUI::DrawMemoryInfo(const ResourceManager& resourceManager,
                                    const PerformanceMetrics& metrics) noexcept {
   const float resourceMemMB = CalculateMemoryUsageMB(resourceManager.GetTotalMemoryUsage());
   const size_t resourceCount = resourceManager.GetResourceCount();
   const ImVec4 memoryColor = metrics.vramUsageMB < kHighMemoryThresholdMB ? kGoodPerformanceColor
                              : metrics.vramUsageMB < kCriticalMemoryThresholdMB
                                 ? kWarningPerformanceColor
                                 : kBadPerformanceColor;
   ImGui::TextColored(memoryColor, "VRAM: %zu MB", metrics.vramUsageMB);
   ImGui::Text("System RAM: %zu MB", metrics.systemMemUsageMB);
   ImGui::Text("Resource MEM: %.3f MB", resourceMemMB);
   ImGui::Text("Resource Count: %zu", resourceCount);
   // Memory usage bar
   if (metrics.vramUsageMB > 0) {
      const float memoryProgress =
         std::min(static_cast<float>(metrics.vramUsageMB) / kCriticalMemoryThresholdMB, 1.0f);
      ImGui::ProgressBar(memoryProgress, ImVec2(-1.0f, 0.0f),
                         std::format("{} MB", metrics.vramUsageMB).c_str());
   }
}

void PerformanceGUI::DrawSceneInfo(const Scene& scene) noexcept {
   const size_t nodeCount = scene.GetNodeCount();
   ImGui::Text("Scene Nodes: %zu", nodeCount);
}

void PerformanceGUI::DrawRenderPassTimings(const PerformanceMetrics& metrics) noexcept {
   ImGui::Text("Geometry:  %.3f ms (%.1f%%)", metrics.geometryPassMs,
               (metrics.geometryPassMs / metrics.frameTimeMs) * 100.0f);
   ImGui::Text("Lighting:  %.3f ms (%.1f%%)", metrics.lightingPassMs,
               (metrics.lightingPassMs / metrics.frameTimeMs) * 100.0f);
   ImGui::Text("Gizmos:    %.3f ms (%.1f%%)", metrics.gizmoPassMs,
               (metrics.gizmoPassMs / metrics.frameTimeMs) * 100.0f);
   ImGui::Text("Particles: %.3f ms (%.1f%%)", metrics.particlePassMs,
               (metrics.particlePassMs / metrics.frameTimeMs) * 100.0f);
   ImGui::Text("ImGui:     %.3f ms (%.1f%%)", metrics.imguiPassMs,
               (metrics.imguiPassMs / metrics.frameTimeMs) * 100.0f);
   const float totalPassTime = metrics.GetTotalRenderPassTime();
   const float overhead = metrics.frameTimeMs - totalPassTime;
   ImGui::Separator();
   ImGui::Text("Total:     %.3f ms", totalPassTime);
   ImGui::Text("Overhead:  %.3f ms (%.1f%%)", overhead, (overhead / metrics.frameTimeMs) * 100.0f);
   // Render pass timing bars
   ImGui::Spacing();
   const float passWidth = ImGui::GetContentRegionAvail().x;
   auto drawPassBar = [&](const char* label, float timeMs, ImVec4 color) {
      const float fraction = timeMs / metrics.frameTimeMs;
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
      ImGui::ProgressBar(fraction, ImVec2(passWidth, 0.0f),
                         std::format("{}: {:.2f}ms", label, timeMs).c_str());
      ImGui::PopStyleColor();
   };
   drawPassBar("Geometry", metrics.geometryPassMs, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
   drawPassBar("Lighting", metrics.lightingPassMs, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
   drawPassBar("Gizmos", metrics.gizmoPassMs, ImVec4(0.3f, 0.3f, 1.0f, 1.0f));
   drawPassBar("Particles", metrics.particlePassMs, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));
   drawPassBar("ImGui", metrics.imguiPassMs, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
}
