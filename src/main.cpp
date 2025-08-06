#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <print>

#include "core/GraphicsAPI.hpp"
#include "core/Window.hpp"
#include "core/RendererFactory.hpp"

int main() {
   try {
      constexpr GraphicsAPI api = GraphicsAPI::OpenGL;
      WindowDesc windowDesc{
         .title = "Graphics Engine",
         .width = 900,
         .height = 900,
         .vsync = true,
         .resizable = true
      };

      Window window(windowDesc, GraphicsAPI::OpenGL);

      std::unique_ptr<IRenderer> renderer = RendererFactory::CreateRenderer(api);
      renderer->Init(window.GetNativeWindow());

      while (!window.ShouldClose()) {
         window.PollEvents();
         if (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window.GetNativeWindow(), true);
         }

         glad_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
         glad_glClear(GL_COLOR_BUFFER_BIT);
         renderer->RenderFrame();
      }
      renderer->Cleanup();
   } catch (const std::exception& err) {
      std::println("Error: {}", err.what());
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}

