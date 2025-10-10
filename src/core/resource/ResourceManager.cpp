#include "core/resource/ResourceManager.hpp"

#include "core/resource/MeshLoader.hpp"

#include <mutex>
#include <numeric>
#include <ranges>
#include <stdexcept>

ResourceManager::ResourceManager(std::unique_ptr<IResourceFactory> factory)
    : m_factory(std::move(factory)), m_nextId(1) {
   if (!m_factory) {
      throw std::invalid_argument("ResourceFactory cannot be null.");
   }
   SetupMaterialTemplates();
}

ResourceManager::~ResourceManager() { UnloadAll(); }

uint64_t ResourceManager::GetNextId() { return m_nextId++; }

template <typename T>
ResourceHandle<T> ResourceManager::RegisterResource(const std::string_view name,
                                                    std::unique_ptr<T> resource,
                                                    std::string_view filepath) {
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

TextureHandle ResourceManager::LoadTexture(const std::string_view name,
                                           const std::string_view filepath,
                                           const bool generateMipmaps, const bool sRGB) {
   auto texture = m_factory->CreateTextureFromFile(filepath, generateMipmaps, sRGB);
   return RegisterResource<ITexture>(name, std::move(texture), filepath);
}

TextureHandle ResourceManager::CreateTexture(const std::string_view name,
                                             const ITexture::CreateInfo& info) {
   auto texture = m_factory->CreateTexture(info);
   return RegisterResource<ITexture>(name, std::move(texture));
}

TextureHandle ResourceManager::CreateTextureColor(const std::string_view name,
                                                  const ITexture::Format format,
                                                  const glm::vec4& color) {
   auto texture = m_factory->CreateTextureColor(format, color);
   return RegisterResource<ITexture>(name, std::move(texture));
}

TextureHandle ResourceManager::CreateDepthTexture(const std::string_view name, const uint32_t width,
                                                  const uint32_t height,
                                                  const ITexture::Format format) {
   auto texture = m_factory->CreateDepthTexture(width, height, format);
   return RegisterResource<ITexture>(name, std::move(texture));
}

TextureHandle ResourceManager::CreateRenderTarget(const std::string_view name, const uint32_t width,
                                                  const uint32_t height,
                                                  const ITexture::Format format,
                                                  const uint32_t samples) {
   auto texture = m_factory->CreateRenderTarget(width, height, format, samples);
   return RegisterResource<ITexture>(name, std::move(texture));
}

MaterialHandle ResourceManager::CreateMaterial(const std::string_view name,
                                               const std::string_view templateName) {
   if (!m_materialTemplates.contains(std::string{templateName}))
      throw std::runtime_error("The material template " + std::string{templateName} +
                               " does not exist.");
   const MaterialTemplate* templ = m_materialTemplates.at(std::string{templateName}).get();
   auto material = m_factory->CreateMaterial(*templ);
   return RegisterResource<IMaterial>(name, std::move(material));
}

MeshHandle ResourceManager::LoadMesh(const std::string_view name,
                                     const std::vector<Vertex>& vertices,
                                     const std::vector<uint32_t>& indices) {
   auto mesh = m_factory->CreateMesh(vertices, indices);
   return RegisterResource<IMesh>(name, std::move(mesh));
}

MeshHandle ResourceManager::LoadSingleMeshFromFile(const std::string_view name,
                                                   const std::string_view filepath) {
   const MeshLoader::MeshData meshData = MeshLoader::LoadSingleMesh(filepath);
   if (meshData.IsEmpty()) {
      return MeshHandle{};
   }
   auto mesh = m_factory->CreateMesh(meshData.vertices, meshData.indices);
   return RegisterResource<IMesh>(name, std::move(mesh), filepath);
}

MeshLoader::SceneData ResourceManager::LoadSceneData(const std::string_view filepath) {
   return MeshLoader::LoadScene(filepath);
}

ITexture* ResourceManager::GetTexture(const TextureHandle& handle) const {
   if (!handle.IsValid())
      return nullptr;
   std::shared_lock lock(m_mutex);
   if (auto it = m_resources.find(handle.GetId()); it != m_resources.end()) {
      return static_cast<ITexture*>(it->second->resource.get());
   }
   return nullptr;
}

IMaterial* ResourceManager::GetMaterial(const MaterialHandle& handle) const {
   if (!handle.IsValid())
      return nullptr;
   std::shared_lock lock(m_mutex);
   if (auto it = m_resources.find(handle.GetId()); it != m_resources.end()) {
      return static_cast<IMaterial*>(it->second->resource.get());
   }
   return nullptr;
}

IMesh* ResourceManager::GetMesh(const MeshHandle& handle) const {
   if (!handle.IsValid())
      return nullptr;
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

IMaterial* ResourceManager::GetMaterial(const std::string_view name) const {
   std::shared_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      if (auto resourceIt = m_resources.find(nameIt->second); resourceIt != m_resources.end()) {
         return static_cast<IMaterial*>(resourceIt->second->resource.get());
      }
   }
   return nullptr;
}

MaterialTemplate* ResourceManager::GetMaterialTemplate(const std::string_view name) const {
   std::shared_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto templateIt = m_materialTemplates.find(nameStr);
       templateIt != m_materialTemplates.end()) {
      return static_cast<MaterialTemplate*>(templateIt->second.get());
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

TextureHandle ResourceManager::GetTextureHandle(const std::string_view name) const {
   std::shared_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      if (auto resourceIt = m_resources.find(nameIt->second); resourceIt != m_resources.end()) {
         return TextureHandle(resourceIt->second->id);
      }
   }
   return TextureHandle{};
}

MaterialHandle ResourceManager::GetMaterialHandle(const std::string_view name) const {
   std::shared_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      if (auto resourceIt = m_resources.find(nameIt->second); resourceIt != m_resources.end()) {
         return MaterialHandle(resourceIt->second->id);
      }
   }
   return MaterialHandle{};
}

