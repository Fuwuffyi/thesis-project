#include "GLRenderer.hpp"

#include <algorithm>
#include <glad/gl.h>
#include <print>
#include <GLFW/glfw3.h>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "core/Window.hpp"
#include "core/Camera.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/Node.hpp"

#include "core/scene/components/RendererComponent.hpp"
#include "core/scene/components/TransformComponent.hpp"

#include "gl/GLFramebuffer.hpp"
#include "gl/GLRenderPass.hpp"
#include "gl/GLShader.hpp"

#include "gl/resource/GLResourceFactory.hpp"

// Stuff for mesh
#include <vector>
#include "GLBuffer.hpp"
#include "GLShader.hpp"
#include "resource/GLTexture.hpp"

// Testing stuff
struct CameraData {
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
};

GLBuffer* cameraUbo = nullptr;

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
   // Create the fullscreen quad for lighting pass
   CreateFullscreenQuad();
   // Load shaders
   LoadShaders();
   // Set initial viewport (sets up g-buffer too)
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
   // Create lighting pass resources
   CreateLightingPass();
   // Create some testing resources
   CreateTestResources();
}

void GLRenderer::FramebufferCallback(const int32_t width, const int32_t height) {
   glViewport(0, 0, width, height);
   if (m_activeCamera) {
      m_activeCamera->SetAspectRatio(static_cast<float>(width) /
                                     static_cast<float>(height));
   }
   CreateGBuffer();
   CreateGeometryPass();
}

void GLRenderer::CreateFullscreenQuad() {
   const std::vector<Vertex> quadVerts = {
      { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f) },
      { glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f) },
      { glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 1.0f) },
      { glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 1.0f) },
   };
   const std::vector<uint32_t> quadInds ={
      0, 1, 2, 2, 3, 0
   };
   m_fullscreenQuad = m_resourceManager->LoadMesh("quad", quadVerts, quadInds);
}

void GLRenderer::LoadShaders() {
   // Setup geometry pass shader
   m_geometryPassShader = std::make_unique<GLShader>();
   m_geometryPassShader->AttachShaderFromFile(GLShader::Type::Vertex, "resources/shaders/gl/geometry_pass.vert");
   m_geometryPassShader->AttachShaderFromFile(GLShader::Type::Fragment, "resources/shaders/gl/geometry_pass.frag");
   m_geometryPassShader->Link();
   // Setup lighting pass shader
   m_lightingPassShader = std::make_unique<GLShader>();
   m_lightingPassShader->AttachShaderFromFile(GLShader::Type::Vertex, "resources/shaders/gl/lighting_pass.vert");
   m_lightingPassShader->AttachShaderFromFile(GLShader::Type::Fragment, "resources/shaders/gl/lighting_pass.frag");
   m_lightingPassShader->Link();
}

void GLRenderer::CreateGBuffer() {
   if (m_gBuffer) {
      m_gBuffer.reset();
   }
   m_gAlbedoTexture = m_resourceManager->CreateRenderTarget("gbuffer_color", m_window->GetWidth(),
                                                            m_window->GetHeight(), ITexture::Format::RGBA8);
   m_gNormalTexture = m_resourceManager->CreateRenderTarget("gbuffer_normals", m_window->GetWidth(),
                                                            m_window->GetHeight(), ITexture::Format::RGB8);
   m_gDepthTexture = m_resourceManager->CreateDepthTexture("gbuffer_depth",
                                                           m_window->GetWidth(), m_window->GetHeight());
   auto* colorTexPtr= reinterpret_cast<GLTexture*>(m_resourceManager->GetTexture(m_gAlbedoTexture));
   auto* normalTexPtr= reinterpret_cast<GLTexture*>(m_resourceManager->GetTexture(m_gNormalTexture));
   auto* depthTexPtr= reinterpret_cast<GLTexture*>(m_resourceManager->GetTexture(m_gDepthTexture));
   GLFramebuffer::CreateInfo gbufferInfo;
   gbufferInfo.width = m_window->GetWidth();
   gbufferInfo.height = m_window->GetHeight();
   gbufferInfo.colorAttachments = {
      {
         colorTexPtr, 0, 0
      },
      {
         normalTexPtr, 0, 0
      },
   };
   gbufferInfo.depthAttachment = {
      depthTexPtr
   };
   m_gBuffer = std::make_unique<GLFramebuffer>(gbufferInfo);
   if (!m_gBuffer->IsComplete()) {
      throw std::runtime_error("G-Buffer creation incomplete:\n" + m_gBuffer->GetStatusString());
   }
}

