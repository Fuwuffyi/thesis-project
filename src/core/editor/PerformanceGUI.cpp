#include "core/editor/PerformanceGUI.hpp"

#include "core/resource/ResourceManager.hpp"
#include "core/scene/Scene.hpp"

#include <imgui.h>

void PerformanceGUI::RenderPeformanceGUI(const ResourceManager& resourceManager,
                                         const Scene& scene) noexcept {
   ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Always);
   ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                            ImGuiWindowFlags_NoNav;
   if (ImGui::Begin("FPS Overlay", nullptr, flags)) {
      const ImGuiIO& io = ImGui::GetIO();
      const float framerate = io.Framerate;
      const float frameTime = 1000.0f / framerate;
      const float memoryUsageMB =
         static_cast<float>(resourceManager.GetTotalMemoryUsage()) / (1024.0f * 1024.0f);
      const size_t nodeCount = scene.GetNodeCount();
      ImGui::Text("FPS: %.1f", framerate);
      ImGui::Text("Frame: %.3f ms", frameTime);
      ImGui::Text("Resource MEM: %.3f MB", memoryUsageMB);
      ImGui::Text("Loaded Node Count: %zu", nodeCount);
   }
   ImGui::End();
}
