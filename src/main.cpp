#include "core/GraphicsAPI.hpp"
#include "core/RendererFactory.hpp"
#include "core/Window.hpp"
#include "core/Camera.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/MeshLoaderHelper.hpp"
#include "core/scene/Node.hpp"
#include "core/scene/components/LightComponent.hpp"
#include "glm/trigonometric.hpp"

#include <GLFW/glfw3.h>

#include <memory>
#include <random>
#include <print>
#include <chrono>

struct MouseState {
   float lastX = 0.0f;
   float lastY = 0.0f;
   float sensitivity = 0.05f;
   bool firstMouse = true;
   bool shouldUpdate = true;
   float yaw = -90.0f;
   float pitch = 0.0f;
};

// TODO: This is a test scene, I need to add a serialization/deserialization system
void CreateFullScene(Scene& scene, ResourceManager& resourceManager, const GraphicsAPI api) {
   // FIXME: Currently vulkan loads texture statically
   Node* sponzaNode = MeshLoaderHelper::LoadSceneAsChildNode(
      scene, scene.GetRootNode(), resourceManager, "sponza", "resources/meshes/sponza.fbx", {}, {});
   sponzaNode->GetTransform()->SetScale(glm::vec3(0.01f));
   sponzaNode->GetTransform()->SetRotation(glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)));

   // Setup light node
   Node* lightsNode = scene.CreateNode("lights");
   lightsNode->GetComponent<TransformComponent>()->SetPosition(glm::vec3(0.0f, 7.0f, 0.0f));
   // Create testing lights
   std::random_device rd;
   std::mt19937 gen(rd());
   std::uniform_real_distribution<float> unitDist(0.0f, 1.0f);
   std::uniform_real_distribution<float> posDist(-7.0f, 7.0f);
   std::uniform_real_distribution<float> angleDist(40.0f, 65.0f);
   std::uniform_real_distribution<float> angleDistTransform(0.0f, 180.0f);
   for (uint32_t i = 0; i < 25; ++i) {
      Node* lightNode = scene.CreateChildNode(lightsNode, "light_" + std::to_string(i));
      TransformComponent* transform = lightNode->GetComponent<TransformComponent>();
      transform->SetPosition(glm::vec3(posDist(gen), posDist(gen), posDist(gen)));
      transform->SetRotation(
         glm::vec3(angleDistTransform(gen), angleDistTransform(gen), angleDistTransform(gen)));
      transform->SetScale(glm::vec3(0.2f));
      LightComponent* light = lightNode->AddComponent<LightComponent>();
      light->SetType(LightComponent::LightType::Spot);
      light->SetColor(glm::vec3(unitDist(gen), unitDist(gen), unitDist(gen)));
      light->SetIntensity(3.0f);
      const float outer = angleDist(gen);
      light->SetOuterCone(glm::radians(outer));
      light->SetInnerCone(glm::radians(5.0f + outer));
   }
}

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
      WindowDesc windowDesc{.title = "Deferred Rendering Engine",
                            .width = 900,
                            .height = 900,
                            .vsync = false,
                            .resizable = true};
      Window window(api, windowDesc);

      // Create the renderer
      std::unique_ptr<IRenderer> renderer = RendererFactory::CreateRenderer(api, &window);
      float deltaTime = 0.0f;

      // Create the scene
      ResourceManager* resourceManager = renderer->GetResourceManager();
      Scene scene("Test scene");
      CreateFullScene(scene, *resourceManager, api);
      renderer->SetActiveScene(&scene);

      // Create the camera
      const glm::vec3 startPos = glm::vec3(2.0f);
      const glm::vec3 forward = glm::normalize(glm::vec3(0.0f) - startPos);
      const glm::quat orientation = glm::quatLookAt(forward, glm::vec3(0.0f, 1.0f, 0.0f));
      const Transform camTransform(startPos, orientation);
      Camera cam(api, camTransform, glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, 1.0f, 0.01f, 100.0f);
      const float camSpeed = 3.0f;
      const float camRotateSpeed = glm::radians(60.0f);
      renderer->SetActiveCamera(&cam);

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
      {
         const glm::vec3 dir = cam.GetViewDirection();
         mouseState.yaw = glm::degrees(std::atan2(dir.x, -dir.z));
         mouseState.pitch = glm::degrees(std::asin(dir.y));
      }
      window.SetCursorVisible(false);
      events->OnCursorPos([&](float xpos, float ypos) {
         if (!mouseState.shouldUpdate)
            return;
         if (mouseState.firstMouse) {
            mouseState.lastX = xpos;
            mouseState.lastY = ypos;
            mouseState.firstMouse = false;
            return;
         }
         const float xoffset = (xpos - mouseState.lastX) * mouseState.sensitivity;
         const float yoffset = (mouseState.lastY - ypos) * mouseState.sensitivity;
         mouseState.lastX = xpos;
         mouseState.lastY = ypos;
         mouseState.yaw -= xoffset;
         mouseState.pitch += yoffset;
         if (mouseState.pitch > 89.0f)
            mouseState.pitch = 89.0f;
         if (mouseState.pitch < -89.0f)
            mouseState.pitch = -89.0f;
         const glm::quat qYaw = glm::angleAxis(glm::radians(mouseState.yaw), glm::vec3(0, 1, 0));
         const glm::quat qPitch =
            glm::angleAxis(glm::radians(mouseState.pitch), glm::vec3(1, 0, 0));
         const glm::quat rotation = qYaw * qPitch;
         cam.GetMutableTransform().SetRotation(rotation);
      });
      events->OnKeyDown(GLFW_KEY_LEFT_ALT, [&](const uint32_t, const uint32_t, const uint32_t) {
         window.SetCursorVisible(true);
         mouseState.shouldUpdate = false;
      });
      events->OnKeyUp(GLFW_KEY_LEFT_ALT, [&](const uint32_t, const uint32_t, const uint32_t) {
         window.SetCursorVisible(false);
         mouseState.shouldUpdate = true;
         mouseState.firstMouse = true;
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
