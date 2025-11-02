#include "core/system/PerformanceMetrics.hpp"

[[nodiscard]] float PerformanceMetrics::GetFPS() const noexcept {
   return frameTimeMs > 0.0f ? 1000.0f / frameTimeMs : 0.0f;
}

[[nodiscard]] float PerformanceMetrics::GetTotalRenderPassTime() const noexcept {
   return geometryPassMs + lightingPassMs + gizmoPassMs + particlePassMs + imguiPassMs;
}

void PerformanceStatistics::Update(const PerformanceMetrics& metrics) noexcept {
   totalFrames++;
   const float fps = metrics.GetFPS();
   if (fps < minFPS && fps > 0.0f)
      minFPS = fps;
   if (fps > maxFPS)
      maxFPS = fps;
   if (metrics.frameTimeMs < minFrameTime && metrics.frameTimeMs > 0.0f)
      minFrameTime = metrics.frameTimeMs;
   if (metrics.frameTimeMs > maxFrameTime)
      maxFrameTime = metrics.frameTimeMs;
   // Running average
   const float alpha = 1.0f / static_cast<float>(totalFrames);
   avgFPS = avgFPS * (1.0f - alpha) + fps * alpha;
   avgFrameTime = avgFrameTime * (1.0f - alpha) + metrics.frameTimeMs * alpha;
   avgGeometryPassMs = avgGeometryPassMs * (1.0f - alpha) + metrics.geometryPassMs * alpha;
   avgLightingPassMs = avgLightingPassMs * (1.0f - alpha) + metrics.lightingPassMs * alpha;
   avgGizmoPassMs = avgGizmoPassMs * (1.0f - alpha) + metrics.gizmoPassMs * alpha;
   avgParticlePassMs = avgParticlePassMs * (1.0f - alpha) + metrics.particlePassMs * alpha;
   avgImguiPassMs = avgImguiPassMs * (1.0f - alpha) + metrics.imguiPassMs * alpha;
}

void PerformanceStatistics::Reset() noexcept { *this = PerformanceStatistics{}; }
