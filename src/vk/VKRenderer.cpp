#include "VKRenderer.hpp"

#include <print>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

VKRenderer::VKRenderer(GLFWwindow* windowHandle) : IRenderer(windowHandle) {
}

VKRenderer::~VKRenderer() {
   ImGui_ImplVulkan_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

void VKRenderer::RenderFrame() {
   ImGui_ImplVulkan_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();
   ImGui::ShowDemoWindow();
   ImGui::Render();
   // ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}

