#include "core/scene/MeshLoaderHelper.hpp"

#include "core/scene/Node.hpp"
#include "core/scene/Scene.hpp"
#include "core/scene/components/RendererComponent.hpp"
#include "core/scene/components/TransformComponent.hpp"

#include "core/resource/ResourceManager.hpp"
#include "core/resource/IMaterial.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

Node* MeshLoaderHelper::LoadSceneIntoScene(Scene& scene, ResourceManager& resourceManager,
                                           const std::string& sceneName,
                                           const std::string& filepath,
                                           const MeshLoadOptions& options,
                                           const std::vector<MaterialHandle>& materials) {
   // Load the scene data
   const MeshLoader::SceneData sceneData = resourceManager.LoadSceneData(filepath);
   if (sceneData.IsEmpty()) {
      return nullptr;
   }
   // Create a parent node for the entire scene
   const std::string parentName =
      options.nodePrefix.empty() ? sceneName : options.nodePrefix + sceneName;
   Node* parentNode = scene.CreateNode(SanitizeNodeName(parentName));
   // Create the hierarchy recursively
   CreateSceneHierarchy(scene, parentNode, resourceManager, sceneData, sceneData.rootNode, options,
                        materials);
   return parentNode;
}

Node* MeshLoaderHelper::LoadSceneAsChildNode(Scene& scene, Node* parent,
                                             ResourceManager& resourceManager,
                                             const std::string& sceneName,
                                             const std::string& filepath,
                                             const MeshLoadOptions& options,
                                             const std::vector<MaterialHandle>& materials) {
   if (!parent) {
      return LoadSceneIntoScene(scene, resourceManager, sceneName, filepath, options, materials);
   }
   // Load the scene data
   const MeshLoader::SceneData sceneData = resourceManager.LoadSceneData(filepath);
   if (sceneData.IsEmpty()) {
      return nullptr;
   }
   // Create a child node to group the scene
   const std::string groupName =
      options.nodePrefix.empty() ? sceneName : options.nodePrefix + sceneName;
   Node* childNode = scene.CreateChildNode(parent, SanitizeNodeName(groupName));
   // Create the hierarchy recursively
   CreateSceneHierarchy(scene, childNode, resourceManager, sceneData, sceneData.rootNode, options,
                        materials);
   return childNode;
}

Node* MeshLoaderHelper::LoadMeshIntoScene(Scene& scene, ResourceManager& resourceManager,
                                          const std::string& meshName, const std::string& filepath,
                                          const MeshLoadOptions& options,
                                          const std::vector<MaterialHandle>& materials) {
   // Load as single combined mesh for compatibility
   const MeshHandle meshHandle = resourceManager.LoadSingleMeshFromFile(meshName, filepath);
   if (!meshHandle.IsValid()) {
      return nullptr;
   }
   // Create node with mesh
   const std::string nodeName =
      options.nodePrefix.empty() ? meshName : options.nodePrefix + meshName;
   Node* node = scene.CreateNode(SanitizeNodeName(nodeName));
   // Get default material if none provided
   MaterialHandle material;
   if (!materials.empty() && materials[0].IsValid()) {
      material = materials[0];
   } else {
      material = resourceManager.GetMaterialHandle("default_pbr");
   }
   // Add renderer component
   node->AddComponent<RendererComponent>(meshHandle, material);
   return node;
}