MeshHandle ResourceManager::GetMeshHandle(const std::string_view name) const {
   std::shared_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      if (auto resourceIt = m_resources.find(nameIt->second); resourceIt != m_resources.end()) {
         return MeshHandle(resourceIt->second->id);
      }
   }
   return MeshHandle{};
}

void ResourceManager::RemoveResource(const uint64_t id) {
   if (auto it = m_resources.find(id); it != m_resources.end()) {
      m_nameToId.erase(it->second->name);
      m_resources.erase(it);
   }
}

void ResourceManager::UnloadTexture(const TextureHandle& handle) {
   if (!handle.IsValid())
      return;
   std::unique_lock lock(m_mutex);
   RemoveResource(handle.GetId());
}

void ResourceManager::UnloadMaterial(const MaterialHandle& handle) {
   if (!handle.IsValid())
      return;
   std::unique_lock lock(m_mutex);
   RemoveResource(handle.GetId());
}

void ResourceManager::UnloadMesh(const MeshHandle& handle) {
   if (!handle.IsValid())
      return;
   std::unique_lock lock(m_mutex);
   RemoveResource(handle.GetId());
}

void ResourceManager::UnloadTexture(const std::string_view name) {
   std::unique_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      RemoveResource(nameIt->second);
   }
}

void ResourceManager::UnloadMaterial(const std::string_view name) {
   std::unique_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      RemoveResource(nameIt->second);
   }
}

void ResourceManager::UnloadMesh(const std::string_view name) {
   std::unique_lock lock(m_mutex);
   const std::string nameStr{name};
   if (auto nameIt = m_nameToId.find(nameStr); nameIt != m_nameToId.end()) {
      RemoveResource(nameIt->second);
   }
}

void ResourceManager::UnloadAll() {
   std::unique_lock lock(m_mutex);
   m_resources.clear();
   m_materialTemplates.clear();
   m_nameToId.clear();
}

size_t ResourceManager::GetTotalMemoryUsage() const {
   std::shared_lock lock(m_mutex);
   auto memoryUsages = m_resources | std::ranges::views::values |
                       std::ranges::views::transform(
                          [](const auto& entry) { return entry->resource->GetMemoryUsage(); });
   return std::accumulate(memoryUsages.begin(), memoryUsages.end(), size_t{0});
}

size_t ResourceManager::GetResourceCount() const {
   std::shared_lock lock(m_mutex);
   return m_resources.size();
}

std::vector<std::pair<ITexture*, std::string>> ResourceManager::GetAllTexturesNamed() {
   std::shared_lock lock(m_mutex);
   std::vector<std::pair<ITexture*, std::string>> textures;
   auto textureEntries =
      m_resources | std::views::values | std::views::filter([](const auto& entry) {
         return entry && entry->resource && entry->resource->GetType() == ResourceType::Texture;
      }) |
      std::views::transform([](const auto& entry) {
         return std::make_pair(static_cast<ITexture*>(entry->resource.get()), entry->name);
      });
   std::ranges::copy(textureEntries, std::back_inserter(textures));
   return textures;
}

