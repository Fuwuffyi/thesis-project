#pragma once

#include "GraphicsAPI.hpp"
#include "EventSystem.hpp"

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
   Window(const GraphicsAPI api, const WindowDesc& desc);
   ~Window();

   Window(const Window&) = delete;
   Window& operator=(const Window&) = delete;

   bool ShouldClose() const;
   void PollEvents();

   // Event handling
   void OnFramebufferResize(int width, int height);
   void SetResizeCallback(std::function<void(int, int)> callback);

   void SetCursorVisible(const bool visible) const;

   GLFWwindow* GetNativeWindow() const;
   EventSystem* GetEventSystem();
   uint32_t GetWidth() const;
   uint32_t GetHeight() const;
   GraphicsAPI GetAPI() const;

  private:
   static void ErrorCallback(int error, const char* description);
   static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
   static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
   static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
   static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
   static void ResizeCallback(GLFWwindow* window, int width, int height);

  private:
   GraphicsAPI m_api;
   GLFWwindow* m_window = nullptr;
   uint32_t m_width = 0;
   uint32_t m_height = 0;
   std::function<void(int, int)> m_resizeCallback;
   EventSystem m_eventSystem;
};
