#pragma once

#include "Transform.hpp"
#include "GraphicsAPI.hpp"

class Camera {
public:
   Camera(const Transform& transform, const glm::vec3& up, const float fov, const float aspectRatio,
          const float near, const float far, const GraphicsAPI api = GraphicsAPI::OpenGL);

   const glm::vec3& GetViewDirection();
   const glm::vec3& GetRightVector();
   const glm::vec3& GetUpVector();

   float GetFOV() const;
   void SetFOV(const float newFov);

   void SetAspectRatio(const float newRatio);

   const glm::mat4& GetViewMatrix();
   const glm::mat4& GetProjectionMatrix();
   const glm::mat4& GetCameraMatrix();

   const Transform& GetTransform() const;
   Transform& GetMutableTransform();

private:
   const GraphicsAPI m_api;

   Transform m_transform;
   glm::vec3 m_up;

   float m_fov;
   float m_aspectRatio;
   float m_near;
   float m_far;

   glm::mat4 m_view;
   glm::mat4 m_proj;
   glm::mat4 m_camera;

};

