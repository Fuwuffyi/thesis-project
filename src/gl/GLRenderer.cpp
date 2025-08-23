#include "GLRenderer.hpp"

#include <algorithm>
#include <glad/gl.h>
#include <print>
#include <GLFW/glfw3.h>

#include "../core/Window.hpp"
#include "../core/Camera.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "resource/GLResourceFactory.hpp"

#include "../core/scene/Scene.hpp"
#include "../core/scene/Node.hpp"
#include "../core/scene/components/TransformComponent.hpp"

// Stuff for mesh
#include <vector>
#include "GLShader.hpp"
#include "resource/GLMesh.hpp"
#include "GLSampler.hpp"
#include "resource/GLTexture.hpp"

// Testing stuff
struct CameraData {
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
   {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
   {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
   {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
   {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
   {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {0.0f, 0.0f}},
   {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {1.0f, 0.0f}},
   {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {1.0f, 1.0f}},
   {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.5f}, {0.0f, 1.0f}},
   {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.0f, 0.0f}, {0.0f, 0.0f}},
   {{-0.5f, -0.5f,  0.5f}, {0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}},
   {{-0.5f,  0.5f,  0.5f}, {0.5f, 0.0f, 0.0f}, {1.0f, 1.0f}},
   {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.0f, 0.0f}, {0.0f, 1.0f}},
   {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
   {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
   {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
   {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
   {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
   {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
   {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
   {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
   {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.5f, 0.0f}, {0.0f, 0.0f}},
   {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.5f, 0.0f}, {1.0f, 0.0f}},
   {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.5f, 0.0f}, {1.0f, 1.0f}},
   {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f}},
};

const std::vector<uint16_t> indices = {
   0, 1, 2, 2, 3, 0,
   4, 5, 6, 6, 7, 4,
   8, 9, 10, 10, 11, 8,
   12, 13, 14, 14, 15, 12,
   16, 17, 18, 18, 19, 16,
   20, 21, 22, 22, 23, 20
};

GLShader* shader = nullptr;
GLBuffer* cameraUbo = nullptr;
GLSampler* sampler = nullptr;

MeshHandle mesh;
TextureHandle texture;

GLRenderer::GLRenderer(Window* window)
   :
   IRenderer(window)
{
   // Load OpenGL function pointers
   if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
      throw std::runtime_error("GLAD init failed.");
   }
   // Setup resource manager
   m_resourceManager = std::make_unique<ResourceManager>(
      std::make_unique<GLResourceFactory>()
   );
   // Setup imgui
   SetupImgui();
   // Set initial viewport
   FramebufferCallback(
      static_cast<int32_t>(m_window->GetWidth()),
      static_cast<int32_t>(m_window->GetHeight())
   );
   // Setup framebuffer callback
   window->SetResizeCallback(
      [this](int32_t width, int32_t height) {
         FramebufferCallback(width, height);
      }
   );
   // Setup depth testing
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);
   // Cull back faces
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glFrontFace(GL_CCW);

   CreateTestResources();
}

void GLRenderer::FramebufferCallback(const int32_t width, const int32_t height) {
   glViewport(0, 0, width, height);
   if (m_activeCamera) {
      m_activeCamera->SetAspectRatio(static_cast<float>(width) /
                                     static_cast<float>(height));
   }
}

void GLRenderer::CreateTestResources() {
   // Create shader
   shader = new GLShader();
   shader->AttachShaderFromFile(GLShader::Type::Vertex, "resources/shaders/gl/test.vert");
   shader->AttachShaderFromFile(GLShader::Type::Fragment, "resources/shaders/gl/test.frag");
   shader->Link();
   // Load mesh and texture from resource manager
   mesh = m_resourceManager->LoadMesh("test_cube", vertices, indices);
   texture = m_resourceManager->LoadTexture("test_texture", "resources/textures/texture_base.jpg");
   // Create camera UBO
   cameraUbo = new GLBuffer(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw);
   const CameraData camData{};
   cameraUbo->UploadData(&camData, sizeof(CameraData));
   cameraUbo->BindBase(0);
   // Create sampler
   sampler = new GLSampler(GLSampler::CreateAnisotropic(16.0f));
}

GLRenderer::~GLRenderer() {
   DestroyImgui();
   delete sampler;
   delete shader;
   delete cameraUbo;
}

void GLRenderer::SetupImgui() {
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   if (!ImGui_ImplGlfw_InitForOpenGL(m_window->GetNativeWindow(), true)) {
      throw std::runtime_error("ImGUI initialization failed.");
   }
   if (!ImGui_ImplOpenGL3_Init("#version 460")) {
      throw std::runtime_error("ImGUI initialization failed.");
   }
}

void GLRenderer::DestroyImgui() {
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

void GLRenderer::RenderImgui() {
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();
   const ImGuiViewport* viewport = ImGui::GetMainViewport();
   // FPS Overlay
   {
      ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
      ImGui::SetNextWindowBgAlpha(0.35f);
      ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
         ImGuiWindowFlags_AlwaysAutoResize |
         ImGuiWindowFlags_NoSavedSettings |
         ImGuiWindowFlags_NoFocusOnAppearing |
         ImGuiWindowFlags_NoNav;
      ImGui::Begin("FPS Overlay", nullptr, flags);
      ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
      ImGui::Text("Frame: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
      ImGui::End();
   }
   // Scene graph
   {
      ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 300, viewport->WorkPos.y)); // 300 px width
      ImGui::SetNextWindowSize(ImVec2(300, viewport->WorkSize.y));
      ImGui::SetNextWindowBgAlpha(0.35f);
      ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
         ImGuiWindowFlags_NoMove |
         ImGuiWindowFlags_NoCollapse |
         ImGuiWindowFlags_NoResize |
         ImGuiWindowFlags_NoSavedSettings |
         ImGuiWindowFlags_NoFocusOnAppearing |
         ImGuiWindowFlags_NoNav;
      ImGui::Begin("Scene Graph", nullptr, flags);
      Node* root = m_activeScene->GetRootNode();
      std::vector<Node*> nodesToCheck{root};
      uint32_t id = 0;
      while (!nodesToCheck.empty()) {
         auto c = nodesToCheck.back();
         nodesToCheck.pop_back();
         // Get transform
         TransformComponent* comp = c->GetComponent<TransformComponent>();
         if (ImGui::TreeNode(std::to_string(++id).c_str())) {
            if (comp) {
               glm::vec3 posInput = comp->m_transform.GetPosition();
               if (ImGui::DragFloat3("Position", &posInput.x, 0.01f, 0.0f, 10000.0f)) {
                  comp->m_transform.SetPosition(posInput);
               }
               glm::quat rotInput = comp->m_transform.GetRotation();
               if (ImGui::DragFloat4("Rotation", &rotInput.x, 0.01f, 0.0f, 3.14f)) {
                  comp->m_transform.SetRotation(rotInput);
               }
               glm::vec3 scaleInput = comp->m_transform.GetScale();
               if (ImGui::DragFloat3("Scale", &scaleInput.x, 0.01f, 0.0f, 10000.0f)) {
                  comp->m_transform.SetScale(scaleInput);
               }
            }
            ImGui::TreePop();
         }
         // Add all child elements to nodes to traverse
         std::ranges::for_each(c->GetChildren(), [&](auto& x) { nodesToCheck.push_back(x.get()); });
      }
      ImGui::End();
   }
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GLRenderer::RenderFrame() {
   // Clear the screen
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   // Test pipeline with cube
   shader->Use();
   const CameraData camData = {
      m_activeCamera->GetViewMatrix(),
      m_activeCamera->GetProjectionMatrix()
   };
   cameraUbo->UpdateData(&camData, sizeof(CameraData));
   shader->BindUniformBlock("CameraData", 0);
   // Set up shader texture test
   ITexture* tex = m_resourceManager->GetTexture(texture);
   if (tex) {
      tex->Bind(1);
      sampler->BindUnit(1);
   }
   // Draw the scene
   Node* root = m_activeScene->GetRootNode();
   std::vector<Node*> nodesToCheck{root};
   while (!nodesToCheck.empty()) {
      auto c = nodesToCheck.back();
      nodesToCheck.pop_back();
      // Get transform
      TransformComponent* comp = c->GetComponent<TransformComponent>();
      // Draw mesh
      IMesh* meshC = m_resourceManager->GetMesh(mesh);
      if (meshC) {
         shader->SetMat4("model", comp->m_transform.GetTransformMatrix());
         meshC->Draw();
      }
      // Add all child elements to nodes to traverse
      std::ranges::for_each(c->GetChildren(), [&](auto& x) { nodesToCheck.push_back(x.get()); });
   }
   // Render Ui
   RenderImgui();
   // Swap buffers
   glfwSwapBuffers(m_window->GetNativeWindow());
}

