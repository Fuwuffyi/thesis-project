#pragma once

#include "core/GraphicsAPI.hpp"

#include "core/resource/IMaterial.hpp"
#include "core/resource/ITexture.hpp"

#include <glm/glm.hpp>
#include <array>
#include <string_view>

class Node;
class IMaterial;
class ITexture;
class RendererComponent;
class ResourceManager;

class MaterialEditor final {
  public:
   explicit constexpr MaterialEditor(ResourceManager* const resourceManager,
                                     const GraphicsAPI api) noexcept
       : m_resourceManager(resourceManager), m_api(api) {}

   MaterialEditor(const MaterialEditor&) = delete;
   MaterialEditor& operator=(const MaterialEditor&) = delete;
   MaterialEditor(MaterialEditor&&) = delete;
   MaterialEditor& operator=(MaterialEditor&&) = delete;
   ~MaterialEditor() = default;

   void DrawMaterialBrowser();
   void DrawMaterialProperties() const;
   void DrawTextureBrowser() const;
   void DrawRendererComponentInspector(Node* const node, RendererComponent* const renderer) const;

   void DrawMaterialCreationDialog();

  private:
   void DrawMaterialParameterEditor(IMaterial* const material) const;
   void DrawTextureSlotEditor(IMaterial* const material, const std::string_view textureName,
                              const std::string_view displayName) const;

   void DrawTexturePreview(const ITexture* const texture,
                           const glm::vec2& size = glm::vec2{64.0f, 64.0f}) const;
   [[nodiscard]] constexpr uint32_t GetTextureId(const ITexture* const texture) const noexcept;

   [[nodiscard]] std::string FindMaterialName(const MaterialHandle& handle) const;
   [[nodiscard]] std::string FindTextureName(const TextureHandle& handle) const;

  private:
   ResourceManager* const m_resourceManager;
   const GraphicsAPI m_api;

   // UI State
   IMaterial* m_selectedMaterial{nullptr};
   std::string m_selectedMaterialName;
   bool m_showMaterialCreation{false};

   // Material creation state
   std::array<char, 256> m_newMaterialName{};
   std::string m_selectedTemplate{"PBR"};
};
