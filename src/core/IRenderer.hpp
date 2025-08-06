#pragma once

// Forward declaration for GLFW stuff
struct GLFWwindow;

class IRenderer {
public:
   virtual void RenderFrame() = 0;
   virtual ~IRenderer() = default;

   IRenderer(const IRenderer&) = delete;
   IRenderer& operator=(const IRenderer&) = delete;
protected:
   IRenderer(GLFWwindow* windowHandle);
   GLFWwindow* m_windowHandle = nullptr;
};