void GLRenderer::CreateGeometryPass() {
   GLRenderPass::CreateInfo geometryPassInfo;
   geometryPassInfo.framebuffer = m_gBuffer.get();
   geometryPassInfo.colorAttachments = {
      {GLRenderPass::LoadOp::Clear, GLRenderPass::StoreOp::Store, {0.0f, 0.0f, 0.0f, 1.0f}},
      {GLRenderPass::LoadOp::Clear, GLRenderPass::StoreOp::Store, {0.0f, 0.0f, 0.0f, 1.0f}},
   };
   geometryPassInfo.depthStencilAttachment = {
      GLRenderPass::LoadOp::Clear, GLRenderPass::StoreOp::Store,
      GLRenderPass::LoadOp::DontCare, GLRenderPass::StoreOp::DontCare,
      1.0f, 0
   };
   geometryPassInfo.renderState.depthTest = GLRenderPass::DepthTest::Less;
   geometryPassInfo.renderState.cullMode = GLRenderPass::CullMode::Back;
   geometryPassInfo.renderState.primitiveType = GLRenderPass::PrimitiveType::Triangles;
   m_geometryPass = std::make_unique<GLRenderPass>(geometryPassInfo);
}

void GLRenderer::CreateLightingPass() {
   GLRenderPass::CreateInfo lightingPassInfo;
   lightingPassInfo.framebuffer = nullptr;
   lightingPassInfo.colorAttachments = {
      {GLRenderPass::LoadOp::Clear, GLRenderPass::StoreOp::Store, {0.0f, 0.0f, 0.0f, 1.0f}},
   };
   lightingPassInfo.depthStencilAttachment = {
      GLRenderPass::LoadOp::DontCare, GLRenderPass::StoreOp::DontCare,
      GLRenderPass::LoadOp::DontCare, GLRenderPass::StoreOp::DontCare,
      1.0f, 0
   };
   lightingPassInfo.renderState.depthTest = GLRenderPass::DepthTest::Always;
   lightingPassInfo.renderState.cullMode = GLRenderPass::CullMode::Back;
   lightingPassInfo.renderState.primitiveType = GLRenderPass::PrimitiveType::Triangles;
   m_lightingPass = std::make_unique<GLRenderPass>(lightingPassInfo);
}

void GLRenderer::CreateTestResources() {
   // Create camera UBO
   cameraUbo = new GLBuffer(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw);
   const CameraData camData{};
   cameraUbo->UploadData(&camData, sizeof(CameraData));
   cameraUbo->BindBase(0);
}

