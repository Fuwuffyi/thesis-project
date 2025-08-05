#include "GLRenderer.hpp"

#include <glad/gl.h>
#include <print>
#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void GLRenderer::Init(GLFWwindow* windowHandle) {
   if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
      throw std::runtime_error("GLAD init failed");
   }
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   if (!ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)windowHandle, true)) {
      throw std::runtime_error("ImGUI initialization failed");
   }
   if (!ImGui_ImplOpenGL3_Init("#version 460")) {
      throw std::runtime_error("ImGUI initialization failed");
   }
}

void GLRenderer::RenderFrame() {
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();
   ImGui::ShowDemoWindow();
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GLRenderer::Cleanup() {
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

