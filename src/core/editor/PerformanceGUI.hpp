#pragma once

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

   static void RenderPeformanceGUI(const ResourceManager& resourceManager,
                                   const Scene& scene) noexcept;
};
