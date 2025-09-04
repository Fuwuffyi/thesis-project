#pragma once

#include "core/resource/IResourceFactory.hpp"
#include "core/resource/ResourceHandle.hpp"

#include "core/resource/MaterialTemplate.hpp"

#include <shared_mutex>
#include <unordered_map>

class ResourceManager final {
  public:
   struct LoadedMeshGroup {
      std::vector<MeshHandle> subMeshes;
      std::vector<size_t> materialIndices;
      std::string filepath;

      bool IsValid() const noexcept;
   };

   explicit ResourceManager(std::unique_ptr<IResourceFactory> factory);
   ~ResourceManager();

   ResourceManager(const ResourceManager&) = delete;
   ResourceManager& operator=(const ResourceManager&) = delete;

   // Texture management
   TextureHandle LoadTexture(const std::string_view name, const std::string_view filepath,
                             const bool generateMipmaps = true, const bool sRGB = true);
   TextureHandle CreateTexture(const std::string_view name, const ITexture::CreateInfo& info);
   TextureHandle CreateTextureColor(const std::string_view name, const ITexture::Format format,
                                    const glm::vec4& color);
   TextureHandle CreateDepthTexture(const std::string_view name, const uint32_t width,
                                    const uint32_t height,
                                    const ITexture::Format format = ITexture::Format::Depth32F);
   TextureHandle CreateRenderTarget(const std::string_view name, const uint32_t width,
                                    const uint32_t height,
                                    const ITexture::Format format = ITexture::Format::RGBA8,
                                    const uint32_t samples = 1);
   // Material management
   MaterialHandle CreateMaterial(const std::string_view name, const std::string_view templateName);
   // Mesh management
   MeshHandle LoadMesh(const std::string_view name, const std::vector<Vertex>& vertices,
                       const std::vector<uint32_t>& indices);
   LoadedMeshGroup LoadMeshFromFile(const std::string_view name, const std::string_view filepath);
   MeshHandle LoadSingleMeshFromFile(const std::string_view name, const std::string_view filepath);
   // Resource access
   ITexture* GetTexture(const TextureHandle& handle) const;
   IMaterial* GetMaterial(const MaterialHandle& handle) const;
   IMesh* GetMesh(const MeshHandle& handle) const;
   // Resource access by name
   ITexture* GetTexture(const std::string_view name) const;
   IMaterial* GetMaterial(const std::string_view name) const;
   IMesh* GetMesh(const std::string_view name) const;
   TextureHandle GetTextureHandle(const std::string_view name) const;
   MaterialHandle GetMaterialHandle(const std::string_view name) const;
   MeshHandle GetMeshHandle(const std::string_view name) const;
   const LoadedMeshGroup* GetMeshGroup(const std::string_view name) const;
   // Resource management
   void UnloadTexture(const TextureHandle& handle);
   void UnloadMaterial(const MaterialHandle& handle);
   void UnloadMesh(const MeshHandle& handle);
   void UnloadTexture(const std::string_view name);
   void UnloadMaterial(const std::string_view name);
   void UnloadMesh(const std::string_view name);
   void UnloadMeshGroup(const std::string_view name);
   // Utility methods
   void UnloadAll();
   size_t GetTotalMemoryUsage() const;
   size_t GetResourceCount() const;
   std::vector<std::pair<ITexture*, std::string>> GetAllTexturesNamed();
   std::vector<std::pair<IMaterial*, std::string>> GetAllMaterialsNamed();
   std::vector<std::pair<const MaterialTemplate&, std::string>> GetAllMaterialTemplatesNamed();
   std::vector<std::pair<IMesh*, std::string>> GetAllMeshesNamed();

  private:
   uint64_t GetNextId();
   template <typename T>
   ResourceHandle<T> RegisterResource(const std::string_view name, std::unique_ptr<T> resource,
                                      const std::string_view filepath = {});

   void RemoveResource(const uint64_t id);

   void SetupMaterialTemplates();

  private:
   struct ResourceEntry {
      std::unique_ptr<IResource> resource;
      std::string name;
      std::string filepath;
      uint64_t id;
      size_t refCount;
   };

   std::unique_ptr<IResourceFactory> m_factory;
   std::unordered_map<uint64_t, std::unique_ptr<ResourceEntry>> m_resources;
   std::unordered_map<std::string, std::unique_ptr<MaterialTemplate>> m_materialTemplates;
   std::unordered_map<std::string, uint64_t> m_nameToId;
   std::unordered_map<std::string, LoadedMeshGroup> m_meshGroups;

   mutable std::shared_mutex m_mutex;
   uint64_t m_nextId;
};
