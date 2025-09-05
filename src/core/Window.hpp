#pragma once

#include "core/GraphicsAPI.hpp"
#include "core/EventSystem.hpp"

#include <string>

// Forward declaration for GLFW stuff
struct GLFWwindow;

struct WindowDesc final {
   std::string title = "Default Window";
   uint32_t width = 1280;
   uint32_t height = 720;
   bool vsync = true;
   bool resizable = true;
};

class Window final {
  public:
   explicit Window(const GraphicsAPI api, const WindowDesc& desc);
   ~Window() noexcept;

   Window(const Window&) = delete;
   Window& operator=(const Window&) = delete;
   Window(Window&&) noexcept = delete;
   Window& operator=(Window&&) noexcept = delete;

   [[nodiscard]] bool ShouldClose() const noexcept;
   void SetShouldClose(const bool shouldClose) const;
   void PollEvents();

   void OnFramebufferResize(const int32_t width, const int32_t height);
   void SetResizeCallback(std::function<void(const int32_t, const int32_t)> callback);

   void SetCursorVisible(const bool visible) const;

   [[nodiscard]] GLFWwindow* GetNativeWindow() const noexcept;
   [[nodiscard]] EventSystem* GetEventSystem() noexcept;
   [[nodiscard]] uint32_t GetWidth() const noexcept;
   [[nodiscard]] uint32_t GetHeight() const noexcept;
   [[nodiscard]] GraphicsAPI GetAPI() const noexcept;

  private:
   static void ErrorCallback(const int32_t error, const char* description);
   static void FramebufferSizeCallback(GLFWwindow* window, const int32_t width,
                                       const int32_t height);
   static void KeyCallback(GLFWwindow* window, const int32_t key, const int32_t scancode,
                           const int32_t action, const int32_t mods);
   static void MouseButtonCallback(GLFWwindow* window, const int32_t button, const int32_t action,
                                   const int32_t mods);
   static void CursorPosCallback(GLFWwindow* window, const double xpos, const double ypos);
   static void ResizeCallback(GLFWwindow* window, const int32_t width, const int32_t height);

  private:
   GraphicsAPI m_api{};
   GLFWwindow* m_window{nullptr};
   uint32_t m_width{0};
   uint32_t m_height{0};
   std::function<void(int, int)> m_resizeCallback;
   EventSystem m_eventSystem;
};
