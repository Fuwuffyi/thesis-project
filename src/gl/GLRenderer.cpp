#include "GLRenderer.hpp"

#include <glad/gl.h>
#include <print>
#include <GLFW/glfw3.h>

#include "../core/Window.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Stuff for mesh
#include <vector>
#include "GLVertexBuffer.hpp"
#include "../core/Vertex.hpp"

// Testing mesh
const std::vector<Vertex> vertices = {
   { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
   { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
   { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }},
   { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }}
};
const std::vector<uint16_t> indices = {
   0, 1, 2, 2, 3, 0
};
GLVertexBuffer* mesh = nullptr;

GLRenderer::GLRenderer(Window* window)
   :
   IRenderer(window)
{
   // Set context for current window
   glfwMakeContextCurrent(m_window->GetNativeWindow());
   // Load OpenGL function pointers
   if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
      throw std::runtime_error("GLAD init failed.");
   }
   // Set initial viewport
   GLRenderer::FramebufferCallback(
      static_cast<int32_t>(m_window->GetWidth()),
      static_cast<int32_t>(m_window->GetHeight())
   );
   // Setup framebuffer callback
   window->SetResizeCallback(GLRenderer::FramebufferCallback);
   // Initialize ImGui
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   if (!ImGui_ImplGlfw_InitForOpenGL(m_window->GetNativeWindow(), true)) {
      throw std::runtime_error("ImGUI initialization failed.");
   }
   if (!ImGui_ImplOpenGL3_Init("#version 460")) {
      throw std::runtime_error("ImGUI initialization failed.");
   }


   CreateTestMesh();
}

void GLRenderer::FramebufferCallback(const int32_t width, const int32_t height) {
   glViewport(0, 0, width, height);
}

void GLRenderer::CreateTestMesh() {
   mesh = new GLVertexBuffer();
   mesh->UploadData(vertices, indices);
}

GLRenderer::~GLRenderer() {
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();

   delete mesh;
}

void GLRenderer::RenderFrame() {
   // Clear the screen
   glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);

   mesh->Draw();

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
   glfwSwapBuffers(m_window->GetNativeWindow());
}

