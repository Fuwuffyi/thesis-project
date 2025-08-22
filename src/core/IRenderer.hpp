#pragma once

// Forward declaration for Window stuff

class Window;
class Camera;
class Scene;

class IRenderer {
public:
   virtual ~IRenderer() = default;

   virtual void RenderFrame() = 0;
   void SetActiveCamera(Camera* cam);
   void SetActiveScene(Scene* scene);

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
   Scene* m_activeScene;
};

