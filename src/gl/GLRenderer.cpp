#include "gl/GLRenderer.hpp"

#include "core/Camera.hpp"
#include "core/Window.hpp"
#include "core/scene/Node.hpp"
#include "core/scene/Scene.hpp"
#include "core/scene/components/ParticleSystemComponent.hpp"
#include "core/scene/components/RendererComponent.hpp"
#include "core/scene/components/LightComponent.hpp"
#include "core/editor/MaterialEditor.hpp"
#include "core/editor/PerformanceGUI.hpp"
#include "core/resource/ResourceManager.hpp"

#include "core/system/SystemInfo.hpp"

#include "gl/GLFramebuffer.hpp"
#include "gl/GLRenderPass.hpp"
#include "gl/GLShader.hpp"
#include "gl/resource/GLMesh.hpp"
#include "gl/resource/GLResourceFactory.hpp"

#include <print>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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

struct LightsData {
   alignas(4) uint32_t lightCount;
   std::array<LightData, GLRenderer::MAX_LIGHTS> lights;
};

GLRenderer::GLRenderer(Window* window) : IRenderer(window) {
   // Load OpenGL function pointers with error checking
   if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) [[unlikely]] {
      throw std::runtime_error("GLAD initialization failed");
   }
   // Setup gl validation layers
#ifndef NDEBUG
   glEnable(GL_DEBUG_OUTPUT);
   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
   glDebugMessageCallback(
      [](uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length,
         const char* message, const void* userParam) { std::println("GL Debug: {}", message); },
      nullptr);
#endif
   // Initialize resource manager
   m_resourceManager = std::make_unique<ResourceManager>(std::make_unique<GLResourceFactory>());
   m_materialEditor =
      std::make_unique<MaterialEditor>(m_resourceManager.get(), GraphicsAPI::OpenGL);
   // Initialize subsystems
   SetupImgui();
   CreateUtilityMeshes();
   CreateDefaultMaterial();
   LoadShaders();
   CreateUBOs();
   // Setup resize callback
   m_window->SetResizeCallback(
      [this](int32_t width, int32_t height) noexcept { FramebufferCallback(width, height); });
   // Setup initial viewport and framebuffers
   FramebufferCallback(m_window->GetWidth(), m_window->GetHeight());
}

GLRenderer::~GLRenderer() { DestroyImgui(); }

void GLRenderer::FramebufferCallback(const int32_t width, const int32_t height) noexcept {
   glViewport(0, 0, width, height);
   if (m_activeCamera) [[likely]] {
      m_activeCamera->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
   }
   // Recreate framebuffers and render passes
   CreateGeometryFBO();
   CreateGeometryPass();
   CreateLightingFBO();
   CreateLightingPass();
   CreateGizmoPass();
   CreateParticlePass();
}

void GLRenderer::CreateUtilityMeshes() {
   // Create fullscreen quad for lighting pass
   constexpr std::array<Vertex, 4> quadVerts = {{
      {glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f)},
      {glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f)},
      {glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 1.0f)},
      {glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 1.0f)},
   }};
   constexpr std::array<uint32_t, 6> quadInds = {0, 1, 2, 2, 3, 0};
   const std::vector<Vertex> quadVertVec(quadVerts.begin(), quadVerts.end());
   const std::vector<uint32_t> quadIndVec(quadInds.begin(), quadInds.end());
   m_fullscreenQuad = m_resourceManager->LoadMesh("quad", quadVertVec, quadIndVec);
   // Create wireframe cube for gizmos
   constexpr std::array<Vertex, 10> cubeVerts = {
      {// Cube vertices
       {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(0, 0)},
       {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(1, 0)},
       {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(1, 1)},
       {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(0, 1)},
       {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(0, 0)},
       {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(1, 0)},
       {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(1, 1)},
       {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(0, 1)},
       // Arrow line vertices
       {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(0, 0)},
       {glm::vec3(0.0f, 0.0f, -0.8f), glm::vec3(0, 0, -1), glm::vec2(0, 1)}}};
   constexpr std::array<uint32_t, 26> cubeInds = {0, 1, 1, 5, 5, 4, 4, 0, 3, 2, 2, 6, 6,
                                                  7, 7, 3, 0, 3, 1, 2, 5, 6, 4, 7, 8, 9};
   const std::vector<Vertex> cubeVertVec(cubeVerts.begin(), cubeVerts.end());
   const std::vector<uint32_t> cubeIndVec(cubeInds.begin(), cubeInds.end());
   m_lineCube = m_resourceManager->LoadMesh("unit_cube", cubeVertVec, cubeIndVec);
}