std::vector<std::pair<IMaterial*, std::string>> ResourceManager::GetAllMaterialsNamed() {
   std::shared_lock lock(m_mutex);
   std::vector<std::pair<IMaterial*, std::string>> materials;
   auto materialEntries =
      m_resources | std::views::values | std::views::filter([](const auto& entry) {
         return entry && entry->resource && entry->resource->GetType() == ResourceType::Material;
      }) |
      std::views::transform([](const auto& entry) {
         return std::make_pair(static_cast<IMaterial*>(entry->resource.get()), entry->name);
      });
   std::ranges::copy(materialEntries, std::back_inserter(materials));
   return materials;
}

std::vector<std::pair<const MaterialTemplate&, std::string>>
ResourceManager::GetAllMaterialTemplatesNamed() {
   std::shared_lock lock(m_mutex);
   std::vector<std::pair<const MaterialTemplate&, std::string>> templates;
   templates.reserve(m_materialTemplates.size());
   for (const auto& [name, templatePtr] : m_materialTemplates) {
      if (templatePtr && templatePtr->IsFinalized()) {
         templates.emplace_back(*templatePtr, name);
      }
   }
   return templates;
}

std::vector<std::pair<IMesh*, std::string>> ResourceManager::GetAllMeshesNamed() {
   std::shared_lock lock(m_mutex);
   std::vector<std::pair<IMesh*, std::string>> meshes;
   auto meshEntries =
      m_resources | std::views::values | std::views::filter([](const auto& entry) {
         return entry && entry->resource && entry->resource->GetType() == ResourceType::Mesh;
      }) |
      std::views::transform([](const auto& entry) {
         return std::make_pair(static_cast<IMesh*>(entry->resource.get()), entry->name);
      });
   std::ranges::copy(meshEntries, std::back_inserter(meshes));
   return meshes;
}

void ResourceManager::SetupMaterialTemplates() {
   // Create default textures
   const auto defAlbedo =
      CreateTextureColor("default_albedo", ITexture::Format::RGBA8, glm::vec4(1.0f));
   const auto defNormal = CreateTextureColor("default_normal", ITexture::Format::RGB8,
                                             glm::vec4(0.5f, 0.5f, 1.0f, 0.0f));
   const auto defRough =
      CreateTextureColor("default_roughness", ITexture::Format::R8, glm::vec4(1.0f));
   const auto defMetal =
      CreateTextureColor("default_metallic", ITexture::Format::R8, glm::vec4(0.0f));
   const auto defAO = CreateTextureColor("default_ao", ITexture::Format::R8, glm::vec4(1.0f));
   std::unique_ptr<MaterialTemplate> pbrTemplate = std::make_unique<MaterialTemplate>("PBR");
   // PBR Params
   pbrTemplate->AddParameter("albedo", ParameterDescriptor::Type::Vec3, glm::vec3(1.0f));
   pbrTemplate->AddParameter("metallic", ParameterDescriptor::Type::Float, 1.0f);
   pbrTemplate->AddParameter("roughness", ParameterDescriptor::Type::Float, 1.0f);
   pbrTemplate->AddParameter("ao", ParameterDescriptor::Type::Float, 1.0f);
   // PBR Textures
   pbrTemplate->AddTexture("albedoTexture", 0, "albedoSampler", defAlbedo);
   pbrTemplate->AddTexture("normalTexture", 1, "normalSampler", defNormal);
   pbrTemplate->AddTexture("roughnessTexture", 2, "roughnessSampler", defRough);
   pbrTemplate->AddTexture("metallicTexture", 3, "metallicSampler", defMetal);
   pbrTemplate->AddTexture("aoTexture", 4, "aoSampler", defAO);
   // Add the PBR material template
   pbrTemplate->Finalize();
   m_materialTemplates[std::string{pbrTemplate->GetName()}] = std::move(pbrTemplate);
}

template ResourceHandle<ITexture> ResourceManager::RegisterResource<ITexture>(
   std::string_view, std::unique_ptr<ITexture>, std::string_view);
template ResourceHandle<IMaterial> ResourceManager::RegisterResource<IMaterial>(
   std::string_view, std::unique_ptr<IMaterial>, std::string_view);
template ResourceHandle<IMesh> ResourceManager::RegisterResource<IMesh>(std::string_view,
                                                                        std::unique_ptr<IMesh>,
                                                                        std::string_view);
