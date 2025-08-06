#include "../core/IRenderer.hpp"

class VKRenderer : public IRenderer {
public:
   VKRenderer(GLFWwindow* windowHandle);
   ~VKRenderer();
   void RenderFrame() override;
};

