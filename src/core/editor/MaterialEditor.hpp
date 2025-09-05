#pragma once

#include <imgui.h>
#include <cstdint>
#include <string>

class Node;
class IMaterial;
class ITexture;
class RendererComponent;
class ResourceManager;

class MaterialEditor final {
  public:
   explicit MaterialEditor(ResourceManager* resourceManager);

   // Main material browser/editor window
   void DrawMaterialBrowser();

   // Material property editor for selected material
   void DrawMaterialProperties();

   // Texture browser with drag/drop support
   void DrawTextureBrowser();

   // Enhanced renderer component inspector with drag/drop
   void DrawRendererComponentInspector(Node* node, RendererComponent* renderer);

   // Handle drag/drop operations
   void HandleMaterialDragDrop(RendererComponent* renderer, size_t subMeshIndex = SIZE_MAX);
   void HandleTextureDragDrop(IMaterial* material, const std::string& textureSlot);

   // Material creation dialog
   void DrawMaterialCreationDialog();

  private:
   ResourceManager* m_resourceManager;

   // UI State
   IMaterial* m_selectedMaterial = nullptr;
   std::string m_selectedMaterialName;
   bool m_showMaterialBrowser = true;
   bool m_showTextureBrowser = true;
   bool m_showMaterialCreation = false;

   // Material creation state
   char m_newMaterialName[256] = {};
   std::string m_selectedTemplate = "PBR";

   // Helper functions
   void DrawMaterialParameterEditor(IMaterial* material);
   void DrawTextureSlotEditor(IMaterial* material, const std::string& textureName,
                              const std::string& displayName);
   bool DrawDragDropSource(const std::string& type, const std::string& name,
                           const std::string& displayText);
   bool DrawDragDropTarget(const std::string& type);
   std::string GetDragDropPayload();

   // Texture preview helpers
   void DrawTexturePreview(ITexture* texture, const ImVec2& size = ImVec2(64, 64));
   uint32_t GetTextureId(ITexture* texture); // Platform-specific texture ID for ImGui
};