void GLRenderer::CreateDefaultMaterial() {
   m_defaultMaterial = m_resourceManager->CreateMaterial("default_pbr", "PBR");
   if (auto* material = m_resourceManager->GetMaterial(m_defaultMaterial); material) [[likely]] {
      material->SetParameter("albedo", glm::vec3(1.0f));
      material->SetParameter("metallic", 1.0f);
      material->SetParameter("roughness", 1.0f);
      material->SetParameter("ao", 1.0f);
   }
}

void GLRenderer::LoadShaders() {
   const auto createShader = [](const char* vertPath, const char* fragPath) {
      auto shader = std::make_unique<GLShader>();
      shader->AttachShaderFromFile(GLShader::Type::Vertex, vertPath);
      shader->AttachShaderFromFile(GLShader::Type::Fragment, fragPath);
      shader->Link();
      return shader;
   };
   m_geometryPassShader = createShader("resources/shaders/gl/geometry_pass.vert",
                                       "resources/shaders/gl/geometry_pass.frag");

   m_lightingPassShader = createShader("resources/shaders/gl/lighting_pass.vert",
                                       "resources/shaders/gl/lighting_pass.frag");
   m_gizmoPassShader =
      createShader("resources/shaders/gl/gizmo_pass.vert", "resources/shaders/gl/gizmo_pass.frag");
   m_particlePassShader = createShader("resources/shaders/gl/particle_pass.vert",
                                       "resources/shaders/gl/particle_pass.frag");
}

void GLRenderer::CreateGeometryFBO() {
   m_gBuffer.reset();
   // Create G-buffer textures
   m_gAlbedoTexture = m_resourceManager->CreateRenderTarget(
      "gbuffer_color", m_window->GetWidth(), m_window->GetHeight(), ITexture::Format::RGBA8);
   m_gNormalTexture = m_resourceManager->CreateRenderTarget(
      "gbuffer_normals", m_window->GetWidth(), m_window->GetHeight(), ITexture::Format::RGBA16F);
   m_gDepthTexture = m_resourceManager->CreateDepthTexture("gbuffer_depth", m_window->GetWidth(),
                                                           m_window->GetHeight());
   // Get texture pointers
   const auto* colorTexPtr =
      reinterpret_cast<const GLTexture*>(m_resourceManager->GetTexture(m_gAlbedoTexture));
   const auto* normalTexPtr =
      reinterpret_cast<const GLTexture*>(m_resourceManager->GetTexture(m_gNormalTexture));
   const auto* depthTexPtr =
      reinterpret_cast<const GLTexture*>(m_resourceManager->GetTexture(m_gDepthTexture));
   // Create framebuffer
   const GLFramebuffer::CreateInfo gbufferInfo{
      .colorAttachments = {{colorTexPtr, 0, 0}, {normalTexPtr, 0, 0}},
      .depthAttachment = {depthTexPtr},
      .stencilAttachment = {},
      .width = m_window->GetWidth(),
      .height = m_window->GetHeight()};
   m_gBuffer = std::make_unique<GLFramebuffer>(GLFramebuffer::Create(gbufferInfo));
}

void GLRenderer::CreateLightingFBO() {
   m_lightingFbo.reset();
   m_lightingColorTexture =
      m_resourceManager->CreateRenderTarget("lighting_color", m_window->GetWidth(),
                                            m_window->GetHeight(), ITexture::Format::SRGB8_ALPHA8);
   m_lightingDepthTexture = m_resourceManager->CreateDepthTexture(
      "lighting_depth", m_window->GetWidth(), m_window->GetHeight());
   const auto* colorTexPtr =
      reinterpret_cast<const GLTexture*>(m_resourceManager->GetTexture(m_lightingColorTexture));
   const auto* depthTexPtr =
      reinterpret_cast<const GLTexture*>(m_resourceManager->GetTexture(m_lightingDepthTexture));
   const GLFramebuffer::CreateInfo fboInfo{.colorAttachments = {{colorTexPtr, 0, 0}},
                                           .depthAttachment = {depthTexPtr},
                                           .stencilAttachment = {},
                                           .width = m_window->GetWidth(),
                                           .height = m_window->GetHeight()};
   m_lightingFbo = std::make_unique<GLFramebuffer>(GLFramebuffer::Create(fboInfo));
}

