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

#include "core/editor/PerformanceGUI.hpp"

#include "gl/GLFramebuffer.hpp"
#include "gl/GLRenderPass.hpp"
#include "gl/GLShader.hpp"

#include "gl/resource/GLResourceFactory.hpp"
#include "gl/resource/GLTexture.hpp"

#include <algorithm>
#include <glad/gl.h>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <vector>

// Structs for UBOs
struct CameraData {
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
   alignas(16) glm::vec3 viewPos;
};

struct LightData {
   alignas(4) uint32_t lightType;
   alignas(16) glm::vec3 position;
   alignas(16) glm::vec3 direction;
   alignas(16) glm::vec3 color;
   alignas(4) float intensity;
   alignas(4) float constant;
   alignas(4) float linear;
   alignas(4) float quadratic;
   alignas(4) float innerCone;
   alignas(4) float outerCone;
};

constexpr size_t MAX_LIGHTS = 256;
struct LightsData {
   alignas(4) uint32_t lightCount;
   LightData lights[MAX_LIGHTS];
};

GLRenderer::GLRenderer(Window* window) : IRenderer(window) {
   // Load OpenGL function pointers
   if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
      throw std::runtime_error("GLAD init failed.");
   }
   // Setup resource manager
   m_resourceManager = std::make_unique<ResourceManager>(std::make_unique<GLResourceFactory>());
   m_materialEditor = std::make_unique<MaterialEditor>(m_resourceManager.get());
   // Setup imgui
   SetupImgui();
   // Create the fullscreen quad for lighting pass
   CreateFullscreenQuad();
   // Create a default material
   CreateDefaultMaterial();
   // Load shaders
   LoadShaders();
   // Create shader ubos
   CreateUBOs();
   // Set initial viewport (sets up g-buffer too)
   FramebufferCallback(static_cast<int32_t>(m_window->GetWidth()),
                       static_cast<int32_t>(m_window->GetHeight()));
   // Setup framebuffer callback
   window->SetResizeCallback(
      [this](int32_t width, int32_t height) { FramebufferCallback(width, height); });
   // Create lighting pass resources
   CreateLightingPass();
}

void GLRenderer::FramebufferCallback(const int32_t width, const int32_t height) {
   glViewport(0, 0, width, height);
   if (m_activeCamera) {
      m_activeCamera->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
   }
   CreateGBuffer();
   CreateGeometryPass();
}

void GLRenderer::CreateFullscreenQuad() {
   const std::vector<Vertex> quadVerts = {
      {glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f)},
      {glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f)},
      {glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 1.0f)},
      {glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 1.0f)},
   };
   const std::vector<uint32_t> quadInds = {0, 1, 2, 2, 3, 0};
   m_fullscreenQuad = m_resourceManager->LoadMesh("quad", quadVerts, quadInds);
}

void GLRenderer::CreateDefaultMaterial() {
   m_defaultMaterial = m_resourceManager->CreateMaterial("default_pbr", "PBR");
   if (IMaterial* material = m_resourceManager->GetMaterial(m_defaultMaterial)) {
      material->SetParameter("albedo", glm::vec3(0.8f, 0.8f, 0.8f));
      material->SetParameter("metallic", 0.0f);
      material->SetParameter("roughness", 0.8f);
      material->SetParameter("ao", 1.0f);
   }
}

void GLRenderer::LoadShaders() {
   // Setup geometry pass shader
   m_geometryPassShader = std::make_unique<GLShader>();
   m_geometryPassShader->AttachShaderFromFile(GLShader::Type::Vertex,
                                              "resources/shaders/gl/geometry_pass.vert");
   m_geometryPassShader->AttachShaderFromFile(GLShader::Type::Fragment,
                                              "resources/shaders/gl/geometry_pass.frag");
   m_geometryPassShader->Link();
   // Setup lighting pass shader
   m_lightingPassShader = std::make_unique<GLShader>();
   m_lightingPassShader->AttachShaderFromFile(GLShader::Type::Vertex,
                                              "resources/shaders/gl/lighting_pass.vert");
   m_lightingPassShader->AttachShaderFromFile(GLShader::Type::Fragment,
                                              "resources/shaders/gl/lighting_pass.frag");
   m_lightingPassShader->Link();
}

