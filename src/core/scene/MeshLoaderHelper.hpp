#pragma once

#include "core/resource/ResourceManager.hpp"

class Node;
class Scene;

class MeshLoaderHelper final {
  public:
   struct MeshLoadOptions final {
      std::string nodePrefix = "";
   };

   static Node* LoadMeshIntoScene(Scene& scene, ResourceManager& resourceManager,
                                  const std::string& meshName, const std::string& filepath,
                                  const MeshLoadOptions& options,
                                  const std::vector<MaterialHandle>& materials);
   static Node* LoadMeshAsChildNode(Node* parent, ResourceManager& resourceManager,
                                    const std::string& meshName, const std::string& filepath,
                                    const MeshLoadOptions& options,
                                    const std::vector<MaterialHandle>& materials);

  private:
   static void CreateNodesForMeshGroup(Node* parentNode, ResourceManager& resourceManager,
                                       const ResourceManager::LoadedMeshGroup& meshGroup,
                                       const MeshLoadOptions& options,
                                       const std::vector<MaterialHandle>& materials);
   static std::string GenerateNodeName(const std::string& baseName, const size_t index);
};