void GLRenderer::CreateGeometryPass() {
   const GLRenderPass::CreateInfo geometryPassInfo{
      .framebuffer = m_gBuffer.get(),
      .colorAttachments =
         {
            {GLRenderPass::LoadOp::Clear, GLRenderPass::StoreOp::Store, {0.0f, 0.0f, 0.0f, 1.0f}},
            {GLRenderPass::LoadOp::Clear, GLRenderPass::StoreOp::Store, {0.5f, 0.5f, 1.0f, 0.0f}},
         },
      .depthStencilAttachment = {.depthLoadOp = GLRenderPass::LoadOp::Clear,
                                 .depthStoreOp = GLRenderPass::StoreOp::Store,
                                 .stencilLoadOp = GLRenderPass::LoadOp::DontCare,
                                 .stencilStoreOp = GLRenderPass::StoreOp::DontCare,
                                 .depthClearValue = 1.0f,
                                 .stencilClearValue = 0},
      .renderState = {.depthTest = GLRenderPass::DepthTest::Less,
                      .depthWrite = true,
                      .cullMode = GLRenderPass::CullMode::Back,
                      .frontFaceCCW = true,
                      .blendMode = GLRenderPass::BlendMode::None,
                      .primitiveType = GLRenderPass::PrimitiveType::Triangles},
      .shader = m_geometryPassShader.get()};
   m_geometryPass = std::make_unique<GLRenderPass>(geometryPassInfo);
}

void GLRenderer::CreateLightingPass() {
   const GLRenderPass::CreateInfo lightingPassInfo{
      .framebuffer = m_lightingFbo.get(),
      .colorAttachments =
         {
            {GLRenderPass::LoadOp::Clear, GLRenderPass::StoreOp::Store, {0.0f, 0.0f, 0.0f, 1.0f}},
         },
      .depthStencilAttachment = {.depthLoadOp = GLRenderPass::LoadOp::DontCare,
                                 .depthStoreOp = GLRenderPass::StoreOp::DontCare,
                                 .stencilLoadOp = GLRenderPass::LoadOp::DontCare,
                                 .stencilStoreOp = GLRenderPass::StoreOp::DontCare,
                                 .depthClearValue = 1.0f,
                                 .stencilClearValue = 0},
      .renderState = {.depthTest = GLRenderPass::DepthTest::Disabled,
                      .cullMode = GLRenderPass::CullMode::Back,
                      .primitiveType = GLRenderPass::PrimitiveType::Triangles},
      .shader = m_lightingPassShader.get()};
   m_lightingPass = std::make_unique<GLRenderPass>(lightingPassInfo);
}

void GLRenderer::CreateGizmoPass() {
   const GLRenderPass::CreateInfo gizmoPassInfo{
      .framebuffer = m_lightingFbo.get(),
      .colorAttachments =
         {
            {GLRenderPass::LoadOp::Load, GLRenderPass::StoreOp::Store, {0.0f, 0.0f, 0.0f, 1.0f}},
         },
      .depthStencilAttachment = {.depthLoadOp = GLRenderPass::LoadOp::Load,
                                 .depthStoreOp = GLRenderPass::StoreOp::Store,
                                 .stencilLoadOp = GLRenderPass::LoadOp::DontCare,
                                 .stencilStoreOp = GLRenderPass::StoreOp::DontCare,
                                 .depthClearValue = 1.0f,
                                 .stencilClearValue = 0},
      .renderState = {.depthTest = GLRenderPass::DepthTest::LessEqual,
                      .cullMode = GLRenderPass::CullMode::None,
                      .primitiveType = GLRenderPass::PrimitiveType::Lines},
      .shader = m_gizmoPassShader.get()};
   m_gizmoPass = std::make_unique<GLRenderPass>(gizmoPassInfo);
}

