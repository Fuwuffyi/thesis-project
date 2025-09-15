#include "core/editor/MaterialEditor.hpp"

#include "core/GraphicsAPI.hpp"
#include "core/resource/ResourceManager.hpp"
#include "core/scene/components/RendererComponent.hpp"

#include "core/resource/IMaterial.hpp"
#include "core/resource/ITexture.hpp"

#include "gl/resource/GLTexture.hpp"
#include "vk/resource/VulkanTexture.hpp"

#include <imgui.h>

MaterialEditor::MaterialEditor(ResourceManager* const resourceManager,
                               const GraphicsAPI api) noexcept
    : m_resourceManager(resourceManager), m_api(api) {}

void MaterialEditor::DrawMaterialBrowser() {
   ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                            ImGuiWindowFlags_NoNav;
   if (ImGui::Begin("Material Browser", nullptr, flags)) {
      // Create new material button
      if (ImGui::Button("Create New Material")) {
         m_showMaterialCreation = true;
      }
      ImGui::Separator();
      // List all materials
      const auto materials = m_resourceManager->GetAllMaterialsNamed();
      for (const auto& [material, name] : materials) {
         const bool isSelected = (material == m_selectedMaterial);
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
   if (!m_selectedMaterial) [[unlikely]]
      return;
   ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                            ImGuiWindowFlags_NoNav;
   ImGui::Begin("Material Properties", nullptr, flags);
   ImGui::Text("Material: %s", m_selectedMaterialName.c_str());
   ImGui::Text("Template: %s", m_selectedMaterial->GetTemplateName().data());
   ImGui::Separator();
   DrawMaterialParameterEditor(m_selectedMaterial);
   ImGui::End();
}

void MaterialEditor::DrawTextureBrowser() {
   ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                            ImGuiWindowFlags_NoNav;
   if (ImGui::Begin("Texture Browser", nullptr, flags)) {
      constexpr uint32_t columns = 4;
      constexpr uint32_t imgSize = 96;
      ImGui::BeginChild("TextureScrollRegion", ImVec2(0, 0), false,
                        ImGuiWindowFlags_HorizontalScrollbar);
      ImGui::Columns(columns, nullptr, false);
      const auto textures = m_resourceManager->GetAllTexturesNamed();
      for (const auto& [texture, name] : textures) {
         if (texture) {
            const uint32_t texId = GetTextureId(texture);
            ImGui::PushID(name.c_str());
            ImGui::ImageButton(name.c_str(), static_cast<ImTextureID>(texId),
                               ImVec2(static_cast<float>(imgSize), static_cast<float>(imgSize)));
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

void MaterialEditor::DrawRendererComponentInspector(Node* const node,
                                                    RendererComponent* const renderer) {
   if (!renderer) [[unlikely]]
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
      // Material assignment
      ImGui::Text("Material:");
      // Current material display
      const MaterialHandle currentMat = renderer->GetMaterial();
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
         if (const ImGuiPayload* const payload = ImGui::AcceptDragDropPayload("MATERIAL")) {
            const std::string materialName(static_cast<const char*>(payload->Data));
            const MaterialHandle newMat = m_resourceManager->GetMaterialHandle(materialName);
            if (newMat.IsValid()) {
               renderer->SetMaterial(newMat);
            }
         }
         ImGui::EndDragDropTarget();
      }
   }
}

void MaterialEditor::DrawMaterialCreationDialog() {
   if (!m_showMaterialCreation) [[unlikely]]
      return;
   ImGui::OpenPopup("Create Material");
   if (ImGui::BeginPopupModal("Create Material", &m_showMaterialCreation)) {
      ImGui::InputText("Material Name", m_newMaterialName, sizeof(m_newMaterialName));
      // Template selection
      auto realTemplates = m_resourceManager->GetAllMaterialTemplatesNamed();
      std::vector<std::string> templateNames;
      for (auto& [t, n] : realTemplates) {
         templateNames.push_back(n);
      }
      std::vector<const char*> templateNamePtrs;
      templateNamePtrs.reserve(templateNames.size());
      for (auto& s : templateNames) {
         templateNamePtrs.push_back(s.c_str());
      }
      static int32_t currentTemplate = 0;
      ImGui::Combo("Template", &currentTemplate, templateNamePtrs.data(),
                   static_cast<int32_t>(templateNamePtrs.size()));
      m_selectedTemplate = templateNames[currentTemplate];
      if (ImGui::Button("Create")) {
         if (strlen(m_newMaterialName) > 0) {
            const MaterialHandle newMat =
               m_resourceManager->CreateMaterial(m_newMaterialName, m_selectedTemplate);
            if (newMat.IsValid()) {
               m_selectedMaterial = m_resourceManager->GetMaterial(newMat);
               m_selectedMaterialName = m_newMaterialName;
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

void MaterialEditor::DrawMaterialParameterEditor(IMaterial* const material) {
   if (!material) [[unlikely]]
      return;
   const auto matTempl = m_resourceManager->GetMaterialTemplate(material->GetTemplateName());
   if (!matTempl) [[unlikely]]
      return;
   for (const auto& [paramName, desc] : matTempl->GetParameters()) {
      const MaterialParam param = material->GetParameter(paramName);
      switch (desc.type) {
         case ParameterDescriptor::Type::Float: {
            float val = std::get<float>(param);
            if (ImGui::SliderFloat(paramName.c_str(), &val, 0.0f, 1.0f)) {
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         case ParameterDescriptor::Type::Int: {
            int32_t val = std::get<int32_t>(param);
            if (ImGui::InputInt(paramName.c_str(), &val)) {
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         case ParameterDescriptor::Type::UInt: {
            uint32_t val = std::get<uint32_t>(param);
            if (ImGui::InputScalar(paramName.c_str(), ImGuiDataType_U32, &val)) {
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         case ParameterDescriptor::Type::Vec2: {
            glm::vec2 val = std::get<glm::vec2>(param);
            if (ImGui::DragFloat2(paramName.c_str(), &val.x, 0.01f)) {
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         case ParameterDescriptor::Type::Vec3: {
            glm::vec3 val = std::get<glm::vec3>(param);
            if (ImGui::ColorEdit3(paramName.c_str(), &val.x)) {
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         case ParameterDescriptor::Type::Vec4: {
            glm::vec4 val = std::get<glm::vec4>(param);
            if (ImGui::ColorEdit4(paramName.c_str(), &val.x)) {
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         case ParameterDescriptor::Type::Mat2: {
            glm::mat2 val = std::get<glm::mat2>(param);
            float matValues[4] = {val[0][0], val[0][1], val[1][0], val[1][1]};
            if (ImGui::InputFloat4(paramName.c_str(), matValues)) {
               val[0][0] = matValues[0];
               val[0][1] = matValues[1];
               val[1][0] = matValues[2];
               val[1][1] = matValues[3];
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         case ParameterDescriptor::Type::Mat3: {
            glm::mat3 val = std::get<glm::mat3>(param);
            float matValues[9] = {val[0][0], val[0][1], val[0][2], val[1][0], val[1][1],
                                  val[1][2], val[2][0], val[2][1], val[2][2]};
            if (ImGui::InputFloat3(paramName.c_str(), matValues) &&
                ImGui::InputFloat3((paramName + "_row2").c_str(), matValues + 3) &&
                ImGui::InputFloat3((paramName + "_row3").c_str(), matValues + 6)) {
               for (int row = 0; row < 3; ++row)
                  for (int col = 0; col < 3; ++col)
                     val[row][col] = matValues[row * 3 + col];
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         case ParameterDescriptor::Type::Mat4: {
            glm::mat4 val = std::get<glm::mat4>(param);
            float matValues[16];
            for (int row = 0; row < 4; ++row)
               for (int col = 0; col < 4; ++col)
                  matValues[row * 4 + col] = val[row][col];
            if (ImGui::InputFloat4(paramName.c_str(), matValues) &&
                ImGui::InputFloat4((paramName + "_row2").c_str(), matValues + 4) &&
                ImGui::InputFloat4((paramName + "_row3").c_str(), matValues + 8) &&
                ImGui::InputFloat4((paramName + "_row4").c_str(), matValues + 12)) {
               for (int row = 0; row < 4; ++row)
                  for (int col = 0; col < 4; ++col)
                     val[row][col] = matValues[row * 4 + col];
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         default:
            break;
      }
   }
   ImGui::Separator();
   // Texture slots
   for (const auto& [texName, texDesc] : matTempl->GetTextures()) {
      DrawTextureSlotEditor(material, texName, texDesc.name);
   }
}

void MaterialEditor::DrawTextureSlotEditor(IMaterial* const material,
                                           const std::string_view textureName,
                                           const std::string_view displayName) {
   if (!material->HasTexture(textureName)) [[unlikely]]
      return;
   const TextureHandle currentTex = material->GetTexture(textureName);
   const ITexture* const texture = m_resourceManager->GetTexture(currentTex);
   ImGui::Text("%s:", displayName.data());
   // Show texture information if available
   if (texture) {
      ImGui::SameLine();
      ImGui::Text("(%ux%u)", texture->GetWidth(), texture->GetHeight());
   }
   if (texture) {
      const uint32_t texId = GetTextureId(texture);
      ImGui::PushID(textureName.data());
      ImGui::ImageButton(textureName.data(), static_cast<ImTextureID>(texId), ImVec2(64, 64));
      // Show tooltip with texture details
      if (ImGui::IsItemHovered()) {
         ImGui::BeginTooltip();
         // Find the texture's resource name
         std::string texNameForTooltip = "Unknown";
         const auto textures = m_resourceManager->GetAllTexturesNamed();
         for (const auto& [tex, name] : textures) {
            if (m_resourceManager->GetTextureHandle(name) == currentTex) {
               texNameForTooltip = name;
               break;
            }
         }
         ImGui::Text("Texture: %s", texNameForTooltip.c_str());
         ImGui::Text("Size: %ux%u", texture->GetWidth(), texture->GetHeight());
         ImGui::Image(static_cast<ImTextureID>(texId), ImVec2(128, 128), ImVec2(0, 1),
                      ImVec2(1, 0));
         ImGui::EndTooltip();
      }
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
      // Button to clear texture (reset to default)
      ImGui::SameLine();
      if (ImGui::Button("Clear")) {
         // Get default texture from template
         const auto availableTemplates = m_resourceManager->GetAllMaterialTemplatesNamed();
         for (const auto& [templateRef, name] : availableTemplates) {
            if (name == material->GetTemplateName()) {
               const auto& textureDescs = templateRef.GetTextures();
               const std::string textureNameStr{textureName};
               if (const auto it = textureDescs.find(textureNameStr); it != textureDescs.end()) {
                  material->SetTexture(textureName, it->second.defaultTexture);
                  break;
               }
            }
         }
      }
      ImGui::PopID();
   } else {
      ImGui::Button("None", ImVec2(64, 64));
   }
   // Accept drops here
   if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* const payload = ImGui::AcceptDragDropPayload("TEXTURE")) {
         const std::string textureNameStr(static_cast<const char*>(payload->Data));
         const TextureHandle newTex = m_resourceManager->GetTextureHandle(textureNameStr);
         if (newTex.IsValid()) {
            material->SetTexture(textureName, newTex);
         }
      }
      ImGui::EndDragDropTarget();
   }
}

void MaterialEditor::DrawTexturePreview(const ITexture* const texture,
                                        const glm::vec2& size) const {
   if (!texture) [[unlikely]]
      return;
   const uint32_t textureId = GetTextureId(texture);
   if (textureId != 0) {
      ImGui::Image((ImTextureID)(intptr_t)textureId, ImVec2(size.x, size.y), ImVec2(0, 1),
                   ImVec2(1, 0));
   }
}

uint32_t MaterialEditor::GetTextureId(const ITexture* const texture) const noexcept {
   if (!texture) [[unlikely]]
      return 0;
   if (m_api == GraphicsAPI::OpenGL) {
      const GLTexture* const glTexture = static_cast<const GLTexture*>(texture);
      return glTexture->GetId();
   } else {
      const VulkanTexture* const vkTexture = static_cast<const VulkanTexture*>(texture);
      return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vkTexture->GetDescriptorSet()));
   }
}