Node* MeshLoaderHelper::CreateSceneHierarchy(Scene& scene, Node* parentNode,
                                             ResourceManager& resourceManager,
                                             const MeshLoader::SceneData& sceneData,
                                             const MeshLoader::SceneNode& sceneNode,
                                             const MeshLoadOptions& options,
                                             const std::vector<MaterialHandle>& materials) {
   if (!parentNode) {
      return nullptr;
   }
   Node* currentNode = parentNode;
   // If this scene node has its own name and is not the root, create a child node for it
   if (sceneNode.name != "RootNode" && sceneNode.name != parentNode->GetName()) {
      const std::string sanitizedName = SanitizeNodeName(sceneNode.name);
      currentNode = scene.CreateChildNode(parentNode, sanitizedName);
      // Apply transform from the scene node if enabled
      if (options.applyTransforms) {
         ApplyTransformToNode(currentNode, sceneNode.transform);
      }
   }
   // Create mesh resources and renderer components for this node
   if (sceneNode.HasMeshes()) {
      for (size_t meshIndex : sceneNode.meshIndices) {
         if (meshIndex >= sceneData.meshes.size()) {
            continue;
         }
         const auto& meshData = sceneData.meshes[meshIndex];
         if (meshData.IsEmpty()) {
            continue;
         }
         // Create unique mesh name
         const std::string meshName =
            GenerateUniqueMeshName(sceneNode.name + "_" + meshData.name, meshIndex);
         // Create the mesh resource
         const MeshHandle meshHandle =
            resourceManager.LoadMesh(meshName, meshData.vertices, meshData.indices);
         if (!meshHandle.IsValid()) {
            continue;
         }
         // Determine material to use
         MaterialHandle material;
         if (materials.size() > meshData.materialIndex &&
             materials[meshData.materialIndex].IsValid()) {
            material = materials[meshData.materialIndex];
         } else {
            material = resourceManager.GetMaterialHandle("default_pbr");
         }
         // If we have multiple meshes in this node, create child nodes for each
         Node* meshNode = currentNode;
         if (sceneNode.meshIndices.size() > 1) {
            const std::string meshNodeName = SanitizeNodeName(
               meshData.name.empty() ? ("Mesh_" + std::to_string(meshIndex)) : meshData.name);
            meshNode = scene.CreateChildNode(currentNode, meshNodeName);
            // Apply mesh-specific transform if needed
            if (options.applyTransforms && meshData.transform != sceneNode.transform &&
                meshData.transform != glm::mat4(1.0f)) {
               ApplyTransformToNode(meshNode, meshData.transform);
            }
         }
         // Add renderer component
         meshNode->AddComponent<RendererComponent>(meshHandle, material);
      }
   }
   // Recursively create child nodes if hierarchy preservation is enabled
   if (options.preserveHierarchy) {
      for (const auto& childSceneNode : sceneNode.children) {
         CreateSceneHierarchy(scene, currentNode, resourceManager, sceneData, childSceneNode,
                              options, materials);
      }
   }
   return currentNode;
}

void MeshLoaderHelper::ApplyTransformToNode(Node* node, const glm::mat4& transform) {
   if (!node || transform == glm::mat4(1.0f)) {
      return;
   }
   // Decompose the matrix into translation, rotation, and scale
   glm::vec3 scale;
   glm::quat rotation;
   glm::vec3 translation;
   glm::vec3 skew;
   glm::vec4 perspective;
   if (glm::decompose(transform, scale, rotation, translation, skew, perspective)) {
      if (TransformComponent* transformComp = node->GetComponent<TransformComponent>()) {
         transformComp->SetPosition(translation);
         transformComp->SetRotation(glm::eulerAngles(rotation));
         transformComp->SetScale(scale);
         node->MarkTransformDirty();
      }
   }
}

std::string MeshLoaderHelper::GenerateUniqueMeshName(const std::string& baseName, size_t index) {
   if (baseName.empty()) {
      return "Mesh_" + std::to_string(index);
   }
   return SanitizeNodeName(baseName) + "_" + std::to_string(index);
}

std::string MeshLoaderHelper::SanitizeNodeName(const std::string& name) {
   if (name.empty()) {
      return "Node";
   }
   std::string sanitized = name;
   // Replace invalid characters with underscores
   for (char& c : sanitized) {
      if (!std::isalnum(c) && c != '_' && c != '-') {
         c = '_';
      }
   }
   // Ensure it doesn't start with a number
   if (std::isdigit(sanitized[0])) {
      sanitized = "Node_" + sanitized;
   }
   return sanitized;
}
