#include "Window.hpp"
#include <GLFW/glfw3.h>
#include <print>

Window::Window(const WindowDesc& desc, GraphicsAPI api) {
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
   } else if (api == GraphicsAPI::Vulkan) {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   } else {
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
   glfwSetWindowUserPointer(m_window, this);
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
}

GLFWwindow* Window::GetNativeWindow() const {
   return m_window;
}

uint32_t Window::GetWidth() const {
   return m_width;
}

uint32_t Window::GetHeight() const {
   return m_height;
}