GLRenderer::~GLRenderer() {
   DestroyImgui();
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
      ImGui::Text("Resource MEM: %.3f MB", static_cast<float>(m_resourceManager->GetTotalMemoryUsage()) /
                  (1024.0f * 1024.0f));
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
      if (m_activeScene) {
         // Recursive function to display hierarchy
         std::function<void(Node*)> displayNodeHierarchy = [&](Node* node) {
            if (!node) return;
            ImGui::PushID(node);
            // Tree node display
            bool hasChildren = node->GetChildCount() > 0;
            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | 
               ImGuiTreeNodeFlags_OpenOnDoubleClick |
               ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!hasChildren) {
               nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }
            bool nodeOpen = ImGui::TreeNodeEx(node->GetName().c_str(), nodeFlags);
            // Show transform controls when node is selected/opened
            if (nodeOpen) {
               bool nodeActive = node->IsActive();
               if (ImGui::Checkbox("Active", &nodeActive)) {
                  node->SetActive(nodeActive);
               }
            }
            if (nodeOpen || !hasChildren) {
               if (TransformComponent* comp = node->GetComponent<TransformComponent>()) {
                  ImGui::PushID("transform");
                  glm::vec3 posInput = comp->GetTransform().GetPosition();
                  if (ImGui::DragFloat3("Position", &posInput.x, 0.01f)) {
                     comp->SetPosition(posInput);
                     node->MarkTransformDirty();
                  }
                  glm::vec3 eulerAngles = glm::degrees(comp->GetTransform().GetEulerAngles());
                  if (ImGui::DragFloat3("Rotation", &eulerAngles.x, 0.1f, -180.0f, 180.0f)) {
                     comp->SetRotation(glm::radians(eulerAngles));
                     node->MarkTransformDirty();
                  }
                  glm::vec3 scaleInput = comp->GetTransform().GetScale();
                  if (ImGui::DragFloat3("Scale", &scaleInput.x, 0.01f, 0.01f, 100.0f)) {
                     comp->SetScale(scaleInput);
                     node->MarkTransformDirty();
                  }
                  ImGui::PopID();
               }
            }
            // Display children
            if (nodeOpen && hasChildren) {
               node->ForEachChild([&](Node* child) {
                  displayNodeHierarchy(child);
               }, false);
               ImGui::TreePop();
            }
            ImGui::PopID();
         };
         // Start from root
         Node* root = m_activeScene->GetRootNode();
         if (root) {
            displayNodeHierarchy(root);
         }
      }
      ImGui::End();
   }
   // Show textures
   {
      const uint32_t columns = 4;
      const uint32_t imgSize = 128;
      ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - 600));
      ImGui::SetNextWindowSize(ImVec2(columns * imgSize, 600));
      ImGui::SetNextWindowBgAlpha(0.35f);
      ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
         ImGuiWindowFlags_NoResize |
         ImGuiWindowFlags_NoSavedSettings |
         ImGuiWindowFlags_NoFocusOnAppearing |
         ImGuiWindowFlags_NoNav;
      if (ImGui::Begin("Texture Browser", nullptr, flags)) {
         ImGui::BeginChild("TextureScrollRegion", ImVec2(0, 0),
                           false, ImGuiWindowFlags_HorizontalScrollbar);
         ImGui::Columns(columns, nullptr, false);
         const auto namedTextures = m_resourceManager->GetAllTexturesNamed();
         for (const auto& tex : namedTextures) {
            if (tex.first) {
               const GLuint texId = static_cast<GLTexture*>(tex.first)->GetId();
               ImGui::Image(
                  (ImTextureID)(intptr_t)texId,
                  ImVec2(imgSize, imgSize),
                  ImVec2(0, 1),
                  ImVec2(1, 0)
               );
               ImGui::TextWrapped(tex.second.c_str());
            }
            ImGui::NextColumn();
         }
         ImGui::Columns(1);
         ImGui::EndChild();
      }
      ImGui::End();
   }
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GLRenderer::RenderFrame() {
   // Update the camera
   const CameraData camData = {
      m_activeCamera->GetViewMatrix(),
      m_activeCamera->GetProjectionMatrix()
   };
   cameraUbo->UpdateData(&camData, sizeof(CameraData));
   m_geometryPass->Begin();
   m_geometryPass->SetShader(m_geometryPassShader.get());
   m_geometryPassShader->BindUniformBlock("CameraData", 0);
   // Set up shader texture test
   const ITexture* tex = m_resourceManager->GetTexture("testing_albedo");
   if (tex) {
      tex->Bind(1);
   }
   // Draw the scene
   if (m_activeScene) {
      m_activeScene->UpdateTransforms();
      m_activeScene->ForEachNode([&](const Node* node) {
         // Skip inactive nodes
         if (!node->IsActive()) return;
         if (const auto* renderer = node->GetComponent<RendererComponent>()) {
            // If not visible, do not render
            if (!renderer->IsVisible()) return;
            // Get the mesh and check if valid
            const IMesh* mesh = m_resourceManager->GetMesh(renderer->GetMesh());
            if (mesh && mesh->IsValid()) {
               // If has position, load it in
               if (const Transform* worldTransform = node->GetWorldTransform()) {
                  // Set up transformation matrix for rendering
                  m_geometryPassShader->SetMat4("model", worldTransform->GetTransformMatrix());
               }
               // Render the mesh
               mesh->Draw();
            }
         }
      });
   }
   m_geometryPass->End();
   // Lighting pass
   m_lightingPass->Begin();
   m_lightingPass->SetShader(m_lightingPassShader.get());
   // Bind g buffer textures
   const ITexture* gAlbedoTex = m_resourceManager->GetTexture(m_gAlbedoTexture);
   const ITexture* gNormalTex = m_resourceManager->GetTexture(m_gNormalTexture);
   const ITexture* gDepthTex = m_resourceManager->GetTexture(m_gDepthTexture);
   if (gDepthTex && gAlbedoTex && gNormalTex) {
      gAlbedoTex->Bind(1);
      gNormalTex->Bind(2);
      gDepthTex->Bind(3);
   }
   // Draw fullscreen mesh
   const IMesh* quadMesh = m_resourceManager->GetMesh(m_fullscreenQuad);
   if (quadMesh && quadMesh->IsValid()) {
      quadMesh->Draw();
   }
   m_lightingPass->End();
   // Render Ui
   RenderImgui();
   // Swap buffers
   glfwSwapBuffers(m_window->GetNativeWindow());
}

