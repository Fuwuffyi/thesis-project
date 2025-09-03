#include "core/scene/MeshLoaderHelper.hpp"

#include "core/scene/Node.hpp"
#include "core/scene/Scene.hpp"
#include "core/scene/components/RendererComponent.hpp"

#include "core/resource/IMaterial.hpp"

Node* MeshLoaderHelper::LoadMeshIntoScene(Scene& scene, ResourceManager& resourceManager,
                                          const std::string& meshName, const std::string& filepath,
                                          const MeshLoadOptions& options) {
   // Load the mesh group from file
   const ResourceManager::LoadedMeshGroup meshGroup =
      resourceManager.LoadMeshFromFile(meshName, filepath);
   if (!meshGroup.IsValid()) {
      return nullptr;
   }
   // Create a parent node group to load meshes to
   const std::string parentName =
      options.nodePrefix.empty() ? meshName : options.nodePrefix + meshName;
   Node* parentNode = scene.CreateNode(parentName);
   CreateNodesForMeshGroup(parentNode, resourceManager, meshGroup, options);
   return parentNode;
}

Node* MeshLoaderHelper::LoadMeshAsChildNode(Node* parent, ResourceManager& resourceManager,
                                            const std::string& meshName,
                                            const std::string& filepath,
                                            const MeshLoadOptions& options) {
   if (!parent) {
      return nullptr;
   }
   // Load the mesh group from file
   const ResourceManager::LoadedMeshGroup meshGroup =
      resourceManager.LoadMeshFromFile(meshName, filepath);
   if (!meshGroup.IsValid()) {
      return nullptr;
   }
   Node* groupNode = nullptr;
   // Create a child node to group all sub-meshes
   const std::string groupName =
      options.nodePrefix.empty() ? meshName : options.nodePrefix + meshName;
   auto childNode = std::make_unique<Node>(groupName);
   childNode->AddComponent<TransformComponent>();
   groupNode = childNode.get();
   parent->AddChild(std::move(childNode));
   CreateNodesForMeshGroup(groupNode, resourceManager, meshGroup, options);
   return groupNode;
}

void MeshLoaderHelper::CreateNodesForMeshGroup(Node* parentNode, ResourceManager& resourceManager,
                                               const ResourceManager::LoadedMeshGroup& meshGroup,
                                               const MeshLoadOptions& options) {
   if (!parentNode || !meshGroup.IsValid()) {
      return;
   }
   if (options.createSeparateNodes) {
      // Create separate nodes for each sub-mesh
      for (size_t i = 0; i < meshGroup.subMeshes.size(); ++i) {
         const MeshHandle& meshHandle = meshGroup.subMeshes[i];
         const std::string nodeName = GenerateNodeName(parentNode->GetName() + "_SubMesh", i);
         std::unique_ptr<Node> childNode = std::make_unique<Node>(nodeName);
         childNode->AddComponent<TransformComponent>();
         // Create renderer component with single mesh
         // TODO: Setup default material
         RendererComponent* renderer = childNode->AddComponent<RendererComponent>(meshHandle);
         // Set material index for reference
         RendererComponent::SubMeshRenderer subMeshRenderer;
         subMeshRenderer.mesh = meshHandle;
         renderer->AddSubMeshRenderer(subMeshRenderer);
         parentNode->AddChild(std::move(childNode));
      }
   } else {
      // Create single node with multiple sub-mesh renderers
      auto* renderer = parentNode->GetComponent<RendererComponent>();
      if (!renderer) {
         renderer = parentNode->AddComponent<RendererComponent>();
      }
      // Clear existing sub-meshes and add new ones
      renderer->ClearSubMeshes();
      for (size_t i = 0; i < meshGroup.subMeshes.size(); ++i) {
         const auto& meshHandle = meshGroup.subMeshes[i];
         RendererComponent::SubMeshRenderer subMeshRenderer;
         subMeshRenderer.mesh = meshHandle;
         // TODO: Setup default material
         subMeshRenderer.material = {};
         renderer->AddSubMeshRenderer(subMeshRenderer);
      }
   }
}

std::string MeshLoaderHelper::GenerateNodeName(const std::string& baseName, const size_t index) {
   return baseName + "_" + std::to_string(index);
}
