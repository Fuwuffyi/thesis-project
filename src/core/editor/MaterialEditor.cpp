#include "core/editor/MaterialEditor.hpp"
#include <imgui.h>

#include "core/GraphicsAPI.hpp"
#include "core/resource/ResourceManager.hpp"
#include "core/scene/components/RendererComponent.hpp"

#include "core/resource/IMaterial.hpp"
#include "core/resource/ITexture.hpp"

#include "gl/resource/GLTexture.hpp"
#include "vk/resource/VulkanTexture.hpp"

MaterialEditor::MaterialEditor(ResourceManager* resourceManager, const GraphicsAPI api)
    : m_resourceManager(resourceManager), m_api(api) {}

void MaterialEditor::DrawMaterialBrowser() {
   if (!m_showMaterialBrowser)
      return;

   if (ImGui::Begin("Material Browser", &m_showMaterialBrowser)) {
      // Create new material button
      if (ImGui::Button("Create New Material")) {
         m_showMaterialCreation = true;
      }

      ImGui::Separator();

      // List all materials
      const auto materials = m_resourceManager->GetAllMaterialsNamed();

      for (const auto& [material, name] : materials) {
         bool isSelected = (material == m_selectedMaterial);

         if (ImGui::Selectable(name.c_str(), isSelected)) {
            m_selectedMaterial = material;
            m_selectedMaterialName = name;
         }

         // Drag source for material
         if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("MATERIAL", name.c_str(), name.size() + 1);
            ImGui::Text("Material: %s", name.c_str());
            ImGui::EndDragDropSource();
         }
      }
   }
   ImGui::End();

   // Draw material creation dialog
   if (m_showMaterialCreation) {
      DrawMaterialCreationDialog();
   }
}

void MaterialEditor::DrawMaterialProperties() {
   if (!m_selectedMaterial)
      return;

   ImGui::Begin("Material Properties");

   ImGui::Text("Material: %s", m_selectedMaterialName.c_str());
   ImGui::Text("Template: %s", m_selectedMaterial->GetTemplateName().c_str());

   ImGui::Separator();

   DrawMaterialParameterEditor(m_selectedMaterial);

   ImGui::End();
}

void MaterialEditor::DrawTextureBrowser() {
   if (!m_showTextureBrowser)
      return;

   if (ImGui::Begin("Texture Browser", &m_showTextureBrowser)) {
      const uint32_t columns = 4;
      const uint32_t imgSize = 96;

      ImGui::BeginChild("TextureScrollRegion", ImVec2(0, 0), false,
                        ImGuiWindowFlags_HorizontalScrollbar);
      ImGui::Columns(columns, nullptr, false);

      const auto textures = m_resourceManager->GetAllTexturesNamed();

      for (const auto& [texture, name] : textures) {
         if (texture) {
            uint32_t texId = GetTextureId(texture);

            // Give each image a unique ID so ImGui knows which item is being interacted with
            ImGui::PushID(name.c_str());

            // Use ImageButton so the preview is an active widget and receives mouse events.
            // frame_padding = 0 to remove the visible button frame; adjust as you like.
            ImGui::ImageButton(name.c_str(), static_cast<ImTextureID>(texId),
                               ImVec2((float)imgSize, (float)imgSize));

            // Now begin the drag source while the image button is the last active item.
            if (ImGui::BeginDragDropSource()) {
               ImGui::SetDragDropPayload("TEXTURE", name.c_str(), name.size() + 1); // include null
               ImGui::Text("Texture: %s", name.c_str());
               ImGui::Image((ImTextureID)(intptr_t)texId, ImVec2(48, 48), ImVec2(0, 1),
                            ImVec2(1, 0));
               ImGui::EndDragDropSource();
            }
            ImGui::PopID();
            // Info
            ImGui::TextWrapped("%s", name.c_str());
            ImGui::Text("%ux%u", texture->GetWidth(), texture->GetHeight());
         }
         ImGui::NextColumn();
      }
      ImGui::Columns(1);
      ImGui::EndChild();
   }
   ImGui::End();
}