void GLRenderer::CreateParticlePass() {
   const GLRenderPass::CreateInfo particlePassInfo{
      .framebuffer = m_lightingFbo.get(),
      .colorAttachments =
         {
            {GLRenderPass::LoadOp::Load, GLRenderPass::StoreOp::Store, {0.0f, 0.0f, 0.0f, 1.0f}},
         },
      .depthStencilAttachment = {.depthLoadOp = GLRenderPass::LoadOp::Load,
                                 .depthStoreOp = GLRenderPass::StoreOp::Store,
                                 .stencilLoadOp = GLRenderPass::LoadOp::DontCare,
                                 .stencilStoreOp = GLRenderPass::StoreOp::DontCare,
                                 .depthClearValue = 1.0f,
                                 .stencilClearValue = 0},
      .renderState = {.depthTest = GLRenderPass::DepthTest::Less,
                      .depthWrite = false,
                      .cullMode = GLRenderPass::CullMode::None,
                      .frontFaceCCW = true,
                      .blendMode = GLRenderPass::BlendMode::Alpha,
                      .primitiveType = GLRenderPass::PrimitiveType::Triangles},
      .shader = m_particlePassShader.get()};
   m_particlePass = std::make_unique<GLRenderPass>(particlePassInfo);
}

void GLRenderer::CreateUBOs() {
   // Create camera UBO
   m_cameraUbo = std::make_unique<GLBuffer>(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw);
   const CameraData camData{};
   m_cameraUbo->UploadData(&camData, sizeof(CameraData));
   m_cameraUbo->BindBase(CAMERA_UBO_BINDING);
   // Create lights UBO
   m_lightsUbo = std::make_unique<GLBuffer>(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw);
   const LightsData lightData{};
   m_lightsUbo->UploadData(&lightData, sizeof(LightsData));
   m_lightsUbo->BindBase(LIGHTS_UBO_BINDING);
   // Create particle instance buffer
   m_particleInstanceCapacity = 100000;
   m_particleInstanceVBO =
      std::make_unique<GLBuffer>(GLBuffer::Type::Array, GLBuffer::Usage::DynamicDraw);
}

