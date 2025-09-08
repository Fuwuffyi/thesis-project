#pragma once

#include "core/Transform.hpp"
#include "core/GraphicsAPI.hpp"

class Camera final {
  public:
   Camera(const GraphicsAPI api, const Transform& transform, const glm::vec3& up, const float fov,
          const float aspectRatio, const float near, const float far) noexcept;

   [[nodiscard]] glm::vec3 GetViewDirection() const noexcept;
   [[nodiscard]] glm::vec3 GetRightVector() const noexcept;
   [[nodiscard]] glm::vec3 GetUpVector() const noexcept;

   [[nodiscard]] constexpr float GetFOV() const noexcept { return m_fov; }
   void SetFOV(const float newFov) noexcept;

   void SetAspectRatio(const float newRatio) noexcept;

   [[nodiscard]] const glm::mat4& GetViewMatrix();
   [[nodiscard]] const glm::mat4& GetProjectionMatrix();
   [[nodiscard]] const glm::mat4& GetCameraMatrix();

   [[nodiscard]] constexpr const Transform& GetTransform() const noexcept { return m_transform; }

   constexpr Transform& GetMutableTransform() noexcept {
      m_viewDirty = true;
      m_cameraDirty = true;
      return m_transform;
   }

  private:
   void RecalculateView() noexcept;
   void RecalculateProjection() noexcept;

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

   bool m_viewDirty;
   bool m_projDirty;
   bool m_cameraDirty;
};
