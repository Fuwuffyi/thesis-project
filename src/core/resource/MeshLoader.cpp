#include "core/resource/MeshLoader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <print>
#include <ranges>

size_t MeshLoader::SubMesh::GetVertexCount() const noexcept {
   return vertices.size();
}

size_t MeshLoader::SubMesh::GetIndexCount() const noexcept {
   return indices.size();
}

bool MeshLoader::MeshData::IsEmpty() const noexcept {
   return subMeshes.empty();
}

size_t MeshLoader::MeshData::GetSubMeshCount() const noexcept {
   return subMeshes.size();
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>> MeshLoader::MeshData::GetCombinedData() const {
   std::vector<Vertex> combinedVertices;
   std::vector<uint32_t> combinedIndices;
   uint32_t vertexOffset = 0;
   const size_t totalVertices = std::ranges::fold_left(
      subMeshes | std::ranges::views::transform(&SubMesh::GetVertexCount),
      0UZ, std::plus<>{});
   const size_t totalIndices = std::ranges::fold_left(
      subMeshes | std::ranges::views::transform(&SubMesh::GetIndexCount),
      0UZ, std::plus<>{});
   combinedVertices.reserve(totalVertices);
   combinedIndices.reserve(totalIndices);
   for (const SubMesh& subMesh : subMeshes) {
      // Add vertices
      std::ranges::copy(subMesh.vertices, std::back_inserter(combinedVertices));
      // Add indices with offset using ranges
      auto offsetIndices = subMesh.indices
         | std::views::transform([vertexOffset](uint32_t index) { 
            return index + vertexOffset;
         });
      std::ranges::copy(offsetIndices, std::back_inserter(combinedIndices));
      vertexOffset += static_cast<uint32_t>(subMesh.vertices.size());
   }
   return {std::move(combinedVertices), std::move(combinedIndices)};
}

MeshLoader::MeshData MeshLoader::LoadMesh(std::string_view filepath) {
   // TODO: Load multiple mesh into split meshes based on material
   MeshData meshData;
   meshData.filepath = filepath;
   Assimp::Importer importer;
   // Set processing flags for optimal meshing
   const unsigned int flags =
      aiProcess_Triangulate |
      aiProcess_FlipUVs |
      aiProcess_GenSmoothNormals |
      aiProcess_JoinIdenticalVertices |
      aiProcess_ImproveCacheLocality |
      aiProcess_RemoveRedundantMaterials |
      aiProcess_OptimizeMeshes |
      aiProcess_ValidateDataStructure |
      aiProcess_SortByPType;
   // Setup importer
   const aiScene* scene = importer.ReadFile(std::string{filepath}, flags);
   if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
      std::println("Assimp failed to load sceen: {}",  importer.GetErrorString());
      return meshData;
   }
   // Process all nodes starting from root
   Internal::ProcessNode(scene, scene->mRootNode, meshData.subMeshes);
   return meshData;
}

void MeshLoader::Internal::ProcessNode(const aiScene* scene, const aiNode* node, std::vector<MeshLoader::SubMesh>& subMeshes) {
   // Process all meshes in current node
   for (size_t i = 0; i < node->mNumMeshes; ++i) {
      const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      MeshLoader::SubMesh subMesh = MeshLoader::Internal::ProcessMesh(scene, mesh);
      // Set mesh name from node name if available
      if (node->mName.length > 0) {
         subMesh.name = std::string(node->mName.C_Str()) + "_" + std::to_string(i);
      } else {
         subMesh.name = "SubMesh_" + std::to_string(subMeshes.size());
      }
      subMeshes.push_back(std::move(subMesh));
   }
   // Process all child nodes recursively
   for (size_t i = 0; i < node->mNumChildren; ++i) {
      ProcessNode(scene, node->mChildren[i], subMeshes);
   }
}

MeshLoader::SubMesh MeshLoader::Internal::ProcessMesh(const aiScene* scene, const aiMesh* mesh) {
   MeshLoader::SubMesh subMesh;
   subMesh.vertices.reserve(mesh->mNumVertices);
   subMesh.indices.reserve(mesh->mNumFaces * 3);
   // Load mesh data
   MeshLoader::Internal::ExtractVertexData(mesh, subMesh.vertices);
   MeshLoader::Internal::ExtractIndexData(mesh, subMesh.indices);
   // Set mesh name
   if (mesh->mName.length > 0) {
      subMesh.name = std::string(mesh->mName.C_Str());
   }
   // TODO: Process material values
   // if (mesh->mMaterialIndex >= 0) {
   //     subMesh.materialIndex = mesh->mMaterialIndex;
   // }
   return subMesh;
}

void MeshLoader::Internal::ExtractVertexData(const aiMesh* mesh, std::vector<Vertex>& vertices) {
   vertices.clear();
   vertices.reserve(mesh->mNumVertices);
   for (size_t i = 0; i < mesh->mNumVertices; ++i) {
      Vertex vertex;
      // Position
      const auto& pos = mesh->mVertices[i];
      vertex.position = glm::vec3(pos.x, pos.y, pos.z);
      // Normal, use generated normals
      if (mesh->HasNormals()) {
         const auto& normal = mesh->mNormals[i];
         vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
      } else {
         vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);  // Default up vector
      }
      // Texture coordinates (use first set if available)
      if (mesh->mTextureCoords[0]) {
         const auto& uv = mesh->mTextureCoords[0][i];
         vertex.uv = glm::vec2(uv.x, uv.y);
      } else {
         vertex.uv = glm::vec2(0.0f);
      }
      vertices.emplace_back(std::move(vertex));
   }
}

void MeshLoader::Internal::ExtractIndexData(const aiMesh* mesh, std::vector<uint32_t>& indices) {
   indices.clear();
   indices.reserve(mesh->mNumFaces * 3);
   for (size_t i = 0; i < mesh->mNumFaces; ++i) {
      const aiFace face = mesh->mFaces[i];
      if (face.mNumIndices == 3) {
         for (uint32_t j = 0; j < 3; ++j) {
            const uint32_t index = face.mIndices[j];
            if (index <= UINT32_MAX) {
               indices.push_back(index);
            } else {
               std::println(stderr, "Index overflow: {} exceeds UINT32_MAX", index);
               indices.push_back(0);  // Fallback to first vertex
            }
         }
      }
   }
}

