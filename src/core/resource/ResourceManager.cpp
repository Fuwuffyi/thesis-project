#include "core/resource/ResourceManager.hpp"

#include <mutex>
#include <numeric>
#include <ranges>

ResourceManager::ResourceManager(std::unique_ptr<IResourceFactory> factory)
   :
   m_factory(std::move(factory)),
   m_nextId(1)
{
   if (!m_factory) {
      throw std::invalid_argument("ResourceFactory cannot be null.");
   }
}

ResourceManager::~ResourceManager() {
   UnloadAll();
}

uint64_t ResourceManager::GetNextId() {
   return m_nextId++;
}

template<typename T>
ResourceHandle<T> ResourceManager::RegisterResource(const std::string_view name,
                                                    std::unique_ptr<T> resource, std::string_view filepath) {
   if (!resource || !resource->IsValid()) {
      return ResourceHandle<T>{};
   }
   std::unique_lock lock(m_mutex);
   const std::string nameStr{name};
   // Check if resource with this name already exists
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      // Replace existing resource
      const uint64_t existingId = nameIt->second;
      if (auto resourceIt = m_resources.find(existingId); resourceIt != m_resources.end()) {
         resourceIt->second->resource = std::move(resource);
         resourceIt->second->filepath = filepath;
         return ResourceHandle<T>(existingId);
      }
   }
   // Create new resource entry
   const uint64_t id = GetNextId();
   auto entry = std::make_unique<ResourceEntry>();
   entry->resource = std::move(resource);
   entry->name = nameStr;
   entry->filepath = filepath;
   entry->id = id;
   m_resources[id] = std::move(entry);
   m_nameToId[nameStr] = id;
   return ResourceHandle<T>(id);
}

TextureHandle ResourceManager::LoadTexture(const std::string_view name, const std::string_view filepath,
                                           const bool generateMipmaps, const bool sRGB) {
   auto texture = m_factory->CreateTextureFromFile(filepath, generateMipmaps, sRGB);
   return RegisterResource<ITexture>(name, std::move(texture), filepath);
}

TextureHandle ResourceManager::CreateTexture(const std::string_view name, const ITexture::CreateInfo& info) {
   auto texture = m_factory->CreateTexture(info);
   return RegisterResource<ITexture>(name, std::move(texture));
}

TextureHandle ResourceManager::CreateDepthTexture(const std::string_view name, const uint32_t width, const uint32_t height,
                                                  const ITexture::Format format) {
   auto texture = m_factory->CreateDepthTexture(width, height, format);
   return RegisterResource<ITexture>(name, std::move(texture));
}

TextureHandle ResourceManager::CreateRenderTarget(const std::string_view name, const uint32_t width, const uint32_t height,
                                                  const ITexture::Format format, const uint32_t samples) {
   auto texture = m_factory->CreateRenderTarget(width, height, format, samples);
   return RegisterResource<ITexture>(name, std::move(texture));
}

MeshHandle ResourceManager::LoadMesh(const std::string_view name, const std::vector<Vertex>& vertices,
                                     const std::vector<uint32_t>& indices) {
   auto mesh = m_factory->CreateMesh(vertices, indices);
   return RegisterResource<IMesh>(name, std::move(mesh));
}

MeshHandle ResourceManager::LoadMeshFromFile(const std::string_view name, const std::string_view filepath) {
   auto mesh = m_factory->CreateMeshFromFile(filepath);
   return RegisterResource<IMesh>(name, std::move(mesh), filepath);
}

ITexture* ResourceManager::GetTexture(const TextureHandle& handle) const {
   if (!handle.IsValid()) return nullptr;
   std::shared_lock lock(m_mutex);
   if (auto it = m_resources.find(handle.GetId()); it != m_resources.end()) {
      return static_cast<ITexture*>(it->second->resource.get());
   }
   return nullptr;
}

IMesh* ResourceManager::GetMesh(const MeshHandle& handle) const {
   if (!handle.IsValid()) return nullptr;
   std::shared_lock lock(m_mutex);
   if (auto it = m_resources.find(handle.GetId()); it != m_resources.end()) {
      return static_cast<IMesh*>(it->second->resource.get());
   }
   return nullptr;
}

