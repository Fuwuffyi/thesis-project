#include <GLFW/glfw3.h>
#include <print>

#include "core/RendererFactory.hpp"
#include "core/Window.hpp"
#include "core/Camera.hpp"

int main() {
   try {
      constexpr GraphicsAPI api = GraphicsAPI::Vulkan;

      // Create the window
      WindowDesc windowDesc{
         .title = "Deferred Rendering Engine",
         .width = 900,
         .height = 900,
         .vsync = false,
         .resizable = true
      };
      Window window(api, windowDesc);
      window.SetCursorVisible(false);

      // Create the camera
      const glm::vec3 startPos = glm::vec3(2.0f);
      const glm::vec3 forward = glm::normalize(glm::vec3(0.0f) - startPos);
      const glm::quat orientation = glm::quatLookAt(forward, glm::vec3(0.0f, 1.0f, 0.0f));
      Transform camTransform(startPos, orientation);
      Camera cam(camTransform, glm::vec3(0.0f, 1.0f, 0.0f), 90.0f,
                 1.0f, 0.01f, 100.0f, api);

      // Create the renderer
      std::unique_ptr<IRenderer> renderer = RendererFactory::CreateRenderer(api, &window);
      renderer->SetActiveCamera(&cam);

      // Setup events
      EventSystem* events = window.GetEventSystem();

      // ALT toggles cursor
      events->OnKeyDown(GLFW_KEY_LEFT_ALT, [&](const uint32_t, const uint32_t, const uint32_t) {
         window.SetCursorVisible(true);
      });
      events->OnKeyUp(GLFW_KEY_LEFT_ALT, [&](const uint32_t, const uint32_t, const uint32_t) {
         window.SetCursorVisible(false);
      });

      // ESC to close window
      events->OnKeyDown(GLFW_KEY_ESCAPE, [&](const uint32_t, const uint32_t, const uint32_t) {
         glfwSetWindowShouldClose(window.GetNativeWindow(), true);
      });

      // Main loop
      while (!window.ShouldClose()) {
         window.PollEvents();
         renderer->RenderFrame();
      }
   } catch (const std::exception& err) {
      std::println("Error: {}", err.what());
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}

