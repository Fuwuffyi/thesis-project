#pragma once

#include <string>

#include <glm/glm.hpp>

#include "core/GraphicsAPI.hpp"

class Node;
class IMaterial;
class ITexture;
class RendererComponent;
class ResourceManager;

class MaterialEditor final {
  public:
   explicit MaterialEditor(ResourceManager* const resourceManager, const GraphicsAPI api) noexcept;

   MaterialEditor(const MaterialEditor&) = delete;
   MaterialEditor& operator=(const MaterialEditor&) = delete;
   MaterialEditor(MaterialEditor&&) = delete;
   MaterialEditor& operator=(MaterialEditor&&) = delete;

   // Main material browser/editor window
   void DrawMaterialBrowser();
   void DrawMaterialProperties();
   void DrawTextureBrowser();
   void DrawRendererComponentInspector(Node* const node, RendererComponent* const renderer);

   // Handle drag/drop operations
   void HandleMaterialDragDrop(RendererComponent* const renderer,
                               const size_t subMeshIndex = SIZE_MAX);
   void HandleTextureDragDrop(IMaterial* const material, const std::string_view textureSlot);

   // Material creation dialog
   void DrawMaterialCreationDialog();

  private:
   // Helper functions
   void DrawMaterialParameterEditor(IMaterial* const material);
   void DrawTextureSlotEditor(IMaterial* const material, const std::string_view textureName,
                              const std::string_view displayName);
   [[nodiscard]] bool DrawDragDropSource(const std::string_view type, const std::string_view name,
                                         const std::string_view displayText);
   [[nodiscard]] bool DrawDragDropTarget(const std::string_view type);
   [[nodiscard]] std::string GetDragDropPayload() const;

   // Texture preview helpers
   void DrawTexturePreview(const ITexture* const texture,
                           const glm::vec2& size = glm::vec2(64, 64)) const;
   [[nodiscard]] uint32_t GetTextureId(const ITexture* const texture) const noexcept;

  private:
   ResourceManager* const m_resourceManager;
   const GraphicsAPI m_api;

   // UI State
   IMaterial* m_selectedMaterial{nullptr};
   std::string m_selectedMaterialName;
   bool m_showMaterialBrowser{true};
   bool m_showTextureBrowser{true};
   bool m_showMaterialCreation{false};

   // Material creation state
   char m_newMaterialName[256]{};
   std::string m_selectedTemplate{"PBR"};
};
