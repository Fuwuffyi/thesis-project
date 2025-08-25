#include "core/GraphicsAPI.hpp"
#include "core/RendererFactory.hpp"
#include "core/Window.hpp"
#include "core/Camera.hpp"

#include "core/resource/ResourceHandle.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/Node.hpp"
#include "core/scene/components/TransformComponent.hpp"
#include "core/scene/components/RendererComponent.hpp"

#include <memory>
#include <print>
#include <chrono>

struct MouseState {
   float lastX = 0.0f;
   float lastY = 0.0f;
   float sensitivity = 0.05f;
   bool firstMouse = true;
   bool shouldUpdate = true;
};

// Testing mesh
const std::vector<Vertex> vertices = {
   {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
   {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
   {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
   {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
   {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {0.0f, 0.0f}},
   {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {1.0f, 0.0f}},
   {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {1.0f, 1.0f}},
   {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {0.0f, 1.0f}},
   {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.0f, 0.0f}, {0.0f, 0.0f}},
   {{-0.5f, -0.5f,  0.5f}, {0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}},
   {{-0.5f,  0.5f,  0.5f}, {0.5f, 0.0f, 0.0f}, {1.0f, 1.0f}},
   {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.0f, 0.0f}, {0.0f, 1.0f}},
   {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
   {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
   {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
   {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
   {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
   {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
   {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
   {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
   {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.5f, 0.0f}, {0.0f, 0.0f}},
   {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.5f, 0.0f}, {1.0f, 0.0f}},
   {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.5f, 0.0f}, {1.0f, 1.0f}},
   {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f}},
};

const std::vector<uint16_t> indices = {
   0, 1, 2, 2, 3, 0,
   4, 5, 6, 6, 7, 4,
   8, 9, 10, 10, 11, 8,
   12, 13, 14, 14, 15, 12,
   16, 17, 18, 18, 19, 16,
   20, 21, 22, 22, 23, 20
};

int main(int argc, char* argv[]) {
   // Check argv for api
   GraphicsAPI api = GraphicsAPI::Vulkan;
   for (int32_t i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "-v") {
         api = GraphicsAPI::Vulkan;
      } else if (arg == "-g") {
         api = GraphicsAPI::OpenGL;
      } else {
         return 1;
      }
   }
   // Main program
   try {
      // Create the window
      WindowDesc windowDesc{
         .title = "Deferred Rendering Engine",
         .width = 900,
         .height = 900,
         .vsync = false,
         .resizable = true
      };
      Window window(api, windowDesc);

      // Create the renderer
      std::unique_ptr<IRenderer> renderer = RendererFactory::CreateRenderer(api, &window);
      float deltaTime = 0.0f;

      // Get renderer resource manager
      ResourceManager* resourceManager = renderer->GetResourceManager();
      const MeshHandle testMesh = resourceManager->LoadMesh("testing_cube", vertices, indices);
      // FIXME: Currently vulkan loads texture (is static)
      if (api == GraphicsAPI::OpenGL) {
         resourceManager->LoadTexture("testing_albedo", "resources/textures/texture_base.jpg", true, true);
      }
      resourceManager->LoadTexture("testing_displacement", "resources/textures/texture_displ.jpg", true, false);
      resourceManager->LoadTexture("testing_normal", "resources/textures/texture_normal.jpg", true, false);
      resourceManager->LoadTexture("testing_roughness", "resources/textures/texture_roughness.jpg", true, false);

      // Create the camera
      const glm::vec3 startPos = glm::vec3(2.0f);
      const glm::vec3 forward = glm::normalize(glm::vec3(0.0f) - startPos);
      const glm::quat orientation = glm::quatLookAt(forward, glm::vec3(0.0f, 1.0f, 0.0f));
      const Transform camTransform(startPos, orientation);
      Camera cam(api, camTransform, glm::vec3(0.0f, 1.0f, 0.0f), 90.0f,
                 1.0f, 0.01f, 100.0f);
      const float camSpeed = 3.0f;
      const float camRotateSpeed = glm::radians(60.0f);
      renderer->SetActiveCamera(&cam);

      // Create the scene
      Scene scene("Test scene");
      Node* node2 = scene.CreateNode("Cube 0");
      node2->AddComponent(std::make_unique<TransformComponent>());
      node2->AddComponent(std::make_unique<RendererComponent>(testMesh));
      node2->GetTransform()->SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
      Node* node3 = scene.CreateNode("Cube 1");
      node3->AddComponent(std::make_unique<TransformComponent>());
      node3->AddComponent(std::make_unique<RendererComponent>(testMesh));
      node3->GetTransform()->SetPosition(glm::vec3(2.0f, 0.0f, 0.0f));
      Node* node4 = scene.CreateNode("Cube 2");
      node4->AddComponent(std::make_unique<TransformComponent>());
      node4->AddComponent(std::make_unique<RendererComponent>(testMesh));
      node4->GetTransform()->SetPosition(glm::vec3(0.0f, 0.0f, 2.0f));
      scene.UpdateScene(0.0f);
      renderer->SetActiveScene(&scene);

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
      MouseState mouseState;
      window.SetCursorVisible(false);
      events->OnCursorPos([&](float xpos, float ypos) {
         if (!mouseState.shouldUpdate) return;
         if (mouseState.firstMouse) {
            mouseState.lastX = xpos;
            mouseState.lastY = ypos;
            mouseState.firstMouse = false;
         }
         float xoffset = xpos - mouseState.lastX;
         float yoffset = mouseState.lastY - ypos;
         mouseState.lastX = xpos;
         mouseState.lastY = ypos;
         xoffset *= mouseState.sensitivity;
         yoffset *= mouseState.sensitivity;
         Transform& transform = cam.GetMutableTransform();
         glm::quat rotation = transform.GetRotation();
         float pitch = glm::degrees(glm::asin(cam.GetViewDirection().y));
         pitch += yoffset;
         if (pitch > 89.0f) yoffset -= pitch - 89.0f;
         if (pitch < -89.0f) yoffset -= pitch + 89.0f;
         rotation = glm::normalize(glm::angleAxis(glm::radians(-xoffset),
                                                  glm::vec3(0.0f, 1.0f, 0.0f)) * rotation);
         rotation = glm::normalize(glm::angleAxis(glm::radians(pitch),
                                                  cam.GetRightVector()) * rotation);
         transform.SetRotation(rotation);
      });
      events->OnKeyDown(GLFW_KEY_LEFT_ALT, [&](const uint32_t, const uint32_t, const uint32_t) {
         window.SetCursorVisible(true);
         mouseState.shouldUpdate = false;
      });
      events->OnKeyUp(GLFW_KEY_LEFT_ALT, [&](const uint32_t, const uint32_t, const uint32_t) {
         window.SetCursorVisible(false);
         mouseState.shouldUpdate = true;
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

