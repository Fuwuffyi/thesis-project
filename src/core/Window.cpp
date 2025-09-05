#include "Window.hpp"
#include "core/GraphicsAPI.hpp"

#include <GLFW/glfw3.h>
#include <print>

Window::Window(const GraphicsAPI api, const WindowDesc& desc) : m_api(api) {
#ifndef NDEBUG
   glfwSetErrorCallback(Window::ErrorCallback);
#endif
   if (!glfwInit()) {
      std::println("GLFW initialization failed");
      throw std::runtime_error("GLFW init failed");
   }
   switch (api) {
      case GraphicsAPI::OpenGL:
         glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
         glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
         glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
         glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
         glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
         break;
      case GraphicsAPI::Vulkan:
         glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
         break;
      default:
         glfwTerminate();
         throw std::runtime_error("Unsupported Graphics API");
   }
   glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);
   m_window = glfwCreateWindow(static_cast<int32_t>(desc.width), static_cast<int32_t>(desc.height),
                               desc.title.c_str(), nullptr, nullptr);
   if (!m_window) {
      glfwTerminate();
      throw std::runtime_error("Failed to create window");
   }
   m_width = desc.width;
   m_height = desc.height;
   if (api == GraphicsAPI::OpenGL) {
      glfwMakeContextCurrent(m_window);
      if (!desc.vsync) {
         glfwSwapInterval(0);
      }
   }
   glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
   glfwSetWindowUserPointer(m_window, this);
   glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);
   glfwSetKeyCallback(m_window, KeyCallback);
   glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
   glfwSetCursorPosCallback(m_window, CursorPosCallback);
   glfwSetWindowSizeCallback(m_window, ResizeCallback);
}

Window::~Window() noexcept {
   if (m_window) {
      glfwDestroyWindow(m_window);
      m_window = nullptr;
   }
   glfwTerminate();
}

bool Window::ShouldClose() const noexcept { return m_window && glfwWindowShouldClose(m_window); }

void Window::SetShouldClose(const bool shouldClose) const {
   glfwSetWindowShouldClose(m_window, shouldClose);
}

void Window::PollEvents() {
   glfwPollEvents();
   m_eventSystem.ProcessHeldEvents();
}

void Window::OnFramebufferResize(const int32_t width, const int32_t height) {
   m_width = static_cast<uint32_t>(width);
   m_height = static_cast<uint32_t>(height);
   if (m_resizeCallback) {
      m_resizeCallback(width, height);
   }
}

void Window::SetResizeCallback(std::function<void(const int32_t, const int32_t)> callback) {
   m_resizeCallback = std::move(callback);
}

void Window::SetCursorVisible(const bool visible) const {
   glfwSetInputMode(m_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, const int32_t width,
                                     const int32_t height) {
   if (auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
      w->OnFramebufferResize(width, height);
   }
}

void Window::ErrorCallback(const int32_t error, const char* description) {
   std::println("GLFW Error {}: {}", error, description);
}

void Window::KeyCallback(GLFWwindow* window, const int32_t key, const int32_t scancode,
                         const int32_t action, const int mods) {
   if (auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
      w->m_eventSystem.HandleKeyEvent(static_cast<uint32_t>(key), static_cast<uint32_t>(scancode),
                                      static_cast<uint32_t>(action), static_cast<uint32_t>(mods));
   }
}

void Window::MouseButtonCallback(GLFWwindow* window, const int32_t button, const int32_t action,
                                 const int32_t mods) {
   if (auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
      w->m_eventSystem.HandleMouseEvent(static_cast<uint32_t>(button),
                                        static_cast<uint32_t>(action), static_cast<uint32_t>(mods));
   }
}

void Window::CursorPosCallback(GLFWwindow* window, const double xpos, const double ypos) {
   if (auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
      w->m_eventSystem.HandleCursorPos(static_cast<float>(xpos), static_cast<float>(ypos));
   }
}

void Window::ResizeCallback(GLFWwindow* window, const int32_t width, const int32_t height) {
   if (auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window))) {
      w->m_eventSystem.HandleResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
   }
}

GLFWwindow* Window::GetNativeWindow() const noexcept { return m_window; }
EventSystem* Window::GetEventSystem() noexcept { return &m_eventSystem; }
uint32_t Window::GetWidth() const noexcept { return m_width; }
uint32_t Window::GetHeight() const noexcept { return m_height; }
GraphicsAPI Window::GetAPI() const noexcept { return m_api; }
