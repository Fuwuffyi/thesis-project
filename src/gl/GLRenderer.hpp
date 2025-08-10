#pragma once

#include <cstdint>

#include "../core/IRenderer.hpp"

class GLRenderer : public IRenderer {
public:
   GLRenderer(Window* window);
   ~GLRenderer() override;
   void RenderFrame() override;
private:
   static void FramebufferCallback(const int32_t width, const int32_t height);

   void CreateTestMesh();
};

