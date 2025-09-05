#pragma once

#include "core/IRenderer.hpp"
#include "core/editor/MaterialEditor.hpp"

#include "core/resource/ResourceManager.hpp"

#include "gl/GLBuffer.hpp"

class GLFramebuffer;
class GLShader;
class GLRenderPass;

class GLRenderer : public IRenderer {
  public:
   GLRenderer(Window* window);
   ~GLRenderer() override;
   void RenderFrame() override;

  private:
   void SetupImgui() override;
   void RenderImgui() override;
   void DestroyImgui() override;

   void FramebufferCallback(const int32_t width, const int32_t height);
   
   ResourceManager* GetResourceManager() override;

   void CreateFullscreenQuad();
   void CreateDefaultMaterial();

   void LoadShaders();

   void CreateUBOs();

   void CreateGBuffer();
   void CreateGeometryPass();
   void CreateLightingPass();

  private:
   MaterialHandle m_defaultMaterial;
   MeshHandle m_fullscreenQuad;
   // Create UBOs for shader data
   std::unique_ptr<GLBuffer> m_cameraUbo = nullptr;
   std::unique_ptr<GLBuffer> m_lightsUbo = nullptr;
   // Geometry pass things
   TextureHandle m_gDepthTexture;
   TextureHandle m_gAlbedoTexture; // RGB color + A AO
   TextureHandle m_gNormalTexture; // RG encoded normal + B roughness + A metallic
   std::unique_ptr<GLFramebuffer> m_gBuffer;
   std::unique_ptr<GLRenderPass> m_geometryPass;
   std::unique_ptr<GLShader> m_geometryPassShader;
   // Lighting pass things
   std::unique_ptr<GLRenderPass> m_lightingPass;
   std::unique_ptr<GLShader> m_lightingPassShader;
   // ResourceManager
   std::unique_ptr<ResourceManager> m_resourceManager;
   std::unique_ptr<MaterialEditor> m_materialEditor;
};