ITexture* ResourceManager::GetTexture(const std::string_view name) const {
   std::shared_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      if (auto resourceIt = m_resources.find(nameIt->second); resourceIt != m_resources.end()) {
         return static_cast<ITexture*>(resourceIt->second->resource.get());
      }
   }
   return nullptr;
}

IMesh* ResourceManager::GetMesh(const std::string_view name) const {
   std::shared_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      if (auto resourceIt = m_resources.find(nameIt->second); resourceIt != m_resources.end()) {
         return static_cast<IMesh*>(resourceIt->second->resource.get());
      }
   }
   return nullptr;
}

void ResourceManager::RemoveResource(const uint64_t id) {
   if (auto it = m_resources.find(id); it != m_resources.end()) {
      m_nameToId.erase(it->second->name);
      m_resources.erase(it);
   }
}

void ResourceManager::UnloadTexture(const TextureHandle& handle) {
   if (!handle.IsValid()) return;
   std::unique_lock lock(m_mutex);
   RemoveResource(handle.GetId());
}

void ResourceManager::UnloadMesh(const MeshHandle& handle) {
   if (!handle.IsValid()) return;
   std::unique_lock lock(m_mutex);
   RemoveResource(handle.GetId());
}

void ResourceManager::UnloadTexture(const std::string& name) {
   std::unique_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      RemoveResource(nameIt->second);
   }
}

void ResourceManager::UnloadMesh(const std::string& name) {
   std::unique_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      RemoveResource(nameIt->second);
   }
}

void ResourceManager::UnloadAll() {
   std::unique_lock lock(m_mutex);
   m_resources.clear();
   m_nameToId.clear();
}

size_t ResourceManager::GetTotalMemoryUsage() const {
   std::shared_lock lock(m_mutex);
   auto memoryUsages = m_resources
      | std::ranges::views::values
      | std::ranges::views::transform([](const auto& entry) {
         return entry->resource->GetMemoryUsage();
      });
   return std::accumulate(memoryUsages.begin(), memoryUsages.end(), size_t{0});
}

size_t ResourceManager::GetResourceCount() const {
   std::shared_lock lock(m_mutex);
   return m_resources.size();;
}

std::vector<std::pair<ITexture*, std::string>> ResourceManager::GetAllTexturesNamed() {
   std::shared_lock lock(m_mutex);
   std::vector<std::pair<ITexture*, std::string>> textures;
   auto textureEntries = m_resources 
      | std::views::values
      | std::views::filter([](const auto& entry) {
         return entry && entry->resource && 
         entry->resource->GetType() == ResourceType::Texture;
      })
      | std::views::transform([](const auto& entry) {
         return std::make_pair(
            static_cast<ITexture*>(entry->resource.get()),
            entry->name
         );
      });
   std::ranges::copy(textureEntries, std::back_inserter(textures));
   return textures;
}

std::vector<std::pair<IMesh*, std::string>> ResourceManager::GetAllMeshesNamed() {
   std::shared_lock lock(m_mutex);
   std::vector<std::pair<IMesh*, std::string>> meshes;
   auto meshEntries = m_resources 
      | std::views::values
      | std::views::filter([](const auto& entry) {
         return entry && entry->resource && 
         entry->resource->GetType() == ResourceType::Mesh;
      })
      | std::views::transform([](const auto& entry) {
         return std::make_pair(
            static_cast<IMesh*>(entry->resource.get()),
            entry->name
         );
      });
   std::ranges::copy(meshEntries, std::back_inserter(meshes));
   return meshes;
}


template ResourceHandle<ITexture> ResourceManager::RegisterResource<ITexture>(
   std::string_view, std::unique_ptr<ITexture>, std::string_view);
template ResourceHandle<IMesh> ResourceManager::RegisterResource<IMesh>(
   std::string_view, std::unique_ptr<IMesh>, std::string_view);

