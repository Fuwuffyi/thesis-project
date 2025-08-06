#pragma once

#include "../core/IRenderer.hpp"

class GLRenderer : public IRenderer {
public:
   GLRenderer(GLFWwindow* windowHandle);
   ~GLRenderer() override;
   void RenderFrame() override;
};

