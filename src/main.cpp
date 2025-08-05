#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <print>

#include "core/GraphicsAPI.hpp"
#include "core/Window.hpp"
#include "gl/GLRenderer.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
   glad_glViewport(0, 0, width, height);
}

int main() {
   Window window({
      "A window",
      900,
      900,
      0,
      false
   }, GraphicsAPI::OpenGL);
   GLRenderer renderer;
   glfwMakeContextCurrent(window.GetNativeWindow());
   renderer.Init(window.GetNativeWindow());
   glad_glViewport(0, 0, 800, 600);
   glfwSetFramebufferSizeCallback(window.GetNativeWindow(), framebuffer_size_callback);

   while (!glfwWindowShouldClose(window.GetNativeWindow())) {
      if (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
         glfwSetWindowShouldClose(window.GetNativeWindow(), true);
      }

      glad_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glad_glClear(GL_COLOR_BUFFER_BIT);

      renderer.RenderFrame();

      glfwSwapBuffers(window.GetNativeWindow());
      glfwPollEvents();
   }
   renderer.Cleanup();
   return EXIT_SUCCESS;
}

