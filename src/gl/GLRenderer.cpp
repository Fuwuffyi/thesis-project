#include "GLRenderer.hpp"

#include <glad/gl.h>
#include <print>
#include <GLFW/glfw3.h>

#include "../core/Window.hpp"
#include "../core/Camera.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Stuff for mesh
#include <vector>
#include "GLShader.hpp"
#include "GLBuffer.hpp"
#include "GLVertexArray.hpp"
#include "../core/Vertex.hpp"

// Testing mesh
using glm::value_ptr;

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}};
const std::vector<uint16_t> indices = {
   0, 1, 2, 2, 3, 0
};

GLShader* shader = nullptr;
GLBuffer* vbo = nullptr;
GLBuffer* ebo = nullptr;
GLVertexArray* vao = nullptr;

GLRenderer::GLRenderer(Window* window)
   :
   IRenderer(window)
{
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
   // Setup depth testing
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);
   // Cull back faces
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glFrontFace(GL_CCW);
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
   shader = new GLShader();
   shader->AttachShaderFromFile(GLShader::Type::Vertex, "resources/shaders/gl/test.vert");
   shader->AttachShaderFromFile(GLShader::Type::Fragment, "resources/shaders/gl/test.frag");
   shader->Link();

   vbo = new GLBuffer(GLBuffer::Type::Array, GLBuffer::Usage::StaticDraw);
   vbo->UploadData(vertices);

   ebo = new GLBuffer(GLBuffer::Type::Element, GLBuffer::Usage::StaticDraw);
   ebo->UploadData(indices);

   vao = new GLVertexArray();
   vao->AttachVertexBuffer(*vbo,0, 0, sizeof(Vertex));
   vao->AttachElementBuffer(*ebo);
   vao->SetupVertexAttributes();
}

GLRenderer::~GLRenderer() {
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();

   delete vbo;
   delete ebo;
   delete vao;
}

void GLRenderer::RenderFrame() {
   // Clear the screen
   glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   shader->Use();
   glm::mat4 modl = glm::mat4(1.0f);
   shader->SetMat4("model", modl);
   shader->SetMat4("proj", m_activeCamera->GetProjectionMatrix());
   shader->SetMat4("view", m_activeCamera->GetViewMatrix());
   vao->DrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT);

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

