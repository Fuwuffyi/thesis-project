#include "GLRenderer.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "core/Window.hpp"
#include "core/Camera.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/Node.hpp"

#include "core/scene/components/TransformComponent.hpp"
#include "core/scene/components/RendererComponent.hpp"
#include "core/scene/components/LightComponent.hpp"

#include "gl/GLFramebuffer.hpp"
#include "gl/GLRenderPass.hpp"
#include "gl/GLShader.hpp"

#include "gl/resource/GLResourceFactory.hpp"
#include "gl/resource/GLTexture.hpp"

#include <algorithm>
#include <glad/gl.h>
#include <imgui.h>
#include <print>
#include <GLFW/glfw3.h>
#include <vector>

// Structs for UBOs
struct CameraData {
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
};

struct LightData {
   uint32_t lightCount;
};

struct MaterialData {
   float val = 0.0f;
};

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
   // Create shader ubos
   CreateUBOs();
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
                                                            m_window->GetHeight(), ITexture::Format::RGBA16F);
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
      {GLRenderPass::LoadOp::Clear, GLRenderPass::StoreOp::Store, {0.5f, 0.5f, 1.0f, 0.0f}},
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

void GLRenderer::CreateUBOs() {
   // Create camera UBO
   m_cameraUbo = std::make_unique<GLBuffer>(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw);
   const CameraData camData{};
   m_cameraUbo->UploadData(&camData, sizeof(CameraData));
   m_cameraUbo->BindBase(0);
   // Create lighting UBO
   m_lightsUbo = std::make_unique<GLBuffer>(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw);
   const LightData lightData{};
   m_lightsUbo->UploadData(&lightData, sizeof(LightData));
   m_lightsUbo->BindBase(1);
   // Create material UBO
   m_materialUbo = std::make_unique<GLBuffer>(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw);
   const MaterialData matData{};
   m_materialUbo->UploadData(&matData, sizeof(MaterialData));
   m_materialUbo->BindBase(2);
}

GLRenderer::~GLRenderer() {
   DestroyImgui();
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
      static Node* selectedNode = nullptr;
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
            ImGuiTreeNodeFlags nodeFlags =
               ImGuiTreeNodeFlags_OpenOnArrow |
               ImGuiTreeNodeFlags_OpenOnDoubleClick |
               ImGuiTreeNodeFlags_SpanAvailWidth |
               (hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf);
            if (selectedNode == node) nodeFlags |= ImGuiTreeNodeFlags_Selected;
            bool open = ImGui::TreeNodeEx("##treenode", nodeFlags, node->GetName().c_str());
            // Select object
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
               selectedNode = node;
            }
            // Show active checkbox on right side
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 25);
            bool active = node->IsActive();
            if (ImGui::Checkbox("##active", &active)) {
               node->SetActive(active);
            }
            // Children
            if (open) {
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
      if (selectedNode) {
         ImGui::SetNextWindowBgAlpha(0.35f);
         ImGui::Begin("Inspector");
         if (TransformComponent* comp = selectedNode->GetComponent<TransformComponent>())
            comp->DrawInspector(selectedNode);
         if (RendererComponent* comp = selectedNode->GetComponent<RendererComponent>())
            comp->DrawInspector(selectedNode);
         if (LightComponent* comp = selectedNode->GetComponent<LightComponent>())
            comp->DrawInspector(selectedNode);
         if (ImGui::Button("Add component"))
            ImGui::OpenPopup("add_component_popup");
         if (ImGui::BeginPopup("add_component_popup")) {
            if (!selectedNode->HasComponent<TransformComponent>() && ImGui::Button("Transform")) {
               selectedNode->AddComponent<TransformComponent>();
               ImGui::CloseCurrentPopup();
            }
            if (!selectedNode->HasComponent<RendererComponent>() && ImGui::Button("Renderer")) {
               selectedNode->AddComponent<RendererComponent>();
               ImGui::CloseCurrentPopup();
            }
            if (!selectedNode->HasComponent<LightComponent>() && ImGui::Button("Light")) {
               selectedNode->AddComponent<LightComponent>();
               ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
         }
         ImGui::End();
      }
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
   m_cameraUbo->UpdateData(&camData, sizeof(CameraData));
   m_geometryPass->Begin();
   m_geometryPass->SetShader(m_geometryPassShader.get());
   // Set up shader texture test
   if (const ITexture* tex = m_resourceManager->GetTexture("testing_albedo")) {
      tex->Bind(3);
   }
   // Draw the scene
   if (m_activeScene) {
      m_activeScene->UpdateTransforms();
      m_activeScene->ForEachNode([&](const Node* node) {
         // Skip inactive nodes
         if (!node->IsActive()) return;
         if (const auto* renderer = node->GetComponent<RendererComponent>()) {
            // If not visible, do not render
            if (!renderer->IsVisible() || !renderer->HasMesh()) return;
            // If has position, load it in
            if (const Transform* worldTransform = node->GetWorldTransform()) {
               // Set up transformation matrix for rendering
               m_geometryPassShader->SetMat4("model", worldTransform->GetTransformMatrix());
            }
            // Render the mesh
            if (renderer->IsMultiMesh()) {
               for (const auto& subMeshRenderer : renderer->GetSubMeshRenderers()) {
                  if (!subMeshRenderer.visible) continue;
                  if (const IMesh* mesh = m_resourceManager->GetMesh(subMeshRenderer.mesh)) {
                     mesh->Draw();
                  }
               }
            } else {
               if (const IMesh* mesh = m_resourceManager->GetMesh(renderer->GetMesh())) {
                  mesh->Draw();
               }
            }
         }
      });
   }
   m_geometryPass->End();
   // Lighting pass
   m_lightingPass->Begin();
   m_lightingPass->SetShader(m_lightingPassShader.get());
   // Bind g buffer textures
   if (const ITexture* tex = m_resourceManager->GetTexture(m_gAlbedoTexture)) {
      tex->Bind(3);
   }
   if (const ITexture* tex = m_resourceManager->GetTexture(m_gNormalTexture)) {
      tex->Bind(4);
   }
   if (const ITexture* tex = m_resourceManager->GetTexture(m_gDepthTexture)) {
      tex->Bind(5);
   }
   // Draw fullscreen mesh
   if (const IMesh* quadMesh = m_resourceManager->GetMesh(m_fullscreenQuad)) {
      quadMesh->Draw();
   }
   m_lightingPass->End();
   // Render Ui
   RenderImgui();
   // Swap buffers
   glfwSwapBuffers(m_window->GetNativeWindow());
}

