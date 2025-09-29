#pragma once

#include "core/IRenderer.hpp"
#include "core/resource/IMaterial.hpp"
#include "core/resource/IMesh.hpp"
#include "core/resource/ITexture.hpp"

#include "gl/GLBuffer.hpp"

#include <memory>

class GLFramebuffer;
class GLShader;
class GLRenderPass;
class MaterialEditor;
class Window;

class GLRenderer : public IRenderer {
  public:
   explicit GLRenderer(Window* window);
   ~GLRenderer() override;

   GLRenderer(const GLRenderer&) = delete;
   GLRenderer& operator=(const GLRenderer&) = delete;
   GLRenderer(GLRenderer&&) noexcept = default;
   GLRenderer& operator=(GLRenderer&&) noexcept = default;

   void RenderFrame() override;

  private:
   void SetupImgui() override;
   void RenderImgui() override;
   void DestroyImgui() override;

   void FramebufferCallback(const int32_t width, const int32_t height) noexcept;

   void CreateUtilityMeshes();
   void CreateDefaultMaterial();
   void LoadShaders();
   void CreateUBOs();

   // Render pass creation methods
   void CreateGeometryFBO();
   void CreateGeometryPass();
   void CreateLightingFBO();
   void CreateLightingPass();
   void CreateGizmoPass();

   void UpdateCameraUBO() noexcept;
   void UpdateLightsUBO() noexcept;
   void BindGBufferTextures() const noexcept;
   void RenderGeometry() const noexcept;
   void RenderLighting() const noexcept;
   void RenderGizmos() const noexcept;

   [[nodiscard]] ResourceManager* GetResourceManager() const noexcept override;

  public:
   static constexpr size_t MAX_LIGHTS = 256;

  private:
   double m_lastFrameTime{0};
   float m_deltaTime{0};
   // Default resources
   MaterialHandle m_defaultMaterial;
   MeshHandle m_fullscreenQuad;
   MeshHandle m_lineCube;
   // Create UBOs for shader data
   std::unique_ptr<GLBuffer> m_cameraUbo;
   std::unique_ptr<GLBuffer> m_lightsUbo;
   // Geometry pass things
   TextureHandle m_gDepthTexture;
   TextureHandle m_gAlbedoTexture; // RGB color + A AO
   TextureHandle m_gNormalTexture; // RG encoded normal + B roughness + A metallic
   std::unique_ptr<GLFramebuffer> m_gBuffer;
   std::unique_ptr<GLRenderPass> m_geometryPass;
   std::unique_ptr<GLShader> m_geometryPassShader;
   // Lighting pass things
   TextureHandle m_lightingColorTexture;
   TextureHandle m_lightingDepthTexture;
   std::unique_ptr<GLFramebuffer> m_lightingFbo;
   std::unique_ptr<GLRenderPass> m_lightingPass;
   std::unique_ptr<GLShader> m_lightingPassShader;
   // Gizmo pass things
   std::unique_ptr<GLRenderPass> m_gizmoPass;
   std::unique_ptr<GLShader> m_gizmoPassShader;
   // ResourceManager
   std::unique_ptr<ResourceManager> m_resourceManager;
   std::unique_ptr<MaterialEditor> m_materialEditor;
   // Texture binding slots
   static constexpr uint32_t MATERIAL_BINDING_SLOT = 2;
   static constexpr uint32_t GBUFFER_ALBEDO_SLOT = 3;
   static constexpr uint32_t GBUFFER_NORMAL_SLOT = 4;
   static constexpr uint32_t GBUFFER_DEPTH_SLOT = 5;
   static constexpr uint32_t CAMERA_UBO_BINDING = 0;
   static constexpr uint32_t LIGHTS_UBO_BINDING = 1;
};