void GLRenderer::SetupImgui() {
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   if (!ImGui_ImplGlfw_InitForOpenGL(m_window->GetNativeWindow(), true)) [[unlikely]] {
      throw std::runtime_error("ImGui GLFW initialization failed");
   }
   if (!ImGui_ImplOpenGL3_Init("#version 460")) [[unlikely]] {
      throw std::runtime_error("ImGui OpenGL3 initialization failed");
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
   if (m_activeScene) [[likely]] {
      m_activeScene->DrawInspector(*m_materialEditor);
   }
   // Draw material editor
   m_materialEditor->DrawMaterialBrowser();
   m_materialEditor->DrawMaterialProperties();
   m_materialEditor->DrawTextureBrowser();
   // FPS Overlay
   PerformanceGUI::RenderPerformanceGUI(*m_resourceManager.get(), *m_activeScene,
                                        m_currentFrameMetrics);
   // Render end
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GLRenderer::UpdateCameraUBO() noexcept {
   if (!m_activeCamera) [[unlikely]]
      return;
   const CameraData camData{.view = m_activeCamera->GetViewMatrix(),
                            .proj = m_activeCamera->GetProjectionMatrix(),
                            .viewPos = m_activeCamera->GetTransform().GetPosition()};
   m_cameraUbo->UpdateData(&camData, sizeof(CameraData));
}

void GLRenderer::UpdateLightsUBO() noexcept {
   if (!m_activeScene) [[unlikely]]
      return;
   LightsData lightsData{};
   lightsData.lightCount = 0;
   m_activeScene->ForEachNode([&](const Node* node) {
      if (!node->IsActive() || lightsData.lightCount >= MAX_LIGHTS) [[unlikely]]
         return;
      const auto* lightComp = node->GetComponent<LightComponent>();
      if (lightComp) [[likely]] {
         auto& light = lightsData.lights[lightsData.lightCount];
         light.lightType = static_cast<uint32_t>(lightComp->GetType());
         light.color = lightComp->GetColor();
         light.intensity = lightComp->GetIntensity();
         light.constant = lightComp->GetConstant();
         light.linear = lightComp->GetLinear();
         light.quadratic = lightComp->GetQuadratic();
         const auto* transform = node->GetWorldTransform();
         light.position = transform->GetPosition();
         light.direction = transform->GetForward();
         light.innerCone = lightComp->GetInnerCone();
         light.outerCone = lightComp->GetOuterCone();
         ++lightsData.lightCount;
      }
   });
   m_lightsUbo->UpdateData(&lightsData, sizeof(LightsData));
}

void GLRenderer::BindGBufferTextures() const noexcept {
   if (const auto* tex = m_resourceManager->GetTexture(m_gAlbedoTexture); tex) [[likely]] {
      tex->Bind(GBUFFER_ALBEDO_SLOT);
   }
   if (const auto* tex = m_resourceManager->GetTexture(m_gNormalTexture); tex) [[likely]] {
      tex->Bind(GBUFFER_NORMAL_SLOT);
   }
   if (const auto* tex = m_resourceManager->GetTexture(m_gDepthTexture); tex) [[likely]] {
      tex->Bind(GBUFFER_DEPTH_SLOT);
   }
}

void GLRenderer::RenderGeometry() const noexcept {
   if (!m_activeScene) [[unlikely]]
      return;
   m_activeScene->ForEachNode([this](const Node* node) {
      if (!node->IsActive()) [[unlikely]]
         return;
      const auto* renderer = node->GetComponent<RendererComponent>();
      if (!renderer || !renderer->IsVisible() || !renderer->HasMesh()) [[unlikely]]
         return;
      // Set transformation matrix
      if (const auto* worldTransform = node->GetWorldTransform(); worldTransform) [[likely]] {
         m_geometryPassShader->SetMat4("model", worldTransform->GetTransformMatrix());
      }
      // Render mesh with material
      const auto* mesh = m_resourceManager->GetMesh(renderer->GetMesh());
      auto* material = m_resourceManager->GetMaterial(renderer->GetMaterial());
      if (mesh && material) [[likely]] {
         material->Bind(MATERIAL_BINDING_SLOT, *m_resourceManager);
         mesh->Draw();
      }
   });
}

void GLRenderer::RenderLighting() const noexcept {
   BindGBufferTextures();
   if (const auto* quadMesh = m_resourceManager->GetMesh(m_fullscreenQuad); quadMesh) [[likely]] {
      quadMesh->Draw();
   }
}

void GLRenderer::RenderGizmos() const noexcept {
   if (!m_activeScene) [[unlikely]]
      return;
   const auto* cubeMesh = m_resourceManager->GetMesh(m_lineCube);
   const auto* glCubeMesh = dynamic_cast<const GLMesh*>(cubeMesh);
   if (!glCubeMesh) [[unlikely]]
      return;
   m_activeScene->ForEachNode([&](const Node* node) {
      if (!node->IsActive()) [[unlikely]]
         return;
      const auto* lightComp = node->GetComponent<LightComponent>();
      if (lightComp) [[likely]] {
         m_gizmoPassShader->SetMat4("model", node->GetWorldTransform()->GetTransformMatrix());
         m_gizmoPassShader->SetVec3("gizmoColor", lightComp->GetColor());
         glCubeMesh->Draw(m_gizmoPass->GetPrimitiveType());
      }
   });
}

void GLRenderer::RenderParticles() noexcept {
   if (!m_activeScene) [[unlikely]]
      return;
   const auto* quadMesh = m_resourceManager->GetMesh(m_fullscreenQuad);
   if (!quadMesh) [[unlikely]]
      return;
   const auto* glQuadMesh = dynamic_cast<const GLMesh*>(quadMesh);
   if (!glQuadMesh) [[unlikely]]
      return;
   m_activeScene->ForEachNode([&](const Node* node) {
      if (!node->IsActive()) [[unlikely]]
         return;
      const auto* particles = node->GetComponent<ParticleSystemComponent>();
      if (!particles) [[unlikely]]
         return;
      const auto& instanceData = particles->GetInstanceData();
      const uint32_t activeCount = particles->GetActiveParticleCount();
      if (activeCount == 0) [[unlikely]]
         return;
      // Ensure instance buffer is large enough
      const size_t requiredSize = activeCount * sizeof(ParticleInstanceData);
      if (activeCount > m_particleInstanceCapacity) {
         m_particleInstanceCapacity = activeCount * 2;
      }
      // Upload instance data
      m_particleInstanceVBO->UploadData(std::span(instanceData.data(), activeCount));
      // Setup instanced vertex attributes
      const uint32_t vao = reinterpret_cast<uintptr_t>(glQuadMesh->GetNativeHandle());
      glBindVertexArray(vao);
      m_particleInstanceVBO->Bind();
      for (uint32_t i = 0; i < 4; ++i) {
         glEnableVertexAttribArray(3 + i);
         glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData),
                               reinterpret_cast<void*>(i * sizeof(glm::vec4)));
         glVertexAttribDivisor(3 + i, 1);
      }
      glEnableVertexAttribArray(7);
      glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData),
                            reinterpret_cast<void*>(sizeof(glm::mat4)));
      glVertexAttribDivisor(7, 1);
      glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(glQuadMesh->GetIndexCount()),
                              GL_UNSIGNED_SHORT, nullptr, static_cast<GLsizei>(activeCount));
      // Cleanup divisors
      for (uint32_t i = 0; i < 4; ++i) {
         glVertexAttribDivisor(3 + i, 0);
      }
      glVertexAttribDivisor(7, 0);
      glBindVertexArray(0);
   });
}

