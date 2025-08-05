#pragma once

// Forward declaration for GLFW stuff
struct GLFWwindow;

class IRenderer {
public:
   virtual void Init(GLFWwindow* windowHandle) = 0;
   virtual void RenderFrame() = 0;
   virtual void Cleanup() = 0;
   virtual ~IRenderer() = default;
};

