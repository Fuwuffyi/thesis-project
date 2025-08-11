#pragma once

// Forward declaration for Window stuff

class Window;
class Camera;

class IRenderer {
public:
   virtual void RenderFrame(Camera& cam) = 0;
   virtual ~IRenderer() = default;

   IRenderer(const IRenderer&) = delete;
   IRenderer& operator=(const IRenderer&) = delete;
protected:
   IRenderer(Window* window);
   Window* m_window;
};

