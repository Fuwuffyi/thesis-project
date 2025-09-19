#include "core/editor/MaterialEditor.hpp"

#include "core/resource/ResourceManager.hpp"
#include "core/scene/components/RendererComponent.hpp"

#include "gl/resource/GLTexture.hpp"
#include "vk/resource/VulkanTexture.hpp"

#include <imgui.h>
#include <algorithm>
#include <format>
#include <ranges>

constexpr ImGuiWindowFlags kDefaultWindowFlags =
   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

constexpr uint32_t kTextureBrowserColumns = 4;
constexpr uint32_t kTextureBrowserImageSize = 96;
constexpr uint32_t kTexturePreviewSize = 64;
constexpr uint32_t kTextureTooltipSize = 128;

constexpr glm::vec2 kButtonSize{150.0f, 0.0f};
constexpr glm::vec2 kImageButtonSize{64.0f, 64.0f};
constexpr glm::vec2 kImageSize{48.0f, 48.0f};

// Drag drop payload types
constexpr std::string_view kMaterialPayload = "MATERIAL";
constexpr std::string_view kTexturePayload = "TEXTURE";

void MaterialEditor::DrawMaterialBrowser() {
   const ImGuiViewport* vp = ImGui::GetMainViewport();
   ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
   if (ImGui::Begin("Material Browser", nullptr, kDefaultWindowFlags)) {
      // Create new material button
      if (ImGui::Button("Create New Material")) {
         m_showMaterialCreation = true;
      }
      ImGui::Separator();
      // List all materials with better performance using ranges
      const auto materials = m_resourceManager->GetAllMaterialsNamed();
      for (const auto& [material, name] : materials) {
         const bool isSelected = (material == m_selectedMaterial);
         if (ImGui::Selectable(name.c_str(), isSelected)) {
            m_selectedMaterial = material;
            m_selectedMaterialName = name;
         }
         // Drag source for material
         if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(kMaterialPayload.data(), name.c_str(), name.size() + 1);
            ImGui::Text("Material: %s", name.c_str());
            ImGui::EndDragDropSource();
         }
      }
   }
   ImGui::End();
   // Draw material creation dialog
   if (m_showMaterialCreation) [[unlikely]] {
      DrawMaterialCreationDialog();
   }
}

void MaterialEditor::DrawMaterialProperties() const {
   if (!m_selectedMaterial) [[unlikely]]
      return;
   const ImGuiViewport* vp = ImGui::GetMainViewport();
   ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
   if (ImGui::Begin("Material Properties", nullptr, kDefaultWindowFlags)) {
      ImGui::Text("Material: %s", m_selectedMaterialName.c_str());
      ImGui::Text("Template: %s", m_selectedMaterial->GetTemplateName().data());
      ImGui::Separator();
      DrawMaterialParameterEditor(m_selectedMaterial);
   }
   ImGui::End();
}

void MaterialEditor::DrawTextureBrowser() const {
   const ImGuiViewport* vp = ImGui::GetMainViewport();
   ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
   if (ImGui::Begin("Texture Browser", nullptr, kDefaultWindowFlags)) {
      ImGui::BeginChild("TextureScrollRegion", ImVec2(0, 0), false,
                        ImGuiWindowFlags_HorizontalScrollbar);
      if (ImGui::BeginTable("TextureTable", kTextureBrowserColumns)) {
         const auto textures = m_resourceManager->GetAllTexturesNamed();
         for (const auto& [texture, name] : textures) {
            if (!texture) [[unlikely]]
               continue;
            ImGui::TableNextColumn();
            const ImTextureID texId = GetTextureId(texture);
            ImGui::PushID(name.c_str());
            constexpr ImVec2 imgButtonSize{kTextureBrowserImageSize, kTextureBrowserImageSize};
            ImGui::ImageButton(name.c_str(), texId, imgButtonSize);
            // Drag source
            if (ImGui::BeginDragDropSource()) {
               ImGui::SetDragDropPayload(kTexturePayload.data(), name.c_str(), name.size() + 1);
               ImGui::Text("Texture: %s", name.c_str());
               ImGui::Image(texId, {kImageSize.x, kImageSize.y}, ImVec2(0, 1), ImVec2(1, 0));
               ImGui::EndDragDropSource();
            }
            ImGui::PopID();
            // Info
            ImGui::TextWrapped("%s", name.c_str());
            ImGui::Text("%ux%u", texture->GetWidth(), texture->GetHeight());
         }
         ImGui::EndTable();
      }
      ImGui::EndChild();
   }
   ImGui::End();
}