void GLRenderer::RenderFrame() {
   // Calculate delta time
   const double currentTime = glfwGetTime();
   m_deltaTime = static_cast<float>(currentTime - m_lastFrameTime);
   m_lastFrameTime = currentTime;
   const auto cpuFrameStart = std::chrono::high_resolution_clock::now();
   ImGuiIO& io = ImGui::GetIO();
   io.DeltaTime = m_deltaTime;
   // Update UBOs
   UpdateCameraUBO();
   UpdateLightsUBO();
   m_gpuTimer.Begin("GeometryPass");
   // Geometry pass
   m_geometryPass->Begin();
   if (m_activeScene) [[likely]] {
      m_activeScene->UpdateScene(m_deltaTime);
      m_activeScene->UpdateTransforms();
      RenderGeometry();
   }
   m_geometryPass->End();
   m_gpuTimer.End("GeometryPass");
   // Copy depth buffer from G-buffer to lighting framebuffer
   m_gBuffer->BlitTo(*m_lightingFbo, 0, 0, m_window->GetWidth(), m_window->GetHeight(), 0, 0,
                     m_window->GetWidth(), m_window->GetHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
   // Lighting pass
   m_gpuTimer.Begin("LightingPass");
   m_lightingPass->Begin();
   RenderLighting();
   m_lightingPass->End();
   m_gpuTimer.End("LightingPass");
   // Gizmo pass
   m_gpuTimer.Begin("GizmoPass");
   m_gizmoPass->Begin();
   RenderGizmos();
   m_gizmoPass->End();
   m_gpuTimer.End("GizmoPass");
   // Particle pass
   m_gpuTimer.Begin("ParticlePass");
   m_particlePass->Begin();
   RenderParticles();
   m_particlePass->End();
   m_gpuTimer.End("ParticlePass");
   // Blit final result to screen
   m_lightingFbo->BlitToScreen(m_window->GetWidth(), m_window->GetHeight(), GL_COLOR_BUFFER_BIT,
                               GL_NEAREST);
   // Render UI
   m_gpuTimer.Begin("ImGuiPass");
   RenderImgui();
   m_gpuTimer.End("ImGuiPass");
   // Swap buffers
   glfwSwapBuffers(m_window->GetNativeWindow());
   // End time
   const auto cpuFrameEnd = std::chrono::high_resolution_clock::now();
   const float cpuTimeMs =
      std::chrono::duration<float, std::milli>(cpuFrameEnd - cpuFrameStart).count();
   // Build performance metrics
   m_currentFrameMetrics.frameTimeMs = m_deltaTime * 1000.0f;
   m_currentFrameMetrics.cpuTimeMs = cpuTimeMs;
   m_currentFrameMetrics.geometryPassMs = m_gpuTimer.GetElapsedMs("GeometryPass");
   m_currentFrameMetrics.lightingPassMs = m_gpuTimer.GetElapsedMs("LightingPass");
   m_currentFrameMetrics.gizmoPassMs = m_gpuTimer.GetElapsedMs("GizmoPass");
   m_currentFrameMetrics.particlePassMs = m_gpuTimer.GetElapsedMs("ParticlePass");
   m_currentFrameMetrics.imguiPassMs = m_gpuTimer.GetElapsedMs("ImGuiPass");
   m_currentFrameMetrics.gpuTimeMs =
      m_currentFrameMetrics.geometryPassMs + m_currentFrameMetrics.lightingPassMs +
      m_currentFrameMetrics.gizmoPassMs + m_currentFrameMetrics.particlePassMs +
      m_currentFrameMetrics.imguiPassMs;
   m_currentFrameMetrics.vramUsageMB = SystemInfoN::GetOpenGLMemoryUsageMB();
   m_currentFrameMetrics.systemMemUsageMB = SystemInfoN::GetSystemMemoryUsageMB();
   m_currentFrameMetrics.cpuUtilization = SystemInfoN::GetCPUUtilization();
}

ResourceManager* GLRenderer::GetResourceManager() const noexcept { return m_resourceManager.get(); }
