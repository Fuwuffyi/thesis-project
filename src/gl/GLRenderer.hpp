#pragma once

#include <cstdint>

#include "core/IRenderer.hpp"

#include "core/resource/ResourceHandle.hpp"

class GLFramebuffer;
class GLShader;
class GLRenderPass;

class GLRenderer : public IRenderer {
public:
   GLRenderer(Window *window);
   ~GLRenderer() override;
   void RenderFrame() override;

private:
   void SetupImgui() override;
   void RenderImgui() override;
   void DestroyImgui() override;

   void FramebufferCallback(const int32_t width, const int32_t height);

   void CreateFullscreenQuad();

   void LoadShaders();

   void CreateGBuffer();
   void CreateGeometryPass();
   void CreateLightingPass();

   // TODO: Remove once scene impl complete
   void CreateTestResources();

private:
   MeshHandle m_fullscreenQuad;
   // Geometry pass things
   TextureHandle m_gDepthTexture;
   TextureHandle m_gAlbedoTexture;
   TextureHandle m_gNormalTexture;
   std::unique_ptr<GLFramebuffer> m_gBuffer;
   std::unique_ptr<GLRenderPass> m_geometryPass;
   std::unique_ptr<GLShader> m_geometryPassShader;
   // Lighting pass things
   std::unique_ptr<GLRenderPass> m_lightingPass;
   std::unique_ptr<GLShader> m_lightingPassShader;
};

