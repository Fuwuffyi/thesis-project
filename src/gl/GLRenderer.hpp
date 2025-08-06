#include "../core/IRenderer.hpp"

class GLRenderer : public IRenderer {
public:
   void Init(GLFWwindow* windowHandle) override;
   void RenderFrame() override;
   void Cleanup() override;

private:
   GLFWwindow* m_windowHandle = nullptr;
};

