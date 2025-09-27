#include "core/resource/MeshLoader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <print>
#include <ranges>

size_t MeshLoader::MeshData::GetVertexCount() const noexcept { return vertices.size(); }

size_t MeshLoader::MeshData::GetIndexCount() const noexcept { return indices.size(); }

bool MeshLoader::MeshData::IsEmpty() const noexcept { return vertices.empty() || indices.empty(); }

bool MeshLoader::SceneData::IsEmpty() const noexcept { return meshes.empty(); }

size_t MeshLoader::SceneData::GetMeshCount() const noexcept { return meshes.size(); }

MeshLoader::SceneData MeshLoader::LoadScene(const std::string_view filepath) {
   SceneData sceneData;
   sceneData.filepath = filepath;
   Assimp::Importer importer;
   const uint32_t flags = aiProcess_Triangulate | aiProcess_FlipUVs |
                              aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
                              aiProcess_ImproveCacheLocality | aiProcess_RemoveRedundantMaterials |
                              aiProcess_ValidateDataStructure | aiProcess_SortByPType;
   const aiScene* scene = importer.ReadFile(std::string{filepath}, flags);
   if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
      std::println("Assimp failed to load scene: {}", importer.GetErrorString());
      return sceneData;
   }
   // Initialize root node
   sceneData.rootNode.name = scene->mRootNode->mName.length > 0
                                ? std::string(scene->mRootNode->mName.C_Str())
                                : "RootNode";
   sceneData.rootNode.transform = ConvertMatrix(scene->mRootNode->mTransformation);
   // Process the scene hierarchy
   ProcessNode(scene, scene->mRootNode, sceneData, sceneData.rootNode);
   return sceneData;
}

MeshLoader::MeshData MeshLoader::LoadSingleMesh(std::string_view filepath) {
   Assimp::Importer importer;
   const uint32_t flags = aiProcess_Triangulate | aiProcess_FlipUVs |
                              aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
                              aiProcess_ImproveCacheLocality | aiProcess_RemoveRedundantMaterials |
                              aiProcess_OptimizeMeshes | aiProcess_ValidateDataStructure |
                              aiProcess_SortByPType | aiProcess_PreTransformVertices;
   const aiScene* scene = importer.ReadFile(std::string{filepath}, flags);
   if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode ||
       scene->mNumMeshes == 0) {
      std::println("Assimp failed to load mesh: {}", importer.GetErrorString());
      return MeshData{};
   }
   // Combine all meshes into one (without baking transforms)
   MeshData combinedMesh;
   combinedMesh.name = std::string(filepath.substr(filepath.find_last_of("/\\") + 1));
   combinedMesh.transform = glm::mat4(1.0f);
   uint32_t vertexOffset = 0;
   // Calculate total sizes
   size_t totalVertices = 0;
   size_t totalIndices = 0;
   for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
      totalVertices += scene->mMeshes[i]->mNumVertices;
      totalIndices += scene->mMeshes[i]->mNumFaces * 3;
   }
   combinedMesh.vertices.reserve(totalVertices);
   combinedMesh.indices.reserve(totalIndices);
   // Combine all meshes
   for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
      const aiMesh* mesh = scene->mMeshes[i];
      // Extract vertices
      std::vector<Vertex> meshVertices;
      ExtractVertexData(mesh, meshVertices);
      std::ranges::copy(meshVertices, std::back_inserter(combinedMesh.vertices));
      // Extract indices with offset
      std::vector<uint32_t> meshIndices;
      ExtractIndexData(mesh, meshIndices);
      const auto offsetIndices = meshIndices | std::views::transform([vertexOffset](uint32_t index) {
                              return index + vertexOffset;
                           });
      std::ranges::copy(offsetIndices, std::back_inserter(combinedMesh.indices));
      vertexOffset += static_cast<uint32_t>(meshVertices.size());
   }
   return combinedMesh;
}

void MeshLoader::ProcessNode(const aiScene* scene, const aiNode* node, SceneData& sceneData,
                             SceneNode& parentNode) {
   // Process all meshes in current node
   for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
      const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      MeshData meshData = ProcessMesh(scene, mesh);
      // Set mesh name from node or mesh name
      if (node->mName.length > 0) {
         meshData.name = std::string(node->mName.C_Str());
         if (node->mNumMeshes > 1) {
            meshData.name += "_Mesh_" + std::to_string(i);
         }
      } else if (mesh->mName.length > 0) {
         meshData.name = std::string(mesh->mName.C_Str());
      } else {
         meshData.name = "Mesh_" + std::to_string(sceneData.meshes.size());
      }
      // Store transform
      meshData.transform = ConvertMatrix(node->mTransformation);
      // Add mesh to scene and reference in current node
      const size_t meshIndex = sceneData.meshes.size();
      sceneData.meshes.push_back(std::move(meshData));
      parentNode.meshIndices.push_back(meshIndex);
   }
   // Process child nodes
   parentNode.children.reserve(node->mNumChildren);
   for (uint32_t i = 0; i < node->mNumChildren; ++i) {
      const aiNode* childNode = node->mChildren[i];
      SceneNode sceneChild;
      sceneChild.name = childNode->mName.length > 0 ? std::string(childNode->mName.C_Str())
                                                    : "Node_" + std::to_string(i);
      sceneChild.transform = ConvertMatrix(childNode->mTransformation);
      ProcessNode(scene, childNode, sceneData, sceneChild);
      parentNode.children.push_back(std::move(sceneChild));
   }
}

MeshLoader::MeshData MeshLoader::ProcessMesh(const aiScene* scene, const aiMesh* mesh) {
   MeshData meshData;
   meshData.vertices.reserve(mesh->mNumVertices);
   meshData.indices.reserve(mesh->mNumFaces * 3);
   // Extract vertex and index data
   ExtractVertexData(mesh, meshData.vertices);
   ExtractIndexData(mesh, meshData.indices);
   // Set mesh name
   if (mesh->mName.length > 0) {
      meshData.name = std::string(mesh->mName.C_Str());
   }
   meshData.materialIndex = mesh->mMaterialIndex;
   return meshData;
}

void MeshLoader::ExtractVertexData(const aiMesh* mesh, std::vector<Vertex>& vertices) {
   vertices.clear();
   vertices.reserve(mesh->mNumVertices);
   for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
      Vertex vertex;
      // Position
      const auto& pos = mesh->mVertices[i];
      vertex.position = glm::vec3(pos.x, pos.y, pos.z);
      // Normal
      if (mesh->HasNormals()) {
         const auto& normal = mesh->mNormals[i];
         vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
      } else {
         vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default up vector
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

void MeshLoader::ExtractIndexData(const aiMesh* mesh, std::vector<uint32_t>& indices) {
   indices.clear();
   indices.reserve(mesh->mNumFaces * 3);
   for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
      const aiFace face = mesh->mFaces[i];
      if (face.mNumIndices == 3) {
         for (uint32_t j = 0; j < 3; ++j) {
            const uint32_t index = face.mIndices[j];
            if (index <= UINT32_MAX) {
               indices.push_back(static_cast<uint32_t>(index));
            } else {
               std::println(stderr, "Index overflow: {} exceeds UINT32_MAX", index);
               indices.push_back(0); // Fallback to first vertex
            }
         }
      }
   }
}

glm::mat4 MeshLoader::ConvertMatrix(const auto& aiMat) {
   return glm::mat4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
                    aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3, aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
}
