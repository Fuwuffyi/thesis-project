#pragma once

#include <cstdint>

#include "core/IRenderer.hpp"

#include "gl/GLFramebuffer.hpp"
#include "gl/GLRenderPass.hpp"

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

   void CreateGBuffer();
   void CreateGeometryPass();

   // TODO: Remove once scene impl complete
   void CreateTestResources();
private:
   std::unique_ptr<GLFramebuffer> m_gBuffer;
   std::unique_ptr<GLRenderPass> m_geometryPass;
};

