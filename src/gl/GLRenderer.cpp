#include "GLRenderer.hpp"

#include <glad/gl.h>
#include <print>
#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void GLRenderer::Init(GLFWwindow* windowHandle) {
   // Set context for current window
   m_windowHandle = windowHandle;
   glfwMakeContextCurrent(windowHandle);

   // Load OpenGL function pointers
   if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
      throw std::runtime_error("GLAD init failed");
   }

   // Set initial viewport
   int32_t width, height;
   glfwGetFramebufferSize(windowHandle, &width, &height);
   glViewport(0, 0, width, height);

   // Initialize ImGui
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
   // Clear the screen
   glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);

   // Render ImGUI
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   // Show window
   ImGui::ShowDemoWindow();

   // End ImGUI Render
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

   // Swap buffers
   glfwSwapBuffers(m_windowHandle);
}

void GLRenderer::Cleanup() {
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

