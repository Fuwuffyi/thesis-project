#pragma once

#include "core/resource/ResourceManager.hpp"

class Node;
class Scene;

class MeshLoaderHelper final {
public:
   struct MeshLoadOptions {
      bool createSeparateNodes = true;
      std::string nodePrefix = "";
   };

   static Node* LoadMeshIntoScene(Scene& scene, ResourceManager& resourceManager,
                                  const std::string& meshName, const std::string& filepath,
                                  const MeshLoadOptions& options);
   static Node* LoadMeshAsChildNode(Node* parent, ResourceManager& resourceManager,
                                    const std::string& meshName, const std::string& filepath,
                                    const MeshLoadOptions& options);

private:
   static void CreateNodesForMeshGroup(Node* parentNode,
                                       const ResourceManager::LoadedMeshGroup& meshGroup,
                                       const MeshLoadOptions& options);
   static std::string GenerateNodeName(const std::string& baseName, const size_t index);
};

