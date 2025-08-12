#include "Window.hpp"
#include "core/GraphicsAPI.hpp"

#include <GLFW/glfw3.h>
#include <print>

Window::Window(const GraphicsAPI api, const WindowDesc& desc)
   :
   m_api(api)
{
   // Set error callback
   glfwSetErrorCallback(Window::ErrorCallback);
   // Initialie GLFW
   if (!glfwInit()) {
      std::println("GLFW initialization failed");
      throw std::runtime_error("GLFW init failed");
   }
   // Setup window flags based on API
   if (api == GraphicsAPI::OpenGL) {
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
      glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
   } else if (api == GraphicsAPI::Vulkan) {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   } else {
      glfwTerminate();
      throw std::runtime_error("Unsupported Graphics API");
   }
   glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);

   // Create the glfw window
   m_window = glfwCreateWindow(
      static_cast<int>(desc.width),
      static_cast<int>(desc.height),
      desc.title.c_str(),
      nullptr,
      nullptr
   );
   if (!m_window) {
      glfwTerminate();
      throw std::runtime_error("Failed to create window");
   }
   m_width = desc.width;
   m_height = desc.height;
   // Set main window if GL
   if (api == GraphicsAPI::OpenGL) {
      glfwMakeContextCurrent(m_window);
   }
   // Setup callback configuration
   glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
   glfwSetWindowUserPointer(m_window, this);
   glfwSetFramebufferSizeCallback(m_window, Window::FramebufferSizeCallback);
   glfwSetKeyCallback(m_window, Window::KeyCallback);
   glfwSetMouseButtonCallback(m_window, Window::MouseButtonCallback);
   glfwSetCursorPosCallback(m_window, Window::CursorPosCallback);
   glfwSetWindowSizeCallback(m_window, Window::ResizeCallback);
}

Window::~Window() {
   if (m_window) {
      glfwDestroyWindow(m_window);
      m_window = nullptr;
   }
   glfwTerminate();
}

bool Window::ShouldClose() const {
   return m_window && glfwWindowShouldClose(m_window);
}

void Window::PollEvents() {
   glfwPollEvents();
   m_eventSystem.ProcessHeldEvents();
}

void Window::OnFramebufferResize(int32_t width, int32_t height) {
   m_width = static_cast<uint32_t>(width);
   m_height = static_cast<uint32_t>(height);
   if (m_resizeCallback) {
      m_resizeCallback(width, height);
   }
}

void Window::SetResizeCallback(std::function<void(int, int)> callback) {
   m_resizeCallback = std::move(callback);
}

void Window::SetCursorVisible(const bool visible) const {
   glfwSetInputMode(m_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
   Window* windowPtr = static_cast<Window*>(glfwGetWindowUserPointer(window));
   if (windowPtr) {
      windowPtr->OnFramebufferResize(width, height);
   }
}

void Window::ErrorCallback(int error, const char* description) {
   std::println("GLFW Error {}: {}", error, description);
}

void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
   Window* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
   w->m_eventSystem.HandleKeyEvent(static_cast<uint32_t>(key), static_cast<uint32_t>(scancode),
                                   static_cast<uint32_t>(action), static_cast<uint32_t>(mods));
}

void Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
   Window* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
   w->m_eventSystem.HandleMouseEvent(static_cast<uint32_t>(button), static_cast<uint32_t>(action),
                                     static_cast<uint32_t>(mods));
}

void Window::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
   Window* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
   w->m_eventSystem.HandleCursorPos(static_cast<float>(xpos), static_cast<float>(ypos));
}

void Window::ResizeCallback(GLFWwindow* window, int width, int height) {
   Window* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
   w->m_eventSystem.HandleResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

GLFWwindow* Window::GetNativeWindow() const {
   return m_window;
}

EventSystem* Window::GetEventSystem() {
   return &m_eventSystem;
}

uint32_t Window::GetWidth() const {
   return m_width;
}

uint32_t Window::GetHeight() const {
   return m_height;
}

GraphicsAPI Window::GetAPI() const {
   return m_api;
}

