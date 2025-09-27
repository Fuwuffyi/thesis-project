#pragma once

#include "core/Vertex.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

// Forward declarations for ASSIMP
struct aiScene;
struct aiNode;
struct aiMesh;

class MeshLoader final {
  public:
   // Structure to hold data for a single mesh
   struct MeshData {
      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;
      std::string name;
      uint32_t materialIndex{0};
      glm::mat4 transform{glm::mat4(1.0f)};

      size_t GetVertexCount() const noexcept;
      size_t GetIndexCount() const noexcept;
      bool IsEmpty() const noexcept;
   };

   // Structure to hold scene hierarchy information
   struct SceneNode {
      std::string name;
      glm::mat4 transform{glm::mat4(1.0f)};
      std::vector<size_t> meshIndices;
      std::vector<SceneNode> children;

      bool HasMeshes() const noexcept { return !meshIndices.empty(); }
   };

   // Complete scene data with hierarchy
   struct SceneData {
      std::vector<MeshData> meshes;
      SceneNode rootNode;
      std::string filepath;

      bool IsEmpty() const noexcept;
      size_t GetMeshCount() const noexcept;
   };

   static SceneData LoadScene(std::string_view filepath);
   static MeshData LoadSingleMesh(std::string_view filepath);

  private:
   static void ProcessNode(const aiScene* scene, const aiNode* node, SceneData& sceneData,
                           SceneNode& parentNode);
   static MeshData ProcessMesh(const aiScene* scene, const aiMesh* mesh);
   static void ExtractVertexData(const aiMesh* mesh, std::vector<Vertex>& vertices);
   static void ExtractIndexData(const aiMesh* mesh, std::vector<uint32_t>& indices);
   static glm::mat4 ConvertMatrix(const auto& aiMat);
};
