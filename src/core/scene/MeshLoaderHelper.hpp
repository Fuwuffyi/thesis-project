#pragma once

#include "core/resource/IMaterial.hpp"
#include "core/resource/MeshLoader.hpp"

class ResourceManager;
class Node;
class Scene;

class MeshLoaderHelper final {
  public:
   struct MeshLoadOptions final {
      std::string nodePrefix{""};
      bool preserveHierarchy{true};
      bool applyTransforms{true};
   };

   // Load a scene with full hierarchy preservation
   static Node* LoadSceneIntoScene(Scene& scene, ResourceManager& resourceManager,
                                   const std::string& sceneName, const std::string& filepath,
                                   const MeshLoadOptions& options,
                                   const std::vector<MaterialHandle>& materials = {});

   // Load as child node with hierarchy
   static Node* LoadSceneAsChildNode(Scene& scene, Node* parent, ResourceManager& resourceManager,
                                     const std::string& sceneName, const std::string& filepath,
                                     const MeshLoadOptions& options,
                                     const std::vector<MaterialHandle>& materials = {});

   // Load as single combined mesh
   static Node* LoadMeshIntoScene(Scene& scene, ResourceManager& resourceManager,
                                  const std::string& meshName, const std::string& filepath,
                                  const MeshLoadOptions& options,
                                  const std::vector<MaterialHandle>& materials = {});

  private:
   static Node* CreateSceneHierarchy(Scene& scene, Node* parentNode,
                                     ResourceManager& resourceManager,
                                     const MeshLoader::SceneData& sceneData,
                                     const MeshLoader::SceneNode& sceneNode,
                                     const MeshLoadOptions& options,
                                     const std::vector<MaterialHandle>& materials);

   static void ApplyTransformToNode(Node* node, const glm::mat4& transform);
   static std::string GenerateUniqueMeshName(const std::string& baseName, size_t index);
   static std::string SanitizeNodeName(const std::string& name);
};
