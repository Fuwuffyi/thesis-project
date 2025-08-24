#include "ResourceManager.hpp"

ResourceManager::ResourceManager(std::unique_ptr<IResourceFactory> factory)
   :
   m_factory(std::move(factory)),
   m_nextId(1)
{}

ResourceManager::~ResourceManager() {
   UnloadAll();
}

uint64_t ResourceManager::GetNextId() {
   return m_nextId++;
}

template<typename T>
ResourceHandle<T> ResourceManager::RegisterResource(const std::string& name,
                                                    std::unique_ptr<T> resource, const std::string& filepath) {
   std::lock_guard<std::mutex> lock(m_mutex);
   // Check if resource with this name already exists
   const auto nameIt = m_nameToId.find(name);
   if (nameIt != m_nameToId.end()) {
      // Replace existing resource
      const uint64_t existingId = nameIt->second;
      auto& entry = m_resources[existingId];
      entry->resource = std::move(resource);
      entry->filepath = filepath;
      return ResourceHandle<T>(existingId);
   }
   // Create new resource entry
   const uint64_t id = GetNextId();
   auto entry = std::make_unique<ResourceEntry>();
   entry->resource = std::move(resource);
   entry->name = name;
   entry->filepath = filepath;
   entry->id = id;
   entry->refCount = 0;
   m_resources[id] = std::move(entry);
   m_nameToId[name] = id;
   return ResourceHandle<T>(id);
}

TextureHandle ResourceManager::LoadTexture(const std::string& name, const std::string& filepath,
                                           const bool generateMipmaps, const bool sRGB) {
   auto texture = m_factory->CreateTextureFromFile(filepath, generateMipmaps, sRGB);
   if (!texture || !texture->IsValid()) {
      return TextureHandle();
   }
   return RegisterResource<ITexture>(name, std::move(texture), filepath);
}

TextureHandle ResourceManager::CreateTexture(const std::string& name, const ITexture::CreateInfo& info) {
   auto texture = m_factory->CreateTexture(info);
   if (!texture || !texture->IsValid()) {
      return TextureHandle();
   }
   return RegisterResource<ITexture>(name, std::move(texture));
}

TextureHandle ResourceManager::CreateDepthTexture(const std::string& name, const uint32_t width, const uint32_t height,
                                                  const ITexture::Format format) {
   auto texture = m_factory->CreateDepthTexture(width, height, format);
   if (!texture || !texture->IsValid()) {
      return TextureHandle();
   }
   return RegisterResource<ITexture>(name, std::move(texture));
}

TextureHandle ResourceManager::CreateRenderTarget(const std::string& name, const uint32_t width, const uint32_t height,
                                                  const ITexture::Format format, const uint32_t samples) {
   auto texture = m_factory->CreateRenderTarget(width, height, format, samples);
   if (!texture || !texture->IsValid()) {
      return TextureHandle();
   }
   return RegisterResource<ITexture>(name, std::move(texture));
}

MeshHandle ResourceManager::LoadMesh(const std::string& name, const std::vector<Vertex>& vertices,
                                     const std::vector<uint16_t>& indices) {
   auto mesh = m_factory->CreateMesh(vertices, indices);
   if (!mesh || !mesh->IsValid()) {
      return MeshHandle();
   }
   return RegisterResource<IMesh>(name, std::move(mesh));
}

MeshHandle ResourceManager::LoadMeshFromFile(const std::string& name, const std::string& filepath) {
   auto mesh = m_factory->CreateMeshFromFile(filepath);
   if (!mesh || !mesh->IsValid()) {
      return MeshHandle();
   }
   return RegisterResource<IMesh>(name, std::move(mesh), filepath);
}

ITexture* ResourceManager::GetTexture(const TextureHandle& handle) {
   if (!handle.IsValid()) return nullptr;
   std::lock_guard<std::mutex> lock(m_mutex);
   const auto it = m_resources.find(handle.GetId());
   if (it != m_resources.end()) {
      return static_cast<ITexture*>(it->second->resource.get());
   }
   return nullptr;
}

