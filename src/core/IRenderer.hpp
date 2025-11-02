#pragma once

#include "core/system/PerformanceMetrics.hpp"

class Window;
class Camera;
class Scene;
class ResourceManager;

class IRenderer {
  public:
   virtual ~IRenderer() = default;

   IRenderer(const IRenderer&) = delete;
   IRenderer& operator=(const IRenderer&) = delete;
   IRenderer(IRenderer&&) noexcept = delete;
   IRenderer& operator=(IRenderer&&) noexcept = delete;

   virtual void RenderFrame() = 0;

   void SetActiveCamera(Camera* cam) noexcept;
   void SetActiveScene(Scene* scene) noexcept;

   [[nodiscard]] virtual ResourceManager* GetResourceManager() const noexcept = 0;

   [[nodiscard]] constexpr const PerformanceMetrics& GetCurrentFrameMetrics() const noexcept {
      return m_currentFrameMetrics;
   }

  protected:
   explicit IRenderer(Window* window) noexcept;
   virtual void SetupImgui() = 0;
   virtual void RenderImgui() = 0;
   virtual void DestroyImgui() = 0;

  protected:
   Window* m_window{nullptr};
   Camera* m_activeCamera{nullptr};
   Scene* m_activeScene{nullptr};
   PerformanceMetrics m_currentFrameMetrics;
};