void MaterialEditor::DrawRendererComponentInspector(Node* node, RendererComponent* renderer) {
   if (!renderer)
      return;

   if (ImGui::CollapsingHeader(
          "Renderer", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
      // Basic renderer properties
      bool visible = renderer->IsVisible();
      if (ImGui::Checkbox("Visible", &visible)) {
         renderer->SetVisible(visible);
      }

      bool castsShadows = renderer->CastsShadows();
      if (ImGui::Checkbox("Casts Shadows", &castsShadows)) {
         renderer->SetCastsShadows(castsShadows);
      }

      bool receivesShadows = renderer->ReceivesShadows();
      if (ImGui::Checkbox("Receives Shadows", &receivesShadows)) {
         renderer->SetReceivesShadows(receivesShadows);
      }

      ImGui::Separator();

      // Material assignment for single mesh
      if (!renderer->IsMultiMesh()) {
         ImGui::Text("Material:");

         // Current material display
         MaterialHandle currentMat = renderer->GetMaterial();
         std::string matName = "None";
         if (currentMat.IsValid()) {
            // Find material name (you might want to store this in ResourceManager)
            const auto materials = m_resourceManager->GetAllMaterialsNamed();
            for (const auto& [mat, name] : materials) {
               if (m_resourceManager->GetMaterialHandle(name) == currentMat) {
                  matName = name;
                  break;
               }
            }
         }

         ImGui::SameLine();
         ImGui::Button(matName.c_str(), ImVec2(150, 0));

         // Drag drop target
         if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL")) {
               std::string materialName((char*)payload->Data);
               MaterialHandle newMat = m_resourceManager->GetMaterialHandle(materialName);
               if (newMat.IsValid()) {
                  renderer->SetMaterial(newMat);
               }
            }
            ImGui::EndDragDropTarget();
         }
      }
      // Multi-mesh material assignment
      else {
         ImGui::Text("Sub-Meshes:");

         auto& subMeshes = renderer->GetSubMeshRenderers();
         for (size_t i = 0; i < subMeshes.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));

            // Sub-mesh visibility
            bool subVisible = subMeshes[i].visible;
            if (ImGui::Checkbox("##visible", &subVisible)) {
               renderer->SetSubMeshVisible(i, subVisible);
            }

            ImGui::SameLine();
            ImGui::Text("SubMesh %zu:", i);

            // Material assignment
            MaterialHandle subMat = subMeshes[i].material;
            std::string subMatName = "None";
            if (subMat.IsValid()) {
               const auto materials = m_resourceManager->GetAllMaterialsNamed();
               for (const auto& [mat, name] : materials) {
                  if (m_resourceManager->GetMaterialHandle(name) == subMat) {
                     subMatName = name;
                     break;
                  }
               }
            }

            ImGui::SameLine();
            ImGui::Button(subMatName.c_str(), ImVec2(120, 0));

            // Drag drop target for sub-mesh
            if (ImGui::BeginDragDropTarget()) {
               if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL")) {
                  std::string materialName((char*)payload->Data);
                  MaterialHandle newMat = m_resourceManager->GetMaterialHandle(materialName);
                  if (newMat.IsValid()) {
                     renderer->SetSubMeshMaterial(i, newMat);
                  }
               }
               ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
         }
      }
   }
}

void MaterialEditor::DrawMaterialCreationDialog() {
   if (!m_showMaterialCreation)
      return;

   ImGui::OpenPopup("Create Material");

   if (ImGui::BeginPopupModal("Create Material", &m_showMaterialCreation)) {
      ImGui::InputText("Material Name", m_newMaterialName, sizeof(m_newMaterialName));

      // Template selection (you'd get this from ResourceManager)
      const char* templates[] = {"PBR"}; // Add more as available
      int currentTemplate = 0;
      ImGui::Combo("Template", &currentTemplate, templates, IM_ARRAYSIZE(templates));
      m_selectedTemplate = templates[currentTemplate];

      if (ImGui::Button("Create")) {
         if (strlen(m_newMaterialName) > 0) {
            try {
               MaterialHandle newMat =
                  m_resourceManager->CreateMaterial(m_newMaterialName, m_selectedTemplate);
               if (newMat.IsValid()) {
                  m_selectedMaterial = m_resourceManager->GetMaterial(newMat);
                  m_selectedMaterialName = m_newMaterialName;
               }
            } catch (const std::exception& e) {
               // Handle creation error
            }

            // Clear form
            m_newMaterialName[0] = '\0';
            m_showMaterialCreation = false;
         }
      }

      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
         m_newMaterialName[0] = '\0';
         m_showMaterialCreation = false;
      }

      ImGui::EndPopup();
   }
}

