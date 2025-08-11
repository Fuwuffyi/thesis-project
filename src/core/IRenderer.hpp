#pragma once

// Forward declaration for Window stuff

class Window;
class Camera;

class IRenderer {
public:
   virtual ~IRenderer() = default;

   virtual void RenderFrame() = 0;
   void SetActiveCamera(Camera* cam);

   IRenderer(const IRenderer&) = delete;
   IRenderer& operator=(const IRenderer&) = delete;
protected:
   IRenderer(Window* window);
   Window* m_window;
   Camera* m_activeCamera;
};

