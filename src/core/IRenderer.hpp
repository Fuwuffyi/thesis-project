#pragma once

// Forward declaration for Window stuff

class Window;
class Camera;

class IRenderer {
public:
   virtual ~IRenderer() = default;

   virtual void RenderFrame() = 0;
   void SetActiveCamera(Camera* cam);

   IRenderer(const IRenderer&) = delete;
   IRenderer& operator=(const IRenderer&) = delete;
protected:
   IRenderer(Window* window);
   virtual void SetupImgui() = 0;
   virtual void RenderImgui() = 0;
   virtual void DestroyImgui() = 0;
protected:
   Window* m_window;
   Camera* m_activeCamera;
};

