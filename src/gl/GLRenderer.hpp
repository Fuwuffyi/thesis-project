#pragma once

#include "../core/IRenderer.hpp"

class GLRenderer : public IRenderer {
public:
   GLRenderer(Window* window);
   ~GLRenderer() override;
   void RenderFrame() override;
private:
   void FramebufferCallback();

   void CreateTestMesh();
};

