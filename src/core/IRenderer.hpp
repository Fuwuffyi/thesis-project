#pragma once

// Forward declaration for Window stuff

class Window;

class IRenderer {
public:
   virtual void RenderFrame() = 0;
   virtual ~IRenderer() = default;

   IRenderer(const IRenderer&) = delete;
   IRenderer& operator=(const IRenderer&) = delete;
protected:
   IRenderer(Window* window);
   Window* m_window;
};

