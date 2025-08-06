#pragma once

#include "GraphicsAPI.hpp"
#include <cstdint>
#include <string>
#include <functional>

// Forward declaration for GLFW stuff
struct GLFWwindow;

struct WindowDesc {
   std::string title = "Graphics Engine";
   uint32_t width = 1280;
   uint32_t height = 720;
   bool vsync = true;
   bool resizable = true;
};

class Window {
public:
   Window(const WindowDesc& desc, GraphicsAPI api);
   ~Window();

   Window(const Window&) = delete;
   Window& operator=(const Window&) = delete;

   bool ShouldClose() const;
   void PollEvents();

   // Event handling
   void OnFramebufferResize(int width, int height);
   void SetResizeCallback(std::function<void(int, int)> callback);

   GLFWwindow* GetNativeWindow() const;
   uint32_t GetWidth() const;
   uint32_t GetHeight() const;
   GraphicsAPI GetAPI() const;
private:
   GraphicsAPI m_api;
   GLFWwindow* m_window = nullptr;
   uint32_t m_width = 0;
   uint32_t m_height = 0;
   std::function<void(int, int)> m_resizeCallback;
};

