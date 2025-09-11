#include "core/scene/MeshLoaderHelper.hpp"

#include "core/scene/Node.hpp"
#include "core/scene/Scene.hpp"
#include "core/scene/components/RendererComponent.hpp"

#include "core/resource/IMaterial.hpp"

Node* MeshLoaderHelper::LoadMeshIntoScene(Scene& scene, ResourceManager& resourceManager,
                                          const std::string& meshName, const std::string& filepath,
                                          const MeshLoadOptions& options,
                                          const std::vector<MaterialHandle>& materials) {
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
   CreateNodesForMeshGroup(scene, parentNode, resourceManager, meshGroup, options, materials);
   return parentNode;
}

Node* MeshLoaderHelper::LoadMeshAsChildNode(Scene& scene, Node* parent,
                                            ResourceManager& resourceManager,
                                            const std::string& meshName,
                                            const std::string& filepath,
                                            const MeshLoadOptions& options,
                                            const std::vector<MaterialHandle>& materials) {
   if (!parent) {
      return nullptr;
   }
   // Load the mesh group from file
   const ResourceManager::LoadedMeshGroup meshGroup =
      resourceManager.LoadMeshFromFile(meshName, filepath);
   if (!meshGroup.IsValid()) {
      return nullptr;
   }
   // Create a child node to group all sub-meshes
   const std::string groupName =
      options.nodePrefix.empty() ? meshName : options.nodePrefix + meshName;
   Node* childNode = scene.CreateChildNode(parent, groupName);
   childNode->AddComponent<TransformComponent>();
   CreateNodesForMeshGroup(scene, childNode, resourceManager, meshGroup, options, materials);
   return childNode;
}

void MeshLoaderHelper::CreateNodesForMeshGroup(Scene& scene, Node* parentNode,
                                               ResourceManager& resourceManager,
                                               const ResourceManager::LoadedMeshGroup& meshGroup,
                                               const MeshLoadOptions& options,
                                               const std::vector<MaterialHandle>& materials) {
   const MaterialHandle defaultMat = resourceManager.GetMaterialHandle("default_pbr");
   if (!parentNode || !meshGroup.IsValid()) {
      return;
   }
   // Create separate nodes for each sub-mesh
   for (size_t i = 0; i < meshGroup.subMeshes.size(); ++i) {
      const MeshHandle& meshHandle = meshGroup.subMeshes[i];
      const size_t materialIndex = meshGroup.materialIndices[i];
      const std::string nodeName = GenerateNodeName(parentNode->GetName() + "_SubMesh", i);
      Node* childNode = scene.CreateChildNode(parentNode, nodeName);
      childNode->AddComponent<TransformComponent>();
      // Create renderer component with single mesh
      RendererComponent* renderer = childNode->AddComponent<RendererComponent>(
         meshHandle, materials.size() > materialIndex ? materials[materialIndex] : defaultMat);
   }
}

std::string MeshLoaderHelper::GenerateNodeName(const std::string& baseName, const size_t index) {
   return baseName + "_" + std::to_string(index);
}
