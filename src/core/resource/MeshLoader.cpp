#include "MeshLoader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <print>
#include <algorithm>

bool MeshLoader::MeshData::IsEmpty() const {
   return subMeshes.empty();
}

size_t MeshLoader::MeshData::GetSubMeshCount() const {
   return subMeshes.size();
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>> MeshLoader::MeshData::GetCombinedData() const {
   std::vector<Vertex> combinedVertices;
   std::vector<uint32_t> combinedIndices;
   uint32_t vertexOffset = 0;
   for (const auto& subMesh : subMeshes) {
      // Add vertices
      combinedVertices.insert(combinedVertices.end(), 
                              subMesh.vertices.begin(), 
                              subMesh.vertices.end());
      // Add indices with offset
      for (const uint32_t index : subMesh.indices) {
         combinedIndices.push_back(index + vertexOffset);
      }
      vertexOffset += static_cast<uint32_t>(subMesh.vertices.size());
   }
   return std::make_pair(std::move(combinedVertices), std::move(combinedIndices));
}

MeshLoader::MeshData MeshLoader::LoadMesh(const std::string& filepath) {
   MeshData meshData;
   meshData.filepath = filepath;
   Assimp::Importer importer;
   // Set processing flags for optimal mesh loading
   const unsigned int flags = aiProcess_Triangulate |
      aiProcess_FlipUVs |
      // aiProcess_GenNormals |
      aiProcess_GenSmoothNormals |
      aiProcess_JoinIdenticalVertices |
      aiProcess_ImproveCacheLocality |
      aiProcess_RemoveRedundantMaterials |
      aiProcess_OptimizeMeshes |
      aiProcess_ValidateDataStructure;
   // Setup importer
   const aiScene* scene = importer.ReadFile(filepath, flags);
   if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
      std::println("ASSIMP Error: {}",  importer.GetErrorString());
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
   vertices.reserve(mesh->mNumVertices);
   for (size_t i = 0; i < mesh->mNumVertices; ++i) {
      Vertex vertex;
      // Position
      vertex.position.x = mesh->mVertices[i].x;
      vertex.position.y = mesh->mVertices[i].y;
      vertex.position.z = mesh->mVertices[i].z;
      // Normal, use generated normals
      if (mesh->HasNormals()) {
         vertex.normal.x = mesh->mNormals[i].x;
         vertex.normal.y = mesh->mNormals[i].y;
         vertex.normal.z = mesh->mNormals[i].z;
      } else {
         vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
      }
      // Texture coordinates (use first set if available)
      if (mesh->mTextureCoords[0]) {
         vertex.uv.x = mesh->mTextureCoords[0][i].x;
         vertex.uv.y = mesh->mTextureCoords[0][i].y;
      } else {
         vertex.uv = glm::vec2(0.0f, 0.0f);
      }
      vertices.push_back(vertex);
   }
}

void MeshLoader::Internal::ExtractIndexData(const aiMesh* mesh, std::vector<uint32_t>& indices) {
   indices.reserve(mesh->mNumFaces * 3);
   for (size_t i = 0; i < mesh->mNumFaces; ++i) {
      const aiFace face = mesh->mFaces[i];
      if (face.mNumIndices == 3) {
         for (size_t j = 0; j < face.mNumIndices; ++j) {
            // Check for uint16_t overflow
            if (face.mIndices[j] > UINT32_MAX) {
               std::println("Warning: Index overflow, mesh too large for uint32_t indices.");
               indices.push_back(0);
            } else {
               indices.push_back(static_cast<uint32_t>(face.mIndices[j]));
            }
         }
      }
   }
}

