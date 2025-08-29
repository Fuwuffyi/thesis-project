#pragma once

#include "core/Vertex.hpp"

#include <string>
#include <vector>

// Forward declarations for ASSIMP
struct aiScene;
struct aiNode;
struct aiMesh;

namespace MeshLoader {
   // Structure to hold data for a single sub-mesh
   struct SubMesh {
      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;
      std::string name;
      uint32_t materialIndex = 0;

      size_t GetVertexCount() const noexcept;
      size_t GetIndexCount() const noexcept;
   };

   // Structure to hold all mesh data from a file
   struct MeshData {
      std::vector<SubMesh> subMeshes;
      std::string filepath;

      bool IsEmpty() const noexcept;
      size_t GetSubMeshCount() const noexcept;
      std::pair<std::vector<Vertex>, std::vector<uint32_t>> GetCombinedData() const;
   };

   MeshData LoadMesh(std::string_view filepath);

   namespace Internal {
      void ProcessNode(const aiScene* scene, const aiNode* node, std::vector<SubMesh>& subMeshes);
      SubMesh ProcessMesh(const aiScene* scene, const aiMesh* mesh);
      void ExtractVertexData(const aiMesh* mesh, std::vector<Vertex>& vertices);
      void ExtractIndexData(const aiMesh* mesh, std::vector<uint32_t>& indices);
   };
};