void MaterialEditor::DrawRendererComponentInspector(Node* const node,
                                                    RendererComponent* const renderer) const {
   if (!renderer) [[unlikely]]
      return;
   constexpr ImGuiTreeNodeFlags headerFlags =
      ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_NoTreePushOnOpen;
   if (ImGui::CollapsingHeader("Renderer", headerFlags)) {
      // Basic renderer properties with better layout
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
      // Material assignment with improved performance
      ImGui::Text("Material:");
      ImGui::SameLine();
      const MaterialHandle currentMat = renderer->GetMaterial();
      const std::string matName = currentMat.IsValid() ? FindMaterialName(currentMat) : "None";
      ImGui::Button(matName.c_str(), {kButtonSize.x, kButtonSize.y});
      // Drag drop target
      if (ImGui::BeginDragDropTarget()) {
         if (const ImGuiPayload* const payload =
                ImGui::AcceptDragDropPayload(kMaterialPayload.data())) {
            const std::string materialName{static_cast<const char*>(payload->Data)};
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
      ImGui::InputText("Material Name", m_newMaterialName.data(), m_newMaterialName.size());
      // Template selection
      const auto realTemplates = m_resourceManager->GetAllMaterialTemplatesNamed();
      std::vector<std::string> templateNames;
      templateNames.reserve(realTemplates.size());
      const auto nameView =
         realTemplates | std::views::transform([](const auto& pair) { return pair.second; });
      std::ranges::copy(nameView, std::back_inserter(templateNames));
      std::vector<const char*> templateNamePtrs;
      templateNamePtrs.reserve(templateNames.size());
      const auto ptrView =
         templateNames | std::views::transform([](const auto& name) { return name.c_str(); });
      std::ranges::copy(ptrView, std::back_inserter(templateNamePtrs));
      static int32_t currentTemplate = 0;
      currentTemplate =
         std::clamp(currentTemplate, 0, static_cast<int32_t>(templateNames.size() - 1));
      ImGui::Combo("Template", &currentTemplate, templateNamePtrs.data(),
                   static_cast<int32_t>(templateNamePtrs.size()));
      if (!templateNames.empty()) {
         m_selectedTemplate = templateNames[currentTemplate];
      }
      if (ImGui::Button("Create")) {
         if (m_newMaterialName[0] != '\0') {
            const MaterialHandle newMat =
               m_resourceManager->CreateMaterial(m_newMaterialName.data(), m_selectedTemplate);
            if (newMat.IsValid()) {
               m_selectedMaterial = m_resourceManager->GetMaterial(newMat);
               m_selectedMaterialName = m_newMaterialName.data();
            }
            // Clear form
            m_newMaterialName.fill('\0');
            m_showMaterialCreation = false;
         }
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
         m_newMaterialName.fill('\0');
         m_showMaterialCreation = false;
      }
      ImGui::EndPopup();
   }
}

void MaterialEditor::DrawMaterialParameterEditor(IMaterial* const material) const {
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
            std::array<float, 4> matValues{val[0][0], val[0][1], val[1][0], val[1][1]};
            if (ImGui::InputFloat4(paramName.c_str(), matValues.data())) {
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
            std::array<float, 9> matValues{};
            for (uint32_t row = 0; row < 3; ++row) {
               for (uint32_t col = 0; col < 3; ++col) {
                  matValues[row * 3 + col] = val[row][col];
               }
            }
            const std::string row2Label = std::format("{}_row2", paramName);
            const std::string row3Label = std::format("{}_row3", paramName);
            bool changed = ImGui::InputFloat3(paramName.c_str(), matValues.data());
            changed |= ImGui::InputFloat3(row2Label.c_str(), matValues.data() + 3);
            changed |= ImGui::InputFloat3(row3Label.c_str(), matValues.data() + 6);
            if (changed) {
               for (uint32_t row = 0; row < 3; ++row) {
                  for (uint32_t col = 0; col < 3; ++col) {
                     val[row][col] = matValues[row * 3 + col];
                  }
               }
               material->SetParameter(paramName, val);
               material->UpdateUBO();
            }
            break;
         }
         case ParameterDescriptor::Type::Mat4: {
            glm::mat4 val = std::get<glm::mat4>(param);
            std::array<float, 16> matValues{};
            for (uint32_t row = 0; row < 4; ++row) {
               for (uint32_t col = 0; col < 4; ++col) {
                  matValues[row * 4 + col] = val[row][col];
               }
            }
            const std::string row2Label = std::format("{}_row2", paramName);
            const std::string row3Label = std::format("{}_row3", paramName);
            const std::string row4Label = std::format("{}_row4", paramName);
            bool changed = ImGui::InputFloat4(paramName.c_str(), matValues.data());
            changed |= ImGui::InputFloat4(row2Label.c_str(), matValues.data() + 4);
            changed |= ImGui::InputFloat4(row3Label.c_str(), matValues.data() + 8);
            changed |= ImGui::InputFloat4(row4Label.c_str(), matValues.data() + 12);
            if (changed) {
               for (uint32_t row = 0; row < 4; ++row) {
                  for (uint32_t col = 0; col < 4; ++col) {
                     val[row][col] = matValues[row * 4 + col];
                  }
               }
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
                                           const std::string_view displayName) const {
   if (!material->HasTexture(textureName)) [[unlikely]]
      return;
   const TextureHandle currentTex = material->GetTexture(textureName);
   const ITexture* const texture = m_resourceManager->GetTexture(currentTex);
   ImGui::Text("%s:", displayName.data());
   ImGui::PushID(textureName.data());
   if (texture) {
      const ImTextureID texIdPtr = GetTextureId(texture);
      ImGui::ImageButton(textureName.data(), texIdPtr, {kImageButtonSize.x, kImageButtonSize.y});
      // Find texture name for payload (cached lookup would be better)
      const std::string texNameForPayload =
         currentTex.IsValid() ? FindTextureName(currentTex) : "Unknown";
      // Drag source
      if (ImGui::BeginDragDropSource()) {
         ImGui::SetDragDropPayload(kTexturePayload.data(), texNameForPayload.c_str(),
                                   texNameForPayload.size() + 1);
         ImGui::Text("Texture: %s", texNameForPayload.c_str());
         ImGui::Image(texIdPtr, {kImageSize.x, kImageSize.y}, ImVec2(0, 1), ImVec2(1, 0));
         ImGui::EndDragDropSource();
      }
      // Drag target
      if (ImGui::BeginDragDropTarget()) {
         if (const ImGuiPayload* const payload =
                ImGui::AcceptDragDropPayload(kTexturePayload.data())) {
            const std::string textureNameStr{static_cast<const char*>(payload->Data)};
            const TextureHandle newTex = m_resourceManager->GetTextureHandle(textureNameStr);
            if (newTex.IsValid()) {
               material->SetTexture(textureName, newTex);
            }
         }
         ImGui::EndDragDropTarget();
      }
      // Tooltip
      if (ImGui::IsItemHovered()) {
         ImGui::BeginTooltip();
         ImGui::Text("Texture: %s", texNameForPayload.c_str());
         ImGui::Text("Size: %ux%u", texture->GetWidth(), texture->GetHeight());
         constexpr ImVec2 tooltipSize{kTextureTooltipSize, kTextureTooltipSize};
         ImGui::Image(texIdPtr, tooltipSize, ImVec2(0, 1), ImVec2(1, 0));
         ImGui::EndTooltip();
      }
      ImGui::SameLine();
      // Clear button
      if (ImGui::Button("Clear")) {
         const auto availableTemplates = m_resourceManager->GetAllMaterialTemplatesNamed();
         auto templateIt = std::ranges::find_if(availableTemplates, [&material](const auto& pair) {
            return pair.second == material->GetTemplateName();
         });
         if (templateIt != availableTemplates.end()) {
            const auto& textureDescs = templateIt->first.GetTextures();
            const std::string textureNameStr{textureName};
            if (const auto texIt = textureDescs.find(textureNameStr); texIt != textureDescs.end()) {
               material->SetTexture(textureName, texIt->second.defaultTexture);
            }
         }
      }
   } else {
      ImGui::Button("None", {kImageButtonSize.x, kImageButtonSize.y});
      // Drag target for empty slots
      if (ImGui::BeginDragDropTarget()) {
         if (const ImGuiPayload* const payload =
                ImGui::AcceptDragDropPayload(kTexturePayload.data())) {
            const std::string textureNameStr{static_cast<const char*>(payload->Data)};
            const TextureHandle newTex = m_resourceManager->GetTextureHandle(textureNameStr);
            if (newTex.IsValid()) {
               material->SetTexture(textureName, newTex);
            }
         }
         ImGui::EndDragDropTarget();
      }
   }
   ImGui::PopID();
}

void MaterialEditor::DrawTexturePreview(const ITexture* const texture,
                                        const glm::vec2& size) const {
   if (!texture) [[unlikely]]
      return;
   if (texture->IsValid()) {
      const ImTextureID texIdPtr = GetTextureId(texture);
      ImGui::Image(texIdPtr, ImVec2(size.x, size.y), ImVec2(0, 1), ImVec2(1, 0));
   }
}

constexpr uint64_t MaterialEditor::GetTextureId(const ITexture* const texture) const noexcept {
   if (!texture) [[unlikely]]
      return 0;
   switch (m_api) {
      case GraphicsAPI::OpenGL: {
         const auto* const glTexture = static_cast<const GLTexture*>(texture);
         return (ImTextureID)glTexture->GetId();
      }
      // TODO: Fix vulkan case?
      case GraphicsAPI::Vulkan: {
         const auto* const vkTexture = static_cast<const VulkanTexture*>(texture);
         return (ImTextureID)vkTexture->GetDescriptorSet();
      }
      default:
         return 0;
   }
}

std::string MaterialEditor::FindMaterialName(const MaterialHandle& handle) const {
   const auto materials = m_resourceManager->GetAllMaterialsNamed();
   auto materialIt = std::ranges::find_if(materials, [this, &handle](const auto& pair) {
      return m_resourceManager->GetMaterialHandle(pair.second) == handle;
   });
   return materialIt != materials.end() ? materialIt->second : "Unknown";
}

std::string MaterialEditor::FindTextureName(const TextureHandle& handle) const {
   const auto textures = m_resourceManager->GetAllTexturesNamed();
   auto textureIt = std::ranges::find_if(textures, [this, &handle](const auto& pair) {
      return m_resourceManager->GetTextureHandle(pair.second) == handle;
   });
   return textureIt != textures.end() ? textureIt->second : "Unknown";
}
