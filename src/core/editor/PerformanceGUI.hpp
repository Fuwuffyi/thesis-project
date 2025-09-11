#pragma once

class ResourceManager;
class Scene;

class PerformanceGUI final {
  public:
   static void RenderPeformanceGUI(const ResourceManager& resourceManager, const Scene& scene);
};
