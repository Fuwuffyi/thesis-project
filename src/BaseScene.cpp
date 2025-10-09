#include "BaseScene.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/Node.hpp"

#include "core/scene/MeshLoaderHelper.hpp"

#include "core/scene/components/LightComponent.hpp"
#include "core/scene/components/ParticleSystemComponent.hpp"

// TODO: This is a test scene, I need to add a serialization/deserialization system
void LoadBaseScene(Scene& scene, ResourceManager& resourceManager, const GraphicsAPI api) {
   // FIXME: Currently vulkan loads texture statically
   Node* sponzaNode = MeshLoaderHelper::LoadSceneAsChildNode(
      scene, scene.GetRootNode(), resourceManager, "sponza", "resources/meshes/sponza.fbx", {}, {});
   sponzaNode->GetTransform()->SetScale(glm::vec3(0.01f));
   sponzaNode->GetTransform()->SetRotation(glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)));

   // Setup particle node
   Node* particlesNode = scene.CreateNode("particles");
   particlesNode->GetComponent<TransformComponent>()->SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
   particlesNode->AddComponent<ParticleSystemComponent>();

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
   Node* sunNode = scene.CreateChildNode(lightsNode, "light_sun");
   TransformComponent* sunTransform = sunNode->GetComponent<TransformComponent>();
   sunTransform->SetRotation(glm::vec3(-45.0f, 45.0f, 0.0f));
   LightComponent* lightSun = sunNode->AddComponent<LightComponent>();
   lightSun->SetType(LightComponent::LightType::Directional);
   lightSun->SetColor(glm::vec3(1.0f, 1.0f, 1.0f));
   lightSun->SetIntensity(0.45f);
}