void MaterialEditor::DrawMaterialParameterEditor(IMaterial* material) {
   if (!material)
      return;

   // This is a simplified version - you'd need to introspect the material template
   // to get the actual parameters. For PBR materials:

   // Albedo
   if (material->HasParameter("albedo")) {
      MaterialParam albedoParam = material->GetParameter("albedo");
      if (std::holds_alternative<glm::vec3>(albedoParam)) {
         glm::vec3 albedo = std::get<glm::vec3>(albedoParam);
         if (ImGui::ColorEdit3("Albedo", &albedo.x)) {
            material->SetParameter("albedo", albedo);
            material->UpdateUBO();
         }
      }
   }

   // Metallic
   if (material->HasParameter("metallic")) {
      MaterialParam metallicParam = material->GetParameter("metallic");
      if (std::holds_alternative<float>(metallicParam)) {
         float metallic = std::get<float>(metallicParam);
         if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
            material->SetParameter("metallic", metallic);
            material->UpdateUBO();
         }
      }
   }

   // Roughness
   if (material->HasParameter("roughness")) {
      MaterialParam roughnessParam = material->GetParameter("roughness");
      if (std::holds_alternative<float>(roughnessParam)) {
         float roughness = std::get<float>(roughnessParam);
         if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
            material->SetParameter("roughness", roughness);
            material->UpdateUBO();
         }
      }
   }

   // AO
   if (material->HasParameter("ao")) {
      MaterialParam aoParam = material->GetParameter("ao");
      if (std::holds_alternative<float>(aoParam)) {
         float ao = std::get<float>(aoParam);
         if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f)) {
            material->SetParameter("ao", ao);
            material->UpdateUBO();
         }
      }
   }

   ImGui::Separator();
   ImGui::Text("Textures:");

   // Texture slots
   DrawTextureSlotEditor(material, "albedoTexture", "Albedo");
   DrawTextureSlotEditor(material, "normalTexture", "Normal");
   DrawTextureSlotEditor(material, "roughnessTexture", "Roughness");
   DrawTextureSlotEditor(material, "metallicTexture", "Metallic");
   DrawTextureSlotEditor(material, "aoTexture", "AO");
}

void MaterialEditor::DrawTextureSlotEditor(IMaterial* material, const std::string& textureName,
                                           const std::string& displayName) {
   if (!material->HasTexture(textureName))
      return;

   TextureHandle currentTex = material->GetTexture(textureName);
   ITexture* texture = m_resourceManager->GetTexture(currentTex);

   ImGui::Text("%s:", displayName.c_str());
   ImGui::SameLine();

   if (texture) {
      uint32_t texId = GetTextureId(texture);

      // Use the texture slot name as the ID so Push/Pop are stable
      ImGui::PushID(textureName.c_str());

      // Make the preview an ImageButton so it captures mouse input (clicks/drag)
      ImGui::ImageButton(textureName.c_str(), static_cast<ImTextureID>(texId), ImVec2(32, 32));

      // Find the texture's resource name to use as the payload (reverse lookup)
      std::string texNameForPayload = "Unknown";
      const auto textures = m_resourceManager->GetAllTexturesNamed();
      for (const auto& [tex, name] : textures) {
         if (m_resourceManager->GetTextureHandle(name) == currentTex) {
            texNameForPayload = name;
            break;
         }
      }

      if (ImGui::BeginDragDropSource()) {
         ImGui::SetDragDropPayload("TEXTURE", texNameForPayload.c_str(),
                                   texNameForPayload.size() + 1);
         ImGui::Text("Texture: %s", texNameForPayload.c_str());
         ImGui::Image((ImTextureID)(intptr_t)texId, ImVec2(48, 48), ImVec2(0, 1), ImVec2(1, 0));
         ImGui::EndDragDropSource();
      }

      ImGui::PopID();
   } else {
      ImGui::Button("None", ImVec2(32, 32));
   }

   // Accept drops here
   if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE")) {
         std::string textureNameStr((char*)payload->Data);
         TextureHandle newTex = m_resourceManager->GetTextureHandle(textureNameStr);
         if (newTex.IsValid()) {
            material->SetTexture(textureName, newTex);
         }
      }
      ImGui::EndDragDropTarget();
   }
}

void MaterialEditor::DrawTexturePreview(ITexture* texture, const ImVec2& size) {
   if (!texture)
      return;

   uint32_t textureId = GetTextureId(texture);
   if (textureId != 0) {
      ImGui::Image((ImTextureID)(intptr_t)textureId, size, ImVec2(0, 1), ImVec2(1, 0));
   }
}

uint32_t MaterialEditor::GetTextureId(ITexture* texture) {
   if (!texture)
      return 0;

   if (m_api == GraphicsAPI::OpenGL) {
      GLTexture* glTexture = static_cast<GLTexture*>(texture);
      return glTexture->GetId();
   } else {
      VulkanTexture* vkTexture = static_cast<VulkanTexture*>(texture);
      return (ImTextureID)vkTexture->GetDescriptorSet();
   }
}