IMesh* ResourceManager::GetMesh(const MeshHandle& handle) {
   if (!handle.IsValid()) return nullptr;
   std::lock_guard<std::mutex> lock(m_mutex);
   const auto it = m_resources.find(handle.GetId());
   if (it != m_resources.end()) {
      return static_cast<IMesh*>(it->second->resource.get());
   }
   return nullptr;
}

ITexture* ResourceManager::GetTexture(const std::string& name) {
   std::lock_guard<std::mutex> lock(m_mutex);
   const auto nameIt = m_nameToId.find(name);
   if (nameIt != m_nameToId.end()) {
      const auto resourceIt = m_resources.find(nameIt->second);
      if (resourceIt != m_resources.end()) {
         return static_cast<ITexture*>(resourceIt->second->resource.get());
      }
   }
   return nullptr;
}

IMesh* ResourceManager::GetMesh(const std::string& name) {
   std::lock_guard<std::mutex> lock(m_mutex);
   const auto nameIt = m_nameToId.find(name);
   if (nameIt != m_nameToId.end()) {
      const auto resourceIt = m_resources.find(nameIt->second);
      if (resourceIt != m_resources.end()) {
         return static_cast<IMesh*>(resourceIt->second->resource.get());
      }
   }
   return nullptr;
}

void ResourceManager::RemoveResource(const uint64_t id) {
   const auto it = m_resources.find(id);
   if (it != m_resources.end()) {
      m_nameToId.erase(it->second->name);
      m_resources.erase(it);
   }
}

void ResourceManager::UnloadTexture(const TextureHandle& handle) {
   if (!handle.IsValid()) return;
   std::lock_guard<std::mutex> lock(m_mutex);
   RemoveResource(handle.GetId());
}

void ResourceManager::UnloadMesh(const MeshHandle& handle) {
   if (!handle.IsValid()) return;
   std::lock_guard<std::mutex> lock(m_mutex);
   RemoveResource(handle.GetId());
}

void ResourceManager::UnloadTexture(const std::string& name) {
   std::lock_guard<std::mutex> lock(m_mutex);
   const auto nameIt = m_nameToId.find(name);
   if (nameIt != m_nameToId.end()) {
      RemoveResource(nameIt->second);
   }
}

void ResourceManager::UnloadMesh(const std::string& name) {
   std::lock_guard<std::mutex> lock(m_mutex);
   const auto nameIt = m_nameToId.find(name);
   if (nameIt != m_nameToId.end()) {
      RemoveResource(nameIt->second);
   }
}

void ResourceManager::UnloadAll() {
   std::lock_guard<std::mutex> lock(m_mutex);
   m_resources.clear();
   m_nameToId.clear();
}

size_t ResourceManager::GetTotalMemoryUsage() const {
   std::lock_guard<std::mutex> lock(m_mutex);
   size_t total = 0;
   for (const auto& [id, entry] : m_resources) {
      total += entry->resource->GetMemoryUsage();
   }
   return total;
}

size_t ResourceManager::GetResourceCount() const {
   std::lock_guard<std::mutex> lock(m_mutex);
   return m_resources.size();
}

std::vector<std::pair<ITexture*, std::string>> ResourceManager::GetAllTexturesNamed() {
   std::lock_guard<std::mutex> lock(m_mutex);
   std::vector<std::pair<ITexture*, std::string>> textures;
   textures.reserve(m_resources.size());
   for (auto& [id, entry] : m_resources) {
      if (entry && entry->resource) {
         if (auto* tex = dynamic_cast<ITexture*>(entry->resource.get())) {
            textures.emplace_back(tex, entry->name);
         }
      }
   }
   return textures;
}

std::vector<std::pair<IMesh*, std::string>> ResourceManager::GetAllMeshesNamed() {
   std::lock_guard<std::mutex> lock(m_mutex);
   std::vector<std::pair<IMesh*, std::string>> meshes;
   meshes.reserve(m_resources.size());
   for (auto& [id, entry] : m_resources) {
      if (entry && entry->resource) {
         if (auto* mesh = dynamic_cast<IMesh*>(entry->resource.get())) {
            meshes.emplace_back(mesh, entry->name);
         }
      }
   }
   return meshes;
}

