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
         .title = "Graphics Engine",
         .width = 900,
         .height = 900,
         .vsync = false,
         .resizable = true
      };
      Window window(windowDesc, api);

      // Create the camera
      Transform camTransform(glm::vec3(2.0f, 2.0f, 2.0f));
      const glm::vec3 forward = glm::normalize(glm::vec3(0.0f) - camTransform.GetPosition());
      const glm::quat orientation = glm::quatLookAt(forward, glm::vec3(0.0f, 1.0f, 0.0f));
      camTransform.SetRotation(orientation);
      Camera cam(camTransform, glm::vec3(0.0f, 1.0f, 0.0f), 90.0f,
                 1.0f, 0.01f, 100.0f, api);

      // Create the renderer
      std::unique_ptr<IRenderer> renderer = RendererFactory::CreateRenderer(api, &window);
      renderer->SetActiveCamera(&cam);

      // Main loop
      while (!window.ShouldClose()) {
         window.PollEvents();
         if (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window.GetNativeWindow(), true);
         }
         if (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_W) == GLFW_PRESS) {
            Transform& t = cam.GetMutableTransform();
            t.SetPosition(t.GetPosition() + glm::vec3(0.001f, 0.0f, 0.0f));
         }
         if (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_A) == GLFW_PRESS) {
            Transform& t = cam.GetMutableTransform();
            t.SetPosition(t.GetPosition() + glm::vec3(0.0f, 0.0f, -0.001f));
         }
         if (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_S) == GLFW_PRESS) {
            Transform& t = cam.GetMutableTransform();
            t.SetPosition(t.GetPosition() + glm::vec3(-0.001f, 0.0f, 0.0f));
         }
         if (glfwGetKey(window.GetNativeWindow(), GLFW_KEY_D) == GLFW_PRESS) {
            Transform& t = cam.GetMutableTransform();
            t.SetPosition(t.GetPosition() + glm::vec3(0.0f, 0.0f, 0.001f));
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

