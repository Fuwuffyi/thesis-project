#include "core/RendererFactory.hpp"
#include "core/Window.hpp"
#include "core/Camera.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/Node.hpp"

#include <print>
#include <chrono>

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

      // Create the scene
      Scene scene;

      // Create the camera
      const glm::vec3 startPos = glm::vec3(2.0f);
      const glm::vec3 forward = glm::normalize(glm::vec3(0.0f) - startPos);
      const glm::quat orientation = glm::quatLookAt(forward, glm::vec3(0.0f, 1.0f, 0.0f));
      const Transform camTransform(startPos, orientation);
      Camera cam(api, camTransform, glm::vec3(0.0f, 1.0f, 0.0f), 90.0f,
                 1.0f, 0.01f, 100.0f);
      const float camSpeed = 3.0f;
      const float camRotateSpeed = glm::radians(60.0f);

      // Create the renderer
      std::unique_ptr<IRenderer> renderer = RendererFactory::CreateRenderer(api, &window);
      renderer->SetActiveCamera(&cam);
      renderer->SetActiveScene(&scene);
      float deltaTime = 0.0f;

      // Setup events
      EventSystem* events = window.GetEventSystem();

      // Movement key handlers using held callbacks
      events->OnKeyHeld(GLFW_KEY_W, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::vec3 position = transform.GetPosition();
         position += cam.GetViewDirection() * camSpeed * deltaTime;
         transform.SetPosition(position);
      });
      events->OnKeyHeld(GLFW_KEY_S, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::vec3 position = transform.GetPosition();
         position -= cam.GetViewDirection() * camSpeed * deltaTime;
         transform.SetPosition(position);
      });
      events->OnKeyHeld(GLFW_KEY_A, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::vec3 position = transform.GetPosition();
         position -= cam.GetRightVector() * camSpeed * deltaTime;
         transform.SetPosition(position);
      });
      events->OnKeyHeld(GLFW_KEY_D, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::vec3 position = transform.GetPosition();
         position += cam.GetRightVector() * camSpeed * deltaTime;
         transform.SetPosition(position);
      });
      events->OnKeyHeld(GLFW_KEY_SPACE, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::vec3 position = transform.GetPosition();
         position += glm::vec3(0.0f, 1.0f, 0.0f) * camSpeed * deltaTime;
         transform.SetPosition(position);
      });
      events->OnKeyHeld(GLFW_KEY_LEFT_SHIFT, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::vec3 position = transform.GetPosition();
         position -= glm::vec3(0.0f, 1.0f, 0.0f) * camSpeed * deltaTime;
         transform.SetPosition(position);
      });
      // Rotation stuff
      events->OnKeyHeld(GLFW_KEY_I, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::quat rotation = transform.GetRotation();
         // Pitch up: rotate around right vector
         rotation = glm::normalize(glm::angleAxis(camRotateSpeed * deltaTime, cam.GetRightVector()) * rotation);
         transform.SetRotation(rotation);
      });
      events->OnKeyHeld(GLFW_KEY_K, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::quat rotation = transform.GetRotation();
         // Pitch down: rotate around right vector
         rotation = glm::normalize(glm::angleAxis(-camRotateSpeed * deltaTime, cam.GetRightVector()) * rotation);
         transform.SetRotation(rotation);
      });
      events->OnKeyHeld(GLFW_KEY_J, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::quat rotation = transform.GetRotation();
         // Yaw left: rotate around world up vector
         rotation = glm::normalize(glm::angleAxis(camRotateSpeed * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * rotation);
         transform.SetRotation(rotation);
      });
      events->OnKeyHeld(GLFW_KEY_L, [&](const uint32_t, const uint32_t, const uint32_t) {
         Transform& transform = cam.GetMutableTransform();
         glm::quat rotation = transform.GetRotation();
         // Yaw right: rotate around world up vector
         rotation = glm::normalize(glm::angleAxis(-camRotateSpeed * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * rotation);
         transform.SetRotation(rotation);
      });
      // ESC to close window
      events->OnKeyDown(GLFW_KEY_ESCAPE, [&](const uint32_t, const uint32_t, const uint32_t) {
         glfwSetWindowShouldClose(window.GetNativeWindow(), true);
      });

      // Main loop
      auto lastTime = std::chrono::high_resolution_clock::now();
      while (!window.ShouldClose()) {
         auto currentTime = std::chrono::high_resolution_clock::now();
         deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
         lastTime = currentTime;

         window.PollEvents();
         renderer->RenderFrame();
      }
   } catch (const std::exception& err) {
      std::println("Error: {}", err.what());
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}

