#include <GLFW/glfw3.h>
#include <print>

#include "core/GraphicsAPI.hpp"
#include "core/Window.hpp"
#include "core/RendererFactory.hpp"

int main() {
   try {
      constexpr GraphicsAPI api = GraphicsAPI::Vulkan;
      // Create the window
      WindowDesc windowDesc{
         .title = "Graphics Engine",
         .width = 900,
         .height = 900,
         .vsync = false,
         .resizable = true
      };

      Window window(windowDesc, api);

      // Create the renderer
      std::unique_ptr<IRenderer> renderer = RendererFactory::CreateRenderer(api, window);

      // Main loop
      while (!window.ShouldClose()) {
         window.PollEvents();
         if (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window.GetNativeWindow(), true);
         }
         renderer->RenderFrame();
      }

      // Clean up resources
   } catch (const std::exception& err) {
      std::println("Error: {}", err.what());
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}

