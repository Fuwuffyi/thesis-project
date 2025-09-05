#include "core/editor/PerformanceGUI.hpp"

#include "core/resource/ResourceManager.hpp"

#include <imgui.h>

void PerformanceGUI::RenderPeformanceGUI(const ResourceManager& resourceManager) {
   ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
   ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                            ImGuiWindowFlags_NoNav;
   ImGui::Begin("FPS Overlay", nullptr, flags);
   ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
   ImGui::Text("Frame: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
   ImGui::Text("Resource MEM: %.3f MB",
               static_cast<float>(resourceManager.GetTotalMemoryUsage()) / (1024.0f * 1024.0f));
   ImGui::End();
}