void GLRenderer::CreateGBuffer() {
   if (m_gBuffer) {
      m_gBuffer.reset();
   }
   m_gAlbedoTexture = m_resourceManager->CreateRenderTarget(
      "gbuffer_color", m_window->GetWidth(), m_window->GetHeight(), ITexture::Format::RGBA8);
   m_gNormalTexture = m_resourceManager->CreateRenderTarget(
      "gbuffer_normals", m_window->GetWidth(), m_window->GetHeight(), ITexture::Format::RGBA16F);
   m_gDepthTexture = m_resourceManager->CreateDepthTexture("gbuffer_depth", m_window->GetWidth(),
                                                           m_window->GetHeight());
   auto* colorTexPtr =
      reinterpret_cast<GLTexture*>(m_resourceManager->GetTexture(m_gAlbedoTexture));
   auto* normalTexPtr =
      reinterpret_cast<GLTexture*>(m_resourceManager->GetTexture(m_gNormalTexture));
   auto* depthTexPtr = reinterpret_cast<GLTexture*>(m_resourceManager->GetTexture(m_gDepthTexture));
   GLFramebuffer::CreateInfo gbufferInfo;
   gbufferInfo.width = m_window->GetWidth();
   gbufferInfo.height = m_window->GetHeight();
   gbufferInfo.colorAttachments = {
      {colorTexPtr, 0, 0},
      {normalTexPtr, 0, 0},
   };
   gbufferInfo.depthAttachment = {depthTexPtr};
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
   geometryPassInfo.depthStencilAttachment = {GLRenderPass::LoadOp::Clear,
                                              GLRenderPass::StoreOp::Store,
                                              GLRenderPass::LoadOp::DontCare,
                                              GLRenderPass::StoreOp::DontCare,
                                              1.0f,
                                              0};
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
   lightingPassInfo.depthStencilAttachment = {GLRenderPass::LoadOp::DontCare,
                                              GLRenderPass::StoreOp::DontCare,
                                              GLRenderPass::LoadOp::DontCare,
                                              GLRenderPass::StoreOp::DontCare,
                                              1.0f,
                                              0};
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
   m_lightsUbo->UploadData(&lightData, sizeof(LightsData));
   m_lightsUbo->BindBase(1);
}

GLRenderer::~GLRenderer() { DestroyImgui(); }

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
   m_activeScene->DrawInspector(*m_materialEditor);
   // Draw material editor
   m_materialEditor->DrawMaterialBrowser();
   m_materialEditor->DrawMaterialProperties();
   m_materialEditor->DrawTextureBrowser();
   // FPS Overlay
   PerformanceGUI::RenderPeformanceGUI(*m_resourceManager.get());
   // Show materials
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GLRenderer::RenderFrame() {
   // Update the camera
   const CameraData camData = {
      m_activeCamera->GetViewMatrix(),
      m_activeCamera->GetProjectionMatrix(),
      m_activeCamera->GetTransform().GetPosition(),
   };
   m_cameraUbo->UpdateData(&camData, sizeof(CameraData));
   m_geometryPass->Begin();
   m_geometryPass->SetShader(m_geometryPassShader.get());
   // Draw the scene
   if (m_activeScene) {
      m_activeScene->UpdateTransforms();
      m_activeScene->ForEachNode([&](const Node* node) {
         // Skip inactive nodes
         if (!node->IsActive())
            return;
         if (const auto* renderer = node->GetComponent<RendererComponent>()) {
            // If not visible, do not render
            if (!renderer->IsVisible() || !renderer->HasMesh())
               return;
            // If has position, load it in
            if (const Transform* worldTransform = node->GetWorldTransform()) {
               // Set up transformation matrix for rendering
               m_geometryPassShader->SetMat4("model", worldTransform->GetTransformMatrix());
            }
            // Render the mesh
            if (renderer->IsMultiMesh()) {
               for (const auto& subMeshRenderer : renderer->GetSubMeshRenderers()) {
                  if (!subMeshRenderer.visible)
                     continue;
                  if (const IMesh* mesh = m_resourceManager->GetMesh(subMeshRenderer.mesh)) {
                     if (IMaterial* material =
                            m_resourceManager->GetMaterial(subMeshRenderer.material)) {
                        material->Bind(2, *m_resourceManager.get());
                        mesh->Draw();
                     }
                  }
               }
            } else {
               if (const IMesh* mesh = m_resourceManager->GetMesh(renderer->GetMesh())) {
                  if (IMaterial* material =
                         m_resourceManager->GetMaterial(renderer->GetMaterial())) {
                     material->Bind(2, *m_resourceManager.get());
                     mesh->Draw();
                  }
               }
            }
         }
      });
   }
   m_geometryPass->End();
   // Lighting pass
   m_lightingPass->Begin();
   m_lightingPass->SetShader(m_lightingPassShader.get());
   LightsData lightsData{};
   lightsData.lightCount = 0;
   if (m_activeScene) {
      m_activeScene->ForEachNode([&](Node* node) {
         if (const LightComponent* lightComp = node->GetComponent<LightComponent>();
             lightComp && lightsData.lightCount < MAX_LIGHTS) {
            if (const TransformComponent* transformComp =
                   node->GetComponent<TransformComponent>()) {
               LightData& light = lightsData.lights[lightsData.lightCount];
               light.lightType = static_cast<uint32_t>(lightComp->GetType());
               light.color = lightComp->GetColor();
               light.intensity = lightComp->GetIntensity();
               light.constant = lightComp->GetConstant();
               light.linear = lightComp->GetLinear();
               light.quadratic = lightComp->GetQuadratic();
               const Transform transform = transformComp->GetTransform();
               light.position = transform.GetPosition();
               light.direction = transform.GetForward();
               // Spot light cone angles
               light.innerCone = lightComp->GetInnerCone();
               light.outerCone = lightComp->GetOuterCone();
               ++lightsData.lightCount;
            }
         }
      });
   }
   m_lightsUbo->UpdateData(&lightsData, sizeof(LightsData));
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

ResourceManager* GLRenderer::GetResourceManager() { return m_resourceManager.get(); }
